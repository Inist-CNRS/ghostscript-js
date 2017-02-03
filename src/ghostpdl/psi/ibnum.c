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


/* Level 2 encoded number reading utilities for Ghostscript */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "opcheck.h"
#include "ierrors.h"
#include "stream.h"
#include "ibnum.h"
#include "imemory.h"		/* for iutil.h */
#include "iutil.h"

/* Define the number of bytes for a given format of encoded number. */
const byte enc_num_bytes[] = {
    enc_num_bytes_values
};

/* ------ Encoded number reading ------ */

/* Set up to read from an encoded number array/string. */
/* Return <0 for error, or a number format. */
int
num_array_format(const ref * op)
{
    int format;

    switch (r_type(op)) {
        case t_string:
            {
                /* Check that this is a legitimate encoded number string. */
                const byte *bp = op->value.bytes;

                if (r_size(op) < 4 || bp[0] != bt_num_array_value)
                    return_error(gs_error_typecheck);
                format = bp[1];
                if (!num_is_valid(format) ||
                    sdecodeshort(bp + 2, format) !=
                    (r_size(op) - 4) / encoded_number_bytes(format)
                    )
                    return_error(gs_error_rangecheck);
            }
            break;
        case t_array:
        case t_mixedarray:
        case t_shortarray:
            format = num_array;
            break;
        default:
            return_error(gs_error_typecheck);
    }
    check_read(*op);
    return format;
}

/* Get the number of elements in an encoded number array/string. */
uint
num_array_size(const ref * op, int format)
{
    return (format == num_array ? r_size(op) :
            (r_size(op) - 4) / encoded_number_bytes(format));
}

/* Get an encoded number from an array/string according to the given format. */
/* Put the value in np->value.{intval,realval}. */
/* Return t_int if integer, t_real if real, t_null if end of stream, */
/* or an error if the format is invalid. */
int
num_array_get(const gs_memory_t *mem, const ref * op, int format, uint index, ref * np)
{
    if (format == num_array) {
        int code = array_get(mem, op, (long)index, np);

        if (code < 0)
            return t_null;
        switch (r_type(np)) {
            case t_integer:
                return t_integer;
            case t_real:
                return t_real;
            default:
                return_error(gs_error_typecheck);
        }
    } else {
        uint nbytes = encoded_number_bytes(format);

        if (index >= (r_size(op) - 4) / nbytes)
            return t_null;
        return sdecode_number(op->value.bytes + 4 + index * nbytes,
                              format, np);
    }
}

/* Internal routine to decode a number in a given format. */
/* Same returns as sget_encoded_number. */
static const double binary_scale[32] = {
#define EXPN2(n) (0.5 / (1L << (n-1)))
    1.0, EXPN2(1), EXPN2(2), EXPN2(3),
    EXPN2(4), EXPN2(5), EXPN2(6), EXPN2(7),
    EXPN2(8), EXPN2(9), EXPN2(10), EXPN2(11),
    EXPN2(12), EXPN2(13), EXPN2(14), EXPN2(15),
    EXPN2(16), EXPN2(17), EXPN2(18), EXPN2(19),
    EXPN2(20), EXPN2(21), EXPN2(22), EXPN2(23),
    EXPN2(24), EXPN2(25), EXPN2(26), EXPN2(27),
    EXPN2(28), EXPN2(29), EXPN2(30), EXPN2(31)
#undef EXPN2
};
int
sdecode_number(const byte * str, int format, ref * np)
{
    switch (format & 0x170) {
        case num_int32:
        case num_int32 + 16:
            if ((format & 31) == 0) {
                np->value.intval = sdecodeint32(str, format);
                return t_integer;
            } else {
                np->value.realval =
                    (double)sdecodeint32(str, format) *
                    binary_scale[format & 31];
                return t_real;
            }
        case num_int16:
            if ((format & 15) == 0) {
                np->value.intval = sdecodeshort(str, format);
                return t_integer;
            } else {
                np->value.realval =
                    sdecodeshort(str, format) *
                    binary_scale[format & 15];
                return t_real;
            }
        case num_float:
            {
                float fval;
                int code = sdecode_float(str, format, &fval);

                if (code < 0)
                    return code;
                np->value.realval = fval;
                return t_real;
            }
        default:
            return_error(gs_error_syntaxerror);	/* invalid format?? */
    }
}

/* ------ Decode number ------ */

/* Decode encoded numbers from a string according to format. */

/* Decode a (16-bit, signed or unsigned) short. */
uint
sdecodeushort(const byte * p, int format)
{
    int a = p[0], b = p[1];

    return (num_is_lsb(format) ? (b << 8) + a : (a << 8) + b);
}
int
sdecodeshort(const byte * p, int format)
{
    int v = (int)sdecodeushort(p, format);

    return (v & 0x7fff) - (v & 0x8000);
}

/* Decode a (32-bit, signed) long. */
int
sdecodeint32(const byte * p, int format)
{
    int a = p[0], b = p[1], c = p[2], d = p[3];
    int v = (num_is_lsb(format) ?
              ((int)d << 24) + ((int)c << 16) + (b << 8) + a :
              ((int)a << 24) + ((int)b << 16) + (c << 8) + d);
    return v;
}

/* Decode a 32-bit number. Return the resukt through a pointer */
/* to work around a gcc 4.2.1 bug on PowerPC, bug 689586 */
static void
sdecodebits32(const byte * p, int format, bits32 *v)
{
  int a = p[0], b = p[1], c = p[2], d = p[3];
  *v = (num_is_lsb(format) ?
            ((long)d << 24) + ((long)c << 16) + (b << 8) + a :
            ((long)a << 24) + ((long)b << 16) + (c << 8) + d);

}

/* Decode a float.  We assume that native floats occupy 32 bits. */
/* If the float is an IEEE NaN or Inf, return gs_error_undefinedresult. */
int
sdecode_float(const byte * p, int format, float *pfnum)
{
    bits32 lnum;

    if ((format & ~(num_msb | num_lsb)) == num_float_native) {
        /*
         * Just read 4 bytes and interpret them as a float, ignoring
         * any indication of byte ordering.
         */
        memcpy(pfnum, p, 4);
#if !ARCH_FLOATS_ARE_IEEE
        return 0;		/* no way to check for anomalies */
#endif
        lnum = *(bits32 *)pfnum;
    } else {
        sdecodebits32(p, format, &lnum);

#if !ARCH_FLOATS_ARE_IEEE
        {
            /* We know IEEE floats take 32 bits. */
            /* Convert IEEE float to native float. */
            int sign_expt = lnum >> 23;
            int expt = sign_expt & 0xff;
            long mant = lnum & 0x7fffff;
            float fnum;

            if (expt == 0 && mant == 0)
                fnum = 0;
            else if (expt == 0xff)
                return_error(gs_error_undefinedresult); /* Inf or NaN */
            else {
                mant += 0x800000;
                fnum = (float)ldexp((float)mant, expt - 127 - 23);
            }
            if (sign_expt & 0x100)
                fnum = -fnum;
            *pfnum = fnum;
            return 0;		/* checked for Infs and NaNs above */
        }
#else
        *pfnum = *(float *)&lnum;
#endif
    }
    /*
     * Unfortunately, there is no portable way for testing whether a float
     * is a NaN or Inf.  Do it "by hand" if the input representation is
     * IEEE (which is the case if control arrives here).
     */
    if (!(~lnum & 0x7f800000))	/* i.e. exponent all 1's */
        return_error(gs_error_undefinedresult); /* Inf or NaN */
    return 0;
}
