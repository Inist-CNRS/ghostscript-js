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


/* XPS interpreter - document parsing */

#include "ghostxps.h"

#include <expat.h>

xps_part_t *
xps_new_part(xps_context_t *ctx, const char *name, int size)
{
    xps_part_t *part;

    part = xps_alloc(ctx, sizeof(xps_part_t));
    if (!part) {
        gs_throw(gs_error_VMerror, "out of memory: xps_new_part\n");
        return NULL;
    }
    part->name = xps_strdup(ctx, name);
    part->size = size;
    part->data = xps_alloc(ctx, size);
    if (!part->data) {
        xps_free(ctx, part);
        gs_throw(gs_error_VMerror, "out of memory: xps_new_part\n");
        return NULL;
    }

    return part;
}

void
xps_free_part(xps_context_t *ctx, xps_part_t *part)
{
    xps_free(ctx, part->name);
    xps_free(ctx, part->data);
    xps_free(ctx, part);
}

/*
 * The FixedDocumentSequence and FixedDocument parts determine
 * which parts correspond to actual pages, and the page order.
 */

void
xps_debug_fixdocseq(xps_context_t *ctx)
{
    xps_document_t *fixdoc = ctx->first_fixdoc;
    xps_page_t *page = ctx->first_page;

    if (ctx->start_part)
        dmprintf1(ctx->memory, "start part %s\n", ctx->start_part);

    while (fixdoc)
    {
        dmprintf1(ctx->memory, "fixdoc %s\n", fixdoc->name);
        fixdoc = fixdoc->next;
    }

    while (page)
    {
        dmprintf3(ctx->memory, "page %s w=%d h=%d\n", page->name, page->width, page->height);
        page = page->next;
    }
}

static void
xps_add_fixed_document(xps_context_t *ctx, char *name)
{
    xps_document_t *fixdoc;

    /* Check for duplicates first */
    for (fixdoc = ctx->first_fixdoc; fixdoc; fixdoc = fixdoc->next)
        if (!strcmp(fixdoc->name, name))
            return;

    if_debug1m('|', ctx->memory, "doc: adding fixdoc %s\n", name);

    fixdoc = xps_alloc(ctx, sizeof(xps_document_t));
    if (!fixdoc) {
        gs_throw(gs_error_VMerror, "out of memory: xps_add_fixed_document\n");
        return;
    }
    fixdoc->name = xps_strdup(ctx, name);
    fixdoc->next = NULL;

    if (!ctx->first_fixdoc)
    {
        ctx->first_fixdoc = fixdoc;
        ctx->last_fixdoc = fixdoc;
    }
    else
    {
        ctx->last_fixdoc->next = fixdoc;
        ctx->last_fixdoc = fixdoc;
    }
}

void
xps_free_fixed_documents(xps_context_t *ctx)
{
    xps_document_t *node = ctx->first_fixdoc;
    while (node)
    {
        xps_document_t *next = node->next;
        xps_free(ctx, node->name);
        xps_free(ctx, node);
        node = next;
    }
    ctx->first_fixdoc = NULL;
    ctx->last_fixdoc = NULL;
}

static void
xps_add_fixed_page(xps_context_t *ctx, char *name, int width, int height)
{
    xps_page_t *page;

    /* Check for duplicates first */
    for (page = ctx->first_page; page; page = page->next)
        if (!strcmp(page->name, name))
            return;

    if_debug1m('|', ctx->memory, "doc: adding page %s\n", name);

    page = xps_alloc(ctx, sizeof(xps_page_t));
    if (!page) {
        gs_throw(gs_error_VMerror, "out of memory: xps_add_fixed_page\n");
        return;
    }
    page->name = xps_strdup(ctx, name);
    page->width = width;
    page->height = height;
    page->next = NULL;

    if (!ctx->first_page)
    {
        ctx->first_page = page;
        ctx->last_page = page;
    }
    else
    {
        ctx->last_page->next = page;
        ctx->last_page = page;
    }
}

void
xps_free_fixed_pages(xps_context_t *ctx)
{
    xps_page_t *node = ctx->first_page;
    while (node)
    {
        xps_page_t *next = node->next;
        xps_free(ctx, node->name);
        xps_free(ctx, node);
        node = next;
    }
    ctx->first_page = NULL;
    ctx->last_page = NULL;
}

/*
 * Parse the fixed document sequence structure and _rels/.rels to find the
 * start part. We hook up unique expat handlers for this, since we don't need
 * the full document model.
 */

static void
xps_parse_metadata_imp(void *zp, char *name, char **atts)
{
    xps_context_t *ctx = zp;
    int i;

    if (!strcmp(name, "Relationship"))
    {
        char tgtbuf[1024];
        char *target = NULL;
        char *type = NULL;
        char *id = NULL;

        for (i = 0; atts[i]; i += 2)
        {
            if (!strcmp(atts[i], "Target"))
                target = atts[i + 1];
            if (!strcmp(atts[i], "Type"))
                type = atts[i + 1];
            if (!strcmp(atts[i], "Id"))
                id = atts[i + 1];
        }

        if (target && type)
        {
            xps_absolute_path(tgtbuf, ctx->base_uri, target, sizeof tgtbuf);
            if (!strcmp(type, REL_START_PART) ||
                !strcmp(type, REL_START_PART_OXPS))
                ctx->start_part = xps_strdup(ctx, tgtbuf);
            if (!id)
                gs_warn1("missing relationship Id for %s", target);
        }
    }

    if (!strcmp(name, "DocumentReference"))
    {
        char *source = NULL;
        char srcbuf[1024];

        for (i = 0; atts[i]; i += 2)
        {
            if (!strcmp(atts[i], "Source"))
                source = atts[i + 1];
        }

        if (source)
        {
            xps_absolute_path(srcbuf, ctx->base_uri, source, sizeof srcbuf);
            xps_add_fixed_document(ctx, srcbuf);
        }
    }

    if (!strcmp(name, "PageContent"))
    {
        char *source = NULL;
        char srcbuf[1024];
        int width = 0;
        int height = 0;

        for (i = 0; atts[i]; i += 2)
        {
            if (!strcmp(atts[i], "Source"))
                source = atts[i + 1];
            if (!strcmp(atts[i], "Width"))
                width = atoi(atts[i + 1]);
            if (!strcmp(atts[i], "Height"))
                height = atoi(atts[i + 1]);
        }

        if (source)
        {
            xps_absolute_path(srcbuf, ctx->base_uri, source, sizeof srcbuf);
            xps_add_fixed_page(ctx, srcbuf, width, height);
        }
    }
}

int
xps_parse_metadata(xps_context_t *ctx, xps_part_t *part)
{
    XML_Parser xp;
    int code;
    char buf[1024];
    char *s;

    /* Save directory name part */
    gs_strlcpy(buf, part->name, sizeof buf);
    s = strrchr(buf, '/');
    if (s)
        s[0] = 0;

    /* _rels parts are voodoo: their URI references are from
     * the part they are associated with, not the actual _rels
     * part being parsed.
     */
    s = strstr(buf, "/_rels");
    if (s)
        *s = 0;

    ctx->base_uri = buf;
    ctx->part_uri = part->name;

    xp = XML_ParserCreate(NULL);
    if (!xp)
        return gs_throw(-1, "cannot create XML parser");

    XML_SetUserData(xp, ctx);
    XML_SetParamEntityParsing(xp, XML_PARAM_ENTITY_PARSING_NEVER);
    XML_SetStartElementHandler(xp, (XML_StartElementHandler)xps_parse_metadata_imp);

    code = XML_Parse(xp, (char*)part->data, part->size, 1);

    XML_ParserFree(xp);

    ctx->base_uri = NULL;
    ctx->part_uri = NULL;

    if (code == 0)
        return gs_throw1(-1, "cannot parse XML in part: %s", part->name);

    return 0;
}
