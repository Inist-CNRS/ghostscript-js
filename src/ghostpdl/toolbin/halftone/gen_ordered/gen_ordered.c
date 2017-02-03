/**
 * Ordered Dither Screen Creation Tool.
 **/

/* Copyright (C) 2001-2013 Artifex Software, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sys/stat.h>
#include <math.h>

typedef unsigned char byte;
typedef int bool;
#define false 0
#define true 1
#define RAW_SCREEN_DUMP 0	/* Noisy output for .raw files for detailed debugging */
#define MAXVAL 65535.0
#define MIN(x,y) ((x)>(y)?(y):(x))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define ROUND( a )  ( ( (a) < 0 ) ? (int) ( (a) - 0.5 ) : \
                                                  (int) ( (a) + 0.5 ) )
typedef enum {
    CIRCLE = 0,
    REDBOOK,
    INVERTED,
    RHOMBOID,
    LINE_X,
    LINE_Y,
    DIAMOND1,
    DIAMOND2,
    CUSTOM  /* Must remain last one */
} spottype_t;

typedef struct htsc_point_s {
    double x;
    double y;
} htsc_point_t;

typedef struct htsc_threshpoint {
    int x;
    int y;
    int value;
} htsc_threshpoint_t;

typedef struct htsc_vertices_s {
    htsc_point_t lower_left;
    htsc_point_t upper_left;
    htsc_point_t upper_right;
    htsc_point_t lower_right;
} htsc_vertices_t;

typedef struct htsc_dig_grid_s {
    int width;
    int height;
    int *data;
} htsc_dig_grid_t;

typedef struct htsc_dot_shape_search_s {
    double norm;
    int index_x;
    int index_y;
} htsc_dot_shape_search_t;

typedef struct htsc_vector_s {
   double xy[2];
} htsc_vector_t;

typedef struct htsc_matrix_s {
   htsc_vector_t row[2];
} htsc_matrix_t;

typedef struct htsc_dither_pos_s {
    htsc_point_t *point;
    int number_points;
    int *locations;
} htsc_dither_pos_t;

typedef enum {
   OUTPUT_TOS = 0,
   OUTPUT_PS = 1,
   OUTPUT_PPM = 2,
   OUTPUT_RAW = 3,
   OUTPUT_RAW16 = 4
} output_format_type;

typedef struct htsc_param_s {
    double scr_ang;
    int targ_scr_ang;
    int targ_lpi;
    double vert_dpi;
    double horiz_dpi;
    bool targ_quant_spec;
    int targ_quant;
    int targ_size;
    bool targ_size_spec;
    spottype_t spot_type;
    bool holladay;
    output_format_type output_format;
} htsc_param_t;

void htsc_determine_cell_shape(double *x, double *y, double *v, double *u, 
                               double *N, htsc_param_t params);
double htsc_spot_value(spottype_t spot_type, double x, double y);
int htsc_getpoint(htsc_dig_grid_t *dig_grid, int x, int y);
void htsc_setpoint(htsc_dig_grid_t *dig_grid, int x, int y, int value);
void  htsc_create_dot_mask(htsc_dig_grid_t *dig_grid, int x, int y, int u, int v,
                       double screen_angle, htsc_vertices_t vertices);
void htsc_diffpoint(htsc_point_t point1, htsc_point_t point2,
                   htsc_point_t *diff_point);
double htsc_vertex_dist(htsc_vertices_t vertices, htsc_point_t test_point,
                 double horiz_dpi, double vert_dpi);
double htsc_normpoint(htsc_point_t *diff_point, double horiz_dpi, double vert_dpi);
int htsc_sumsum(htsc_dig_grid_t dig_grid);
void htsc_create_dot_profile(htsc_dig_grid_t *dig_grid, int N, int x, int y,
                            int u, int v, htsc_point_t center, double horiz_dpi,
                            double vert_dpi, htsc_vertices_t vertices,
                            htsc_point_t *one_index, spottype_t spot_type,
                            htsc_matrix_t trans_matrix);
int htsc_allocate_supercell(htsc_dig_grid_t *super_cell, int x, int y, int u,
                           int v, int target_size, bool use_holladay_grid,
                           htsc_dig_grid_t dot_grid, int N, int *S, int *H, int *L);
void htsc_tile_supercell(htsc_dig_grid_t *super_cell, htsc_dig_grid_t *dot_grid,
                 int x, int y, int u, int v, int N);
void htsc_create_holladay_mask(htsc_dig_grid_t super_cell, int H, int L,
                               double gamma, htsc_dig_grid_t *final_mask);
void htsc_create_dither_mask(htsc_dig_grid_t super_cell,
                             htsc_dig_grid_t *final_mask,
                             int num_levels, int y, int x, double vert_dpi,
                             double horiz_dpi, int N, double gamma,
                             htsc_dig_grid_t dot_grid, htsc_point_t one_index);
void htsc_create_nondithered_mask(htsc_dig_grid_t super_cell, int H, int L,
                          double gamma, htsc_dig_grid_t *final_mask);
void htsc_save_screen(htsc_dig_grid_t final_mask, bool use_holladay_grid, int S,
                      htsc_param_t params);
void htsc_set_default_params(htsc_param_t *params);
int htsc_gcd(int a, int b);
int  htsc_lcm(int a, int b);
int htsc_matrix_inverse(htsc_matrix_t matrix_in, htsc_matrix_t *matrix_out);
void htsc_matrix_vector_mult(htsc_matrix_t matrix_in, htsc_vector_t vector_in,
                   htsc_vector_t *vector_out);
#if RAW_SCREEN_DUMP
void htsc_dump_screen(htsc_dig_grid_t *dig_grid, char filename[]);
void htsc_dump_float_image(float *image, int height, int width, float max_val,
                      char filename[]);
void htsc_dump_byte_image(byte *image, int height, int width, float max_val,
                      char filename[]);
#endif

int usage (void) {
    printf ("Usage: gen_ordered [-a target_angle] [-l target_lpi] [-q target_quantization_levels] \n");
    printf ("                 [-r WxH] [-s size_of_supercell] [-d dot_shape_code] \n");
    printf ("                 [ -f [ps | ppm | raw | raw16 | tos] ]\n");
    printf ("a is the desired angle in degrees for the screen\n");
    printf ("b is the desired bit depth of the output threshold\n");
    printf ("  valid only with -ps or the default raw output\n");
    printf ("dot shape codes are as follows: \n");
    printf ("0  CIRCLE \n");
    printf ("1  REDBOOK CIRCLE \n");
    printf ("2  INVERTED \n");
    printf ("3  RHOMBOID \n");
    printf ("4  LINE_X \n");
    printf ("5  LINE_Y \n");
    printf ("6  DIAMOND1 \n");
    printf ("7  DIAMOND2 \n");
    printf ("f [tos | ppm | ps | raw] is the output format\n");
    printf ("     If no output format is indicate a turn on sequence output will be created\n");
    printf ("   ppm is an Portable Pixmap image file\n");
    printf ("   ps indicates postscript style output file\n");
    printf ("   raw is a raw 8-bit binary, row major file\n");
    printf ("   tos indicates to output a turn on sequence which can\n");
    printf ("     be fed into thresh_remap to apply a linearization curve.\n");
    printf ("l is the desired lines per inch (lpi)\n");
    printf ("q is the desired number of quantization (gray) levels\n");
    printf ("r is the device resolution in dots per inch (dpi)\n");
    printf ("  use a single number for r if the resolution is symmetric\n");
    printf ("s is the desired size of the super cell\n");
    return 1;
}

/* Return the pointer to the value for an argument, either at 'arg' or the next argv */
/* Allows for either -ovalue or -o value option syntax */
/* Returns NULL if there is no value (at end of argc arguments) */
static const char * get_arg (int argc, char **argv, int *pi, const char *arg) {
    if (arg[0] != 0) {
        return arg;
    } else {
      (*pi)++;
      if (*pi == argc) {
        return NULL;
      } else {
        return argv[*pi];
      }
    }
}

int
main (int argc, char **argv)
{
    double num_levels;
    const double  pi = 3.14159265358979323846f;
    double x,y,v,u,N;
    int rcode = 0;
    htsc_vertices_t vertices;
    htsc_point_t center, one_index;
    htsc_dig_grid_t dot_grid;
    htsc_dig_grid_t super_cell;
    htsc_dig_grid_t final_mask;
    int code;
    int S, H, L;
    double gamma = 1;
    int i, j, k, m;
    int temp_int;
    htsc_param_t params;
    htsc_matrix_t trans_matrix, trans_matrix_inv;
    int min_size;

    htsc_set_default_params(&params);

    for (i = 1; i < argc; i++) {
      const char *arg = argv[i];
      const char *arg_value;

      if (arg[0] == '-') {
          switch (arg[1]) {
            case 'a':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              params.targ_scr_ang = atoi(arg_value);
              params.targ_scr_ang = params.targ_scr_ang % 90;
              params.scr_ang = params.targ_scr_ang;
              break;
            case 'f':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
	      switch (arg_value[0]) {
		  case 'p':
                    params.output_format = (arg_value[1] == 's') ?
		    		OUTPUT_PS : OUTPUT_PPM;
		    break;
		  case 'r':
                    j = sscanf(arg_value, "raw%d", &k);
		    if (j == 0 || k == 8)
                        params.output_format = OUTPUT_RAW;
		    else
                        params.output_format = OUTPUT_RAW16;
		    break;
		  case 't':
                    params.output_format = OUTPUT_TOS;
		    break;
		  default:
		    goto usage_exit;
	      }
              break;
            case 'd':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              temp_int = atoi(arg_value);
              if (temp_int < 0 || temp_int > CUSTOM)  
                  params.spot_type = CIRCLE;
              else 
                  params.spot_type = temp_int;
              break;
            case 'l':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              params.targ_lpi = atoi(arg_value);
              break;
            case 'q':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              params.targ_quant = atoi(arg_value);
              params.targ_quant_spec = true;
              break;
            case 'r':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              j = sscanf(arg_value, "%dx%d", &k, &m);
              if (j < 1)
                  goto usage_exit;
              params.horiz_dpi = k;
              if (j > 1) {
                  params.vert_dpi = m;
              } else {
                  params.vert_dpi = k;
              }
              break;
            case 's':
	      if ((arg_value = get_arg(argc, argv, &i, arg + 2)) == NULL)
		  goto usage_exit;
              params.targ_size = atoi(arg_value);
              params.targ_size_spec = true;
              break;
            default:
usage_exit:   return usage();
            }
        }
    }
    dot_grid.data = NULL;
    super_cell.data = NULL;
    final_mask.data = NULL;
    /* Get the vector values that define the small cell shape */
    htsc_determine_cell_shape(&x, &y, &v, &u, &N, params);
    /* Figure out how many levels to dither across. */
    if (params.targ_quant_spec) {
        num_levels = ROUND((double) params.targ_quant / N);
    } else {
        num_levels = 1;
    }
    if (num_levels < 1) num_levels = 1;
    if (num_levels == 1) {
        printf("No additional dithering , creating minimal sized periodic screen\n");
        params.targ_size = 1;
    }
    /* Lower left of the cell is at the origin.  Define the other vertices */
    vertices.lower_left.x = 0;
    vertices.lower_left.y = 0;
    vertices.upper_left.x = x;
    vertices.upper_left.y = y;
    vertices.upper_right.x = x + u;
    vertices.upper_right.y = y + v;
    vertices.lower_right.x = u;
    vertices.lower_right.y = v;
    center.x = vertices.upper_right.x / 2.0;
    center.y = vertices.upper_right.y / 2.0;
    /* Create the matrix that is used to get us correctly into the dot shape
       function */
    trans_matrix.row[0].xy[0] = u; 
    trans_matrix.row[0].xy[1] = x;
    trans_matrix.row[1].xy[0] = v;
    trans_matrix.row[1].xy[1] = y;
    code = htsc_matrix_inverse(trans_matrix, &trans_matrix_inv);
    if (code < 0) {
        printf("ERROR! Singular Matrix Inversion!\n");
        return -1;
    }
    /* Create a binary mask that indicates where we need to define the dot turn
       on sequence or dot profile */
    htsc_create_dot_mask(&dot_grid, x, y, u, v, params.scr_ang, vertices);
#if RAW_SCREEN_DUMP
    htsc_dump_screen(&dot_grid, "mask");
#endif
    /* A sanity check */
    if (htsc_sumsum(dot_grid) != -N) {
        printf("ERROR! grid size problem!\n");
        return -1;
    }
    /* Now actually determine the turn on sequence */
    htsc_create_dot_profile(&dot_grid, N, x, y, u, v, center, params.horiz_dpi,
                            params.vert_dpi, vertices, &one_index, 
                            params.spot_type, trans_matrix_inv);
#if RAW_SCREEN_DUMP
    htsc_dump_screen(&dot_grid, "dot_profile");
#endif
    /* Allocate super cell */
    code = htsc_allocate_supercell(&super_cell, x, y, u, v, params.targ_size,
                            params.holladay, dot_grid, N, &S, &H, &L);
    if (code < 0) {
        printf("ERROR! grid size problem!\n");
        return -1;
    }
    /* Make a warning about large requested quantization levels with no -s set */
    if (params.targ_size == 1 && num_levels > 1) {
        printf("To achieve %d quantization levels with the desired lpi,\n", params.targ_quant);
        /* Num_levels is effectively the number of small cells needed.  Note
           this is a rough and crude calculation.  But it is sufficiently good
           to give an approximation. */
        min_size = 
            (int) sqrt((double) (super_cell.width * super_cell.height) * 
                                 num_levels);
        printf("it is necessary to specify a screen size (-s) of at least %d.\n", min_size);
        printf("Note that an even larger size may be needed to reduce pattern artifacts.\n");
        printf("Because no screen size was specified, the minimum possible\n"); 
        printf("size that can approximate the requested angle and lpi will be created\n");
        printf("with %d quantization levels.\n", (int) N);
    }
    /* Go ahead and fill up the super cell grid with our growth dot values */
    htsc_tile_supercell(&super_cell, &dot_grid, x, y, u, v, N);
#if RAW_SCREEN_DUMP
    htsc_dump_screen(&super_cell, "super_cell_tiled");
#endif
    /* If we are using the Holladay grid (non dithered) then we are done. */
    if (params.holladay) {
        htsc_create_holladay_mask(super_cell, H, L, gamma, &final_mask);
    } else {
        if ((super_cell.height == dot_grid.height &&
            super_cell.width == dot_grid.width) || num_levels == 1) {
            htsc_create_nondithered_mask(super_cell, H, L, gamma, &final_mask);
        } else {
            htsc_create_dither_mask(super_cell, &final_mask, num_levels, y, x,
                                    params.vert_dpi, params.horiz_dpi, N, gamma, 
                                    dot_grid, one_index);
        }
    }
    htsc_save_screen(final_mask, params.holladay, S, params);
    if (dot_grid.data != NULL) free(dot_grid.data);
    if (super_cell.data != NULL) free(super_cell.data);
    if (final_mask.data != NULL) free(final_mask.data);
    return rcode;
}

void
htsc_determine_cell_shape(double *x_out, double *y_out, double *v_out, 
                          double *u_out, double *N_out, htsc_param_t params)
{
    double x, y, v, u, N;
    double x_scale, frac, scaled_x, scaled_y;
    double ratio;
    const double  pi = 3.14159265358979323846f;
    double true_angle, lpi;
    double prev_lpi, max_lpi;
    bool use = false;
    double x_use,y_use;

    /* Go through and find the rational angle options that gets us to the
       best LPI.  Pick the one that is just over what is requested.
       That is really our limiting factor here.  Pick it and
       then figure out how much dithering we need to do to get to the proper
       number of levels.  */
    x_scale = params.horiz_dpi / params.vert_dpi;
    frac = tan( params.scr_ang * pi / 180.0 );
    ratio = frac * params.horiz_dpi / params.vert_dpi;
    scaled_x = params.horiz_dpi / params.vert_dpi;
    scaled_y = params.vert_dpi / params.horiz_dpi;
    /* The minimal step is in x */
    prev_lpi = 0;
    if (ratio < 1 && ratio != 0) {
        printf("x\ty\tu\tv\tAngle\tLPI\tLevels\n");
        printf("-----------------------------------------------------------\n");
        for (x = 1; x < 11; x++) {
            x_use = x;
            y=ROUND((double) x_use / ratio);
            true_angle = 180.0 * atan(((double) x_use / params.horiz_dpi) / ( (double) y / params.vert_dpi) ) / pi;
            lpi = 1.0/( sqrt( ((double) y / params.vert_dpi) * ( (double) y / params.vert_dpi) +
                                    ( (double) x_use / params.horiz_dpi) * ((double) x_use / params.horiz_dpi) ));
            v = -x_use / scaled_x;
            u = y * scaled_x;
            N = y *u - x_use * v;
            if (prev_lpi == 0) {
                prev_lpi = lpi;
                if (params.targ_lpi > lpi) {
                    printf("Warning this lpi is not achievable!\n");
                    printf("Resulting screen will be poorly quantized\n");
                    printf("or completely stochastic!\n");
                    use = true;
                }
                max_lpi = lpi;
            }
            if (prev_lpi >= params.targ_lpi && lpi < params.targ_lpi) {
                if (prev_lpi == max_lpi) {
                    printf("Notice lpi is at the maximimum level possible.\n");
                    printf("This may result in poor quantization. \n");
                } 
                /* Reset these to previous x */
                x_use = x - 1;
                y=ROUND((double) x_use / ratio);
                true_angle = 
                    180.0 * atan(((double) x_use / params.horiz_dpi) / ( (double) y / params.vert_dpi) ) / pi;
                lpi = 
                    1.0/( sqrt( ((double) y / params.vert_dpi) * ( (double) y / params.vert_dpi) +
                                        ( (double) x_use / params.horiz_dpi) * ((double) x_use / params.horiz_dpi) ));
                v = -x_use / scaled_x;
                u = y * scaled_x;
                N = y *u - x_use * v;
                use = true;
            }
            if (use == true) {
                if (prev_lpi == max_lpi) {
                    /* Print out the final one that we will use */
                    printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                    x_use,y,u,v,true_angle,lpi,N);
                }
                break;
            }
            printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                            x_use,y,u,v,true_angle,lpi,N);
            prev_lpi = lpi;
        }
        x = x_use;
    }
    if (ratio >= 1 && ratio!=0) {
        /* The minimal step is in y */
        printf("x\ty\tu\tv\tAngle\tLPI\tLevels\n");
        printf("-----------------------------------------------------------\n");
        for (y = 1; y < 11; y++) {
            y_use = y;
            x = ROUND(y_use * ratio);
            /* compute the true angle */
            true_angle = 180.0 * atan((x / params.horiz_dpi) / (y_use / params.vert_dpi)) / pi;
            lpi = 1.0 / sqrt( (y_use / params.vert_dpi) * (y_use / params.vert_dpi) +
                                (x / params.horiz_dpi) * (x / params.horiz_dpi));
            v = ROUND(-x / scaled_x);
            u = ROUND(y_use * scaled_x);
            N = y_use * u - x * v;
            if (prev_lpi == 0) {
                prev_lpi = lpi;
                if (params.targ_lpi > lpi) {
                    printf("Warning this lpi is not achievable!\n");
                    printf("Resulting screen will be poorly quantized\n");
                    printf("or completely stochastic!\n");
                    use = true;
                }
                max_lpi = lpi;
            }
            if (prev_lpi >= params.targ_lpi && lpi < params.targ_lpi) {
                if (prev_lpi == max_lpi) {
                    printf("Warning lpi will be slightly lower than target.\n");
                    printf("An increase will result in poor \n");
                    printf("quantization or a completely stochastic screen!\n");
                } else {
                    /* Reset these to previous x */
                    y_use = y - 1;
                    x = ROUND(y_use * ratio);
                    /* compute the true angle */
                    true_angle = 180.0 * atan((x / params.horiz_dpi) / (y_use / params.vert_dpi)) / pi;
                    lpi = 1.0 / sqrt( (y_use / params.vert_dpi) * (y_use / params.vert_dpi) +
                                        (x / params.horiz_dpi) * (x / params.horiz_dpi));
                    v = ROUND(-x / scaled_x);
                    u = ROUND(y_use * scaled_x);
                    N = y_use * u - x * v;
                }
                use = true;
            }
            if (use == true) {
                if (prev_lpi == max_lpi) {
                    /* Print out the final one that we will use */
                    printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                    x,y_use,u,v,true_angle,lpi,N);
                }
                break;
            }
            printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                            x,y_use,u,v,true_angle,lpi,N);
            prev_lpi = lpi;
        }
        y = y_use;
    }
    if (ratio == 0) {
        /* 0 degrees */
        if (scaled_x >= 1) {
            printf("x\ty\tu\tv\tAngle\tLPI\tLevels\n");
            printf("-----------------------------------------------------------\n");
            for (y = 1; y < 11; y++) {
                y_use = y;
                x = ROUND( y_use * ratio );
                v = ROUND(-x / scaled_x);
                u = ROUND(y_use * scaled_x);
                N = y_use * u - x * v;
                true_angle = 0;
                lpi = 1.0/(double) sqrt( (double) ((y_use / params.vert_dpi) *
                    (y_use / params.vert_dpi) + (x / params.horiz_dpi) * (x / params.horiz_dpi)) );
                if (prev_lpi == 0) {
                    prev_lpi = lpi;
                    if (params.targ_lpi > lpi) {
                        printf("Warning this lpi is not achievable!\n");
                        printf("Resulting screen will be poorly quantized\n");
                        printf("or completely stochastic!\n");
                        use = true;
                    }
                    max_lpi = lpi;
                }
                if (prev_lpi >= params.targ_lpi && lpi < params.targ_lpi) {
                    if (prev_lpi == max_lpi) {
                        printf("Warning lpi will be slightly lower than target.\n");
                        printf("An increase will result in poor \n");
                        printf("quantization or a completely stochastic screen!\n");
                    } else {
                        /* Reset these to previous x */
                        y_use = y - 1;
                        x = ROUND( y_use * ratio );
                        v = ROUND(-x / scaled_x);
                        u = ROUND(y_use * scaled_x);
                        N = y_use * u - x * v;
                        true_angle = 0;
                        lpi = 1.0/(double) sqrt( (double) ((y_use / params.vert_dpi) *
                            (y_use / params.vert_dpi) + (x / params.horiz_dpi) * (x / params.horiz_dpi)) );
                    }
                    use = true;
                }
                if (use == true) {
                    if (prev_lpi == max_lpi) {
                        /* Print out the final one that we will use */
                        printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                        x,y_use,u,v,true_angle,lpi,N);
                    }
                    break;
                }
                printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                x,y_use,u,v,true_angle,lpi,N);
                prev_lpi = lpi;
            }
            y = y_use;
        } else {
            printf("x\ty\tu\tv\tAngle\tLPI\tLevels\n");
            printf("-----------------------------------------------------------\n");
            for (x = 1; x < 11; x++) {
                x_use = x;
                y = ROUND(x_use * ratio);
                true_angle = 0;
                lpi = 1.0/( sqrt( (y / params.vert_dpi) * (y / params.vert_dpi) +
                            (x_use / params.horiz_dpi) * (x_use / params.horiz_dpi) ));
                v = ROUND( -x_use / scaled_x);
                u = ROUND( y * scaled_x);
                N = y  *u - x_use * v;
                if (prev_lpi == 0) {
                    prev_lpi = lpi;
                    if (params.targ_lpi > lpi) {
                        printf("Warning this lpi is not achievable!\n");
                        printf("Resulting screen will be poorly quantized\n");
                        printf("or completely stochastic!\n");
                        use = true;
                    }
                    max_lpi = lpi;
                }
                if (prev_lpi > params.targ_lpi && lpi < params.targ_lpi) {
                    if (prev_lpi == max_lpi) {
                        printf("Warning lpi will be slightly lower than target.\n");
                        printf("An increase will result in poor \n");
                        printf("quantization or a completely stochastic screen!\n");
                    } else {
                        /* Reset these to previous x */
                        x_use = x - 1;
                        y = ROUND(x_use * ratio);
                        true_angle = 0;
                        lpi = 1.0/( sqrt( (y / params.vert_dpi) * (y / params.vert_dpi) +
                                    (x_use / params.horiz_dpi) * (x_use / params.horiz_dpi) ));
                        v = ROUND( -x_use / scaled_x);
                        u = ROUND( y * scaled_x);
                        N = y  *u - x_use * v;
                    }
                    use = true;
                }
                if (use == true) {
                    if (prev_lpi == max_lpi) {
                        /* Print out the final one that we will use */
                        printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                        x_use,y,u,v,true_angle,lpi,N);
                    }
                    break;
                }
                printf("%3.0lf\t%3.0lf\t%3.0lf\t%3.0lf\t%3.1lf\t%3.1lf\t%3.0lf\n",
                                x_use,y,u,v,true_angle,lpi,N);
                prev_lpi = lpi;
            }
            x = x_use;
        }
    }
    *x_out = x;
    *y_out = y;
    *v_out = v;
    *u_out = u;
    *N_out = N;
}

void
htsc_create_dot_profile(htsc_dig_grid_t *dig_grid, int N, int x, int y, int u, int v,
                        htsc_point_t center, double horiz_dpi, double vert_dpi,
                        htsc_vertices_t vertices, htsc_point_t *one_index,
                        spottype_t spot_type, htsc_matrix_t trans_matrix)
{
    int done, dot_index, hole_index, count, index_x, index_y;
    htsc_dot_shape_search_t dot_search;
    int k, val_min;
    htsc_point_t test_point;
    htsc_point_t differ;
    double dist;
    int j;
    htsc_vector_t vector_in, vector_out;

    done = 0;
    dot_index = 1;
    hole_index = N;
    count = 0;
    val_min=MIN(0,v);

    while (!done) {
        /* First perform a search for largest dot value for those remaining 
           dots */
        index_x = 0;
        dot_search.index_x = 0;
        dot_search.index_y = 0;
        dot_search.norm = -100000000; /* Hopefully the dot func is not this small */
        for (k = 0; k < x + u; k++) {
            index_y = 0;
            for (j = val_min; j < y; j++) {
                test_point.x = k + 0.5;
                test_point.y = j + 0.5;
                if ( htsc_getpoint(dig_grid, index_x, index_y) == -1 ) {
                    /* For the spot function we want to make sure that 
                       we are properly adjusted to be in the range from 
                       -1 to +1.  j and k are moving in the transformed 
                       (skewed/rotated) space. First untransform the value */
                    vector_in.xy[0] = k + 0.5;
                    vector_in.xy[1] = j + 0.5;
                    htsc_matrix_vector_mult(trans_matrix, vector_in, 
                                            &vector_out);
                    vector_out.xy[0] = 2.0 * vector_out.xy[0] - 1.0;
                    vector_out.xy[1] = 2.0 * vector_out.xy[1] - 1.0;
                    dist = htsc_spot_value(spot_type, vector_out.xy[0], 
                                           vector_out.xy[1]);
                    if (dist > dot_search.norm) {
                        dot_search.norm = dist;
                        dot_search.index_x = index_x;
                        dot_search.index_y = index_y;
                    }

                }
                index_y++;
            }
            index_x++;
        }
        /* Assign the index for this position */
        htsc_setpoint(dig_grid, dot_search.index_x, dot_search.index_y, 
                      dot_index);
        dot_index++;
        count++;
        if (count == N) {
            done = 1;
            break;
        }
        /* The ones position for the dig_grid is located at the first dot_search 
           entry.  We need this later so grab it now */
        if (count == 1) {
            one_index->x = dot_search.index_x;
            one_index->y = dot_search.index_y;
        }
        /* Now search for the closest one to a vertex (of those remaining).
           and assign the current largest index */
        index_x = 0;
        dot_search.index_x = 0;
        dot_search.index_y = 0;
        dot_search.norm = 10000000000;  /* Or this large */
        for (k = 0; k < x + u; k++) {
            index_y = 0;
            for (j = val_min; j < y; j++) {
                test_point.x = k + 0.5;
                test_point.y = j + 0.5;
                if ( htsc_getpoint(dig_grid, index_x, index_y) == -1 ) {
                    /* For the spot function we want to make sure that 
                       we are properly adjusted to be in the range from 
                       -1 to +1.  j and k are moving in the transformed 
                       (skewed/rotated) space. First untransform the value */
                    vector_in.xy[0] = k + 0.5;
                    vector_in.xy[1] = j + 0.5;
                    htsc_matrix_vector_mult(trans_matrix, vector_in, 
                                            &vector_out);
                    vector_out.xy[0] = 2.0 * vector_out.xy[0] - 1.0;
                    vector_out.xy[1] = 2.0 * vector_out.xy[1] - 1.0;
                    dist = htsc_spot_value(spot_type, vector_out.xy[0], 
                                           vector_out.xy[1]);
                    if (dist < dot_search.norm) {
                        dot_search.norm = dist;
                        dot_search.index_x = index_x;
                        dot_search.index_y = index_y;
                    }
                }
                index_y++;
            }
            index_x++;
        }
        /* Assign the index for this position */
        htsc_setpoint(dig_grid, dot_search.index_x, dot_search.index_y, hole_index);
        hole_index--;
        count++;
        if (count == N) {
            done = 1;
            break;
        }
    }
}

/* Older Euclidean distance approach */
#if 0
void
htsc_create_dot_profile(htsc_dig_grid_t *dig_grid, int N, int x, int y, int u, int v,
                        htsc_point_t center, double horiz_dpi, double vert_dpi,
                        htsc_vertices_t vertices, htsc_point_t *one_index,
                        spottype_t spot_type, htsc_matrix_t trans_matrix)
{
    int done, dot_index, hole_index, count, index_x, index_y;
    htsc_dot_shape_search_t dot_search;
    int k, val_min;
    htsc_point_t test_point;
    htsc_point_t differ;
    double dist;
    int j;

    done = 0;
    dot_index = 1;
    hole_index = N;
    count = 0;
    val_min=MIN(0,v);

    while (!done) {
        /* First perform a search for the closest one to the center
           (of those remaining) and assign the current smallest index */
        index_x = 0;
        dot_search.index_x = 0;
        dot_search.index_y = 0;
        dot_search.norm = 10000000000;
        for (k = 0; k < x + u; k++) {
            index_y = 0;
            for (j = val_min; j < y; j++) {
                test_point.x = k + 0.5;
                test_point.y = j + 0.5;
                if ( htsc_getpoint(dig_grid, index_x, index_y) == -1 ) {
                    htsc_diffpoint(test_point, center, &differ);
                    dist = htsc_normpoint(&differ, horiz_dpi, vert_dpi);
                    if (dist < dot_search.norm) {
                        dot_search.norm = dist;
                        dot_search.index_x = index_x;
                        dot_search.index_y = index_y;
                    }
                }
                index_y++;
            }
            index_x++;
        }
        /* Assign the index for this position */
        htsc_setpoint(dig_grid, dot_search.index_x, dot_search.index_y, dot_index);
        dot_index++;
        count++;
        if (count == N) {
            done = 1;
            break;
        }
        /* The ones position for the dig_grid is locatd at the first dot_search entry */
        /* We need this later so grab it now */
        if (count == 1) {
            one_index->x = dot_search.index_x;
            one_index->y = dot_search.index_y;
        }
        /* Now search for the closest one to a vertix (of those remaining).
           and assign the current largest index */
        index_x = 0;
        dot_search.index_x = 0;
        dot_search.index_y = 0;
        dot_search.norm = 10000000000;
        for (k = 0; k < x + u; k++) {
            index_y = 0;
            for (j = val_min; j < y; j++) {
                test_point.x = k + 0.5;
                test_point.y = j + 0.5;
                if ( htsc_getpoint(dig_grid, index_x, index_y) == -1 ) {
                    /* Find closest to a vertex */
                    dist = htsc_vertex_dist(vertices, test_point, horiz_dpi,
                                            vert_dpi);
                    if (dist < dot_search.norm) {
                        dot_search.norm = dist;
                        dot_search.index_x = index_x;
                        dot_search.index_y = index_y;
                    }
                }
                index_y++;
            }
            index_x++;
        }
        /* Assign the index for this position */
        htsc_setpoint(dig_grid, dot_search.index_x, dot_search.index_y, hole_index);
        hole_index--;
        count++;
        if (count == N) {
            done = 1;
            break;
        }
    }
}
#endif

/* This creates a mask for creating the dot shape */
void
htsc_create_dot_mask(htsc_dig_grid_t *dot_grid, int x, int y, int u, int v,
                 double screen_angle, htsc_vertices_t vertices)
{
    int k,j,val_min,index_x,index_y;
    double slope1, slope2;
    int t1,t2,t3,t4,point_in;
    float b3, b4;
    htsc_point_t test_point;

    if (screen_angle != 0) {
        slope1 = (double) y / (double) x;
        slope2 = (double) v / (double) u;
        val_min=MIN(0,v);
        dot_grid->height = abs(val_min) + y;
        dot_grid->width = x + u;
        dot_grid->data =
            (int *) malloc(dot_grid->height * dot_grid->width * sizeof(int));
        memset(dot_grid->data,0,dot_grid->height * dot_grid->width * sizeof(int));
        index_x = 0;
        for (k = 0; k < (x+u); k++) {
            index_y=0;
            for (j = val_min; j < y; j++) {
                test_point.x = k + 0.5;
                test_point.y = j + 0.5;
                /* test 1 and 2 */
                t1 = (slope1 * test_point.x >= test_point.y);
                t2 = (slope2 * test_point.x <= test_point.y);
                /* test 3 */
                b3 = vertices.upper_left.y - slope2 * vertices.upper_left.x;
                t3 = ((slope2 * test_point.x + b3) > test_point.y);
                /* test 4 */
                b4 = vertices.lower_right.y - slope1 * vertices.lower_right.x;
                t4=((slope1 * test_point.x + b4) < test_point.y);
                point_in = (t1 && t2 && t3 && t4);
                if (point_in) {
                    htsc_setpoint(dot_grid, index_x, index_y, -1);
                }
                index_y++;
            }
            index_x++;
        }
    } else {
        /* All points are valid */
        dot_grid->height = y;
        dot_grid->width = u;
        dot_grid->data = (int *) malloc(y * u * sizeof(int));
        memset(dot_grid->data, -1, y * u * sizeof(int));
        val_min = 0;
    }
    return;
}

double
htsc_vertex_dist(htsc_vertices_t vertices, htsc_point_t test_point,
                 double horiz_dpi, double vert_dpi)
{
    double dist[4], smallest;
    htsc_point_t diff_point;
    int k;

    htsc_diffpoint(vertices.lower_left, test_point, &diff_point);
    dist[0] = htsc_normpoint(&diff_point, horiz_dpi, vert_dpi);
    htsc_diffpoint(vertices.lower_right, test_point, &diff_point);
    dist[1] = htsc_normpoint(&diff_point, horiz_dpi, vert_dpi);
    htsc_diffpoint(vertices.upper_left, test_point, &diff_point);
    dist[2] = htsc_normpoint(&diff_point, horiz_dpi, vert_dpi);
    htsc_diffpoint(vertices.upper_right, test_point, &diff_point);
    dist[3] = htsc_normpoint(&diff_point, horiz_dpi, vert_dpi);
    smallest = dist[0];
    for (k = 1; k < 4; k++) {
        if (dist[k] < smallest) {
            smallest = dist[k];
        }
    }
    return smallest;
}

double
htsc_normpoint(htsc_point_t *diff_point, double horiz_dpi, double vert_dpi)
{
    diff_point->x =  diff_point->x / horiz_dpi;
    diff_point->y =  diff_point->y / vert_dpi;
    return  (diff_point->x * diff_point->x) + (diff_point->y * diff_point->y);
}

int
htsc_getpoint(htsc_dig_grid_t *dig_grid, int x, int y)
{
    return dig_grid->data[ y * dig_grid->width + x];
}

void
htsc_setpoint(htsc_dig_grid_t *dig_grid, int x, int y, int value)
{
    int kk;
    if (x < 0 || x > dig_grid->width-1 || y < 0 || y > dig_grid->height-1) {
        kk = 10000;  /* Here to catch issues during debug */
    }
    dig_grid->data[ y * dig_grid->width + x] = value;
}

void
htsc_diffpoint(htsc_point_t point1, htsc_point_t point2,
              htsc_point_t *diff_point)
{

    diff_point->x = point1.x - point2.x;
    diff_point->y = point1.y - point2.y;
}

int
htsc_sumsum(htsc_dig_grid_t dig_grid)
{

    int pos;
    int grid_size =  dig_grid.width *  dig_grid.height;
    int value = 0;
    int *ptr = dig_grid.data;

    for (pos = 0; pos < grid_size; pos++) {
        value += (*ptr++);
    }
    return value;
}

int
htsc_gcd(int a, int b)
{
    if ( b == 0 ) return a;
    if ( a == 0 ) return b;
    if ( a == 0 && b == 0 ) return 0;
    while (1) {
        a = a % b;
        if (a == 0) {
            return b;
        }
        b = b % a;
        if (b == 0) {
            return a;
        }
    }
}

int
htsc_lcm(int a, int b)
{
    int product = a * b;
    int gcd = htsc_gcd(a,b);
    int lcm;

    lcm = product/gcd;
    return lcm;
}

int
htsc_matrix_inverse(htsc_matrix_t matrix_in, htsc_matrix_t *matrix_out)
{
    double determinant;

    determinant = matrix_in.row[0].xy[0] * matrix_in.row[1].xy[1] -
                  matrix_in.row[0].xy[1] * matrix_in.row[1].xy[0];
    if (determinant == 0)
        return -1;
    matrix_out->row[0].xy[0] = matrix_in.row[1].xy[1] / determinant;
    matrix_out->row[0].xy[1] = -matrix_in.row[0].xy[1] / determinant;
    matrix_out->row[1].xy[0] = -matrix_in.row[1].xy[0] / determinant;
    matrix_out->row[1].xy[1] = matrix_in.row[0].xy[0] / determinant;
    return 0;
}

void
htsc_matrix_vector_mult(htsc_matrix_t matrix_in, htsc_vector_t vector_in,
                   htsc_vector_t *vector_out)
{
    vector_out->xy[0] = matrix_in.row[0].xy[0] * vector_in.xy[0] +
                        matrix_in.row[0].xy[1] * vector_in.xy[1];
    vector_out->xy[1] = matrix_in.row[1].xy[0] * vector_in.xy[0] +
                        matrix_in.row[1].xy[1] * vector_in.xy[1];
}

int
htsc_allocate_supercell(htsc_dig_grid_t *super_cell, int x, int y, int u,
                        int v, int target_size, bool use_holladay_grid,
                        htsc_dig_grid_t dot_grid, int N, int *S, int *H, int *L)
{
    htsc_matrix_t matrix;
    htsc_matrix_t matrix_inv;
    int code;
    int k, j;
    htsc_vector_t vector_in, m_and_n;
    int Dfinal;
    double diff_val[2];
    double m_and_n_round;
    int lcm_value;
    int super_size_x, super_size_y;
    int min_vert_number;
    int a, b;

    /* Use Holladay Algorithm to create rectangular matrix for screening */
    *H = htsc_gcd((int) abs(y), (int) abs(v));
    *L = N / *H;
    /* Compute the shift factor */
    matrix.row[0].xy[0] = x;
    matrix.row[0].xy[1] = u;
    matrix.row[1].xy[0] = y;
    matrix.row[1].xy[1] = v;

    code = htsc_matrix_inverse(matrix, &matrix_inv);
    if (code < 0) {
        printf("ERROR! matrix singular!\n");
        return -1;
    }
    vector_in.xy[1] = *H;
    Dfinal = 0;
    for (k = 1; k < *L+1; k++) {
        vector_in.xy[0] = k;
        htsc_matrix_vector_mult(matrix_inv, vector_in, &m_and_n);
        for (j = 0; j < 2; j++) {
            m_and_n_round = ROUND(m_and_n.xy[j]);
            diff_val[j] = fabs((double) m_and_n.xy[j] -  (double) m_and_n_round);
        }
        if (diff_val[0] < 0.00000001 && diff_val[1] < 0.00000001) {
            Dfinal = k;
            break;
        }
    }
    if (Dfinal == 0) {
        printf("ERROR! computing Holladay Grid\n");
        return -1;
    }
    *S = *L - Dfinal;
  /* Make a large screen of multiple cells and then vary
     the growth rate of the dots to get additional quantization levels
     The macrocell must be H*a by L*b where a and b are integers due to the
     periodicity of the screen. */

    /* To create the Holladay screen (no stocastic stuff),
       select the size H and L to create the matrix i.e. super_size_x
       and super_size_y need to be H by L at least and then just take an
       H by L section. */
    /* Force a periodicity in the screen to avoid the shift factor */
    if (*S != 0) {
        lcm_value = htsc_lcm(*L,*S);
        min_vert_number = *H * lcm_value / *S;
    } else {
        lcm_value = *L;
        min_vert_number = *H;
    }

    a = ceil((float) target_size / (float) lcm_value);
    b = ceil((float) target_size / (float) min_vert_number);

    /* super_cell Size is  b*min_vert_number by a*lcm_value
       create the large cell */

    if (use_holladay_grid) {
        super_size_x = MAX(*L, dot_grid.width);
        super_size_y = MAX(*H, dot_grid.height);
    } else {
        super_size_x = a * lcm_value;
        super_size_y = b * min_vert_number;
    }
    super_cell->height = super_size_y;
    super_cell->width = super_size_x;
    super_cell->data =
        (int *) malloc(super_size_x * super_size_y * sizeof(int));
    memset(super_cell->data, 0, super_size_x * super_size_y * sizeof(int));
    return 0;
}

static void
htsc_supercell_assign_point(int new_k, int new_j, int sc_xsize, int sc_ysize,
                            htsc_dig_grid_t *super_cell, int val, int *num_set )
{
    if (new_k >= 0 && new_j >= 0 && new_k < sc_xsize && new_j < sc_ysize) {
        if (htsc_getpoint(super_cell,new_k,new_j) == 0) {
            htsc_setpoint(super_cell,new_k,new_j,val);
            (*num_set)++;
        }
    }
}

void
htsc_tile_supercell(htsc_dig_grid_t *super_cell, htsc_dig_grid_t *dot_grid,
                 int x, int y, int u, int v, int N)
{
    int sc_ysize = super_cell->height;
    int sc_xsize = super_cell->width;
    int dot_ysize = dot_grid->height;
    int dot_xsize = dot_grid->width;
    int total_num = sc_ysize * sc_xsize;
    bool done = false;
    int k,j;
    int new_k, new_j;
    int num_set = 0;
    int val;

    for (k = 0; k < dot_xsize; k++) {
        for (j = 0; j < dot_ysize; j++) {
            val = htsc_getpoint(dot_grid,k,j);
            if (val > 0) {
                htsc_setpoint(super_cell,k,j,val);
                num_set++;
            }
        }
    }
    if (num_set == total_num) {
            done = true;
    }
    while (!done) {
        for (k = 0; k < sc_xsize; k++) {
            for (j = 0; j < sc_ysize; j++) {
                val = htsc_getpoint(super_cell,k,j);
                if (val != 0) {
                    new_k = k - x;
                    new_j = j - y;
                    htsc_supercell_assign_point(new_k, new_j, sc_xsize, sc_ysize,
                                                super_cell, val, &num_set);
                    new_k = k + x;
                    new_j = j + y;
                    htsc_supercell_assign_point(new_k, new_j, sc_xsize, sc_ysize,
                                                super_cell, val, &num_set);
                    new_k = k - u;
                    new_j = j - v;
                    htsc_supercell_assign_point(new_k, new_j, sc_xsize, sc_ysize,
                                                super_cell, val, &num_set);
                    new_k = k + u;
                    new_j = j + v;
                    htsc_supercell_assign_point(new_k, new_j, sc_xsize, sc_ysize,
                                                super_cell, val, &num_set);
                }
            }
        }
        if (num_set == total_num) {
            done = true;
        }
    }
}

/* Create 2d gaussian filter that varies with respect to coordinate
   spatial resolution */
void
create_2d_gauss_filter(float *filter, int x_size, int y_size,
                       float stdvalx, float stdvaly)
{
    int x_half_size  = (x_size-1)/2;
    int y_half_size = (y_size-1)/2;
    int k,j;
    float arg, val;
    double sum = 0;
    float max_val = 0;
    int total_size = x_size * y_size;
    int index_x, index_y;

    for (j = -y_half_size; j < y_half_size+1; j++) {
        index_y = j + y_half_size;
        for (k = -x_half_size; k < x_half_size+1; k++) {
            arg   = -(k  * k / (stdvalx * stdvalx) +
                      j * j / (stdvaly * stdvaly) ) /2;
            val = (float) exp(arg);
            sum += val;
            if (val > max_val) max_val = val;
            index_x = k + x_half_size;
            filter[index_y * x_size + index_x] = val;
        }
    }
    for (j = 0; j < total_size; j++) {
        filter[j]/=sum;
    }
#if RAW_SCREEN_DUMP
    htsc_dump_float_image(filter, y_size, x_size, max_val/sum, "guass_filt");
#endif
}

/* 2-D convolution (or correlation) with periodic boundary condition */
void
htsc_apply_filter(byte *screen_matrix, int num_cols_sc,
                  int num_rows_sc, float *filter, int num_cols_filt,
                  int num_rows_filt, float *screen_blur,
                  float *max_val, htsc_point_t *max_pos, float *min_val,
                  htsc_point_t *min_pos)
{
    int k,j,kk,jj;
    double sum;
    int half_cols_filt = (num_cols_filt-1)/2;
    int half_rows_filt = (num_rows_filt-1)/2;
    int j_circ,k_circ;
    float fmax_val = -1;
    float fmin_val = 100000000;
    htsc_point_t fmax_pos, fmin_pos;

    for (j = 0; j < num_rows_sc; j++) {
        for (k = 0; k < num_cols_sc; k++) {
            sum = 0.0;
            for (jj = -half_rows_filt; jj <= half_rows_filt; jj++) {
                j_circ = j + jj;
                if (j_circ < 0) {
                    j_circ =
                        (num_rows_sc - ((-j_circ) % num_rows_sc)) % num_rows_sc;
                }
                if (j_circ > (num_rows_sc - 1)) {
                    j_circ = j_circ % num_rows_sc;
                }
                /* In case modulo is of a negative number */
                if (j_circ < 0) 
                    j_circ = j_circ + num_rows_sc;
                for (kk = -half_cols_filt; kk <= half_cols_filt; kk++) {
                    k_circ = k + kk;
                    if (k_circ < 0) {
                        k_circ =
                            (num_cols_sc - ((-k_circ) % num_cols_sc)) % num_cols_sc;
                    }
                    if (k_circ > (num_cols_sc - 1)) {
                        k_circ = k_circ % num_cols_sc;
                    }
                    /* In case modulo is of a negative number */
                    if (k_circ < 0) 
                        k_circ = k_circ + num_cols_sc;
                    sum += (double) screen_matrix[k_circ + j_circ * num_cols_sc] *
                            filter[ (jj + half_rows_filt) * num_cols_filt + (kk + half_cols_filt)];
                }
            }
            screen_blur[j * num_cols_sc + k] = sum;
            if (sum > fmax_val) {
                fmax_val = sum;
                fmax_pos.x = k;
                fmax_pos.y = j;
            }
            if (sum < fmin_val) {
                fmin_val = sum;
                fmin_pos.x = k;
                fmin_pos.y = j;
            }
        }
    }
    *max_val = fmax_val;
    *min_val = fmin_val;
    *max_pos = fmax_pos;
    *min_pos = fmin_pos;
}

int
htsc_add_dots(byte *screen_matrix, int num_cols, int num_rows,
                       double horiz_dpi, double vert_dpi, double lpi_act,
                       unsigned short *pos_x, unsigned short *pos_y,
                       int *locate, int num_dots,
                       htsc_dither_pos_t *dot_level_position, int level_num,
                       int num_dots_add)
{
    double xscale = horiz_dpi / vert_dpi;
    double sigma_y = vert_dpi / lpi_act;
    double sigma_x = sigma_y * xscale;
    int sizefiltx, sizefilty;
    float *filter;
    bool done = false;
    float *screen_blur;
    int white_pos;
    float max_val, min_val;
    htsc_point_t max_pos, min_pos;
    int k,j;
    int dist, curr_dist;
    htsc_dither_pos_t curr_position;

    sizefiltx = ROUND(sigma_x * 4);
    sizefilty = ROUND(sigma_y * 4);
    if ( ((float) sizefiltx / 2.0) == (sizefiltx >> 1)) {
        sizefiltx += 1;
    }
    if ( ((float) sizefilty / 2.0) == (sizefilty >> 1)) {
        sizefilty += 1;
    }
    filter = (float*) malloc(sizeof(float) * sizefilty * sizefiltx);
    create_2d_gauss_filter(filter, sizefiltx, sizefilty, sizefiltx, sizefilty);
    screen_blur = (float*) malloc(sizeof(float) * num_cols * num_rows);

    for (j = 0; j < num_dots_add; j++) {
        /* Perform the blur */
        htsc_apply_filter(screen_matrix, num_cols, num_rows, filter, sizefiltx,
                  sizefilty, screen_blur, &max_val, &max_pos, &min_val, &min_pos);
        /* Find the closest OFF dot to the min position.  */
        white_pos = 0;
        dist = (pos_y[0] - min_pos.y) * (pos_y[0] - min_pos.y) +
               (pos_x[0] - min_pos.x) * (pos_x[0] - min_pos.x);
        for ( k = 1; k < num_dots; k++) {
            curr_dist = (pos_y[k] - min_pos.y) * (pos_y[k] - min_pos.y) +
                        (pos_x[k] - min_pos.x) * (pos_x[k] - min_pos.x);
            if (curr_dist < dist &&
                screen_matrix[pos_x[k] + num_cols * pos_y[k]] == 0) {
                white_pos = k;
                dist = curr_dist;
            }
        }
        /* Set this dot to white */
        screen_matrix[pos_x[white_pos] + num_cols * pos_y[white_pos]] = 1;
        /* Update position information */
        curr_position = dot_level_position[level_num];
        curr_position.point[j].x = pos_x[white_pos];
        curr_position.point[j].y = pos_y[white_pos];
        curr_position.locations[j] = locate[white_pos];
    }
    free(filter);
    free(screen_blur);
    return 0;
}

int
htsc_init_dot_position(byte *screen_matrix, int num_cols, int num_rows,
                       double horiz_dpi, double vert_dpi, double lpi_act,
                       unsigned short *pos_x, unsigned short *pos_y, int num_dots,
                       htsc_dither_pos_t *dot_level_position)
{
    double xscale = horiz_dpi / vert_dpi;
    double sigma_y = vert_dpi / lpi_act;
    double sigma_x = sigma_y * xscale;
    int sizefiltx, sizefilty;
    float *filter;
    bool done = false;
    float *screen_blur;
    int white_pos, black_pos;
    float max_val, min_val;
    htsc_point_t max_pos, min_pos;
    int k;
    int dist, curr_dist;
    bool found_it;

    sizefiltx = ROUND(sigma_x * 4);
    sizefilty = ROUND(sigma_y * 4);
    if ( ((float) sizefiltx / 2.0) == (sizefiltx >> 1)) {
        sizefiltx += 1;
    }
    if ( ((float) sizefilty / 2.0) == (sizefilty >> 1)) {
        sizefilty += 1;
    }
    filter = (float*) malloc(sizeof(float) * sizefilty * sizefiltx);
    create_2d_gauss_filter(filter, sizefiltx, sizefilty, sizefiltx, sizefilty);
    screen_blur = (float*) malloc(sizeof(float) * num_cols * num_rows);
    /* Start moving dots until the whitest and darkest dot are the same */
    while (!done) {
        /* Blur */
        htsc_apply_filter(screen_matrix, num_cols, num_rows, filter, sizefiltx,
                  sizefilty, screen_blur, &max_val, &max_pos, &min_val, &min_pos);
#if RAW_SCREEN_DUMP
        htsc_dump_float_image(screen_blur, num_cols, num_rows, max_val, "blur_one");
#endif
    /* Find the closest on dot to the max position */
        black_pos = 0;
        dist = (pos_y[0] - max_pos.y) * (pos_y[0] - max_pos.y) +
               (pos_x[0] - max_pos.x) * (pos_x[0] - max_pos.x);
        for ( k = 1; k < num_dots; k++) {
            curr_dist = (pos_y[k] - max_pos.y) * (pos_y[k] - max_pos.y) +
                        (pos_x[k] - max_pos.x) * (pos_x[k] - max_pos.x);
            if (curr_dist < dist &&
                screen_matrix[pos_x[k] + num_cols * pos_y[k]] == 1) {
                black_pos = k;
                dist = curr_dist;
            }
        }
        /* Set this dot to black */
        screen_matrix[pos_x[black_pos] + num_cols * pos_y[black_pos]] = 0;
        /* Blur again */
        htsc_apply_filter(screen_matrix, num_cols, num_rows, filter, sizefiltx,
                  sizefilty, screen_blur, &max_val, &max_pos, &min_val, &min_pos);
        /* Find the closest OFF dot to the min position. */
        white_pos = 0;
        dist = (pos_y[0] - min_pos.y) * (pos_y[0] - min_pos.y) +
               (pos_x[0] - min_pos.x) * (pos_x[0] - min_pos.x);
        for ( k = 1; k < num_dots; k++) {
            curr_dist = (pos_y[k] - min_pos.y) * (pos_y[k] - min_pos.y) +
                        (pos_x[k] - min_pos.x) * (pos_x[k] - min_pos.x);
            if (curr_dist < dist &&
                screen_matrix[pos_x[k] + num_cols * pos_y[k]] == 0) {
                white_pos = k;
                dist = curr_dist;
            }
        }
        /* Set this dot to white */
        screen_matrix[pos_x[white_pos] + num_cols * pos_y[white_pos]] = 1;
        /* If it is the same dot as before, then we are done */
        /* There could be a danger here of cycles longer than 2 */
        if (white_pos == black_pos) {
            done = true;
            free(screen_blur);
            free(filter);
            return 0;
        } else {
            /* We need to update our dot position information */
            /* find where the old white one was and replace it */
            found_it = false;
            for (k = 0; k < dot_level_position->number_points; k++) {
                if (dot_level_position->point[k].x == pos_x[black_pos] &&
                    dot_level_position->point[k].y == pos_y[black_pos]) {
                    found_it = true;
                    dot_level_position->point[k].x = pos_x[white_pos];
                    dot_level_position->point[k].y = pos_y[white_pos];
                    dot_level_position->locations[k] =
                        pos_x[white_pos] + pos_y[white_pos] * num_cols;
                    break;
                }
            }
            if (!found_it) {
                printf("ERROR! bug in dot location accounting\n");
                return -1;
            }
        }
    }
    free(filter);
    free(screen_blur);
    return 0;
}

void
htsc_create_dither_mask(htsc_dig_grid_t super_cell, htsc_dig_grid_t *final_mask,
                          int num_levels, int y, int x, double vert_dpi,
                          double horiz_dpi, int N, double gamma,
                          htsc_dig_grid_t dot_grid, htsc_point_t dot_grid_one_index)
{
    int *dot_levels;
    int *locate;
    double percent, perc_val;
    int k, j, h, jj, mm;
    int width_supercell = super_cell.width;
    int height_supercell = super_cell.height;
    int number_points = width_supercell * height_supercell;
    int num_dots = 0;
    double step_size;
    int curr_size;
    int rand_pos, dots_needed;
    int done;
    double lpi_act;
    double vert_scale = ((double) y / vert_dpi);
    double horiz_scale = ((double) x / horiz_dpi);
    byte *screen_matrix;
    unsigned short *pos_x, *pos_y;
    htsc_dither_pos_t *dot_level_pos, *curr_dot_level;
    int count;
    int code;
    int prev_dot_level, curr_num_dots;
    double mag_offset, temp_dbl;
    int *thresholds, val;
    int j_index, k_index, threshold_value;
    int *dot_level_sort;
    bool found;

    lpi_act = 1.0/((double) sqrt( vert_scale * vert_scale +
                                  horiz_scale * horiz_scale));
    if (num_levels > 1) {
        curr_size = 2 * MAX(height_supercell, width_supercell);
        locate = (int*) malloc(sizeof(int) * curr_size);
        screen_matrix = (byte*) malloc(sizeof(byte) * number_points);
        memset(screen_matrix, 0, sizeof(byte) * number_points);
        /* Determine the number of dots in the screen and their index */
        for (j = 0; j < number_points; j++) {
            if (super_cell.data[j] == 1) {
                locate[num_dots] = j;
                num_dots++;
                if (num_dots == (curr_size - 1)) {
                    curr_size = curr_size * 2;
                    locate = (int*) realloc(locate, sizeof(int) * curr_size);
                }
            }
        }
       /* Convert the 1-D locate positions to 2-D positions so that we can
          use a distance metric to the dot center locations. Also allocate
          the structure for our dot positioning information */
        pos_x = (unsigned short*) malloc(sizeof(unsigned short) * num_dots);
        pos_y = (unsigned short*) malloc(sizeof(unsigned short) * num_dots);
        for (k = 0; k < num_dots; k++) {
            pos_x[k] = locate[k] % width_supercell;
            pos_y[k] = (locate[k] - pos_x[k])/width_supercell;
        }
        dot_level_pos =
            (htsc_dither_pos_t*) malloc(sizeof(htsc_dither_pos_t) * num_levels);
        dot_levels = (int*) malloc(sizeof(int) * num_levels);
        percent = 1.0/((float)num_levels+1.0);
        prev_dot_level = 0;
        for ( k = 0; k < num_levels; k++) {
            perc_val = (k + 1) * percent;
            dot_levels[k] = ROUND(num_dots * perc_val);
            curr_num_dots = dot_levels[k] -prev_dot_level;
            prev_dot_level = dot_levels[k];
            dot_level_pos[k].locations = (int*) malloc(sizeof(int) * curr_num_dots);
            dot_level_pos[k].point =
                (htsc_point_t*) malloc(sizeof(htsc_point_t) * curr_num_dots);
            dot_level_pos[k].number_points = curr_num_dots;
        }
        /* An initial random location for the first level  */
        dots_needed = dot_levels[0];
        count = 0;
        if (dots_needed > 0) {
            done = 0;
            while (!done) {
                rand_pos = ROUND((num_dots-1)*(double) rand()/ (double) RAND_MAX);
                if (screen_matrix[locate[rand_pos]] != 1) {
                    screen_matrix[locate[rand_pos]] = 1;
                    dot_level_pos->locations[count] = locate[rand_pos];
                    dot_level_pos->point[count].x = pos_x[rand_pos];
                    dot_level_pos->point[count].y = pos_y[rand_pos];
                    dots_needed--;
                    count++;
                }
                if (dots_needed == 0) {
                    done = 1;
                }
            }
        }
        /* Rearrange these dots into a pleasing pattern */
#if RAW_SCREEN_DUMP
        htsc_dump_byte_image(screen_matrix, width_supercell, height_supercell,
                             1, "screen0_init");
#endif
        code = htsc_init_dot_position(screen_matrix, width_supercell,
                               height_supercell, horiz_dpi, vert_dpi, lpi_act,
                               pos_x, pos_y, num_dots, dot_level_pos);
#if RAW_SCREEN_DUMP
        htsc_dump_byte_image(screen_matrix, width_supercell, height_supercell,
                             1, "screen0_arrange");
#endif
        /* Now we want to introduce more dots at each level */
        for (k = 1; k < num_levels; k++) {
            code = htsc_add_dots(screen_matrix, width_supercell, height_supercell,
                          horiz_dpi, vert_dpi, lpi_act, pos_x, pos_y, locate,
                          num_dots, dot_level_pos, k,
                          dot_level_pos[k].number_points);
#if RAW_SCREEN_DUMP
            {
            char str_name[30];
            sprintf(str_name,"screen%d_arrange",k);
            htsc_dump_byte_image(screen_matrix, width_supercell, height_supercell,
                                 1, str_name);
            }
#endif
        }
        /* Create the threshold mask. */
        step_size = (MAXVAL + 1.0) / (double) N;
        thresholds = (int*) malloc(sizeof(int) * N);
        for (k = 0; k < N; k++) {
            thresholds[N-1-k] = (k + 1) * step_size - (step_size / 2);
        }
        mag_offset =
            (double) (thresholds[0]-thresholds[1]) / (double) (num_levels+1);
        if ( gamma != 1.0) {
            for (k = 0; k < N; k++) {
                temp_dbl = 
                    (double) pow((double) thresholds[k] / MAXVAL, 
                                 (double) gamma);
                thresholds[k] = ROUND(temp_dbl * MAXVAL);
            }
        }
        /* Now use the indices from the large screen to look up the mask and
           apply the offset to the threshold values to dither the rate at which
           the dots turn on */
        /* allocate the mask */
        final_mask->height = height_supercell;
        final_mask->width = width_supercell;
        final_mask->data =
            (int*) malloc(sizeof(int) * height_supercell * width_supercell);
        /* We need to associate the locate index with a particular level
           for the when the dot begins to turn on.  Go through the dot_level_pos
           array to get the values.  Probably should create this earlier and avoid
           this */
        dot_level_sort = (int*) malloc(sizeof(int) * num_dots);
        for (h = 0; h < num_dots; h++) {
            found = false;
            for (jj = 0; jj < num_levels; jj++) {
                curr_dot_level = &(dot_level_pos[jj]);
                for (mm = 0; mm < curr_dot_level->number_points; mm++) {
                    if (pos_x[h] == curr_dot_level->point[mm].x &&
                        pos_y[h] == curr_dot_level->point[mm].y ) {
                        found = true;
                        dot_level_sort[h] = jj + 1;
                        break;  /* Break from position search(within level) */
                    }
                }
                if (found == true) { /* break from level search */
                    break;
                }
            }
            if (found == false) {
                dot_level_sort[h] = 0;
            }
        }

        for (h = 0; h < num_dots; h++) {
            for (j = 0; j < dot_grid.height; j++) {
                for (k = 0; k < dot_grid.width; k++) {
                    val = htsc_getpoint(&dot_grid, k, j);
                    if (val != 0) {
                        /* Assign a offset threshold values */
                        j_index =
                            (pos_y[h] + j - (int) dot_grid_one_index.y) % height_supercell;
                        /* In case we have modulo of a negative number */
                        if (j_index < 0) j_index = j_index + height_supercell;
                        k_index =
                            (pos_x[h] + k - (int) dot_grid_one_index.x) % width_supercell;
                        /* In case we have modulo of a negative number */
                        if (k_index < 0) k_index = k_index + width_supercell;
                        threshold_value = thresholds[val-1] +
                                          mag_offset * dot_level_sort[h];
                        if (threshold_value > MAXVAL) threshold_value = MAXVAL;
                        if (threshold_value < 0) threshold_value = 0;
                        htsc_setpoint(final_mask,k_index,j_index,threshold_value);
                    }
                }
            }
        }
        for (k = 0; k < num_levels; k++) {
            free(dot_level_pos[k].locations);
            free(dot_level_pos[k].point);
        }
        free(locate);
        free(screen_matrix);
        free(pos_x);
        free(pos_y);
        free(dot_level_pos);
        free(dot_levels);
        free(thresholds);
        free(dot_level_sort);
    }
}

void
htsc_create_holladay_mask(htsc_dig_grid_t super_cell, int H, int L,
                          double gamma, htsc_dig_grid_t *final_mask)
{
    double step_size = (MAXVAL + 1.0) /( (double) H * (double) L);
    int k, j;
    double *thresholds;
    int number_points = H * L;
    double half_step = step_size / 2.0;
    double temp;
    int index_point;
    int value;
    double white_scale = 253.0 / 255.0; /* To ensure white is white */

    final_mask->height = H;
    final_mask->width = L;
    final_mask->data = (int *) malloc(H * L * sizeof(int));

    thresholds = (double *) malloc(H * L * sizeof(double));
    for (k = 0; k < number_points; k++) {
         temp = ((k+1) * step_size - half_step) / MAXVAL;
         if ( gamma != 1.0) {
             /* Possible linearization */
            temp = (double) pow((double) temp, (double) gamma);
         }
         thresholds[number_points - k - 1] = 
                                    ROUND(temp * MAXVAL * white_scale + 1);
    }
    memset(final_mask->data, 0, H * L * sizeof(int));

    for (j = 0; j < H; j++) {
        for (k = 0; k < L; k++) {
            index_point = htsc_getpoint(&super_cell,k,j) - 1;
            value = (int) floor(thresholds[index_point]);
            htsc_setpoint(final_mask,k,j,value);
        }
    }
    free(thresholds);
}

void
htsc_create_nondithered_mask(htsc_dig_grid_t super_cell, int H, int L,
                          double gamma, htsc_dig_grid_t *final_mask)
{
    double step_size = (MAXVAL +  1) /( (double) H * (double) L);
    int k, j;
    double *thresholds;
    int number_points = H * L;
    double half_step = step_size / 2.0;
    double temp;
    int index_point;
    int value;
    double white_scale = 253.0 / 255.0; /* To ensure white is white */

    final_mask->height = super_cell.height;
    final_mask->width = super_cell.width;
    final_mask->data = (int *) malloc(super_cell.height * super_cell.width *
                                      sizeof(int));
    thresholds = (double *) malloc(H * L * sizeof(double));
    for (k = 0; k < number_points; k++) {
         temp = ((k+1) * step_size - half_step) / MAXVAL;
         if ( gamma != 1.0) {
             /* Possible linearization */
            temp = (double) pow((double) temp, (double) gamma);
         }
         thresholds[number_points - k - 1] = 
                                    ROUND(temp * MAXVAL * white_scale + 1);
    }
    memset(final_mask->data, 0, super_cell.height * super_cell.width *
                                sizeof(int));
    for (j = 0; j < super_cell.height; j++) {
        for (k = 0; k < super_cell.width; k++) {
            index_point = htsc_getpoint(&super_cell,k,j) - 1;
            value = (int) floor(thresholds[index_point]);
            htsc_setpoint(final_mask,k,j,value);
        }
    }
    free(thresholds);
}

int compare (const void * a, const void * b)
{
    const htsc_threshpoint_t *val_a = a;
    const htsc_threshpoint_t *val_b = b;

  return val_a->value - val_b->value;
}

/* Save turn on order list */
void 
htsc_save_tos(htsc_dig_grid_t final_mask)
{
    int width = final_mask.width;
    int height = final_mask.height;
    int *buff_ptr = final_mask.data;
    FILE *fid;
    htsc_threshpoint_t *values;
    int x, y, k =0;
    int count= height * width;

    fid = fopen("turn_on_seq.out","w");
    fprintf(fid,"# W=%d H=%d\n",width, height);
    /* Do a sort on the values and then output the coordinates */
    /* First get a list made with the unsorted values and coordinates */
    values = 
        (htsc_threshpoint_t *) malloc(sizeof(htsc_threshpoint_t) * width * height);
    if (values == NULL) {
        printf("ERROR! malloc failure in htsc_save_tos!\n");
        return;
    }
    for (y = 0; y < height; y++) {
        for ( x = 0; x < width; x++ ) {
            values[k].value = *buff_ptr;
            values[k].x = x;
            values[k].y = y;
            buff_ptr++;
            k = k + 1;
        }
    }
    /* Sort */
    qsort(values, height * width, sizeof(htsc_threshpoint_t), compare);
    /* Write out */
    for (k = 0; k < count; k++) {
        fprintf(fid,"%d\t%d\n", values[count - 1 - k].x, values[count - 1 - k].y); 
    } 
    free(values);
    fclose(fid);
}

void
htsc_save_screen(htsc_dig_grid_t final_mask, bool use_holladay_grid, int S,
                htsc_param_t params)
{
    char full_file_name[50];
    FILE *fid;
    int x,y;
    int *buff_ptr = final_mask.data;
    int width = final_mask.width;
    int height = final_mask.height;
    byte data;
    unsigned short data_short;
    output_format_type output_format = params.output_format;
    char *output_extension = (output_format == OUTPUT_PS) ? "ps" :
                        ((output_format == OUTPUT_PPM) ? "ppm" : 
			((output_format == OUTPUT_RAW16 ? "16.raw" : "raw")));

    if (output_format == OUTPUT_TOS) {
        /* We need to figure out the turn-on sequence from the threshold 
           array */
        htsc_save_tos(final_mask);
    } else {
        if (use_holladay_grid) {
            sprintf(full_file_name,"Screen_Holladay_Shift%d_%dx%d.%s", S, width,
                    height, output_extension);
        } else {
            sprintf(full_file_name,"Screen_Dithered_%dx%d.%s",width,height,
                    output_extension);
        }
        fid = fopen(full_file_name,"wb");

        if (output_format == OUTPUT_PPM)
            fprintf(fid, "P5\n"
                    "# Halftone threshold array, %s, [%d, %d], S=%d\n"
                    "%d %d\n"
                    "255\n",
                    use_holladay_grid ? "Holladay_Shift" : "Dithered", width, height,
                    S, width, height);
        if (output_format != OUTPUT_PS) {
            /* Both BIN and PPM format write the same binary data */
            if (output_format == OUTPUT_RAW || output_format == OUTPUT_PPM) {
                for (y = 0; y < height; y++) {
                    for ( x = 0; x < width; x++ ) {
                        data_short = (unsigned short) (*buff_ptr & 0xffff);
                        data_short = ROUND((double) data_short * 255.0 / MAXVAL);
                        if (data_short > 255) data_short = 255;
                        data = (byte) (data_short & 0xff);
                        fwrite(&data, sizeof(byte), 1, fid);
                        buff_ptr++;
                    }
                }
            } else {	/* raw16 data */
                for (y = 0; y < height; y++) {
                    for ( x = 0; x < width; x++ ) {
                        data_short = (unsigned short) (*buff_ptr & 0xffff);
                        fwrite(&data_short, sizeof(short), 1, fid);
                        buff_ptr++;
                    }
                }
            }
        } else {	/* ps output format */
            if (params.targ_quant <= 256) {
                /* Output PS HalftoneType 3 dictionary */
                fprintf(fid, "%%!PS\n"
                        "<< /HalftoneType 3\n"
                        "   /Width  %d\n"
                        "   /Height %d\n"
                        "   /Thresholds <\n",
                        width, height);

                for (y = 0; y < height; y++) {
                    for ( x = 0; x < width; x++ ) {
                        data_short = (unsigned short) (*buff_ptr & 0xffff);
                        data_short = ROUND((double) data_short * 255.0 / MAXVAL);
                        if (data_short > 255) data_short = 255;
                        data = (byte) (data_short & 0xff);
                        fprintf(fid, "%02x", data);
                        buff_ptr++;
                        if ((x & 0x1f) == 0x1f && (x != (width - 1)))
                            fprintf(fid, "\n");
                    }
                    fprintf(fid, "\n");
                }
                fprintf(fid, "   >\n"
                        ">>\n"
                        );
            } else {
                /* Output PS HalftoneType 16 dictionary. Single array. */
                fprintf(fid, "%%!PS\n"
			"%% Create a 'filter' from local hex data\n"
			"{ currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter } exec\n");
        	/* hex data follows, 'file' object will be left on stack */
                for (y = 0; y < height; y++) {
                    for ( x = 0; x < width; x++ ) {
                        data_short = (unsigned short) (*buff_ptr & 0xffff);
                        fprintf(fid, "%04x", data_short);
                        buff_ptr++;
                        if ((x & 0xf) == 0x1f && (x != (width - 1)))
                            fprintf(fid, "\n");
                    }
		    fprintf(fid, "\n");	/* end of one row */
		}
		fprintf(fid, ">\n");	/* ASCIIHexDecode EOF */
                fprintf(fid, 
                        "<< /Thresholds 2 index    %% file object for the 16-bit data\n"
                        "   /HalftoneType 16\n"
                        "   /Width  %d\n"
                        "   /Height %d\n"
                        ">>\n"
			"exch pop     %% discard ResuableStreamDecode file leaving the Halftone dict.\n",
			width, height);
            }
        }
        fclose(fid);
    }
}

/* Initialize default values */
void htsc_set_default_params(htsc_param_t *params)
{
    params->scr_ang = 0;
    params->targ_scr_ang = 0;
    params->targ_lpi = 75;
    params->vert_dpi = 300;
    params->horiz_dpi = 300;
    params->targ_quant_spec = false;
    params->targ_quant = 256;
    params->targ_size = 1;
    params->targ_size_spec = false;
    params->spot_type = CIRCLE;
    params->holladay = false;
    params->output_format = OUTPUT_TOS;
}

/* Various spot functions */
double htsc_spot_circle(double x, double y)
{
    return 1.0 - (x*x + y*y);
}

double htsc_spot_redbook(double x, double y)
{
    return (180.0 * (double) cos(x) + 180.0 * (double) cos(y)) / 2.0;
}

double htsc_spot_inverted_round(double x, double y)
{
    return (x*x + y*y) - 1.0;
}

double htsc_spot_rhomboid(double x, double y)
{
    return 1.0 - ((double) fabs(y) * 0.8 + (double) fabs(x)) / 2.0;
}

double htsc_spot_linex(double x, double y)
{
    return 1.0 - (double) fabs(y);
}

double htsc_spot_liney(double x, double y)
{
    return 1.0 - (double) fabs(x);
}

double htsc_spot_diamond(double x, double y)
{
    double abs_y = (double) fabs(y);
    double abs_x = (double) fabs(x);

    if ((abs_y + abs_x) <= 0.75) {
        return 1.0 - (abs_x * abs_x + abs_y * abs_y);
    } else {
        if ((abs_y + abs_x) <= 1.23) {
            return 1.0 - (0.76  * abs_y + abs_x);
        } else {
            return ((abs_x - 1.0) * (abs_x - 1.0) + 
                    (abs_y - 1.0) * (abs_y - 1.0)) - 1.0;
        }
    }
}

double htsc_spot_diamond2(double x, double y)
{
    double xy = (double) fabs(x) + (double) fabs(y);

    if (xy <= 1.0) {
        return 1.0 - xy * xy / 2.0;
    } else {
        return 1.0 - (2.0 * xy * xy - 4.0 * (xy - 1.0) * (xy - 1.0)) / 4.0;
    }
}

double htsc_spot_value(spottype_t spot_type, double x, double y)
{
    switch (spot_type) {
        case CIRCLE:
            return htsc_spot_circle(x,y);
        case REDBOOK:
            return htsc_spot_redbook(x,y);
        case INVERTED:
            return htsc_spot_inverted_round(x,y);
        case RHOMBOID:
            return htsc_spot_rhomboid(x,y);
        case LINE_X:
            return htsc_spot_linex(x,y);
        case LINE_Y:
            return htsc_spot_liney(x,y);
        case DIAMOND1:
            return htsc_spot_diamond(x,y);
        case DIAMOND2:
            return htsc_spot_diamond2(x,y);
        case CUSTOM:  /* A spot (pun intended) for users to define their own */
            return htsc_spot_circle(x,y);
        default:
            return htsc_spot_circle(x,y);
    }
}

#if RAW_SCREEN_DUMP
void
htsc_dump_screen(htsc_dig_grid_t *dig_grid, char filename[])
{
    char full_file_name[50];
    FILE *fid;
    int x,y;
    int *buff_ptr = dig_grid->data;
    int width = dig_grid->width;
    int height = dig_grid->height;
    byte data[3];

    sprintf(full_file_name,"Screen_%s_%dx%dx3.raw",filename,width,height);
    fid = fopen(full_file_name,"wb");

    for (y = 0; y < height; y++) {
        for ( x = 0; x < width; x++ ) {
            if (*buff_ptr < 0) {
                data[0] = 255;
                data[1] = 0;
                data[2] = 0;
            } else if (*buff_ptr > 255) {
                data[0] = 0;
                data[1] = 255;
                data[2] = 0;
            } else {
                data[0] = *buff_ptr;
                data[1] = *buff_ptr;
                data[2] = *buff_ptr;
            }
            fwrite(data,sizeof(unsigned char),3,fid);
            buff_ptr++;
        }
    }
    fclose(fid);
}

void
htsc_dump_float_image(float *image, int height, int width, float max_val,
                      char filename[])
{
    char full_file_name[50];
    FILE *fid;
    int x,y;
    int data;
    byte data_byte;

    sprintf(full_file_name,"Float_%s_%dx%d.raw",filename,width,height);
    fid = fopen(full_file_name,"wb");
    for (y = 0; y < height; y++) {
        for ( x = 0; x < width; x++ ) {
            data = (255.0 * image[x + y * width] / max_val);
            if (data > 255) data = 255;
            if (data < 0) data = 0;
            data_byte = data;
            fwrite(&data_byte,sizeof(byte),1,fid);
        }
    }
    fclose(fid);
}

void
htsc_dump_byte_image(byte *image, int height, int width, float max_val,
                      char filename[])
{
    char full_file_name[50];
    FILE *fid;
    int x,y;
    int data;
    byte data_byte;

    sprintf(full_file_name,"ByteScaled_%s_%dx%d.raw",filename,width,height);
    fid = fopen(full_file_name,"wb");
    for (y = 0; y < height; y++) {
        for ( x = 0; x < width; x++ ) {
            data = (255.0 * image[x + y * width] / max_val);
            if (data > 255) data = 255;
            if (data < 0) data = 0;
            data_byte = data;
            fwrite(&data_byte,sizeof(byte),1,fid);
        }
    }
    fclose(fid);
}
#endif
