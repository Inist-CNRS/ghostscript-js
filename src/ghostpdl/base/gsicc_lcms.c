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


/* gsicc interface to littleCMS */

#ifndef LCMS_USER_ALLOC
#define LCMS_USER_ALLOC 1
#endif

#include "gsicc_cms.h"
#include "lcms.h"
#include "gslibctx.h"
#include "gserrors.h"
#include "gp.h"


#define DUMP_CMS_BUFFER 0
#define DEBUG_LCMS_MEM 0
#define LCMS_BYTES_MASK 0x7

/* Only provide warning about issues in lcms if debug build */
static int
gscms_error(int error_code, const char *error_text){
#ifdef DEBUG
    gs_warn1("cmm error : %s",error_text);
#endif
    return(1);
}

/* Get the number of channels for the profile.
  Input count */
int
gscms_get_input_channel_count(gcmmhprofile_t profile)
{
    icColorSpaceSignature colorspace;

    colorspace = cmsGetColorSpace(profile);
    return(_cmsChannelsOf(colorspace));
}

/* Get the number of output channels for the profile */
int
gscms_get_output_channel_count(gcmmhprofile_t profile)
{
    icColorSpaceSignature colorspace;

    colorspace = cmsGetPCS(profile);
    return(_cmsChannelsOf(colorspace));
}

/* Get the number of colorant names in the clrt tag */
int
gscms_get_numberclrtnames(gcmmhprofile_t profile)
{
    LPcmsNAMEDCOLORLIST lcms_names;

    lcms_names = cmsReadColorantTable(profile, icSigColorantTableTag);
    return(lcms_names->nColors);
}

/* Get the nth colorant name in the clrt tag */
char*
gscms_get_clrtname(gcmmhprofile_t profile, int colorcount, gs_memory_t *memory)
{
    LPcmsNAMEDCOLORLIST lcms_names;
    int length;
    char *name;

    lcms_names = cmsReadColorantTable(profile, icSigColorantTableTag);
    if (colorcount+1 > lcms_names->nColors) return(NULL);
    length = strlen(lcms_names->List[colorcount].Name);
    name = (char*) gs_alloc_bytes(memory, length + 1, "gscms_get_clrtname");
    if (name)
        strcpy(name, lcms_names->List[colorcount].Name);
    return name;
}

/* Get the device space associated with this profile */

gsicc_colorbuffer_t
gscms_get_profile_data_space(gcmmhprofile_t profile)
{
    icColorSpaceSignature colorspace;

    colorspace = cmsGetColorSpace(profile);
    switch( colorspace ) {
        case icSigXYZData:
            return(gsCIEXYZ);
        case icSigLabData:
            return(gsCIELAB);
        case icSigRgbData:
            return(gsRGB);
        case icSigGrayData:
            return(gsGRAY);
        case icSigCmykData:
            return(gsCMYK);
        default:
            return(gsNCHANNEL);
    }
}

/* Get ICC Profile handle from buffer */
gcmmhprofile_t
gscms_get_profile_handle_mem(gs_memory_t *mem, unsigned char *buffer, unsigned int input_size)
{
    cmsSetErrorHandler((cmsErrorHandlerFunction) gscms_error);
    return(cmsOpenProfileFromMem(buffer,input_size));
}

/* Get ICC Profile handle from file ptr */
gcmmhprofile_t
gscms_get_profile_handle_file(gs_memory_t *mem, const char *filename)
{
    return(cmsOpenProfileFromFile(filename, "r"));
}

/* Transform an entire buffer */
int
gscms_transform_color_buffer(gx_device *dev, gsicc_link_t *icclink,
                             gsicc_bufferdesc_t *input_buff_desc,
                             gsicc_bufferdesc_t *output_buff_desc,
                             void *inputbuffer,
                             void *outputbuffer)
{

    cmsHTRANSFORM hTransform = (cmsHTRANSFORM) icclink->link_handle;
    DWORD dwInputFormat,dwOutputFormat,curr_input,curr_output;
    int planar,numbytes,big_endian,hasalpha,k;
    unsigned char *inputpos, *outputpos;
    int numchannels;
#if DUMP_CMS_BUFFER
    FILE *fid_in, *fid_out;
#endif
    /* Although little CMS does  make assumptions about data types in its
       transformations you can change it after the fact.  */
    /* Set us to the proper output type */
    /* Note, we could speed this up by passing back the encoded data type
        to the caller so that we could avoid having to go through this
        computation each time if they are doing multiple calls to this
        operation */
    _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) (LPSTR) hTransform;

    curr_input = p->InputFormat;
    curr_output = p->OutputFormat;

    /* Color space MUST be the same */
    dwInputFormat = COLORSPACE_SH(T_COLORSPACE(curr_input));
    dwOutputFormat = COLORSPACE_SH(T_COLORSPACE(curr_output));

    /* Now set if we have planar, num bytes, endian case, and alpha data to skip */
    /* Planar -- pdf14 case for example */
    planar = input_buff_desc->is_planar;
    dwInputFormat = dwInputFormat | PLANAR_SH(planar);
    planar = output_buff_desc->is_planar;
    dwOutputFormat = dwOutputFormat | PLANAR_SH(planar);

    /* 8 or 16 byte input and output */
    numbytes = input_buff_desc->bytes_per_chan;
    if (numbytes>2) numbytes = 0;  /* littleCMS encodes float with 0 ToDO. */
    dwInputFormat = dwInputFormat | BYTES_SH(numbytes);
    numbytes = output_buff_desc->bytes_per_chan;
    if (numbytes>2) numbytes = 0;
    dwOutputFormat = dwOutputFormat | BYTES_SH(numbytes);

    /* endian */
    big_endian = !input_buff_desc->little_endian;
    dwInputFormat = dwInputFormat | ENDIAN16_SH(big_endian);
    big_endian = !output_buff_desc->little_endian;
    dwOutputFormat = dwOutputFormat | ENDIAN16_SH(big_endian);

    /* number of channels */
    numchannels = input_buff_desc->num_chan;
    dwInputFormat = dwInputFormat | CHANNELS_SH(numchannels);
    numchannels = output_buff_desc->num_chan;
    dwOutputFormat = dwOutputFormat | CHANNELS_SH(numchannels);

    /* alpha, which is passed through unmolested */
    /* ToDo:  Right now we always must have alpha last */
    /* This is really only going to be an issue when we have
       interleaved alpha data */
    hasalpha = input_buff_desc->has_alpha;
    dwInputFormat = dwInputFormat | EXTRA_SH(hasalpha);
    dwOutputFormat = dwOutputFormat | EXTRA_SH(hasalpha);

    /* Change the formaters */
    cmsChangeBuffersFormat(hTransform,dwInputFormat,dwOutputFormat);

    /* littleCMS knows nothing about word boundarys.  As such, we need to do
       this row by row adjusting for our stride.  Output buffer must already
       be allocated. ToDo:  Check issues with plane and row stride and word
       boundry */
    inputpos = (unsigned char *) inputbuffer;
    outputpos = (unsigned char *) outputbuffer;
    if(input_buff_desc->is_planar){
        /* Do entire buffer.  Care must be taken here
           with respect to row stride, word boundry and number
           of source versus output channels.  We may
           need to take a closer look at this. */
        cmsDoTransform(hTransform,inputpos,outputpos,
                        input_buff_desc->plane_stride);
#if DUMP_CMS_BUFFER
        fid_in = gp_fopen("CM_Input.raw","ab");
        fid_out = fp_fopen("CM_Output.raw","ab");
        fwrite((unsigned char*) inputbuffer,sizeof(unsigned char),
                                input_buff_desc->plane_stride * 
                                input_buff_desc->num_chan, fid_in);
        fwrite((unsigned char*) outputbuffer,sizeof(unsigned char),
                                output_buff_desc->plane_stride * 
                                output_buff_desc->num_chan, fid_out);
        fclose(fid_in);
        fclose(fid_out);
#endif
    } else {
        /* Do row by row. */
        for(k = 0; k < input_buff_desc->num_rows ; k++){
            cmsDoTransform(hTransform,inputpos,outputpos,
                            input_buff_desc->pixels_per_row);
            inputpos += input_buff_desc->row_stride;
            outputpos += output_buff_desc->row_stride;
        }
#if DUMP_CMS_BUFFER
        fid_in = gp_fopen("CM_Input.raw","ab");
        fid_out = gp_fopen("CM_Output.raw","ab");
        fwrite((unsigned char*) inputbuffer,sizeof(unsigned char),
                                input_buff_desc->row_stride,fid_in);
        fwrite((unsigned char*) outputbuffer,sizeof(unsigned char),
                                output_buff_desc->row_stride,fid_out);
        fclose(fid_in);
        fclose(fid_out);
#endif
    }
    return 0;
}

/* Transform a single color. We assume we have passed to us the proper number
   of elements of size gx_device_color. It is up to the caller to make sure
   the proper allocations for the colors are there. */
void
gscms_transform_color(gx_device *dev, gsicc_link_t *icclink, void *inputcolor,
                      void *outputcolor, int num_bytes)
{
    cmsHTRANSFORM hTransform = (cmsHTRANSFORM) icclink->link_handle;
    DWORD dwInputFormat,dwOutputFormat,curr_input,curr_output;

    /* For a single color, we are going to use the link as it is
       with the exception of taking care of the word size. */
    _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) (LPSTR) hTransform;
    curr_input = p->InputFormat;
    curr_output = p->OutputFormat;
    /* numbytes = sizeof(gx_color_value); */
    if (num_bytes>2) num_bytes = 0;  /* littleCMS encodes float with 0 ToDO. */
    dwInputFormat = (curr_input & (~LCMS_BYTES_MASK))  | BYTES_SH(num_bytes);
    dwOutputFormat = (curr_output & (~LCMS_BYTES_MASK)) | BYTES_SH(num_bytes);
    /* Change the formatters */
    cmsChangeBuffersFormat(hTransform,dwInputFormat,dwOutputFormat);
    /* Do conversion */
    cmsDoTransform(hTransform,inputcolor,outputcolor,1);
    return 0;
}

/* Get the link from the CMS. TODO:  Add error checking */
gcmmhlink_t
gscms_get_link(gcmmhprofile_t  lcms_srchandle,
               gcmmhprofile_t lcms_deshandle,
               gsicc_rendering_param_t *rendering_params, int cmm_flags,
               gs_memory_t *mem)
{
    DWORD src_data_type,des_data_type;
    icColorSpaceSignature src_color_space,des_color_space;
    int src_nChannels,des_nChannels;
    int lcms_src_color_space, lcms_des_color_space;

   /* First handle all the source stuff */
    src_color_space  = cmsGetColorSpace(lcms_srchandle);
    lcms_src_color_space = _cmsLCMScolorSpace(src_color_space);
    /* littlecms returns -1 for types it does not (but should) understand */
    if (lcms_src_color_space < 0) lcms_src_color_space = 0;
    src_nChannels = _cmsChannelsOf(src_color_space);
    /* For now, just do single byte data, interleaved.  We can change this
      when we use the transformation. */
    src_data_type = (COLORSPACE_SH(lcms_src_color_space)|
                        CHANNELS_SH(src_nChannels)|BYTES_SH(2));

    if (lcms_deshandle != NULL) {
        des_color_space  = cmsGetColorSpace(lcms_deshandle);
    } else {
        /* We must have a device link profile. */
        des_color_space = cmsGetPCS(lcms_deshandle);
    }
    lcms_des_color_space = _cmsLCMScolorSpace(des_color_space);
    if (lcms_des_color_space < 0) lcms_des_color_space = 0;
    des_nChannels = _cmsChannelsOf(des_color_space);
    des_data_type = (COLORSPACE_SH(lcms_des_color_space)|
                        CHANNELS_SH(des_nChannels)|BYTES_SH(2));
/* Create the link */
    return(cmsCreateTransform(lcms_srchandle, src_data_type, lcms_deshandle,
                        des_data_type, rendering_params->rendering_intent,
               (cmm_flags | cmsFLAGS_BLACKPOINTCOMPENSATION | cmsFLAGS_HIGHRESPRECALC |
                cmsFLAGS_NOTCACHE )));
    /* cmsFLAGS_HIGHRESPRECALC)  cmsFLAGS_NOTPRECALC  cmsFLAGS_LOWRESPRECALC*/
}

/* Get the link from the CMS, but include proofing and/or a device link  
   profile. */
gcmmhlink_t
gscms_get_link_proof_devlink(gcmmhprofile_t lcms_srchandle,
                             gcmmhprofile_t lcms_proofhandle,
                             gcmmhprofile_t lcms_deshandle, 
                             gcmmhprofile_t lcms_devlinkhandle, 
                             gsicc_rendering_param_t *rendering_params,
                             bool src_dev_link, int cmm_flags,
                             gs_memory_t *mem)
{
    DWORD src_data_type,des_data_type;
    icColorSpaceSignature src_color_space,des_color_space;
    int src_nChannels,des_nChannels;
    int lcms_src_color_space, lcms_des_color_space;
    cmsHPROFILE hProfiles[5]; 
    int nProfiles = 0;

   /* First handle all the source stuff */
    src_color_space  = cmsGetColorSpace(lcms_srchandle);
    lcms_src_color_space = _cmsLCMScolorSpace(src_color_space);
    /* littlecms returns -1 for types it does not (but should) understand */
    if (lcms_src_color_space < 0) lcms_src_color_space = 0;
    src_nChannels = _cmsChannelsOf(src_color_space);
    /* For now, just do single byte data, interleaved.  We can change this
      when we use the transformation. */
    src_data_type = (COLORSPACE_SH(lcms_src_color_space)|
                        CHANNELS_SH(src_nChannels)|BYTES_SH(2));    
    if (lcms_deshandle != NULL) {
        des_color_space  = cmsGetColorSpace(lcms_deshandle);
    } else {
        /* We must have a device link profile. */
        des_color_space = cmsGetPCS(lcms_deshandle);
    }
    lcms_des_color_space = _cmsLCMScolorSpace(des_color_space);
    if (lcms_des_color_space < 0) lcms_des_color_space = 0;
    des_nChannels = _cmsChannelsOf(des_color_space);
    des_data_type = (COLORSPACE_SH(lcms_des_color_space)|
                        CHANNELS_SH(des_nChannels)|BYTES_SH(2));
    /* lcms proofing transform has a clunky API and can't include the device 
       link profile if we have both. So use cmsCreateMultiprofileTransform 
       instead and round trip the proofing profile. */
    hProfiles[nProfiles++] = lcms_srchandle;
    if (lcms_proofhandle != NULL) {
        hProfiles[nProfiles++] = lcms_proofhandle;
        hProfiles[nProfiles++] = lcms_proofhandle;
    }
    hProfiles[nProfiles++] = lcms_deshandle;
    if (lcms_devlinkhandle != NULL) {
        hProfiles[nProfiles++] = lcms_devlinkhandle;
    }
    return(cmsCreateMultiprofileTransform(hProfiles, nProfiles, src_data_type, 
                                          des_data_type, rendering_params->rendering_intent, 
                                          (cmm_flags | cmsFLAGS_BLACKPOINTCOMPENSATION | 
                                           cmsFLAGS_HIGHRESPRECALC |
                                           cmsFLAGS_NOTCACHE)));
}

/* Do any initialization if needed to the CMS */
int
gscms_create(gs_memory_t *memory)
{
    /* Set our own error handling function */
    cmsSetErrorHandler((cmsErrorHandlerFunction) gscms_error);
    return 0;
}

/* Do any clean up when done with the CMS if needed */
void
gscms_destroy(gs_memory_t *memory)
{
    /* Nothing to do here for lcms */
}

/* Have the CMS release the link */
void
gscms_release_link(gsicc_link_t *icclink)
{
    if (icclink->link_handle !=NULL )
        cmsDeleteTransform(icclink->link_handle);

    icclink->link_handle = NULL;
}

/* Have the CMS release the profile handle */
void
gscms_release_profile(void *profile)
{
    cmsHPROFILE profile_handle;
    LCMSBOOL notok;

    profile_handle = (cmsHPROFILE) profile;
    notok = cmsCloseProfile(profile_handle);
}

/* Named color, color management */
/* Get a device value for the named color.  Since there exist named color
   ICC profiles and littleCMS supports them, we will use
   that format in this example.  However it should be noted
   that this object need not be an ICC named color profile
   but can be a proprietary type table. Some CMMs do not
   support named color profiles.  In that case, or if
   the named color is not found, the caller should use an alternate
   tint transform or other method. If a proprietary
   format (nonICC) is being used, the operators given below must
   be implemented. In every case that I can imagine, this should be
   straight forward.  Note that we allow the passage of a tint
   value also.  Currently the ICC named color profile does not provide
   tint related information, only a value for 100% coverage.
   It is provided here for use in proprietary
   methods, which may be able to provide the desired effect.  We will
   at the current time apply a direct tint operation to the returned
   device value.
   Right now I don't see any reason to have the named color profile
   ever return CIELAB. It will either return device values directly or
   it will return values defined by the output device profile */

int
gscms_transform_named_color(gsicc_link_t *icclink,  float tint_value,
                            const char* ColorName, gx_color_value device_values[] )
{
    cmsHPROFILE hTransform = icclink->link_handle;
    unsigned short *deviceptr = device_values;
    int index;

    /* Check if name is present in the profile */
    /* If it is not return -1.  Caller will need to use an alternate method */
    if((index = cmsNamedColorIndex(hTransform, ColorName)) < 0) return -1;
    /* Get the device value. */
   cmsDoTransform(hTransform,&index,deviceptr,1);
   return(0);
}

/* Create a link to return device values inside the named color profile or link
   it with a destination profile and potentially a proofing profile.  If the
   output_colorspace and the proof_color space are NULL, then we will be
   returning the device values that are contained in the named color profile.
   i.e. in namedcolor_information.  Note that an ICC named color profile
    need NOT contain the device values but must contain the CIELAB values. */
void
gscms_get_name2device_link(gsicc_link_t *icclink, gcmmhprofile_t  lcms_srchandle,
                    gcmmhprofile_t lcms_deshandle, gcmmhprofile_t lcms_proofhandle,
                    gsicc_rendering_param_t *rendering_params)
{
    cmsHPROFILE hTransform;
    _LPcmsTRANSFORM p;
     DWORD dwOutputFormat;
    DWORD lcms_proof_flag;
    int number_colors;

    /* NOTE:  We need to add a test here to check that we even HAVE
    device values in here and NOT just CIELAB values */
    if ( lcms_proofhandle != NULL ){
        lcms_proof_flag = cmsFLAGS_GAMUTCHECK | cmsFLAGS_SOFTPROOFING;
    } else {
        lcms_proof_flag = 0;
    }
    /* Create the transform */
    /* ToDo:  Adjust rendering intent */
    hTransform = cmsCreateProofingTransform(lcms_srchandle, TYPE_NAMED_COLOR_INDEX,
                                            lcms_deshandle, TYPE_CMYK_8,
                                            lcms_proofhandle,INTENT_PERCEPTUAL,
                                            INTENT_ABSOLUTE_COLORIMETRIC,
                                            lcms_proof_flag);
    /* In littleCMS there is no easy way to find out the size of the device
        space returned by the named color profile until after the transform is made.
        Hence we adjust our output format after creating the transform.  It is
        set to CMYK8 initially. */
    p = (_LPcmsTRANSFORM) hTransform;
    number_colors = p->NamedColorList->ColorantCount;
    /* NOTE: Output size of gx_color_value with no color space type check */
    dwOutputFormat =  (CHANNELS_SH(number_colors)|BYTES_SH(sizeof(gx_color_value)));
    /* Change the formaters */
    cmsChangeBuffersFormat(hTransform,TYPE_NAMED_COLOR_INDEX,dwOutputFormat);
    icclink->link_handle = hTransform;
    cmsCloseProfile(lcms_srchandle);
    if(lcms_deshandle) cmsCloseProfile(lcms_deshandle);
    if(lcms_proofhandle) cmsCloseProfile(lcms_proofhandle);
    return;
}

/* Interface of littlecms memory alloc calls hooked into gs allocator.
   For this to be used lcms is built with LCMS_USER_ALLOC.  See lcms.mak */
#if DEBUG_LCMS_MEM
void* _cmsMalloc(unsigned int size)
{
    void *ptr;

#if defined(SHARE_LCMS) && SHARE_LCMS==1
    ptr = malloc(size);
#else
    ptr = gs_alloc_bytes(gs_lib_ctx_get_non_gc_memory_t(), size, "lcms");
#endif
    gs_warn2("lcms malloc (%d) at 0x%x",size,ptr);
    return ptr;
}

void* _cmsCalloc(unsigned int nelts, unsigned int size)
{
    void *ptr;

#if defined(SHARE_LCMS) && SHARE_LCMS==1
    ptr = malloc(nelts * size);
#else
    ptr = gs_alloc_byte_array(gs_lib_ctx_get_non_gc_memory_t(), nelts, size, "lcms");
#endif

    if (ptr != NULL)
        memset(ptr, 0, nelts * size);
    gs_warn2("lcms calloc (%d) at 0x%x",nelts*size,ptr);
    return ptr;
}

void _cmsFree(void *ptr)
{
    if (ptr != NULL) {
        gs_warn1("lcms free at 0x%x",ptr);
#if defined(SHARE_LCMS) && SHARE_LCMS==1
        free(ptr);
#else
        gs_free_object(gs_lib_ctx_get_non_gc_memory_t(), ptr, "lcms");
#endif
    }
}

#else

void* _cmsMalloc(unsigned int size)
{
#if defined(SHARE_LCMS) && SHARE_LCMS==1
    return malloc(size);
#else
    return gs_alloc_bytes(gs_lib_ctx_get_non_gc_memory_t(), size, "lcms");
#endif
}

void* _cmsCalloc(unsigned int nelts, unsigned int size)
{
    void *ptr;

#if defined(SHARE_LCMS) && SHARE_LCMS==1
    ptr = malloc(nelts * size);
#else
    ptr = gs_alloc_byte_array(gs_lib_ctx_get_non_gc_memory_t(), nelts, size, "lcms");
#endif

    if (ptr != NULL)
        memset(ptr, 0, nelts * size);
    return ptr;
}

void _cmsFree(void *ptr)
{
#if defined(SHARE_LCMS) && SHARE_LCMS==1
    free(ptr);
#else
    if (ptr != NULL)
        gs_free_object(gs_lib_ctx_get_non_gc_memory_t(), ptr, "lcms");
#endif
}
#endif
