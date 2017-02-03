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


/* Common structures for the filling algorithm and dropout prevention. */

#ifndef gxfill_INCLUDED
#  define gxfill_INCLUDED

enum
{
    DIR_UP = 1,
    DIR_HORIZONTAL = 0,
    DIR_DOWN = -1,
    DIR_OUT_OF_Y_RANGE = 2
};

/* Define the structure for keeping track of active lines. */
#ifndef active_line_DEFINED
#  define active_line_DEFINED
typedef struct active_line_s active_line;
#endif
struct active_line_s {
    gs_fixed_point start;	/* x,y where line starts */
    gs_fixed_point end; 	/* x,y where line ends */
    gs_fixed_point diff;	/* end - start */
    fixed y_fast_max;		/* can do x_at_y in fixed point */
                                /* if y <= y_fast_max */
    fixed num_adjust;		/* 0 if diff.x >= 0, -diff.y + epsilon if */
                                /* diff.x < 0 and division truncates */
#if ARCH_DIV_NEG_POS_TRUNCATES
    /* neg/pos truncates, we must bias the numberator. */
#  define SET_NUM_ADJUST(alp) \
    (alp)->num_adjust =\
      ((alp)->diff.x >= 0 ? 0 : -(alp)->diff.y + fixed_epsilon)
#  define ADD_NUM_ADJUST(num, alp) ((num) + (alp)->num_adjust)
#  define MAX_MINUS_NUM_ADJUST(alp) ADD_NUM_ADJUST(max_fixed, alp)
#else
    /* neg/pos takes the floor, no special action is needed. */
#  define SET_NUM_ADJUST(alp) DO_NOTHING
#  define ADD_NUM_ADJUST(num, alp) (num)
#  define MAX_MINUS_NUM_ADJUST(alp) max_fixed
#endif
#define SET_AL_POINTS(alp, startp, endp)\
  BEGIN\
    (alp)->diff.y = (endp).y - (startp).y;\
    (alp)->diff.x = (endp).x - (startp).x;\
    SET_NUM_ADJUST(alp);\
    (alp)->y_fast_max = MAX_MINUS_NUM_ADJUST(alp) /\
      (((alp)->diff.x >= 0 ? (alp)->diff.x : -(alp)->diff.x) | 1) +\
      (startp).y;\
    (alp)->start = startp, (alp)->end = endp;\
  END
    /*
     * We know that alp->start.y <= yv <= alp->end.y, because the fill loop
     * guarantees that the only lines being considered are those with this
     * property.
     */
#define AL_X_AT_Y(alp, yv)\
  ((yv) == (alp)->end.y ? (alp)->end.x :\
   ((yv) <= (alp)->y_fast_max ?\
    ADD_NUM_ADJUST(((yv) - (alp)->start.y) * (alp)->diff.x, alp) / (alp)->diff.y :\
    (INCR_EXPR(slow_x),\
     fixed_mult_quo((alp)->diff.x, (yv) - (alp)->start.y, (alp)->diff.y))) +\
   (alp)->start.x)
    fixed x_current;		/* current x position */
    fixed x_next;		/* x position at end of band */
    const segment *pseg;	/* endpoint of this line */
    int direction;		/* direction of line segment */
    bool monotonic_x;		/* "false" means "don't know"; only for scanline. */
    bool monotonic_y;		/* "false" means "don't know"; only for scanline. */
    gx_flattened_iterator fi;
    bool more_flattened;
    int contour_count;
/*
 * "Pending" lines (not reached in the Y ordering yet) use next and prev
 * to order lines by increasing starting Y.  "Active" lines (being scanned)
 * use next and prev to order lines by increasing current X, or if the
 * current Xs are equal, by increasing final X.
 */
    active_line *prev, *next;
/* Link together active_lines allocated individually */
    active_line *alloc_next;
};

typedef struct fill_options_s {
    fixed ymin, ymax;
    const gx_device_color * pdevc;
    gs_logical_operation_t lop;
    bool fill_direct;
    fixed fixed_flat;
    bool fill_by_trapezoids;
    fixed adjust_left, adjust_right;
    fixed adjust_below, adjust_above;
    gx_device *dev;
    const gs_fixed_rect * pbox;
    bool is_spotan;
    int rule;
    dev_proc_fill_rectangle((*fill_rect));
    dev_proc_fill_trapezoid((*fill_trap));
} fill_options;

/* Line list structure */
#ifndef line_list_DEFINED
#  define line_list_DEFINED
typedef struct line_list_s line_list;
#endif
struct line_list_s {
    gs_memory_t *memory;
    active_line *active_area;	/* allocated active_line list */
    active_line *next_active;	/* next allocation slot */
    active_line *limit; 	/* limit of local allocation */
    int close_count;		/* # of added closing lines */
    active_line *y_list;	/* Y-sorted list of pending lines */
    active_line *y_line;	/* most recently inserted line */
    active_line x_head; 	/* X-sorted list of active lines */
#define x_list x_head.next
    active_line *h_list0, *h_list1; /* lists of horizontal lines for y, y1 */
    int *windings;
    int bbox_left, bbox_width;
    int main_dir;
    fixed y_break;
    const fill_options * const fo;
    int contour_count;
    /* Put the arrays last so the scalars will have */
    /* small displacements. */
    /* Allocate a few active_lines locally */
    /* to avoid round trips through the allocator. */
#if ARCH_SMALL_MEMORY
#  define MAX_LOCAL_ACTIVE 6	/* don't overburden the stack */
#  define MAX_LOCAL_SECTION 50
#else
#  define MAX_LOCAL_ACTIVE 20
#  define MAX_LOCAL_SECTION 100
#endif
    active_line local_active[MAX_LOCAL_ACTIVE];
    int local_windings[MAX_LOCAL_ACTIVE];
};

#define LOOP_FILL_RECTANGLE_DIRECT(fo, x, y, w, h)\
  (/*fo->fill_direct*/FILL_DIRECT ?\
   (fo)->fill_rect((fo)->dev, x, y, w, h, (fo)->pdevc->colors.pure) :\
   gx_fill_rectangle_device_rop(x, y, w, h, (fo)->pdevc, (fo)->dev, (fo)->lop))

/* ---------------- Statistics ---------------- */

#if defined(DEBUG) && !defined(GS_THREADSAFE)
struct stats_fill_s {
    long
        fill, fill_alloc, y_up, y_down, horiz, x_step, slow_x, iter, find_y,
        band, band_step, band_fill, afill, slant, slant_shallow, sfill,
        mq_cross, cross_slow, cross_low, order, slow_order;
};
typedef struct stats_fill_s stats_fill_t;
extern stats_fill_t stats_fill;

#  define INCR(x) (++(stats_fill.x))
#  define INCR_EXPR(x) INCR(x)
#  define INCR_BY(x,n) (stats_fill.x += (n))
#else
#  define INCR(x) DO_NOTHING
#  define INCR_EXPR(x) discard(0)
#  define INCR_BY(x,n) DO_NOTHING
#endif

#endif /* gxfill_INCLUDED */
