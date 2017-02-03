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


/* Utilities for PCL XL generation */
#include "math_.h"
#include "string_.h"
#include "gx.h"
#include "stream.h"
#include "gxdevcli.h"
#include "gdevpxat.h"
#include "gdevpxen.h"
#include "gdevpxop.h"
#include "gdevpxut.h"

/* ---------------- High-level constructs ---------------- */

/* Write the file header, including the resolution. */
int
px_write_file_header(stream *s, const gx_device *dev, bool staple)
{
    static const char *const enter_pjl_header =
        "\033%-12345X@PJL SET RENDERMODE=";
    static const char *const set_staple= "\n@PJL SET FINISH=STAPLE";
    static const char *const rendermode_gray = "GRAYSCALE";
    static const char *const rendermode_color = "COLOR";
    static const char *const pjl_resolution =
        "\n@PJL SET RESOLUTION=";
    static const char *const resolution_150 = "150";
    static const char *const resolution_300 = "300";
    static const char *const resolution_600 = "600";
    static const char *const resolution_1200 = "1200";
    static const char *const resolution_2400 = "2400";
    static const char *const file_header =
        "\n@PJL ENTER LANGUAGE = PCLXL\n\
) HP-PCL XL;1;1;Comment Copyright Artifex Sofware, Inc. 2005\000\n";
    static const byte stream_header[] = {
        DA(pxaUnitsPerMeasure),
        DUB(0), DA(pxaMeasure),
        DUB(eBackChAndErrPage), DA(pxaErrorReport),
        pxtBeginSession,
        DUB(0), DA(pxaSourceType),
        DUB(eBinaryLowByteFirst), DA(pxaDataOrg),
        pxtOpenDataSource
    };

    px_put_bytes(s, (const byte *)enter_pjl_header,
                 strlen(enter_pjl_header));

    if (dev->color_info.num_components == 1)
        px_put_bytes(s, (const byte *)rendermode_gray,
                     strlen(rendermode_gray));
    else
        px_put_bytes(s, (const byte *)rendermode_color,
                     strlen(rendermode_color));

    if(staple)
        px_put_bytes(s, (const byte *)set_staple,
                     strlen(set_staple));

    px_put_bytes(s, (const byte *)pjl_resolution,
                 strlen(pjl_resolution));

    if ((uint) (dev->HWResolution[0] + 0.5) == 150)
        px_put_bytes(s, (const byte *)resolution_150,
                     strlen(resolution_150));
    else if ((uint) (dev->HWResolution[0] + 0.5) == 300)
        px_put_bytes(s, (const byte *)resolution_300,
                     strlen(resolution_300));
    else if ((uint) (dev->HWResolution[0] + 0.5) == 1200)
        px_put_bytes(s, (const byte *)resolution_1200,
                     strlen(resolution_1200));
    else if ((uint) (dev->HWResolution[0] + 0.5) == 2400)
        px_put_bytes(s, (const byte *)resolution_2400,
                     strlen(resolution_2400));
    else
        px_put_bytes(s, (const byte *)resolution_600,
                     strlen(resolution_600));
    if ((uint) (dev->HWResolution[1] + 0.5) !=
        (uint) (dev->HWResolution[0] + 0.5)) {
        px_put_bytes(s, (const byte *)"x", strlen("x"));
        if ((uint) (dev->HWResolution[1] + 0.5) == 150)
            px_put_bytes(s, (const byte *)resolution_150,
                         strlen(resolution_150));
        else if ((uint) (dev->HWResolution[1] + 0.5) == 300)
            px_put_bytes(s, (const byte *)resolution_300,
                         strlen(resolution_300));
        else if ((uint) (dev->HWResolution[1] + 0.5) == 1200)
            px_put_bytes(s, (const byte *)resolution_1200,
                         strlen(resolution_1200));
        else if ((uint) (dev->HWResolution[1] + 0.5) == 2400)
            px_put_bytes(s, (const byte *)resolution_2400,
                         strlen(resolution_2400));
        else
            px_put_bytes(s, (const byte *)resolution_600,
                         strlen(resolution_600));
    }

    /* We have to add 2 to the strlen because the next-to-last */
    /* character is a null. */
    px_put_bytes(s, (const byte *)file_header,
                 strlen(file_header) + 2);
    px_put_usp(s, (uint) (dev->HWResolution[0] + 0.5),
               (uint) (dev->HWResolution[1] + 0.5));
    PX_PUT_LIT(s, stream_header);
    return 0;
}

/* Write the page header, including orientation. */
int
px_write_page_header(stream *s, const gx_device *dev)
{
    /* Orientation is deferred until px_write_select_media... */
    return 0;
}

/* Write the media selection command if needed, updating the media size. */
int
px_write_select_media(stream *s, const gx_device *dev,
                      pxeMediaSize_t *pms, byte *media_source,
                      int page, bool Duplex, bool Tumble, int media_type_set, char *media_type)
{
#define MSD(ms, mstr, res, w, h)                                 \
    { ms, mstr, (float)((w) * 1.0 / (res)), (float)((h) * 1.0 / res) },
    static const struct {
        pxeMediaSize_t ms;
        const char *media_name;
        float width, height;
    } media_sizes[] = {
        px_enumerate_media(MSD)
        { pxeMediaSize_next }
    };
#undef MSD
    float w = dev->width / dev->HWResolution[0],
        h = dev->height / dev->HWResolution[1];
    int i;
    pxeMediaSize_t size = eDefaultPaperSize;
    byte tray = eAutoSelect;
    byte orientation = ePortraitOrientation;
    bool match_found = false;

    /* The default is eDefaultPaperSize (=96), but we'll emit CustomMediaSize */
    /* 0.05 = 30@r600 - one of the test files is 36 off and within 5.0/72@600 */
    for (i = countof(media_sizes) - 2; i > 0; --i)
        if (fabs(media_sizes[i].width - w) < 0.05 &&
            fabs(media_sizes[i].height - h) < 0.05 &&
            media_sizes[i].ms < 22 /* HP uses up to 21; Ricoh uses 201-224 */
            ) {
            match_found = true;
            size = media_sizes[i].ms;
            break;
	} else if (fabs(media_sizes[i].height - w) < 0.05 &&
		   fabs(media_sizes[i].width - h) < 0.05 &&
                   media_sizes[i].ms < 22
		   ) {
	    match_found = true;
	    size = media_sizes[i].ms;
	    orientation = eLandscapeOrientation;
	    break;
        }
    /*
     * According to the PCL XL documentation, MediaSize/CustomMediaSize must always
     * be specified, but MediaSource is optional.
     */
    px_put_uba(s, orientation, pxaOrientation);
    if (match_found) {
        /* standard media */
        px_put_uba(s, (byte)size, pxaMediaSize);
    } else {
        /* CustomMediaSize in Inches */
        px_put_rpa(s, w, h, pxaCustomMediaSize);
        px_put_uba(s, (byte)eInch, pxaCustomMediaSizeUnits);
    }

    if (media_source != NULL)
        tray = *media_source;
    /* suppress eAutoSelect if type is set */
    if (!media_type_set || (tray != eAutoSelect))
      px_put_uba(s, tray, pxaMediaSource);
    /* suppress empty(="plain") type if tray is non-auto */
    if (media_type_set)
        if ((tray == eAutoSelect) || strlen(media_type))
          px_put_ubaa(s, (const byte *)media_type, strlen(media_type), pxaMediaType);

    if_debug2('|', "duplex %d tumble %d\n", Duplex, Tumble);
    if (Duplex)
    {
      if (Tumble)
        px_put_uba(s, (byte)eDuplexHorizontalBinding, pxaDuplexPageMode);
      else
        px_put_uba(s, (byte)eDuplexVerticalBinding, pxaDuplexPageMode);

      if (page & 1)
        px_put_uba(s, (byte)eFrontMediaSide, pxaDuplexPageSide);
      else
        px_put_uba(s, (byte)eBackMediaSide, pxaDuplexPageSide);
    }
    else
      px_put_uba(s, (byte)eSimplexFrontSide, pxaSimplexPageMode);

    if (pms)
        *pms = size;

    return 0;
}

/*
 * Write the file trailer.  Note that this takes a FILE *, not a stream *,
 * since it may be called after the stream is closed.
 */
int
px_write_file_trailer(FILE *file)
{
    static const byte file_trailer[] = {
        pxtCloseDataSource,
        pxtEndSession,
        033, '%', '-', '1', '2', '3', '4', '5', 'X'
    };

    fwrite(file_trailer, 1, sizeof(file_trailer), file);
    return 0;
}

/* ---------------- Low-level data output ---------------- */

/* Write a sequence of bytes. */
void
px_put_bytes(stream * s, const byte * data, uint count)
{
    uint used;

    sputs(s, data, count, &used);
}

/* Utilities for writing data values. */
/* H-P printers only support little-endian data, so that's what we emit. */
void
px_put_a(stream * s, px_attribute_t a)
{
    sputc(s, pxt_attr_ubyte);
    sputc(s, (byte)a);
}
void
px_put_ac(stream *s, px_attribute_t a, px_tag_t op)
{
    px_put_a(s, a);
    sputc(s, (byte)op);
}

void
px_put_ub(stream * s, byte b)
{
    sputc(s, pxt_ubyte);
    sputc(s, b);
}
void
px_put_uba(stream *s, byte b, px_attribute_t a)
{
    px_put_ub(s, b);
    px_put_a(s, a);
}

void
px_put_s(stream * s, int i)
{
    sputc(s, (byte) i);
    if (i < 0)
       i |= 0x8000;
    sputc(s, (byte) (i >> 8));
}
void
px_put_us(stream * s, uint i)
{
    sputc(s, pxt_uint16);
    px_put_s(s, i);
}
void
px_put_usa(stream *s, uint i, px_attribute_t a)
{
    px_put_us(s, i);
    px_put_a(s, a);
}
void
px_put_u(stream * s, uint i)
{
    if (i <= 255)
        px_put_ub(s, (byte)i);
    else
        px_put_us(s, i);
}

void
px_put_usp(stream * s, uint ix, uint iy)
{
    spputc(s, pxt_uint16_xy);
    px_put_s(s, ix);
    px_put_s(s, iy);
}
void
px_put_usq_fixed(stream * s, fixed x0, fixed y0, fixed x1, fixed y1)
{
    spputc(s, pxt_uint16_box);
    px_put_s(s, fixed2int(x0));
    px_put_s(s, fixed2int(y0));
    px_put_s(s, fixed2int(x1));
    px_put_s(s, fixed2int(y1));
}

void
px_put_ss(stream * s, int i)
{
    sputc(s, pxt_sint16);
    px_put_s(s, i);
}
void
px_put_ssp(stream * s, int ix, int iy)
{
    sputc(s, pxt_sint16_xy);
    px_put_s(s, ix);
    px_put_s(s, iy);
}

void
px_put_l(stream * s, ulong l)
{
    sputc(s, (byte) l);
    sputc(s, (byte) (l >> 8));
    sputc(s, (byte) (l >> 16));
    sputc(s, (byte) (l >> 24));
}

/*
    The single-precison IEEE float is represented with 32-bit as follows:

    1      8          23
    sign | exponent | matissa_bits

    switch(exponent):
    case 0:
        (-1)^sign * 2^(-126) * 0.<matissa_bits>_base2
    case 0xFF:
        +- infinity
    default: (0x01 - 0xFE)
        (-1)^sign * 2^(exponent - 127) * 1.<matissa_bits>_base2

    The "1." part is not coded since it is always "1.".

    To uses frexp, which returns
        0.<matissa_bits>_base2 * 2^exp
    We need to think of it as:
        1.<matissa_bits,drop_MSB>_base2 * 2^(exp-1)

    2009: the older version of this code has always been wrong (since 2000),
    missing the -1 (the number was wrong by a factor of 2). Checked against
    inserting hexdump code and compared with python snipplets (and pxldis):

    import struct
    for x in struct.pack("f", number):
        hex(ord(x))
*/

void
px_put_r(stream * s, double r)
{				/* Convert to single-precision IEEE float. */
    int exp;
    long mantissa = (long)(frexp(r, &exp) * 0x1000000);

    /* we can go a bit lower than -126 and represent:
           2^(-126) * 0.[22 '0' then '1']_base2 = 2 ^(146) * 0.1_base2
       but it is simplier for such small number to be zero. */
    if (exp < -126)
        mantissa = 0, exp = 0;	/* unnormalized */
    /* put the sign bit in the right place */
    if (mantissa < 0)
        exp += 128, mantissa = -mantissa;
    /* All quantities are little-endian. */
    spputc(s, (byte) mantissa);
    spputc(s, (byte) (mantissa >> 8));
    spputc(s, (byte) (((exp + 126) << 7) + ((mantissa >> 16) & 0x7f)));
    spputc(s, (byte) ((exp + 126) >> 1));
}
void
px_put_rl(stream * s, double r)
{
    spputc(s, pxt_real32);
    px_put_r(s, r);
}

void
px_put_rp(stream * s, double rx, double ry)
{
    spputc(s, pxt_real32_xy);
    px_put_r(s, rx);
    px_put_r(s, ry);
}

void
px_put_rpa(stream * s, double rx, double ry, px_attribute_t a)
{
    px_put_rp(s, rx, ry);
    px_put_a(s, a);
}

/* ubyte_array with attribute */
void
px_put_ubaa(stream * s, const byte * data, uint count, px_attribute_t a)
{
    if ((int)count < 0)
        return;
    spputc(s, pxt_ubyte_array);
    /* uint16 LE length field */
    px_put_us(s, count);
    px_put_bytes(s, data, count);
    px_put_a(s, a);
}

void
px_put_data_length(stream * s, uint num_bytes)
{
    if (num_bytes > 255) {
        spputc(s, pxt_dataLength);
        px_put_l(s, (ulong) num_bytes);
    } else {
        spputc(s, pxt_dataLengthByte);
        spputc(s, (byte) num_bytes);
    }
}
