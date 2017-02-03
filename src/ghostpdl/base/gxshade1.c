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


/* Rendering for non-mesh shadings */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsmatrix.h"		/* for gscoord.h */
#include "gscoord.h"
#include "gspath.h"
#include "gsptype2.h"
#include "gxcspace.h"
#include "gxdcolor.h"
#include "gxfarith.h"
#include "gxfixed.h"
#include "gxgstate.h"
#include "gxpath.h"
#include "gxshade.h"
#include "gxdevcli.h"
#include "gxshade4.h"
#include "gsicc_cache.h"

/* ---------------- Function-based shading ---------------- */

typedef struct Fb_frame_s {	/* A rudiment of old code. */
    gs_rect region;
    gs_client_color cc[4];	/* colors at 4 corners */
    int state;
} Fb_frame_t;

typedef struct Fb_fill_state_s {
    shading_fill_state_common;
    const gs_shading_Fb_t *psh;
    gs_matrix_fixed ptm;	/* parameter space -> device space */
    Fb_frame_t frame;
} Fb_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

static inline void
make_other_poles(patch_curve_t curve[4])
{
    int i, j;

    for (i = 0; i < 4; i++) {
        j = (i + 1) % 4;
        curve[i].control[0].x = (curve[i].vertex.p.x * 2 + curve[j].vertex.p.x) / 3;
        curve[i].control[0].y = (curve[i].vertex.p.y * 2 + curve[j].vertex.p.y) / 3;
        curve[i].control[1].x = (curve[i].vertex.p.x + curve[j].vertex.p.x * 2) / 3;
        curve[i].control[1].y = (curve[i].vertex.p.y + curve[j].vertex.p.y * 2) / 3;
        curve[i].straight = true;
    }
}

/* Transform a point with a fixed-point result. */
static void
gs_point_transform2fixed_clamped(const gs_matrix_fixed * pmat,
                         double x, double y, gs_fixed_point * ppt)
{
    gs_point fpt;

    gs_point_transform(x, y, (const gs_matrix *)pmat, &fpt);
    ppt->x = clamp_coord(fpt.x);
    ppt->y = clamp_coord(fpt.y);
}

static int
Fb_fill_region(Fb_fill_state_t * pfs, const gs_fixed_rect *rect)
{
    patch_fill_state_t pfs1;
    patch_curve_t curve[4];
    Fb_frame_t * fp = &pfs->frame;
    int code;

    memcpy(&pfs1, (shading_fill_state_t *)pfs, sizeof(shading_fill_state_t));
    pfs1.Function = pfs->psh->params.Function;
    code = init_patch_fill_state(&pfs1);
    if (code < 0)
        return code;
    pfs1.maybe_self_intersecting = false;
    pfs1.n_color_args = 2;
    pfs1.rect = *rect;
    gs_point_transform2fixed(&pfs->ptm, fp->region.p.x, fp->region.p.y, &curve[0].vertex.p);
    gs_point_transform2fixed(&pfs->ptm, fp->region.q.x, fp->region.p.y, &curve[1].vertex.p);
    gs_point_transform2fixed(&pfs->ptm, fp->region.q.x, fp->region.q.y, &curve[2].vertex.p);
    gs_point_transform2fixed(&pfs->ptm, fp->region.p.x, fp->region.q.y, &curve[3].vertex.p);
    make_other_poles(curve);
    curve[0].vertex.cc[0] = fp->region.p.x;   curve[0].vertex.cc[1] = fp->region.p.y;
    curve[1].vertex.cc[0] = fp->region.q.x;   curve[1].vertex.cc[1] = fp->region.p.y;
    curve[2].vertex.cc[0] = fp->region.q.x;   curve[2].vertex.cc[1] = fp->region.q.y;
    curve[3].vertex.cc[0] = fp->region.p.x;   curve[3].vertex.cc[1] = fp->region.q.y;
    code = patch_fill(&pfs1, curve, NULL, NULL);
    if (term_patch_fill_state(&pfs1))
        return_error(gs_error_unregistered); /* Must not happen. */
    return code;
}

int
gs_shading_Fb_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
                             const gs_fixed_rect * rect_clip,
                             gx_device * dev, gs_gstate * pgs)
{
    const gs_shading_Fb_t * const psh = (const gs_shading_Fb_t *)psh0;
    gs_matrix save_ctm;
    int xi, yi, code;
    float x[2], y[2];
    Fb_fill_state_t state;

    code = shade_init_fill_state((shading_fill_state_t *) & state, psh0, dev, pgs);
    if (code < 0)
        return code;
    state.psh = psh;
    /****** HACK FOR FIXED-POINT MATRIX MULTIPLY ******/
    gs_currentmatrix((gs_gstate *) pgs, &save_ctm);
    gs_concat((gs_gstate *) pgs, &psh->params.Matrix);
    state.ptm = pgs->ctm;
    gs_setmatrix((gs_gstate *) pgs, &save_ctm);
    /* Compute the parameter X and Y ranges. */
    {
        gs_rect pbox;

        gs_bbox_transform_inverse(rect, &psh->params.Matrix, &pbox);
        x[0] = max(pbox.p.x, psh->params.Domain[0]);
        x[1] = min(pbox.q.x, psh->params.Domain[1]);
        y[0] = max(pbox.p.y, psh->params.Domain[2]);
        y[1] = min(pbox.q.y, psh->params.Domain[3]);
    }
    if (x[0] > x[1] || y[0] > y[1]) {
        /* The region is outside the shading area. */
        if (state.icclink != NULL) gsicc_release_link(state.icclink);
        return 0;
    }
    for (xi = 0; xi < 2; ++xi)
        for (yi = 0; yi < 2; ++yi) {
            float v[2];

            v[0] = x[xi], v[1] = y[yi];
            gs_function_evaluate(psh->params.Function, v,
                                 state.frame.cc[yi * 2 + xi].paint.values);
        }
    state.frame.region.p.x = x[0];
    state.frame.region.p.y = y[0];
    state.frame.region.q.x = x[1];
    state.frame.region.q.y = y[1];
    code = Fb_fill_region(&state, rect_clip);
    if (state.icclink != NULL) gsicc_release_link(state.icclink);
    return code;
}

/* ---------------- Axial shading ---------------- */

typedef struct A_fill_state_s {
    const gs_shading_A_t *psh;
    gs_point delta;
    double length;
    double t0, t1;
    double v0, v1, u0, u1;
} A_fill_state_t;
/****** NEED GC DESCRIPTOR ******/

/* Note t0 and t1 vary over [0..1], not the Domain. */

static int
A_fill_region(A_fill_state_t * pfs, patch_fill_state_t *pfs1)
{
    const gs_shading_A_t * const psh = pfs->psh;
    double x0 = psh->params.Coords[0] + pfs->delta.x * pfs->v0;
    double y0 = psh->params.Coords[1] + pfs->delta.y * pfs->v0;
    double x1 = psh->params.Coords[0] + pfs->delta.x * pfs->v1;
    double y1 = psh->params.Coords[1] + pfs->delta.y * pfs->v1;
    double h0 = pfs->u0, h1 = pfs->u1;
    patch_curve_t curve[4];

    gs_point_transform2fixed(&pfs1->pgs->ctm, x0 + pfs->delta.y * h0, y0 - pfs->delta.x * h0, &curve[0].vertex.p);
    gs_point_transform2fixed(&pfs1->pgs->ctm, x1 + pfs->delta.y * h0, y1 - pfs->delta.x * h0, &curve[1].vertex.p);
    gs_point_transform2fixed(&pfs1->pgs->ctm, x1 + pfs->delta.y * h1, y1 - pfs->delta.x * h1, &curve[2].vertex.p);
    gs_point_transform2fixed(&pfs1->pgs->ctm, x0 + pfs->delta.y * h1, y0 - pfs->delta.x * h1, &curve[3].vertex.p);
    curve[0].vertex.cc[0] = pfs->t0; /* The element cc[1] is set to a dummy value against */
    curve[1].vertex.cc[0] = pfs->t1; /* interrupts while an idle priocessing in gxshade.6.c .  */
    curve[2].vertex.cc[0] = pfs->t1;
    curve[3].vertex.cc[0] = pfs->t0;
    curve[0].vertex.cc[1] = 0; /* The element cc[1] is set to a dummy value against */
    curve[1].vertex.cc[1] = 0; /* interrupts while an idle priocessing in gxshade.6.c .  */
    curve[2].vertex.cc[1] = 0;
    curve[3].vertex.cc[1] = 0;
    make_other_poles(curve);
    return patch_fill(pfs1, curve, NULL, NULL);
}

static inline int
gs_shading_A_fill_rectangle_aux(const gs_shading_t * psh0, const gs_rect * rect,
                            const gs_fixed_rect *clip_rect,
                            gx_device * dev, gs_gstate * pgs)
{
    const gs_shading_A_t *const psh = (const gs_shading_A_t *)psh0;
    gs_function_t * const pfn = psh->params.Function;
    gs_matrix cmat;
    gs_rect t_rect;
    A_fill_state_t state;
    patch_fill_state_t pfs1;
    float d0 = psh->params.Domain[0], d1 = psh->params.Domain[1];
    float dd = d1 - d0;
    double t0, t1;
    gs_point dist;
    int code;

    state.psh = psh;
    code = shade_init_fill_state((shading_fill_state_t *)&pfs1, psh0, dev, pgs);
    if (code < 0)
        return code;
    pfs1.Function = pfn;
    pfs1.rect = *clip_rect;
    code = init_patch_fill_state(&pfs1);
    if (code < 0) {
        if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
        return code;
    }
    pfs1.maybe_self_intersecting = false;
    pfs1.function_arg_shift = 1;
    /*
     * Compute the parameter range.  We construct a matrix in which
     * (0,0) corresponds to t = 0 and (0,1) corresponds to t = 1,
     * and use it to inverse-map the rectangle to be filled.
     */
    cmat.tx = psh->params.Coords[0];
    cmat.ty = psh->params.Coords[1];
    state.delta.x = psh->params.Coords[2] - psh->params.Coords[0];
    state.delta.y = psh->params.Coords[3] - psh->params.Coords[1];
    cmat.yx = state.delta.x;
    cmat.yy = state.delta.y;
    cmat.xx = cmat.yy;
    cmat.xy = -cmat.yx;
    gs_bbox_transform_inverse(rect, &cmat, &t_rect);
    t0 = min(max(t_rect.p.y, 0), 1);
    t1 = max(min(t_rect.q.y, 1), 0);
    state.v0 = t0;
    state.v1 = t1;
    state.u0 = t_rect.p.x;
    state.u1 = t_rect.q.x;
    state.t0 = t0 * dd + d0;
    state.t1 = t1 * dd + d0;
    gs_distance_transform(state.delta.x, state.delta.y, &ctm_only(pgs),
                          &dist);
    state.length = hypot(dist.x, dist.y);	/* device space line length */
    code = A_fill_region(&state, &pfs1);
    if (psh->params.Extend[0] && t0 > t_rect.p.y) {
        if (code < 0) {
            if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
            return code;
        }
        /* Use the general algorithm, because we need the trapping. */
        state.v0 = t_rect.p.y;
        state.v1 = t0;
        state.t0 = state.t1 = t0 * dd + d0;
        code = A_fill_region(&state, &pfs1);
    }
    if (psh->params.Extend[1] && t1 < t_rect.q.y) {
        if (code < 0) {
            if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
            return code;
        }
        /* Use the general algorithm, because we need the trapping. */
        state.v0 = t1;
        state.v1 = t_rect.q.y;
        state.t0 = state.t1 = t1 * dd + d0;
        code = A_fill_region(&state, &pfs1);
    }
    if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
    if (term_patch_fill_state(&pfs1))
        return_error(gs_error_unregistered); /* Must not happen. */
    return code;
}

int
gs_shading_A_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
                            const gs_fixed_rect * rect_clip,
                            gx_device * dev, gs_gstate * pgs)
{
    return gs_shading_A_fill_rectangle_aux(psh0, rect, rect_clip, dev, pgs);
}

/* ---------------- Radial shading ---------------- */

/* Some notes on what I have struggled to understand about the following
 * function. This function renders the 'tube' given by interpolating one
 * circle to another.
 *
 * The first circle is at (x0, y0) with radius r0, and has 'color' t0.
 * The other circle is at (x1, y1) with radius r1, and has 'color' t1.
 *
 * We perform this rendering by approximating each quadrant of the 'tube'
 * by a tensor patch. The tensor patch is formed by taking a curve along
 * 1/4 of the circumference of the first circle, a straight line to the
 * equivalent point on the circumference of the second circle, a curve
 * back along the circumference of the second circle, and then a straight
 * line back to where we started.
 *
 * There is additional logic in this function that forms the directions of
 * the curves differently for different quadrants. This is done to ensure
 * that we always paint 'around' the tube from the back towards the front,
 * so we don't get unexpected regions showing though. This is explained more
 * below.
 *
 * The original code here examined the position change between the two
 * circles dx and dy. Based upon this vector it would pick which quadrant/
 * tensor patch to draw first. It would draw the quadrants/tensor patches
 * in anticlockwise order. Presumably this was intended to be done so that
 * the 'top' quadrant would be drawn last.
 *
 * Unfortunately this did not always work; see bug 692513. If the quadrants
 * were rendered in the order 0,1,2,3, the rendering of 1 was leaving traces
 * on top of 0, which was unexpected.
 *
 * I have therefore altered the code slightly; rather than picking a start
 * quadrant and moving anticlockwise, we now draw the 'undermost' quadrant,
 * then the two adjacent quadrants, then the topmost quadrant.
 *
 * For the purposes of explaination, we shall label the octants as below:
 *
 *     \2|1/       and Quadrants as:       |
 *     3\|/0                            Q1 | Q0 
 *    ---+---                          ----+----
 *     4/|\7                            Q2 | Q3
 *     /5|6\                               |
 *
 * We find (dx,dy), the difference between the centres of the circles.
 * We look to see which octant this falls in. Firstly, this tells us which
 * quadrant of the circle we need to draw first (Octant n, starts with
 * Quadrant floor(n/2)). Secondly, it tells us which direction to form the 
 * tensor patch in; we always want to draw from the side 'closest' to
 * dx/dy to the side further away. This ensures that we don't overwrite
 * pixels in the incorrect order as the patch decomposes.
 */
static int
R_tensor_annulus(patch_fill_state_t *pfs,
    double x0, double y0, double r0, double t0,
    double x1, double y1, double r1, double t1)
{
    double dx = x1 - x0, dy = y1 - y0;
    double d = hypot(dx, dy);
    gs_point p0, p1, pc0, pc1;
    int k, j, code, dirn;
    bool inside = 0;

    /* pc0 and pc1 are the centres of the respective circles. */
    pc0.x = x0, pc0.y = y0;
    pc1.x = x1, pc1.y = y1;
    /* Set p0 up so it's a unit vector giving the direction of 90 degrees
     * to the right of the major axis as we move from p0c to p1c. */
    if (r0 + d <= r1 || r1 + d <= r0) {
        /* One circle is inside another one.
           Use any subdivision,
           but don't depend on dx, dy, which may be too small. */
        p0.x = 0, p0.y = -1, dirn = 0;
        /* Align stripes along radii for faster triangulation : */
        inside = 1;
    } else {
        /* Must generate canonic quadrangle arcs,
           because we approximate them with curves. */
        if(dx >= 0) {
            if (dy >= 0)
                p0.x = 1, p0.y = 0, dirn = (dx >= dy ? 1 : 0);
            else
                p0.x = 0, p0.y = -1, dirn = (dx >= -dy ? 0 : 1);
        } else {
            if (dy >= 0)
                p0.x = 0, p0.y = 1, dirn = (-dx >= dy ? 1 : 0);
            else
                p0.x = -1, p0.y = 0, dirn = (-dx >= -dy ? 0 : 1);
        }
    }
    /* fixme: wish: cut invisible parts off.
       Note : when r0 != r1 the invisible part is not a half circle. */
    for (k = 0; k < 4; k++) {
        gs_point p[12];
        patch_curve_t curve[4];

        /* Set p1 to be 90 degrees anticlockwise from p0 */
        p1.x = -p0.y; p1.y = p0.x;
        if (dirn == 0) { /* Clockwise */
            make_quadrant_arc(p + 0, &pc0, &p1, &p0, r0);
            make_quadrant_arc(p + 6, &pc1, &p0, &p1, r1);
        } else { /* Anticlockwise */
            make_quadrant_arc(p + 0, &pc0, &p0, &p1, r0);
            make_quadrant_arc(p + 6, &pc1, &p1, &p0, r1);
        }
        p[4].x = (p[3].x * 2 + p[6].x) / 3;
        p[4].y = (p[3].y * 2 + p[6].y) / 3;
        p[5].x = (p[3].x + p[6].x * 2) / 3;
        p[5].y = (p[3].y + p[6].y * 2) / 3;
        p[10].x = (p[9].x * 2 + p[0].x) / 3;
        p[10].y = (p[9].y * 2 + p[0].y) / 3;
        p[11].x = (p[9].x + p[0].x * 2) / 3;
        p[11].y = (p[9].y + p[0].y * 2) / 3;
        for (j = 0; j < 4; j++) {
            int jj = (j + inside) % 4;

            if (gs_point_transform2fixed(&pfs->pgs->ctm,         p[j*3 + 0].x, p[j*3 + 0].y, &curve[jj].vertex.p) < 0)
                gs_point_transform2fixed_clamped(&pfs->pgs->ctm, p[j*3 + 0].x, p[j*3 + 0].y, &curve[jj].vertex.p);

            if (gs_point_transform2fixed(&pfs->pgs->ctm,         p[j*3 + 1].x, p[j*3 + 1].y, &curve[jj].control[0]) < 0)
                gs_point_transform2fixed_clamped(&pfs->pgs->ctm, p[j*3 + 1].x, p[j*3 + 1].y, &curve[jj].control[0]);

            if (gs_point_transform2fixed(&pfs->pgs->ctm,         p[j*3 + 2].x, p[j*3 + 2].y, &curve[jj].control[1]) < 0)
                gs_point_transform2fixed_clamped(&pfs->pgs->ctm, p[j*3 + 2].x, p[j*3 + 2].y, &curve[jj].control[1]);
            curve[j].straight = (((j + inside) & 1) != 0);
        }
        curve[(0 + inside) % 4].vertex.cc[0] = t0;
        curve[(1 + inside) % 4].vertex.cc[0] = t0;
        curve[(2 + inside) % 4].vertex.cc[0] = t1;
        curve[(3 + inside) % 4].vertex.cc[0] = t1;
        curve[0].vertex.cc[1] = curve[1].vertex.cc[1] = 0; /* Initialize against FPE. */
        curve[2].vertex.cc[1] = curve[3].vertex.cc[1] = 0; /* Initialize against FPE. */
        code = patch_fill(pfs, curve, NULL, NULL);
        if (code < 0)
            return code;
        /* Move p0 to be ready for the next position */
        if (k == 0) {
            /* p0 moves clockwise */
            p1 = p0;
            p0.x = p1.y; p0.y = -p1.x;
            dirn = 0;
        } else if (k == 1) {
            /* p0 flips sides */
            p0.x = -p0.x; p0.y = -p0.y;
            dirn = 1;
        } else if (k == 2) {
            /* p0 moves anti-clockwise */
            p1 = p0;
            p0.x = -p1.y; p0.y = p1.x;
            dirn = 0;
        }
    }
    return 0;
}

static int
R_outer_circle(patch_fill_state_t *pfs, const gs_rect *rect,
        double x0, double y0, double r0,
        double x1, double y1, double r1,
        double *x2, double *y2, double *r2)
{
    double dx = x1 - x0, dy = y1 - y0;
    double sp, sq, s;

    /* Compute a cone circle, which contacts the rect externally. */
    /* Don't bother with all 4 sides of the rect,
       just do with the X or Y span only,
       so it's not an exact contact, sorry. */
    if (any_abs(dx) > any_abs(dy)) {
        /* Solving :
            x0 + (x1 - x0) * sq + r0 + (r1 - r0) * sq == bbox_px
            (x1 - x0) * sp + (r1 - r0) * sp == bbox_px - x0 - r0
            sp = (bbox_px - x0 - r0) / (x1 - x0 + r1 - r0)

            x0 + (x1 - x0) * sq - r0 - (r1 - r0) * sq == bbox_qx
            (x1 - x0) * sq - (r1 - r0) * sq == bbox_x - x0 + r0
            sq = (bbox_x - x0 + r0) / (x1 - x0 - r1 + r0)
         */
        if (x1 - x0 + r1 - r0 ==  0) /* We checked for obtuse cone. */
            return_error(gs_error_unregistered); /* Must not happen. */
        if (x1 - x0 - r1 + r0 ==  0) /* We checked for obtuse cone. */
            return_error(gs_error_unregistered); /* Must not happen. */
        sp = (rect->p.x - x0 - r0) / (x1 - x0 + r1 - r0);
        sq = (rect->q.x - x0 + r0) / (x1 - x0 - r1 + r0);
    } else {
        /* Same by Y. */
        if (y1 - y0 + r1 - r0 ==  0) /* We checked for obtuse cone. */
            return_error(gs_error_unregistered); /* Must not happen. */
        if (y1 - y0 - r1 + r0 ==  0) /* We checked for obtuse cone. */
            return_error(gs_error_unregistered); /* Must not happen. */
        sp = (rect->p.y - y0 - r0) / (y1 - y0 + r1 - r0);
        sq = (rect->q.y - y0 + r0) / (y1 - y0 - r1 + r0);
    }
    if (sp >= 1 && sq >= 1)
        s = max(sp, sq);
    else if(sp >= 1)
        s = sp;
    else if (sq >= 1)
        s = sq;
    else {
        /* The circle 1 is outside the rect, use it. */
        s = 1;
    }
    if (r0 + (r1 - r0) * s < 0) {
        /* Passed the cone apex, use the apex. */
        s = r0 / (r0 - r1);
        *r2 = 0;
    } else
        *r2 = r0 + (r1 - r0) * s;
    *x2 = x0 + (x1 - x0) * s;
    *y2 = y0 + (y1 - y0) * s;
    return 0;
}

static double
R_rect_radius(const gs_rect *rect, double x0, double y0)
{
    double d, dd;

    dd = hypot(rect->p.x - x0, rect->p.y - y0);
    d = hypot(rect->p.x - x0, rect->q.y - y0);
    dd = max(dd, d);
    d = hypot(rect->q.x - x0, rect->q.y - y0);
    dd = max(dd, d);
    d = hypot(rect->q.x - x0, rect->p.y - y0);
    dd = max(dd, d);
    return dd;
}

static int
R_fill_triangle_new(patch_fill_state_t *pfs, const gs_rect *rect,
    double x0, double y0, double x1, double y1, double x2, double y2, double t)
{
    shading_vertex_t p0, p1, p2;
    patch_color_t *c;
    int code;
    reserve_colors(pfs, &c, 1); /* Can't fail */

    p0.c = c;
    p1.c = c;
    p2.c = c;
    code = gs_point_transform2fixed(&pfs->pgs->ctm, x0, y0, &p0.p);
    if (code >= 0)
        code = gs_point_transform2fixed(&pfs->pgs->ctm, x1, y1, &p1.p);
    if (code >= 0)
        code = gs_point_transform2fixed(&pfs->pgs->ctm, x2, y2, &p2.p);
    if (code >= 0) {
        c->t[0] = c->t[1] = t;
        patch_resolve_color(c, pfs);
        code = mesh_triangle(pfs, &p0, &p1, &p2);
    }
    release_colors(pfs, pfs->color_stack, 1);
    return code;
}

static int
R_obtuse_cone(patch_fill_state_t *pfs, const gs_rect *rect,
        double x0, double y0, double r0,
        double x1, double y1, double r1, double t0, double r_rect,
        bool inwards)
{
    double dx = x1 - x0, dy = y1 - y0, dr = any_abs(r1 - r0);
    double d = hypot(dx, dy);
    /* Assuming dr > d / 3 && d > dr + 1e-7 * (d + dr), see the caller. */
    double r = r_rect * 1.4143; /* A few bigger than sqrt(2). */
    double ax, ay, as; /* Cone apex. */
    double g0; /* The distance from apex to the tangent point of the 0th circle. */
    int code;

    as = r0 / (r0 - r1);
    ax = x0 + (x1 - x0) * as;
    ay = y0 + (y1 - y0) * as;
    g0 = sqrt(dx * dx + dy * dy - dr * dr) * as;
    if (g0 < 1e-7 * r0) {
        /* Nearly degenerate, replace with half-plane. */
        /* Restrict the half plane with triangle, which covers the rect. */
        gs_point p0, p1, p2; /* Right tangent limit, apex limit, left tangent linit,
                                (right, left == when looking from the apex). */

        p0.x = ax - dy * r / d;
        p0.y = ay + dx * r / d;
        p1.x = ax - dx * r / d;
        p1.y = ay - dy * r / d;
        p2.x = ax + dy * r / d;
        p2.y = ay - dx * r / d;
        /* Split into 2 triangles at the apex,
           so that the apex is preciselly covered.
           Especially important when it is not exactly degenerate. */
        code = R_fill_triangle_new(pfs, rect, ax, ay, p0.x, p0.y, p1.x, p1.y, t0);
        if (code < 0)
            return code;
        return R_fill_triangle_new(pfs, rect, ax, ay, p1.x, p1.y, p2.x, p2.y, t0);
    } else {
        /* Compute the "limit" circle so that its
           tangent points are outside the rect. */
        /* Note: this branch is executed when the condition above is false :
           g0 >= 1e-7 * r0 .
           We believe that computing this branch with doubles
           provides enough precision after converting coordinates into 'fixed',
           and that the limit circle radius is not dramatically big.
         */
        double es, er; /* The limit circle parameter, radius. */
        double ex, ey; /* The limit circle centrum. */

        es = as - as * r / g0; /* Always negative. */
        er = r * r0 / g0 ;
        ex = x0 + dx * es;
        ey = y0 + dy * es;
        /* Fill the annulus: */
        code = R_tensor_annulus(pfs, x0, y0, r0, t0, ex, ey, er, t0);
        if (code < 0)
            return code;
        /* Fill entire ending circle to ensure entire rect is covered, but
         * only if we are filling "inwards" (as otherwise we will overwrite
         * all the hard work we have done to this point) */
        if (inwards)
            code = R_tensor_annulus(pfs, ex, ey, er, t0, ex, ey, 0, t0);
        return code;
    }
}

static int
R_tensor_cone_apex(patch_fill_state_t *pfs, const gs_rect *rect,
        double x0, double y0, double r0,
        double x1, double y1, double r1, double t)
{
    double as = r0 / (r0 - r1);
    double ax = x0 + (x1 - x0) * as;
    double ay = y0 + (y1 - y0) * as;

    return R_tensor_annulus(pfs, x1, y1, r1, t, ax, ay, 0, t);
}

/*
 * A map of this code:
 *
 * R_extensions
 * |-> (R_rect_radius)
 * |-> (R_outer_circle)
 * |-> R_obtuse_cone
 * |   |-> R_fill_triangle_new
 * |   |   '-> mesh_triangle
 * |   |       '-> mesh_triangle_rec <--.
 * |   |           |--------------------'
 * |   |           |-> small_mesh_triangle
 * |   |           |   '-> fill_triangle
 * |   |           |       '-> triangle_by_4 <--.
 * |   |           |           |----------------'
 * |   |           |           |-> constant_color_triangle
 * |   |           |           |-> make_wedge_median (etc)
 * |   |           '-----------+--------------------.
 * |   '-------------------.                        |
 * |-> R_tensor_cone_apex  |                        |
 * |   '-------------------+                        |
 * '-> R_tensor_annulus <--'                       \|/
 *     |-> (make_quadrant_arc)                      |
 *     '-> patch_fill                               |
 *         |-> fill_patch <--.                      |
 *         |   |-------------'                      |
 *         |   |------------------------------------+
 *         |   '-> fill_stripe                      |
 *         |       |-----------------------.        |
 *         |      \|/                      |        |
 *         |-> fill_wedges                 |        |
 *             '-> fill_wedges_aux <--.    |        |
 *                 |------------------'   \|/       |
 *                 |----------------> mesh_padding  '
 *                 |                  '----------------------------------.
 *                 '-> wedge_by_triangles <--.      .                    |
 *                     |---------------------'      |                    |
 *                     '-> fill_triangle_wedge <----'                    |
 *                         '-> fill_triangle_wedge_aux                   |
 *                             '-> fill_wedge_trap                       |
 *                                 '-> wedge_trap_decompose              |
 *                                     '-> linear_color_trapezoid        |
 *                                         '-> decompose_linear_color <--|
 *                                             |-------------------------'
 *                                             '-> constant_color_trapezoid
 */
static int
R_extensions(patch_fill_state_t *pfs, const gs_shading_R_t *psh, const gs_rect *rect,
        double t0, double t1, bool Extend0, bool Extend1)
{
    float x0 = psh->params.Coords[0], y0 = psh->params.Coords[1];
    double r0 = psh->params.Coords[2];
    float x1 = psh->params.Coords[3], y1 = psh->params.Coords[4];
    double r1 = psh->params.Coords[5];
    double dx = x1 - x0, dy = y1 - y0, dr = any_abs(r1 - r0);
    double d = hypot(dx, dy), r;
    int code;

    if (dr >= d - 1e-7 * (d + dr)) {
        /* Nested circles, or degenerate. */
        if (r0 > r1) {
            if (Extend0) {
                r = R_rect_radius(rect, x0, y0);
                if (r > r0) {
                    code = R_tensor_annulus(pfs, x0, y0, r, t0, x0, y0, r0, t0);
                    if (code < 0)
                        return code;
                }
            }
            if (Extend1 && r1 > 0)
                return R_tensor_annulus(pfs, x1, y1, r1, t1, x1, y1, 0, t1);
        } else {
            if (Extend1) {
                r = R_rect_radius(rect, x1, y1);
                if (r > r1) {
                    code = R_tensor_annulus(pfs, x1, y1, r, t1, x1, y1, r1, t1);
                    if (code < 0)
                        return code;
                }
            }
            if (Extend0 && r0 > 0)
                return R_tensor_annulus(pfs, x0, y0, r0, t0, x0, y0, 0, t0);
        }
    } else if (dr > d / 3) {
        /* Obtuse cone. */
        if (r0 > r1) {
            if (Extend0) {
                r = R_rect_radius(rect, x0, y0);
                code = R_obtuse_cone(pfs, rect, x0, y0, r0, x1, y1, r1, t0, r, true);
                if (code < 0)
                    return code;
            }
            if (Extend1 && r1 != 0)
                return R_tensor_cone_apex(pfs, rect, x0, y0, r0, x1, y1, r1, t1);
            return 0;
        } else {
            if (Extend1) {
                r = R_rect_radius(rect, x1, y1);
                code = R_obtuse_cone(pfs, rect, x1, y1, r1, x0, y0, r0, t1, r, false);
                if (code < 0)
                    return code;
            }
            if (Extend0 && r0 != 0)
                return R_tensor_cone_apex(pfs, rect, x1, y1, r1, x0, y0, r0, t0);
        }
    } else {
        /* Acute cone or cylinder. */
        double x2, y2, r2, x3, y3, r3;

        if (Extend0) {
            code = R_outer_circle(pfs, rect, x1, y1, r1, x0, y0, r0, &x3, &y3, &r3);
            if (code < 0)
                return code;
            if (x3 != x1 || y3 != y1) {
                code = R_tensor_annulus(pfs, x0, y0, r0, t0, x3, y3, r3, t0);
                if (code < 0)
                    return code;
            }
        }
        if (Extend1) {
            code = R_outer_circle(pfs, rect, x0, y0, r0, x1, y1, r1, &x2, &y2, &r2);
            if (code < 0)
                return code;
            if (x2 != x0 || y2 != y0) {
                code = R_tensor_annulus(pfs, x1, y1, r1, t1, x2, y2, r2, t1);
                if (code < 0)
                    return code;
            }
        }
    }
    return 0;
}

static int
R_fill_rect_with_const_color(patch_fill_state_t *pfs, const gs_fixed_rect *clip_rect, float t)
{
#if 0 /* Disabled because the clist writer device doesn't pass
         the clipping path with fill_recatangle. */
    patch_color_t pc;
    const gs_color_space *pcs = pfs->direct_space;
    gx_device_color dc;
    int code;

    code = gs_function_evaluate(pfs->Function, &t, pc.cc.paint.values);
    if (code < 0)
        return code;
    pcs->type->restrict_color(&pc.cc, pcs);
    code = patch_color_to_device_color(pfs, &pc, &dc);
    if (code < 0)
        return code;
    return gx_fill_rectangle_device_rop(fixed2int_pixround(clip_rect->p.x), fixed2int_pixround(clip_rect->p.y),
                                        fixed2int_pixround(clip_rect->q.x) - fixed2int_pixround(clip_rect->p.x),
                                        fixed2int_pixround(clip_rect->q.y) - fixed2int_pixround(clip_rect->p.y),
                                        &dc, pfs->dev, pfs->pgs->log_op);
#else
    /* Can't apply fill_rectangle, because the clist writer device doesn't pass
       the clipping path with fill_recatangle. Convert into trapezoids instead.
    */
    quadrangle_patch p;
    shading_vertex_t pp[2][2];
    const gs_color_space *pcs = pfs->direct_space;
    patch_color_t pc;
    int code;

    code = gs_function_evaluate(pfs->Function, &t, pc.cc.paint.values);
    if (code < 0)
        return code;
    pcs->type->restrict_color(&pc.cc, pcs);
    pc.t[0] = pc.t[1] = t;
    pp[0][0].p = clip_rect->p;
    pp[0][1].p.x = clip_rect->q.x;
    pp[0][1].p.y = clip_rect->p.y;
    pp[1][0].p.x = clip_rect->p.x;
    pp[1][0].p.y = clip_rect->q.y;
    pp[1][1].p = clip_rect->q;
    pp[0][0].c = pp[0][1].c = pp[1][0].c = pp[1][1].c = &pc;
    p.p[0][0] = &pp[0][0];
    p.p[0][1] = &pp[0][1];
    p.p[1][0] = &pp[1][0];
    p.p[1][1] = &pp[1][1];
    return constant_color_quadrangle(pfs, &p, false);
#endif
}

typedef struct radial_shading_attrs_s {
    double x0, y0;
    double x1, y1;
    double span[2][2];
    double apex;
    bool have_apex;
    bool have_root[2]; /* ongoing contact, outgoing contact. */
    bool outer_contact[2];
    gs_point p[6]; /* 4 corners of the rectangle, p[4] = p[0], p[5] = p[1] */
} radial_shading_attrs_t;

#define Pw2(a) ((a)*(a))

static void
radial_shading_external_contact(radial_shading_attrs_t *rsa, int point_index, double t, double r0, double r1, bool at_corner, int root_index)
{
    double cx = rsa->x0 + (rsa->x1 - rsa->x0) * t;
    double cy = rsa->y0 + (rsa->y1 - rsa->y0) * t;
    double rx = rsa->p[point_index].x - cx;
    double ry = rsa->p[point_index].y - cy;
    double dx = rsa->p[point_index - 1].x - rsa->p[point_index].x;
    double dy = rsa->p[point_index - 1].y - rsa->p[point_index].y;

    if (at_corner) {
        double Dx = rsa->p[point_index + 1].x - rsa->p[point_index].x;
        double Dy = rsa->p[point_index + 1].y - rsa->p[point_index].y;
        bool b1 = (dx * rx + dy * ry >= 0);
        bool b2 = (Dx * rx + Dy * ry >= 0);

        if (b1 & b2)
            rsa->outer_contact[root_index] = true;
    } else {
        if (rx * dy - ry * dx < 0)
            rsa->outer_contact[root_index] = true;
    }
}

static void
store_roots(radial_shading_attrs_t *rsa, const bool have_root[2], const double t[2], double r0, double r1, int point_index, bool at_corner)
{
    int i;

    for (i = 0; i < 2; i++) {
        bool good_root;

        if (!have_root[i])
            continue;
        good_root = (!rsa->have_apex || (rsa->apex <= 0 || r0 == 0 ? t[i] >= rsa->apex : t[i] <= rsa->apex));
        if (good_root) {
            radial_shading_external_contact(rsa, point_index, t[i], r0, r1, at_corner, i);
            if (!rsa->have_root[i]) {
                rsa->span[i][0] = rsa->span[i][1] = t[i];
                rsa->have_root[i] = true;
            } else {
                if (rsa->span[i][0] > t[i])
                    rsa->span[i][0] = t[i];
                if (rsa->span[i][1] < t[i])
                    rsa->span[i][1] = t[i];
            }
        }
    }
}

static void
compute_radial_shading_span_extended_side(radial_shading_attrs_t *rsa, double r0, double r1, int point_index)
{
    double cc, c;
    bool have_root[2] = {false, false};
    double t[2];
    bool by_x = (rsa->p[point_index].x != rsa->p[point_index + 1].x);
    int i;

    /* As t moves from 0 to 1, the circles move from r0 to r1, and from
     * from position p0 to py. For simplicity, adjust so that p0 is at
     * the origin. Consider the projection of the circle drawn at any given
     * time onto the x axis. The range of points would be:
     * p1x*t +/- (r0+(r1-r0)*t). We are interested in the first (and last)
     * moments when the range includes a point c on the x axis. So solve for:
     * p1x*t +/- (r0+(r1-r0)*t) = c. Let cc = p1x.
     * So p1x*t0 + (r1-r0)*t0 = c - r0 => t0 = (c - r0)/(p1x + r1 - r0)
     *    p1x*t1 - (r1-r0)*t1 = c + r0 => t1 = (c + r0)/(p1x - r1 + r0)
     */
    if (by_x) {
        c = rsa->p[point_index].x - rsa->x0;
        cc = rsa->x1 - rsa->x0;
    } else {
        c = rsa->p[point_index].y - rsa->y0;
        cc = rsa->y1 - rsa->y0;
    }
    t[0] = (c - r0) / (cc + r1 - r0);
    t[1] = (c + r0) / (cc - r1 + r0);
    if (t[0] > t[1]) {
        c    = t[0];
        t[0] = t[1];
        t[1] = c;
    }
    for (i = 0; i < 2; i++) {
        double d, d0, d1;

        if (by_x) {
            d = rsa->y1 - rsa->y0 + r0 + (r1 - r0) * t[i];
            d0 = rsa->p[point_index].y;
            d1 = rsa->p[point_index + 1].y;
        } else {
            d = rsa->x1 - rsa->x0 + r0 + (r1 - r0) * t[i];
            d0 = rsa->p[point_index].x;
            d1 = rsa->p[point_index + 1].x;
        }
        if (d1 > d0 ? d0 <= d && d <= d1 : d1 <= d && d <= d0)
            have_root[i] = true;
    }
    store_roots(rsa, have_root, t, r0, r1, point_index, false);
}

static int
compute_radial_shading_span_extended_point(radial_shading_attrs_t *rsa, double r0, double r1, int point_index)
{
    /* As t moves from 0 to 1, the circles move from r0 to r1, and from
     * from position p0 to py. At any given time t, therefore, we
     * paint the points that are distance r0+(r1-r0)*t from point
     * (p0x+(p1x-p0x)*t,p0y+(p1y-p0y)*t) = P(t).
     *
     * To simplify our algebra, adjust so that (p0x, p0y) is at the origin.
     * To find the time(s) t at which the a point q is painted, we therefore
     * solve for t in:
     *
     * |q-P(t)| = r0+(r1-r0)*t
     *
     *   (qx-p1x*t)^2 + (qy-p1y*t)^2 - (r0+(r1-r0)*t)^2 = 0
     * = qx^2 - 2qx.p1x.t + p1x^2.t^2 + qy^2 - 2qy.p1y.t + p1y^2.t^2 -
     *                                   (r0^2 + 2r0(r1-r0)t + (r1-r0)^2.t^2)
     * =   qx^2 + qy^2 - r0^2
     *   + -2(qx.p1x + qy.p1y + r0(r1-r0)).t
     *   + (p1x^2 + p1y^2 - (r1-r0)^2).t^2
     *
     * So solve using the usual t = (-b +/- SQRT(b^2 - 4ac)) where
     *   a = p1x^2 + p1y^2 - (r1-r0)^2
     *   b = -2(qx.p1x + qy.p1y + r0(r1-r0))
     *   c = qx^2 + qy^2 - r0^2
     */
    double p1x = rsa->x1 - rsa->x0;
    double p1y = rsa->y1 - rsa->y0;
    double qx  = rsa->p[point_index].x - rsa->x0;
    double qy  = rsa->p[point_index].y - rsa->y0;
    double a   = (Pw2(p1x) + Pw2(p1y) - Pw2(r0 - r1));
    bool have_root[2] = {false, false};
    double t[2];

    if (fabs(a) < 1e-8) {
        /* Linear equation. */
        /* This case is always the ongoing ellipse contact. */
        double cx = rsa->x0 - (rsa->x1 - rsa->x0) * r0 / (r1 - r0);
        double cy = rsa->y0 - (rsa->y1 - rsa->y0) * r0 / (r1 - r0);

        t[0] = (Pw2(qx) + Pw2(qy))/(cx*qx + cy*qy) / 2;
        have_root[0] = true;
    } else {
        /* Square equation.  No solution if b^2 - 4ac = 0. Equivalently if
         * (b^2)/4 -a.c = 0 === (b/2)^2 - a.c = 0 ===  (-b/2)^2 - a.c = 0 */
        double minushalfb = r0*(r1-r0) + p1x*qx + p1y*qy;
        double c          = Pw2(qx) + Pw2(qy) - Pw2(r0);
        double desc2      = Pw2(minushalfb) - a*c; /* desc2 = 1/4 (b^2-4ac) */

        if (desc2 < 0) {
            return -1; /* The point is outside the shading coverage.
                          Do not shorten, because we didn't observe it in practice. */
        } else {
            double desc1 = sqrt(desc2); /* desc1 = 1/2 SQRT(b^2-4ac) */

            if (a > 0) {
                t[0] = (minushalfb - desc1) / a;
                t[1] = (minushalfb + desc1) / a;
            } else {
                t[0] = (minushalfb + desc1) / a;
                t[1] = (minushalfb - desc1) / a;
            }
            have_root[0] = have_root[1] = true;
        }
    }
    store_roots(rsa, have_root, t, r0, r1, point_index, true);
    if (have_root[0] && have_root[1])
        return 15;
    if (have_root[0])
        return 15 - 4;
    if (have_root[1])
        return 15 - 2;
    return -1;
}

#undef Pw2

static int
compute_radial_shading_span_extended(radial_shading_attrs_t *rsa, double r0, double r1)
{
    int span_type0, span_type1;

    span_type0 = compute_radial_shading_span_extended_point(rsa, r0, r1, 1);
    if (span_type0 == -1)
        return -1;
    span_type1 = compute_radial_shading_span_extended_point(rsa, r0, r1, 2);
    if (span_type0 != span_type1)
        return -1;
    span_type1 = compute_radial_shading_span_extended_point(rsa, r0, r1, 3);
    if (span_type0 != span_type1)
        return -1;
    span_type1 = compute_radial_shading_span_extended_point(rsa, r0, r1, 4);
    if (span_type0 != span_type1)
        return -1;
    compute_radial_shading_span_extended_side(rsa, r0, r1, 1);
    compute_radial_shading_span_extended_side(rsa, r0, r1, 2);
    compute_radial_shading_span_extended_side(rsa, r0, r1, 3);
    compute_radial_shading_span_extended_side(rsa, r0, r1, 4);
    return span_type0;
}

static int
compute_radial_shading_span(radial_shading_attrs_t *rsa, float x0, float y0, double r0, float x1, float y1, double r1, const gs_rect * rect)
{
    /* If the shading area is much larger than the path bbox,
       we want to shorten the shading for a faster rendering.
       If any point of the path bbox falls outside the shading area,
       our math is not applicable, and we render entire shading.
       If the path bbox is inside the shading area,
       we compute 1 or 2 'spans' - the shading parameter intervals,
       which covers the bbox. For doing that we need to resolve
       a square eqation by the shading parameter
       for each corner of the bounding box,
       and for each side of the shading bbox.
       Note the equation to be solved in the user space.
       Since each equation gives 2 roots (because the points are
       strongly inside the shading area), we will get 2 parameter intervals -
       the 'lower' one corresponds to the first (ongoing) contact of
       the running circle, and the second one corresponds to the last (outgoing) contact
       (like in a sun eclipse; well our sun is rectangular).

       Here are few exceptions.

       First, the equation degenerates when the distance sqrt((x1-x0)^2 + (y1-y0)^2)
       appears equal to r0-r1. In this case the base circles do contact,
       and the running circle does contact at the same point.
       The equation degenerates to a linear one.
       Since we don't want float precision noize to affect the result,
       we compute this condition in 'fixed' coordinates.

       Second, Postscript approximates any circle with 3d order beziers.
       This approximation may give a 2% error.
       Therefore using the precise roots may cause a dropout.
       To prevetn them, we slightly modify the base radii.
       However the sign of modification smartly depends
       on the relative sizes of the base circles,
       and on the contact number. Currently we don't want to
       define and debug the smart optimal logic for that,
       so we simply try all 4 variants for each source equation,
       and use the union of intervals.

       Third, we could compute which quarter of the circle
       really covers the path bbox. Using it we could skip
       rendering of uncovering quarters. Currently we do not
       implement this optimization. The general tensor patch algorithm
       will skip uncovering parts.

       Fourth, when one base circle is (almost) inside the other,
       the parameter interval must include the shading apex.
       To know that, we determine whether the contacting circle
       is outside the rectangle (the "outer" contact),
       or it is (partially) inside the rectangle.

       At last, a small shortening of a shading won't give a
       sensible speedup, but it may replace a symmetric function domain
       with an assymmetric one, so that the rendering
       would be asymmetyric for a symmetric shading.
       Therefore we do not perform a small sortening.
       Instead we shorten only if the shading span
       is much smaller that the shading domain.
     */
    const double extent = 1.02;
    int span_type0, span_type1, span_type;

    memset(rsa, 0, sizeof(*rsa));
    rsa->x0 = x0;
    rsa->y0 = y0;
    rsa->x1 = x1;
    rsa->y1 = y1;
    rsa->p[0] = rsa->p[4] = rect->p;
    rsa->p[1].x = rsa->p[5].x = rect->p.x;
    rsa->p[1].y = rsa->p[5].y = rect->q.y;
    rsa->p[2] = rect->q;
    rsa->p[3].x = rect->q.x;
    rsa->p[3].y = rect->p.y;
    rsa->have_apex = any_abs(r1 - r0) > 1e-7 * any_abs(r1 + r0);
    rsa->apex = (rsa->have_apex ? -r0 / (r1 - r0) : 0);
    span_type0 = compute_radial_shading_span_extended(rsa, r0 / extent, r1 * extent);
    if (span_type0 == -1)
        return -1;
    span_type1 = compute_radial_shading_span_extended(rsa, r0 / extent, r1 / extent);
    if (span_type0 != span_type1)
        return -1;
    span_type1 = compute_radial_shading_span_extended(rsa, r0 * extent, r1 * extent);
    if (span_type0 != span_type1)
        return -1;
    span_type1 = compute_radial_shading_span_extended(rsa, r0 * extent, r1 / extent);
    if (span_type1 == -1)
        return -1;
    if (r0 < r1) {
        if (rsa->have_root[0] && !rsa->outer_contact[0])
            rsa->span[0][0] = rsa->apex; /* Likely never happens. Remove ? */
        if (rsa->have_root[1] && !rsa->outer_contact[1])
            rsa->span[1][0] = rsa->apex;
    } else if (r0 > r1) {
        if (rsa->have_root[0] && !rsa->outer_contact[0])
            rsa->span[0][1] = rsa->apex;
        if (rsa->have_root[1] && !rsa->outer_contact[1])
            rsa->span[1][1] = rsa->apex; /* Likely never happens. Remove ? */
    }
    span_type = 0;
    if (rsa->have_root[0] && rsa->span[0][0] < 0)
        span_type |= 1;
    if (rsa->have_root[1] && rsa->span[1][0] < 0)
        span_type |= 1;
    if (rsa->have_root[0] && rsa->span[0][1] > 0 && rsa->span[0][0] < 1)
        span_type |= 2;
    if (rsa->have_root[1] && rsa->span[1][1] > 0 && rsa->span[1][0] < 1)
        span_type |= 4;
    if (rsa->have_root[0] && rsa->span[0][1] > 1)
        span_type |= 8;
    if (rsa->have_root[1] && rsa->span[1][1] > 1)
        span_type |= 8;
    return span_type;
}

static bool
shorten_radial_shading(float *x0, float *y0, double *r0, float *d0, float *x1, float *y1, double *r1, float *d1, double span_[2])
{
    double s0 = span_[0], s1 = span_[1], w;

    if (s0 < 0)
        s0 = 0;
    if (s1 < 0)
        s1 = 0;
    if (s0 > 1)
        s0 = 1;
    if (s1 > 1)
        s1 = 1;
    w = s1 - s0;
    if (w == 0)
        return false; /* Don't pass a degenerate shading. */
    if (w > 0.3)
        return false; /* The span is big, don't shorten it. */
    {	/* Do shorten. */
        double R0 = *r0, X0 = *x0, Y0 = *y0, D0 = *d0;
        double R1 = *r1, X1 = *x1, Y1 = *y1, D1 = *d1;

        *r0 = R0 + (R1 - R0) * s0;
        *x0 = X0 + (X1 - X0) * s0;
        *y0 = Y0 + (Y1 - Y0) * s0;
        *d0 = D0 + (D1 - D0) * s0;
        *r1 = R0 + (R1 - R0) * s1;
        *x1 = X0 + (X1 - X0) * s1;
        *y1 = Y0 + (Y1 - Y0) * s1;
        *d1 = D0 + (D1 - D0) * s1;
    }
    return true;
}

static bool inline
is_radial_shading_large(double x0, double y0, double r0, double x1, double y1, double r1, const gs_rect * rect)
{
    const double d = hypot(x1 - x0, y1 - y0);
    const double area0 = M_PI * r0 * r0 / 2;
    const double area1 = M_PI * r1 * r1 / 2;
    const double area2 = (r0 + r1) / 2 * d;
    const double arbitrary = 8;
    double areaX, areaY;

    /* The shading area is not equal to area0 + area1 + area2
       when one circle is (almost) inside the other.
       We believe that the 'arbitrary' coefficient recovers that
       when it is set greater than 2. */
    /* If one dimension is large enough, the shading parameter span is wide. */
    areaX = (rect->q.x - rect->p.x) * (rect->q.x - rect->p.x);
    if (areaX * arbitrary < area0 + area1 + area2)
        return true;
    areaY = (rect->q.y - rect->p.y) * (rect->q.y - rect->p.y);
    if (areaY * arbitrary < area0 + area1 + area2)
        return true;
    return false;
}

static int
gs_shading_R_fill_rectangle_aux(const gs_shading_t * psh0, const gs_rect * rect,
                            const gs_fixed_rect *clip_rect,
                            gx_device * dev, gs_gstate * pgs)
{
    const gs_shading_R_t *const psh = (const gs_shading_R_t *)psh0;
    float d0 = psh->params.Domain[0], d1 = psh->params.Domain[1];
    float x0 = psh->params.Coords[0], y0 = psh->params.Coords[1];
    double r0 = psh->params.Coords[2];
    float x1 = psh->params.Coords[3], y1 = psh->params.Coords[4];
    double r1 = psh->params.Coords[5];
    radial_shading_attrs_t rsa;
    int span_type; /* <0 - don't shorten, 1 - extent0, 2 - first contact, 4 - last contact, 8 - extent1. */
    int code;
    patch_fill_state_t pfs1;

    if (r0 == 0 && r1 == 0)
        return 0; /* PLRM requires to paint nothing. */
    code = shade_init_fill_state((shading_fill_state_t *)&pfs1, psh0, dev, pgs);
    if (code < 0)
        return code;
    pfs1.Function = psh->params.Function;
    code = init_patch_fill_state(&pfs1);
    if (code < 0) {
        if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
        return code;
    }
    pfs1.function_arg_shift = 1;
    pfs1.rect = *clip_rect;
    pfs1.maybe_self_intersecting = false;
    if (is_radial_shading_large(x0, y0, r0, x1, y1, r1, rect))
        span_type = compute_radial_shading_span(&rsa, x0, y0, r0, x1, y1, r1, rect);
    else
        span_type = -1;
    if (span_type < 0) {
        code = R_extensions(&pfs1, psh, rect, d0, d1, psh->params.Extend[0], false);
        if (code >= 0)
            code = R_tensor_annulus(&pfs1, x0, y0, r0, d0, x1, y1, r1, d1);
        if (code >= 0)
            code = R_extensions(&pfs1, psh, rect, d0, d1, false, psh->params.Extend[1]);
    } else if (span_type == 1) {
        code = R_fill_rect_with_const_color(&pfs1, clip_rect, d0);
    } else if (span_type == 8) {
        code = R_fill_rect_with_const_color(&pfs1, clip_rect, d1);
    } else {
        bool second_interval = true;

        code = 0;
        if (span_type & 1)
            code = R_extensions(&pfs1, psh, rect, d0, d1, psh->params.Extend[0], false);
        if ((code >= 0) && (span_type & 2)) {
            float X0 = x0, Y0 = y0, D0 = d0, X1 = x1, Y1 = y1, D1 = d1;
            double R0 = r0, R1 = r1;

            if ((span_type & 4) && rsa.span[0][1] >= rsa.span[1][0]) {
                double united[2];

                united[0] = rsa.span[0][0];
                united[1] = rsa.span[1][1];
                shorten_radial_shading(&X0, &Y0, &R0, &D0, &X1, &Y1, &R1, &D1, united);
                second_interval = false;
            } else {
                second_interval = shorten_radial_shading(&X0, &Y0, &R0, &D0, &X1, &Y1, &R1, &D1, rsa.span[0]);
            }
            code = R_tensor_annulus(&pfs1, X0, Y0, R0, D0, X1, Y1, R1, D1);
        }
        if (code >= 0 && second_interval) {
            if (span_type & 4) {
                float X0 = x0, Y0 = y0, D0 = d0, X1 = x1, Y1 = y1, D1 = d1;
                double R0 = r0, R1 = r1;

                shorten_radial_shading(&X0, &Y0, &R0, &D0, &X1, &Y1, &R1, &D1, rsa.span[1]);
                code = R_tensor_annulus(&pfs1, X0, Y0, R0, D0, X1, Y1, R1, D1);
            }
        }
        if (code >= 0 && (span_type & 8))
            code = R_extensions(&pfs1, psh, rect, d0, d1, false, psh->params.Extend[1]);
    }
    if (pfs1.icclink != NULL) gsicc_release_link(pfs1.icclink);
    if (term_patch_fill_state(&pfs1))
        return_error(gs_error_unregistered); /* Must not happen. */
    return code;
}

int
gs_shading_R_fill_rectangle(const gs_shading_t * psh0, const gs_rect * rect,
                            const gs_fixed_rect * rect_clip,
                            gx_device * dev, gs_gstate * pgs)
{
    return gs_shading_R_fill_rectangle_aux(psh0, rect, rect_clip, dev, pgs);
}
