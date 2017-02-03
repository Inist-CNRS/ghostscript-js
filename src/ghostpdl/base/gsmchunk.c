/* Copyright (C) 2001-2012 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  7 Mt. Lassen Drive - Suite A-134, San Rafael,
   CA  94903, U.S.A., +1(415)492-9861, for further information.
*/


/* chunk consolidating wrapper on a base memory allocator */

#include "memory_.h"
#include "gx.h"
#include "gsstruct.h"
#include "gxobj.h"
#include "gsstype.h"
#include "gserrors.h"
#include "gsmchunk.h"
#include "gxsync.h"
#include "malloc_.h" /* For MEMENTO */

/* Raw memory procedures */
static gs_memory_proc_alloc_bytes(chunk_alloc_bytes_immovable);
static gs_memory_proc_resize_object(chunk_resize_object);
static gs_memory_proc_free_object(chunk_free_object);
static gs_memory_proc_stable(chunk_stable);
static gs_memory_proc_status(chunk_status);
static gs_memory_proc_free_all(chunk_free_all);
static gs_memory_proc_consolidate_free(chunk_consolidate_free);

/* Object memory procedures */
static gs_memory_proc_alloc_bytes(chunk_alloc_bytes);
static gs_memory_proc_alloc_struct(chunk_alloc_struct);
static gs_memory_proc_alloc_struct(chunk_alloc_struct_immovable);
static gs_memory_proc_alloc_byte_array(chunk_alloc_byte_array);
static gs_memory_proc_alloc_byte_array(chunk_alloc_byte_array_immovable);
static gs_memory_proc_alloc_struct_array(chunk_alloc_struct_array);
static gs_memory_proc_alloc_struct_array(chunk_alloc_struct_array_immovable);
static gs_memory_proc_object_size(chunk_object_size);
static gs_memory_proc_object_type(chunk_object_type);
static gs_memory_proc_alloc_string(chunk_alloc_string);
static gs_memory_proc_alloc_string(chunk_alloc_string_immovable);
static gs_memory_proc_resize_string(chunk_resize_string);
static gs_memory_proc_free_string(chunk_free_string);
static gs_memory_proc_register_root(chunk_register_root);
static gs_memory_proc_unregister_root(chunk_unregister_root);
static gs_memory_proc_enable_free(chunk_enable_free);
static const gs_memory_procs_t chunk_procs =
{
    /* Raw memory procedures */
    chunk_alloc_bytes_immovable,
    chunk_resize_object,
    chunk_free_object,
    chunk_stable,
    chunk_status,
    chunk_free_all,
    chunk_consolidate_free,
    /* Object memory procedures */
    chunk_alloc_bytes,
    chunk_alloc_struct,
    chunk_alloc_struct_immovable,
    chunk_alloc_byte_array,
    chunk_alloc_byte_array_immovable,
    chunk_alloc_struct_array,
    chunk_alloc_struct_array_immovable,
    chunk_object_size,
    chunk_object_type,
    chunk_alloc_string,
    chunk_alloc_string_immovable,
    chunk_resize_string,
    chunk_free_string,
    chunk_register_root,
    chunk_unregister_root,
    chunk_enable_free
};

typedef struct chunk_obj_node_s {
    struct chunk_obj_node_s *next;
    struct chunk_obj_node_s *prev;
    struct chunk_mem_node_s *chunk;
    gs_memory_type_ptr_t type;
    uint size;			/* objlist: client size */
#ifdef DEBUG
    unsigned long sequence;
#endif
} chunk_obj_node_t;

typedef struct chunk_freelist_node_s {
    struct chunk_freelist_node_s *next;
    uint size;			/* size of entire freelist block */
} chunk_freelist_node_t;

/*
 * Note: All objects within a chunk are 'aligned' since we round_up_to_align
 * the free list pointer when removing part of a free area.
 */
typedef struct chunk_mem_node_s {
    uint size;
    uint largest_free;			/* quick check when allocating */
                                        /* NB: largest_free INCLUDES the size of the obj_node */
    bool is_multiple_object_chunk;	/* tells us which list this chunk is on */
    struct chunk_mem_node_s *next;
    struct chunk_mem_node_s *prev;
    chunk_obj_node_t *objlist;	/* head of objects in this chunk (no order) */
    chunk_freelist_node_t *freelist;    /* free list (ordered) */
                                        /* size INCLUDES freelist_node structure */
    /* chunk data follows on the next obj_align_mod aligned boundary */
} chunk_mem_node_t;

typedef struct gs_memory_chunk_s {
    gs_memory_common;		/* interface outside world sees */
    gs_memory_t *target;	/* base allocator */
    chunk_mem_node_t *head_mo_chunk;	/* head of multiple object chunks */
    chunk_mem_node_t *head_so_chunk;	/* head of single object chunks */
    unsigned long used;
    unsigned long max_used;
#ifdef DEBUG
    unsigned long sequence_counter;
    int	     in_use;		/* 0 for idle, 1 in alloc, -1 in free */
#endif
} gs_memory_chunk_t;

#define SIZEOF_ROUND_ALIGN(a) ROUND_UP(sizeof(a), obj_align_mod)

/* ---------- Public constructors/destructors ---------- */

/* Initialize a gs_memory_chunk_t */
int				/* -ve error code or 0 */
gs_memory_chunk_wrap( gs_memory_t **wrapped,	/* chunk allocator init */
                      gs_memory_t * target )	/* base allocator */
{
    /* Use the non-GC allocator of the target. */
    gs_memory_t *non_gc_target = target->non_gc_memory;
    gs_memory_chunk_t *cmem = NULL;

    *wrapped = NULL;		/* don't leave garbage in case we fail */
    if (non_gc_target)
            cmem = (gs_memory_chunk_t *) gs_alloc_bytes_immovable(non_gc_target,
                        sizeof(gs_memory_chunk_t), "gs_malloc_wrap(chunk)");
    if (cmem == 0)
        return_error(gs_error_VMerror);
    cmem->stable_memory = (gs_memory_t *)cmem;	/* we are stable */
    cmem->procs = chunk_procs;
    cmem->gs_lib_ctx = non_gc_target->gs_lib_ctx;
    cmem->non_gc_memory = (gs_memory_t *)cmem;	/* and are not subject to GC */
    cmem->thread_safe_memory = non_gc_target->thread_safe_memory;
    cmem->target = non_gc_target;
    cmem->head_mo_chunk = NULL;
    cmem->head_so_chunk = NULL;
    cmem->used = 0;
    cmem->max_used = 0;
#ifdef DEBUG
    cmem->sequence_counter = 0;
    cmem->in_use = 0;		/* idle */
#endif

    /* Init the chunk management values */
    *wrapped = (gs_memory_t *)cmem;
    return 0;
}

/* Release a chunk memory manager. */
/* Note that this has no effect on the target. */
void
gs_memory_chunk_release(gs_memory_t *mem)
{
    gs_memory_free_all((gs_memory_t *)mem, FREE_ALL_EVERYTHING,
                       "gs_memory_chunk_release");
}

/* Release chunk memory manager, and return the target */
gs_memory_t * /* Always succeeds */
gs_memory_chunk_unwrap(gs_memory_t *mem)
{
    gs_memory_t *tmem;

    if (mem->procs.status != chunk_status) {
        tmem = mem;
    }
    else {
        tmem = ((gs_memory_chunk_t *)mem)->target;
        gs_memory_chunk_release(mem);
    }
    return tmem;
}

/* ---------- Accessors ------------- */

/* Retrieve this allocator's target */
gs_memory_t *
gs_memory_chunk_target(const gs_memory_t *mem)
{
    gs_memory_chunk_t *cmem = (gs_memory_chunk_t *)mem;
    return cmem->target;
}

#ifdef DEBUG
void
gs_memory_chunk_dump_memory(const gs_memory_t *mem)
{
    gs_memory_chunk_t *cmem = (gs_memory_chunk_t *)mem;
    chunk_mem_node_t *head = cmem->head_mo_chunk;	/* dump multiple object chunks first */
    chunk_mem_node_t *current;
    chunk_mem_node_t *next;
    int i;

    dmprintf2(mem, "chunk_dump_memory: current used=%lu, max_used=%lu\n", cmem->used, cmem->max_used);
    if (cmem->in_use != 0)
        dmprintf1(mem, "*** this memory allocator is not idle, used for: %s\n",
                  cmem->in_use < 0 ? "free" : "alloc");
    for (i=0; i<2; i++) {
        current = head;
        while ( current != NULL ) {
            if (current->objlist != NULL) {
                chunk_obj_node_t *obj;

                for (obj= current->objlist; obj != NULL; obj=obj->next)
                    dmprintf4(mem, "chunk_mem leak, obj=0x%lx, size=%d, type=%s, sequence#=%ld\n",
                              (ulong)obj, obj->size, obj->type->sname, obj->sequence);
            }
            next = current->next;
            current = next;
        }
        head = cmem->head_so_chunk;	/* switch to single object chunk list */
    }
}
#endif

/* -------- Private members --------- */

/* Note that all of the data is 'immovable' and is opaque to the base allocator */
/* thus even if it is a GC type of allocator, no GC functions will be applied   */
/* All allocations are done in the target */

/* Procedures */

static void
chunk_mem_node_free_all_remaining(gs_memory_chunk_t *cmem)
{
    chunk_mem_node_t *head = cmem->head_mo_chunk;	/* Free multiple object chunk nodes first */
    gs_memory_t * const target = cmem->target;
    chunk_mem_node_t *current;
    chunk_mem_node_t *next;
    int i;

#ifdef DEBUG
    if (cmem->in_use != 0)
        dmprintf1(target, "*** chunk_mem_node_free_all_remaining: this memory allocator is not idle, used for: %s\n",
                  cmem->in_use < 0 ? "free" : "alloc");
#endif
    for (i=0; i<2; i++) {
        current = head;
        while ( current != NULL ) {
            next = current->next;
            gs_free_object(target, current, "chunk_mem_node_remove");
            current = next;
        }
        cmem->head_mo_chunk = NULL;
        head = cmem->head_so_chunk;	/* switch to single object chunk list */
    }
    cmem->head_so_chunk = NULL;
}

static void
chunk_free_all(gs_memory_t * mem, uint free_mask, client_name_t cname)
{
    gs_memory_chunk_t * const cmem = (gs_memory_chunk_t *)mem;
    gs_memory_t * const target = cmem->target;

#ifdef DEBUG
    if (cmem->in_use != 0)
        dmprintf1(target, "*** chunk_free_all: this memory allocator is not idle, used for: %s\n",
                  cmem->in_use < 0 ? "free" : "alloc");
#endif
    /* Only free the structures and the allocator itself. */
    if (mem->stable_memory) {
        if (mem->stable_memory != mem)
            gs_memory_free_all(mem->stable_memory, free_mask, cname);
        if (free_mask & FREE_ALL_ALLOCATOR)
            mem->stable_memory = 0;
    }
    if (free_mask & FREE_ALL_DATA) {
        chunk_mem_node_free_all_remaining(cmem);
    }
    if (free_mask & FREE_ALL_STRUCTURES) {
        cmem->target = 0;
    }
    if (free_mask & FREE_ALL_ALLOCATOR)
        gs_free_object(target, cmem, cname);
}

extern const gs_memory_struct_type_t st_bytes;

/* round up objects to make sure we have room for a header left */
inline static uint
round_up_to_align(uint size)
{
    uint num_node_headers = (size + SIZEOF_ROUND_ALIGN(chunk_obj_node_t) - 1) / SIZEOF_ROUND_ALIGN(chunk_obj_node_t);

    return num_node_headers * SIZEOF_ROUND_ALIGN(chunk_obj_node_t);
}

#ifdef MEMENTO
/* If we're using memento, make ALL objects single objects (i.e. put them all
 * in their own chunk. */
#define IS_SINGLE_OBJ_SIZE(chunk_size) (1)
#else
#define IS_SINGLE_OBJ_SIZE(chunk_size) \
    (chunk_size > (CHUNK_SIZE>>1))
#endif
#define MULTIPLE_OBJ_CHUNK_SIZE \
    (sizeof(chunk_mem_node_t) + round_up_to_align(CHUNK_SIZE))

/* return -1 on error, 0 on success */
static int
chunk_mem_node_add(gs_memory_chunk_t *cmem, uint size_needed, bool is_multiple_object_chunk,
                        chunk_mem_node_t **newchunk, client_name_t cname)
{
    chunk_mem_node_t *node;
    gs_memory_t *target = cmem->target;
    /* Allocate enough for the chunk header, and the size_needed */
    /* The size needed already includes the object header from caller */
    /* and is already rounded up to the obj_node_t sized elements */
    uint chunk_size = size_needed + SIZEOF_ROUND_ALIGN(chunk_mem_node_t);

    /* caller tells us whether or not to use a single object chunk */
    if (is_multiple_object_chunk && (chunk_size < MULTIPLE_OBJ_CHUNK_SIZE)) {
        chunk_size = MULTIPLE_OBJ_CHUNK_SIZE;	/* the size for collections of objects */
        is_multiple_object_chunk = true;
    } else
        is_multiple_object_chunk = false;

    *newchunk = NULL;
    node = (chunk_mem_node_t *)gs_alloc_bytes_immovable(target, chunk_size, cname);
    if (node == NULL)
        return -1;
    cmem->used += chunk_size;
    if (cmem->used > cmem->max_used)
        cmem->max_used = cmem->used;
    node->size = chunk_size;	/* how much we allocated */
    node->largest_free = chunk_size - SIZEOF_ROUND_ALIGN(chunk_mem_node_t);
    node->is_multiple_object_chunk = is_multiple_object_chunk;
    node->objlist = NULL;
    node->freelist = (chunk_freelist_node_t *)((byte *)(node) + SIZEOF_ROUND_ALIGN(chunk_mem_node_t));
    node->freelist->next = NULL;
    node->freelist->size = node->largest_free;

    /* Put the node at the head of the list (so=single object, mo=multiple object) */
    /* only multiple objects will be have any room in them */
    node->prev = NULL;			/* head of list always has prev == NULL */
    if (is_multiple_object_chunk) {
        if (cmem->head_mo_chunk == NULL) {
            cmem->head_mo_chunk = node;
            node->next = NULL;
        } else {
            node->next = cmem->head_mo_chunk;
            cmem->head_mo_chunk->prev = node;
            cmem->head_mo_chunk = node;
        }
    } else {
        if (cmem->head_so_chunk == NULL) {
            cmem->head_so_chunk = node;
            node->next = NULL;
        } else {
            node->next = cmem->head_so_chunk;
            cmem->head_so_chunk->prev = node;
            cmem->head_so_chunk = node;
        }
    }

    *newchunk = node;	    /* return the chunk we just allocated */
    return 0;
}

static int
chunk_mem_node_remove(gs_memory_chunk_t *cmem, chunk_mem_node_t *addr)
{
    chunk_mem_node_t **p_head = addr->is_multiple_object_chunk ?
                &(cmem->head_mo_chunk) : &(cmem->head_so_chunk);
    chunk_mem_node_t *head = *p_head;
    gs_memory_t * const target = cmem->target;

    cmem->used -= addr->size;
#ifdef DEBUG
#endif

    /* check the head first */
    if (head == NULL) {
        dmprintf(target, "FAIL - no nodes to be removed\n" );
        return -1;
    }
    if (head == addr) {
        *p_head = head->next;
        if (head->next != NULL)
            head->next->prev = NULL;
        gs_free_object(target, head, "chunk_mem_node_remove");
    } else {
        chunk_mem_node_t *current;

        current = addr->prev;
        current->next = addr->next;
        if (addr->next != NULL) {
            addr->next->prev = current;
        }
        gs_free_object(target, addr, "chunk_mem_node_remove");

    }
    return 0;
}

/* all of the allocation routines reduce to the this function */
static byte *
chunk_obj_alloc(gs_memory_t *mem, uint size, gs_memory_type_ptr_t type, client_name_t cname)
{
    gs_memory_chunk_t *cmem = (gs_memory_chunk_t *)mem;
    chunk_mem_node_t *head = cmem->head_mo_chunk;	/* we only scan chunks with space in them */
    uint newsize, free_size;
    chunk_obj_node_t *newobj = NULL;
    chunk_freelist_node_t *free_obj, *prev_free, *new_free;
    chunk_mem_node_t *current = NULL;
    bool rescan_free_list = false;
    bool is_multiple_object_size;

#ifdef DEBUG
    if (cmem->in_use != 0)
        dmprintf1(mem, "*** chunk_obj_alloc: this memory allocator is not idle, used for: %s\n",
                cmem->in_use < 0 ? "free" : "alloc");
    cmem->in_use = 1;	/* alloc */
#endif
    newsize = round_up_to_align(size + SIZEOF_ROUND_ALIGN(chunk_obj_node_t));	/* space we will need */
    is_multiple_object_size = ! IS_SINGLE_OBJ_SIZE(newsize);

    if ( is_multiple_object_size ) {
        /* Search the multiple object chunks for one with a large enough free area */
        for (current = head; current != NULL; current = current->next) {
            if ( current->largest_free >= newsize)
                break;
        }
    }
    if (current == NULL) {
        /* No chunks with enough space or size makes this a single object, allocate one */
        /* If this object is in its own chunk, use the caller's cname when allocating the chunk */
        if (chunk_mem_node_add(cmem, newsize, is_multiple_object_size, &current,
                               is_multiple_object_size ? "chunk_mem_node_add" : cname) < 0) {
#ifdef DEBUG
        if (gs_debug_c('a'))
            dmlprintf1(mem, "[a+]chunk_obj_alloc(chunk_mem_node_add)(%u) Failed.\n", size);
            cmem->in_use = 0;	/* idle */
#endif
            return NULL;
        }
    }
    /* Find the first free area in the current chunk that is big enough */
    /* LATER: might be better to find the 'best fit' */
    prev_free = NULL;		/* NULL means chunk */
    for (free_obj = current->freelist; free_obj != NULL; free_obj=free_obj->next) {
        if (free_obj->size >= newsize)
            break;
        prev_free = free_obj;	/* keep track so we can update link */
    }

    if (free_obj == NULL) {
        dmprintf2(mem, "largest_free value = %d is too large, cannot find room for size = %d\n",
            current->largest_free, newsize);
#ifdef DEBUG
        cmem->in_use = 0;	/* idle */
#endif
        return NULL;
    }

    /* If this free object's size == largest_free, we'll have to re-scan */
    rescan_free_list = current->is_multiple_object_chunk && free_obj->size == current->largest_free;

    /* Make an object in the free_obj we found above, reducing it's size */
    /* and adjusting the free list preserving alignment	*/
    newobj = (chunk_obj_node_t *)free_obj;
    free_size = free_obj->size - newsize;	/* amount remaining */
    new_free = (chunk_freelist_node_t *)((byte *)(free_obj) + newsize);	/* start of remaining free area */

    if (free_size >= sizeof(chunk_obj_node_t)) {
        if (prev_free != NULL)
            prev_free->next = new_free;
        else
            current->freelist = new_free;
        new_free->next = free_obj->next;
        new_free->size = free_size;
        if (rescan_free_list && current->freelist->next == NULL) {
            /* Object was allocated from the single freelist entry, so adjust largest */
            current->largest_free = free_size;
            rescan_free_list = false;
        }
    } else {
       /* Not enough space remaining, just skip around it */
        if (prev_free != NULL)
            prev_free->next = free_obj->next;
        else
            current->freelist = free_obj->next;
    }



#ifdef DEBUG
    memset((byte *)(newobj) + SIZEOF_ROUND_ALIGN(chunk_obj_node_t), 0xa1, newsize - SIZEOF_ROUND_ALIGN(chunk_obj_node_t));
    memset((byte *)(newobj) + SIZEOF_ROUND_ALIGN(chunk_obj_node_t), 0xac, size);
    newobj->sequence = cmem->sequence_counter++;
#endif

    newobj->next = current->objlist;	/* link to start of list */
    newobj->chunk = current;
    newobj->prev = NULL;
    if (current->objlist)
        current->objlist->prev = newobj;
    current->objlist = newobj;
    newobj->size = size;		/* client requested size */
    newobj->type = type;		/* and client desired type */

    /* If we flagged for re-scan to find the new largest_free, do it now */
    if (rescan_free_list) {
        current->largest_free = 0;
        for (free_obj = current->freelist; free_obj != NULL; free_obj=free_obj->next)
            if (free_obj->size > current->largest_free)
                current->largest_free = free_obj->size;
    }

    /* return the client area of the object we allocated */
#ifdef DEBUG
    if (gs_debug_c('A'))
        dmlprintf3(mem, "[a+]chunk_obj_alloc (%s)(%u) = 0x%lx: OK.\n",
                   client_name_string(cname), size, (ulong) newobj);
    cmem->in_use = 0; 	/* idle */
#endif
    return (byte *)(newobj) + SIZEOF_ROUND_ALIGN(chunk_obj_node_t);
}

static byte *
chunk_alloc_bytes_immovable(gs_memory_t * mem, uint size, client_name_t cname)
{
    return chunk_obj_alloc(mem, size, &st_bytes, cname);
}

static byte *
chunk_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    return chunk_obj_alloc(mem, size, &st_bytes, cname);
}

static void *
chunk_alloc_struct_immovable(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
                         client_name_t cname)
{
    return chunk_obj_alloc(mem, pstype->ssize, pstype, cname);
}

static void *
chunk_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
               client_name_t cname)
{
    return chunk_obj_alloc(mem, pstype->ssize, pstype, cname);
}

static byte *
chunk_alloc_byte_array_immovable(gs_memory_t * mem, uint num_elements,
                             uint elt_size, client_name_t cname)
{
    return chunk_alloc_bytes(mem, num_elements * elt_size, cname);
}

static byte *
chunk_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
                   client_name_t cname)
{
    return chunk_alloc_bytes(mem, num_elements * elt_size, cname);
}

static void *
chunk_alloc_struct_array_immovable(gs_memory_t * mem, uint num_elements,
                           gs_memory_type_ptr_t pstype, client_name_t cname)
{
    return chunk_obj_alloc(mem, num_elements * pstype->ssize, pstype, cname);
}

static void *
chunk_alloc_struct_array(gs_memory_t * mem, uint num_elements,
                     gs_memory_type_ptr_t pstype, client_name_t cname)
{
    return chunk_obj_alloc(mem, num_elements * pstype->ssize, pstype, cname);
}

static void *
chunk_resize_object(gs_memory_t * mem, void *ptr, uint new_num_elements, client_name_t cname)
{
    /* This isn't particularly efficient, but it is rarely used */
    chunk_obj_node_t *obj = (chunk_obj_node_t *)(((byte *)ptr) - SIZEOF_ROUND_ALIGN(chunk_obj_node_t));
    ulong new_size = (obj->type->ssize * new_num_elements);
    ulong old_size = obj->size;
    /* get the type from the old object */
    gs_memory_type_ptr_t type = obj->type;
    void *new_ptr;
    gs_memory_chunk_t *cmem = (gs_memory_chunk_t *)mem;
    ulong save_max_used = cmem->max_used;

    if (new_size == old_size)
        return ptr;
    if ((new_ptr = chunk_obj_alloc(mem, new_size, type, cname)) == 0)
        return 0;
    memcpy(new_ptr, ptr, min(old_size, new_size));
    chunk_free_object(mem, ptr, cname);
    cmem->max_used = save_max_used;
    if (cmem->used > cmem->max_used)
        cmem->max_used = cmem->used;
    return new_ptr;
}

static void
chunk_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    gs_memory_chunk_t * const cmem = (gs_memory_chunk_t *)mem;

    if (ptr == NULL )
        return;
    {
        /* back up to obj header */
        int obj_node_size = SIZEOF_ROUND_ALIGN(chunk_obj_node_t);
        chunk_obj_node_t *obj = (chunk_obj_node_t *)(((byte *)ptr) - obj_node_size);
        struct_proc_finalize((*finalize)) = obj->type->finalize;
        chunk_mem_node_t *current;
        chunk_freelist_node_t *free_obj, *prev_free, *new_free;
        chunk_obj_node_t *prev_obj;
        /* space we will free */
        uint freed_size = round_up_to_align(obj->size + obj_node_size);

        if ( finalize != NULL )
            finalize(mem, ptr);
#ifdef DEBUG
        if (cmem->in_use != 0)
            dmprintf1(cmem->target,
                      "*** chunk_free_object: this memory allocator is not idle, used for: %s\n",
                      cmem->in_use < 0 ? "free" : "alloc");
        cmem->in_use = -1;	/* free */
#endif
        /* finalize may change the head_**_chunk doing free of stuff */
        current = obj->chunk;

        /* For large objects, they were given their own chunk -- just remove the node */
        /* For multiple object chunks, we can remove the node if the objlist will become empty */
        if (current->is_multiple_object_chunk == 0 || current->objlist->next == NULL) {
            chunk_mem_node_remove(cmem, current);
#ifdef DEBUG
            cmem->in_use = 0; 	/* idle */
#endif
            return;
        }

        prev_obj = obj->prev;
        /* link around the object being freed */
        if (prev_obj == NULL) {
            current->objlist = obj->next;
            if (obj->next)
                obj->next->prev = NULL;
        } else {
            prev_obj->next = obj->next;
            if (obj->next)
                obj->next->prev = prev_obj;
        }

        if_debug3m('A', cmem->target, "[a-]chunk_free_object(%s) 0x%lx(%u)\n",
                  client_name_string(cname), (ulong) ptr, obj->size);

        /* Add this object's space (including the header) to the free list */
        /* Scan free list to find where this element goes */
        prev_free = NULL;
        new_free = (chunk_freelist_node_t *)obj;
        new_free->size = freed_size;        /* set the size of this free block */
        for (free_obj = current->freelist; free_obj != NULL; free_obj = free_obj->next) {
            if (new_free < free_obj)
                break;
            prev_free = free_obj;
        }
        if (prev_free == NULL) {
            /* this object is before any other free objects */
            new_free->next = current->freelist;
            current->freelist = new_free;
        } else {
            new_free->next = free_obj;
            prev_free->next = new_free;
        }
        /* If the end of this object is adjacent to the next free space,
         * merge the two. Next we'll merge with predecessor (prev_free)
         */
        if (free_obj != NULL) {
            byte *after_obj = (byte*)(new_free) + freed_size;

            if (free_obj <= (chunk_freelist_node_t *)after_obj) {
                /* Object is adjacent to following free space block -- merge it */
                new_free->next = free_obj->next;	/* link around the one being absorbed */
                /* as noted elsewhere, freelist node size is entire block size */
                new_free->size = (byte *)(free_obj) - (byte *)(new_free) + free_obj->size;
            }
        }
        /* the prev_free object precedes this object that is now free,
         * it _may_ be adjacent
         */
        if (prev_free != NULL) {
            byte *after_free = (byte*)(prev_free) + prev_free->size;

            if (new_free <= (chunk_freelist_node_t *)after_free) {
                /* Object is adjacent to prior free space block -- merge it */
                /* NB: this is the common case with LIFO alloc-free patterns */
                /* (LIFO: Last-allocated, first freed) */
                prev_free->size = (byte *)(new_free) - (byte *)(prev_free) + new_free->size;
                prev_free->next = new_free->next;		/* link around 'new_free' area */
                new_free = prev_free;
            }
        }
#ifdef DEBUG
memset((byte *)(new_free) + SIZEOF_ROUND_ALIGN(chunk_freelist_node_t), 0xf1,
       new_free->size - SIZEOF_ROUND_ALIGN(chunk_freelist_node_t));
#endif
        if (current->largest_free < new_free->size)
            current->largest_free = new_free->size;

#ifdef DEBUG
        cmem->in_use = 0; 	/* idle */
#endif
    }
}

static byte *
chunk_alloc_string_immovable(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    /* we just alloc bytes here */
    return chunk_alloc_bytes(mem, nbytes, cname);
}

static byte *
chunk_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    /* we just alloc bytes here */
    return chunk_alloc_bytes(mem, nbytes, cname);
}

static byte *
chunk_resize_string(gs_memory_t * mem, byte * data, uint old_num, uint new_num,
                client_name_t cname)
{
    /* just resize object - ignores old_num */
    return chunk_resize_object(mem, data, new_num, cname);
}

static void
chunk_free_string(gs_memory_t * mem, byte * data, uint nbytes,
              client_name_t cname)
{
    chunk_free_object(mem, data, cname);
}

static void
chunk_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    gs_memory_chunk_t *cmem = (gs_memory_chunk_t *)mem;
    chunk_mem_node_t *current = cmem->head_mo_chunk;	/* we only scan chunks with space in them */
    chunk_freelist_node_t *free_obj;		/* free list object node */
    int tot_free = 0;

    pstat->allocated = cmem->used;
    /* Scan all chunks for free space to calculate the actual amount 'used' */
    for ( ; current != NULL; current = current->next) {
        for (free_obj = current->freelist; free_obj != NULL; free_obj=free_obj->next)
            tot_free += free_obj->size;
    }
    pstat->used = cmem->used - tot_free;
    pstat->max_used = cmem->max_used;

    pstat->is_thread_safe = false;	/* this allocator does not have an internal mutex */
}

static gs_memory_t *
chunk_stable(gs_memory_t * mem)
{
    return mem;
}

static void
chunk_enable_free(gs_memory_t * mem, bool enable)
{
}

static void
chunk_consolidate_free(gs_memory_t *mem)
{
}

/* aceesors to get size and type given the pointer returned to the client */
static uint
chunk_object_size(gs_memory_t * mem, const void *ptr)
{
    chunk_obj_node_t *obj = (chunk_obj_node_t *)(((byte *)ptr) - SIZEOF_ROUND_ALIGN(chunk_obj_node_t));

    return obj->size;
}

static gs_memory_type_ptr_t
chunk_object_type(const gs_memory_t * mem, const void *ptr)
{
    chunk_obj_node_t *obj = (chunk_obj_node_t *)(((byte *)ptr) - SIZEOF_ROUND_ALIGN(chunk_obj_node_t));
    return obj->type;
}

static int
chunk_register_root(gs_memory_t * mem, gs_gc_root_t * rp, gs_ptr_type_t ptype,
                 void **up, client_name_t cname)
{
    return 0;
}

static void
chunk_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp, client_name_t cname)
{
}

#ifdef DEBUG

#define A(obj, size) \
    if ((obj = gs_alloc_bytes(cmem, size, "chunk_alloc_unit_test")) == NULL) { \
        dmprintf(cmem, "chunk alloc failed\n"); \
        return_error(gs_error_VMerror); \
    }

#define F(obj) \
    gs_free_object(cmem, obj, "chunk_alloc_unit_test");

int
chunk_allocator_unit_test(gs_memory_t *mem)
{
    int code;
    gs_memory_t *cmem;
    byte *obj1, *obj2, *obj3, *obj4, *obj5, *obj6, *obj7;

    if ((code = gs_memory_chunk_wrap(&cmem, mem )) < 0) {
        dmprintf1(cmem, "chunk_wrap returned error code: %d\n", code);
        return code;
    }

    /* Allocate a large object */
    A(obj1, 80000);
    F(obj1);
    A(obj1, 80000);

    A(obj2, 3);
    A(obj3, 7);
    A(obj4, 15);
    A(obj5, 16);
    A(obj6, 16);
    A(obj7, 16);

    F(obj2);
    F(obj1);
    F(obj5);
    F(obj4);
    F(obj6);
    F(obj7);
    F(obj3);

    /* cleanup */
    gs_memory_chunk_release(cmem);
    return 0;
}

#endif /* DEBUG */
