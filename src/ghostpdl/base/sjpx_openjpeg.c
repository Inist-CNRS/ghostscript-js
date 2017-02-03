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



/* opj filter implementation using OpenJPeg library */

#include "memory_.h"
#include "gserrors.h"
#include "gdebug.h"
#include "strimpl.h"
#include "sjpx_openjpeg.h"

gs_private_st_simple(st_jpxd_state, stream_jpxd_state,
    "JPXDecode filter state"); /* creates a gc object for our state,
                            defined in sjpx.h */

static int s_opjd_accumulate_input(stream_jpxd_state *state, stream_cursor_read * pr);

static OPJ_SIZE_T sjpx_stream_read(void * p_buffer, OPJ_SIZE_T p_nb_bytes, void * p_user_data)
{
	stream_block *sb = (stream_block *)p_user_data;
	OPJ_SIZE_T len;

	len = sb->size - sb->pos;
        if (sb->size < sb->pos)
		len = 0;
	if (len == 0)
		return (OPJ_SIZE_T)-1;  /* End of file! */
	if ((OPJ_SIZE_T)len > p_nb_bytes)
		len = p_nb_bytes;
	memcpy(p_buffer, sb->data + sb->pos, len);
	sb->pos += len;
	return len;
}

static OPJ_OFF_T sjpx_stream_skip(OPJ_OFF_T skip, void * p_user_data)
{
	stream_block *sb = (stream_block *)p_user_data;

	if (skip > sb->size - sb->pos)
		skip = sb->size - sb->pos;
	sb->pos += skip;
	return sb->pos;
}

static OPJ_BOOL sjpx_stream_seek(OPJ_OFF_T seek_pos, void * p_user_data)
{
	stream_block *sb = (stream_block *)p_user_data;

	if (seek_pos > sb->size)
		return OPJ_FALSE;
	sb->pos = seek_pos;
	return OPJ_TRUE;
}

static void sjpx_error_callback(const char *msg, void *ptr)
{
	dlprintf1("openjpeg error: %s", msg);
}

static void sjpx_info_callback(const char *msg, void *ptr)
{
#ifdef DEBUG
	/* prevent too many messages during normal build */
	dlprintf1("openjpeg info: %s", msg);
#endif
}

static void sjpx_warning_callback(const char *msg, void *ptr)
{
	dlprintf1("openjpeg warning: %s", msg);
}

/* initialize the stream */
static int
s_opjd_init(stream_state * ss)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    state->codec = NULL;

    state->image = NULL;
    state->sb.data= NULL;
    state->sb.size = 0;
    state->sb.pos = 0;
    state->sb.fill = 0;
    state->out_offset = 0;
    state->img_offset = 0;
    state->pdata = NULL;
    state->sign_comps = NULL;
    state->stream = NULL;

    return 0;
}

/* setting the codec format,
   allocating the stream and image structures, and
   initializing the decoder.
 */
static int
s_opjd_set_codec_format(stream_state * ss, OPJ_CODEC_FORMAT format)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    opj_dparameters_t parameters;	/* decompression parameters */

    /* set decoding parameters to default values */
    opj_set_default_decoder_parameters(&parameters);

    /* get a decoder handle */
    state->codec = opj_create_decompress(format);
    if (state->codec == NULL)
        return_error(gs_error_VMerror);

    /* catch events using our callbacks */
    opj_set_error_handler(state->codec, sjpx_error_callback, stderr);
    opj_set_info_handler(state->codec, sjpx_info_callback, stderr);
    opj_set_warning_handler(state->codec, sjpx_warning_callback, stderr);

    if (state->colorspace == gs_jpx_cs_indexed) {
        parameters.flags |= OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;
    }

    /* setup the decoder decoding parameters using user parameters */
    if (!opj_setup_decoder(state->codec, &parameters))
    {
        dlprintf("openjpeg: failed to setup the decoder!\n");
        return ERRC;
    }

    /* open a byte stream */
    state->stream = opj_stream_default_create(OPJ_TRUE);
    if (state->stream == NULL)
    {
        dlprintf("openjpeg: failed to open a byte stream!\n");
        return ERRC;
    }

    opj_stream_set_read_function(state->stream, sjpx_stream_read);
    opj_stream_set_skip_function(state->stream, sjpx_stream_skip);
    opj_stream_set_seek_function(state->stream, sjpx_stream_seek);

    return 0;
}

/* calculate the real component data idx after scaling */
static inline unsigned long
get_scaled_idx(stream_jpxd_state *state, int compno, unsigned long idx, unsigned long x, unsigned long y)
{
    if (state->samescale)
	return idx;
	
    return (y/state->image->comps[compno].dy*state->width + x)/state->image->comps[compno].dx;
}

/* convert state->image from YCrCb to RGB */
static int
s_jpxd_ycc_to_rgb(stream_jpxd_state *state)
{
    unsigned int max_value = ~(-1 << state->image->comps[0].prec); /* maximum of channel value */
    int flip_value = 1 << (state->image->comps[0].prec-1);
    int p[3], q[3], i;
    int sgnd[2];  /* Cr, Cb */
    unsigned long x, y, idx;
    int *row_bufs[3];

    if (state->out_numcomps != 3)
    return -1;

    for (i=0; i<2; i++)
        sgnd[i] = state->image->comps[i+1].sgnd;

    for (i=0; i<3; i++)
    	row_bufs[i] = (int *)gs_alloc_byte_array(state->memory->non_gc_memory, sizeof(int)*state->width, 1, "s_jpxd_ycc_to_rgb");

    if (!row_bufs[0] || !row_bufs[1] || !row_bufs[2])
        return_error(gs_error_VMerror);

    idx=0;
    for (y=0; y<state->height; y++)
    {
        /* backup one row. the real buffer might be overriden when there is scale */
        for (i=0; i<3; i++)
        {
            if ((y % state->image->comps[i].dy) == 0) /* get the buffer once every dy rows */
                memcpy(row_bufs[i], &(state->image->comps[i].data[get_scaled_idx(state, i, idx, 0, y)]), sizeof(int)*state->width/state->image->comps[i].dx);
        }
        for (x=0; x<state->width; x++, idx++)
        {
            for (i = 0; i < 3; i++)
                p[i] = row_bufs[i][x/state->image->comps[i].dx];

            if (!sgnd[0])
                p[1] -= flip_value;
            if (!sgnd[1])
                p[2] -= flip_value;

            /* rotate to RGB */
#ifdef JPX_USE_IRT
            q[1] = p[0] - ((p[1] + p[2])>>2);
            q[0] = p[1] + q[1];
            q[2] = p[2] + q[1];
#else
            q[0] = (int)((double)p[0] + 1.402 * p[2]);
            q[1] = (int)((double)p[0] - 0.34413 * p[1] - 0.71414 * p[2]);
            q[2] = (int)((double)p[0] + 1.772 * p[1]);
#endif
            /* clamp */
            for (i = 0; i < 3; i++){
                if (q[i] < 0) q[i] = 0;
                else if (q[i] > max_value) q[i] = max_value;
            }

            /* write out the pixel */
            for (i = 0; i < 3; i++)
                state->image->comps[i].data[get_scaled_idx(state, i, idx, x, y)] = q[i];
        }
    }

    for (i=0; i<3; i++)
    	gs_free_object(state->memory->non_gc_memory, row_bufs[i],"s_jpxd_ycc_to_rgb");

    return 0;
}

static int decode_image(stream_jpxd_state * const state)
{
    int numprimcomp = 0, alpha_comp = -1, compno, rowbytes;

    /* read header */
    if (!opj_read_header(state->stream, state->codec, &(state->image)))
    {
    	dlprintf("openjpeg: failed to read header\n");
    	return ERRC;
    }

    /* decode the stream and fill the image structure */
    if (!opj_decode(state->codec, state->stream, state->image))
    {
        dlprintf("openjpeg: failed to decode image!\n");
        return ERRC;
    }

    /* check dimension and prec */
    if (state->image->numcomps == 0)
        return ERRC;

    state->width = state->image->comps[0].w;
    state->height = state->image->comps[0].h;
    state->bpp = state->image->comps[0].prec;
    state->samescale = true;
    for(compno = 1; compno < state->image->numcomps; compno++)
    {
        if (state->bpp != state->image->comps[compno].prec)
            return ERRC; /* Not supported. */
        if (state->width < state->image->comps[compno].w)
            state->width = state->image->comps[compno].w;
        if (state->height < state->image->comps[compno].h)
            state->height = state->image->comps[compno].h;
        if (state->image->comps[compno].dx != state->image->comps[0].dx ||
                state->image->comps[compno].dy != state->image->comps[0].dy)
            state->samescale = false;
    }

    /* find alpha component and regular colour component by channel definition */
    for (compno = 0; compno < state->image->numcomps; compno++)
    {
        if (state->image->comps[compno].alpha == 0x00)
            numprimcomp++;
        else if (state->image->comps[compno].alpha == 0x01)
            alpha_comp = compno;
    }

    /* color space and number of components */
    switch(state->image->color_space)
    {
        case OPJ_CLRSPC_GRAY:
            state->colorspace = gs_jpx_cs_gray;
            break;
        case OPJ_CLRSPC_UNKNOWN: /* make the best guess based on number of channels */
        {
            if (numprimcomp < 3)
            {
                state->colorspace = gs_jpx_cs_gray;
            }
            else if (numprimcomp == 4)
            {
                state->colorspace = gs_jpx_cs_cmyk;
             }
            else
            {
                state->colorspace = gs_jpx_cs_rgb;
            }
            break;
        }
        default: /* OPJ_CLRSPC_SRGB, OPJ_CLRSPC_SYCC, OPJ_CLRSPC_EYCC */
            state->colorspace = gs_jpx_cs_rgb;
    }

    state->alpha_comp = -1;
    if (state->alpha)
    {
        state->alpha_comp = alpha_comp;
        state->out_numcomps = 1;
    }
    else
        state->out_numcomps = numprimcomp;

    /* round up bpp 12->16 */
    if (state->bpp == 12)
        state->bpp = 16;

    /* calculate  total data */
    rowbytes =  (state->width*state->bpp*state->out_numcomps+7)/8;
    state->totalbytes = rowbytes*state->height;

    /* convert input from YCC to RGB */
    if (state->image->color_space == OPJ_CLRSPC_SYCC || state->image->color_space == OPJ_CLRSPC_EYCC)
        s_jpxd_ycc_to_rgb(state);

    state->pdata = (int **)gs_alloc_byte_array(state->memory->non_gc_memory, sizeof(int*)*state->image->numcomps, 1, "decode_image(pdata)");
    if (!state->pdata)
        return_error(gs_error_VMerror);

    /* compensate for signed data (signed => unsigned) */
    state->sign_comps = (int *)gs_alloc_byte_array(state->memory->non_gc_memory, sizeof(int)*state->image->numcomps, 1, "decode_image(sign_comps)");
    if (!state->sign_comps)
        return_error(gs_error_VMerror);

    for(compno = 0; compno < state->image->numcomps; compno++)
    {
        if (state->image->comps[compno].sgnd)
            state->sign_comps[compno] = ((state->bpp%8)==0) ? 0x80 : (1<<(state->bpp-1));
        else
            state->sign_comps[compno] = 0;
    }

    return 0;
}

static int process_one_trunk(stream_jpxd_state * const state, stream_cursor_write * pw)
{
     /* read data from image to pw */
     unsigned long out_size = pw->limit - pw->ptr;
     int bytepp1 = state->bpp/8; /* bytes / pixel for one output component */
     int bytepp = state->out_numcomps*state->bpp/8; /* bytes / pixel all components */
     unsigned long write_size = min(out_size-(bytepp?(out_size%bytepp):0), state->totalbytes-state->out_offset);
     unsigned long in_offset = state->out_offset*8/state->bpp/state->out_numcomps; /* component data offset */
     int shift_bit = state->bpp-state->image->comps[0].prec; /*difference between input and output bit-depth*/
     int img_numcomps = min(state->out_numcomps, state->image->numcomps), /* the actual number of channel data used */
             compno;
     unsigned long i; int b;
     byte *pend = pw->ptr+write_size+1; /* end of write data */

     if (state->bpp < 8)
         in_offset = state->img_offset;

     pw->ptr++;

     if (state->alpha && state->alpha_comp == -1)
     {/* return 0xff for all */
         memset(pw->ptr, 0xff, write_size);
         pw->ptr += write_size;
     }
     else if (state->samescale)
     {
         if (state->alpha)
             state->pdata[0] = &(state->image->comps[state->alpha_comp].data[in_offset]);
         else
         {
             for (compno=0; compno<img_numcomps; compno++)
                 state->pdata[compno] = &(state->image->comps[compno].data[in_offset]);
         }
         if (shift_bit == 0 && state->bpp == 8) /* optimized for the most common case */
         {
             while (pw->ptr < pend)
             {
                 for (compno=0; compno<img_numcomps; compno++)
                     *(pw->ptr++) = *(state->pdata[compno]++) + state->sign_comps[compno]; /* copy input buffer to output */
             }
         }
         else
         {
             if ((state->bpp%8)==0)
             {
                 while (pw->ptr < pend)
                 {
                     for (compno=0; compno<img_numcomps; compno++)
                     {
                         for (b=0; b<bytepp1; b++)
                             *(pw->ptr++) = (((*(state->pdata[compno]) << shift_bit) >> (8*(bytepp1-b-1))))
                                                                         + (b==0 ? state->sign_comps[compno] : 0); /* split and shift input int to output bytes */
                         state->pdata[compno]++; 
                     }
                 }
             }
             else
             {   
                 /* shift_bit = 0, bpp < 8 */
                 unsigned long image_total = state->width*state->height;
                 int bt=0; int bit_pos = 0;
                 int rowbytes =  (state->width*state->bpp*state->out_numcomps+7)/8; /*row bytes */
                 int currowcnt = state->out_offset % rowbytes; /* number of bytes filled in current row */
                 int start_comp = (currowcnt*8) % img_numcomps; /* starting component for this round of output*/
                 if (start_comp != 0)
                 {
                     for (compno=start_comp; compno<img_numcomps; compno++)
                     {
                         if (state->img_offset < image_total)
                         {
                             bt <<= state->bpp;
                             bt += *(state->pdata[compno]-1) + state->sign_comps[compno];
                         }
                         bit_pos += state->bpp;
                         if (bit_pos >= 8)
                         {
                             *(pw->ptr++) = bt >> (bit_pos-8);
                             bit_pos -= 8;
                             bt &= (1<<bit_pos)-1;
                         }
                     }
                 }
                 while (pw->ptr < pend)
                 {
                     for (compno=0; compno<img_numcomps; compno++)
                     {
                         if (state->img_offset < image_total)
                         {
                             bt <<= state->bpp;
                             bt += *(state->pdata[compno]++) + state->sign_comps[compno];
                         }
                         bit_pos += state->bpp;
                         if (bit_pos >= 8)
                         {
                             *(pw->ptr++) = bt >> (bit_pos-8);
                             bit_pos -= 8;
                             bt &= (1<<bit_pos)-1;
                         }
                     }
                     state->img_offset++;
                     if (bit_pos != 0 && state->img_offset % state->width == 0)
                     {
                         /* row padding */
                         *(pw->ptr++) = bt << (8 - bit_pos);
                         bit_pos = 0;
                         bt = 0;
                     }
                 }
             }
         }
     }
     else
     {
		 /* sampling required */
         unsigned long y_offset = in_offset / state->width;
         unsigned long x_offset = in_offset % state->width;
         while (pw->ptr < pend)
         {
             if ((state->bpp%8)==0)
             {
                 if (state->alpha)
                 {
                     int in_offset_scaled = (y_offset/state->image->comps[state->alpha_comp].dy*state->width + x_offset)/state->image->comps[state->alpha_comp].dx;
                     for (b=0; b<bytepp1; b++)
                             *(pw->ptr++) = (((state->image->comps[state->alpha_comp].data[in_offset_scaled] << shift_bit) >> (8*(bytepp1-b-1))))
                                                                     + (b==0 ? state->sign_comps[state->alpha_comp] : 0);
                 }
                 else
                 {
                     for (compno=0; compno<img_numcomps; compno++)
                     {
                         int in_offset_scaled = (y_offset/state->image->comps[compno].dy*state->width + x_offset)/state->image->comps[compno].dx;
                         for (b=0; b<bytepp1; b++)
                             *(pw->ptr++) = (((state->image->comps[compno].data[in_offset_scaled] << shift_bit) >> (8*(bytepp1-b-1))))
                                                                             + (b==0 ? state->sign_comps[compno] : 0);
                     }
                 }
                 x_offset++;
                 if (x_offset >= state->width)
                 {
                     y_offset++;
                     x_offset = 0;
                 }
             }
             else
             {
                 unsigned long image_total = state->width*state->height;
                 int compno = state->alpha ? state->alpha_comp : 0;
                 /* only grayscale can have such bit-depth, also shift_bit = 0, bpp < 8 */
                  int bt=0;
                  for (i=0; i<8/state->bpp; i++)
                  {
                      bt = bt<<state->bpp;
                      if (state->img_offset < image_total && !(i!=0 && state->img_offset % state->width == 0))
                      {
                          int in_offset_scaled = (y_offset/state->image->comps[compno].dy*state->width + x_offset)/state->image->comps[compno].dx;
                          bt += state->image->comps[compno].data[in_offset_scaled] + state->sign_comps[compno];
                          state->img_offset++;
                          x_offset++;
                          if (x_offset >= state->width)
                          {
                              y_offset++;
                              x_offset = 0;
                          }
                      }
                   }
                  *(pw->ptr++) = bt;
             }
         }
     }
     state->out_offset += write_size;
     pw->ptr--;
     if (state->out_offset == state->totalbytes)
         return EOFC; /* all data returned */
     else
         return 1; /* need more calls */
 }

/* process a section of the input and return any decoded data.
   see strimpl.h for return codes.
 */
static int
s_opjd_process(stream_state * ss, stream_cursor_read * pr,
                 stream_cursor_write * pw, bool last)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;
    long in_size = pr->limit - pr->ptr;

    if (in_size > 0) 
    {
        /* buffer available data */
        int code = s_opjd_accumulate_input(state, pr);
        if (code < 0) return code;

        if (state->codec == NULL) {
            /* state->sb.size is non-zero after successful
               accumulate_input(); 1 is probably extremely rare */
            if (state->sb.data[0] == 0xFF && ((state->sb.size == 1) || (state->sb.data[1] == 0x4F)))
                code = s_opjd_set_codec_format(ss, OPJ_CODEC_J2K);
            else
                code = s_opjd_set_codec_format(ss, OPJ_CODEC_JP2);
            if (code < 0) return code;
        }
    }

    if (last == 1) 
    {
        if (state->image == NULL)
        {
            int ret = ERRC;
#if OPJ_VERSION_MAJOR >= 2 && OPJ_VERSION_MINOR >= 1
            opj_stream_set_user_data(state->stream, &(state->sb), NULL);
#else
            opj_stream_set_user_data(state->stream, &(state->sb));
#endif
            opj_stream_set_user_data_length(state->stream, state->sb.size);
            ret = decode_image(state);
            if (ret != 0)
                return ret;
        }

        /* copy out available data */
        return process_one_trunk(state, pw);

    }

    /* ask for more data */
    return 0;
}

/* Set the defaults */
static void
s_opjd_set_defaults(stream_state * ss) {
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;

    state->alpha = false;
    state->colorspace = gs_jpx_cs_rgb;
}

/* stream release.
   free all our decoder state.
 */
static void
s_opjd_release(stream_state *ss)
{
    stream_jpxd_state *const state = (stream_jpxd_state *) ss;

    /* empty stream or failed to accumulate */
    if (state->codec == NULL)
        return;

    /* free image data structure */
    if (state->image)
        opj_image_destroy(state->image);
    
    /* free stream */
    if (state->stream)
        opj_stream_destroy(state->stream);
		
    /* free decoder handle */
    if (state->codec)
	opj_destroy_codec(state->codec);

    /* free input buffer */
    if (state->sb.data)
        gs_free_object(state->memory->non_gc_memory, state->sb.data, "s_opjd_release(sb.data)");

    if (state->pdata)
        gs_free_object(state->memory->non_gc_memory, state->pdata, "s_opjd_release(pdata)");

    if (state->sign_comps)
        gs_free_object(state->memory->non_gc_memory, state->sign_comps, "s_opjd_release(sign_comps)");
}


static int
s_opjd_accumulate_input(stream_jpxd_state *state, stream_cursor_read * pr)
{
    long in_size = pr->limit - pr->ptr;

    /* grow the input buffer if needed */
    if (state->sb.size < state->sb.fill + in_size)
    {
        unsigned char *new_buf;
        unsigned long new_size = state->sb.size==0 ? in_size : state->sb.size;

        while (new_size < state->sb.fill + in_size)
            new_size = new_size << 1;

        if_debug1('s', "[s]opj growing input buffer to %lu bytes\n",
                new_size);
        if (state->sb.data == NULL)
            new_buf = (byte *) gs_alloc_byte_array(state->memory->non_gc_memory, new_size, 1, "s_opjd_accumulate_input(alloc)");
        else
            new_buf = (byte *) gs_resize_object(state->memory->non_gc_memory, state->sb.data, new_size, "s_opjd_accumulate_input(resize)");
        if (new_buf == NULL) return_error( gs_error_VMerror);

        state->sb.data = new_buf;
        state->sb.size = new_size;
    }

    /* copy the available input into our buffer */
    /* note that the gs stream library uses offset-by-one
        indexing of its buffers while we use zero indexing */
    memcpy(state->sb.data + state->sb.fill, pr->ptr + 1, in_size);
    state->sb.fill += in_size;
    pr->ptr += in_size;

    return 0;
}

/* stream template */
const stream_template s_jpxd_template = {
    &st_jpxd_state,
    s_opjd_init,
    s_opjd_process,
    1024, 1024,   /* min in and out buffer sizes we can handle
                     should be ~32k,64k for efficiency? */
    s_opjd_release,
    s_opjd_set_defaults
};
