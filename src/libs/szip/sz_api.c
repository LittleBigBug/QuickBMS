/*==============================================================================
The SZIP Science Data Lossless Compression Program is Copyright (C) 2001 Science
& Technology Corporation @ UNM.  All rights released.  Copyright (C) 2003 Lowell
H. Miles and Jack A. Venbrux.  Licensed to ICs Corp. for distribution by the
University of Illinois' National Center for Supercomputing Applications as a
part of the HDF data storage and retrieval file format and software library
products package.  All rights reserved.  Do not modify or use for other
purposes.

SZIP implements an extended Rice adaptive lossless compression algorithm
for sample data.  The primary algorithm was developed by R. F. Rice at
Jet Propulsion Laboratory.

SZIP embodies certain inventions patented by the National Aeronautics &
Space Administration.  United States Patent Nos. 5,448,642, 5,687,255,
and 5,822,457 have been licensed to ICs Corp. for distribution with the
HDF data storage and retrieval file format and software library products.
All rights reserved.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP decompression software
routines and underlying patents is hereby granted by ICs Corp. to all users
of and in conjunction with HDF data storage and retrieval file format and
software library products.

Revocable (in the event of breach by the user or if required by law),
royalty-free, nonexclusive sublicense to use SZIP compression software
routines and underlying patents for non-commercial, scientific use only
is hereby granted by ICs Corp. to users of and in conjunction with HDF
data storage and retrieval file format and software library products.

For commercial use license to SZIP compression software routines and underlying
patents please contact ICs Corp. at ICs Corp., 721 Lochsa Street, Suite 8,
Post Falls, ID 83854.  (208) 262-2008.

==============================================================================*/
/* sz_api.c -- szlib interface to szip functions
 * This code is loosely based on the zlib inflate and deflate
 * code.  See szlib.h for attribution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SZconfig.h"
#include "szlib.h"

long szip_compress_memory(
    int options_mask,
    int bits_per_pixel,
    int pixels_per_block,
    int pixels_per_scanline,
    const void *in,
    long pixels,
    char *out);

long szip_uncompress_memory(
    int new_options_mask,
    int new_bits_per_pixel,
    int new_pixels_per_block,
    int new_pixels_per_scanline,
    const char *in,
    long in_bytes,
    void *out,
    long out_pixels);

int szip_check_params(
    int bits_per_pixel,
    int pixels_per_block,
    int pixels_per_scanline,
    long image_pixels,
    char **msg);

int
SZ_CompressInit(sz_stream *strm)
{
    int rv;

    if (strm == SZ_NULL)
        return SZ_STREAM_ERROR;

    strm->hidden = 0;
    strm->msg = SZ_NULL;

    strm->state = SZ_INPUT_IMAGE;

    if (strm->hidden == SZ_NULL)
        {
        strm->hidden = (sz_hidden_data *) malloc(sizeof(sz_hidden_data));
        if (strm->hidden == SZ_NULL)
            return SZ_MEM_ERROR;

        memset(strm->hidden, 0, sizeof(sz_hidden_data));
        }

    rv = szip_check_params(strm->bits_per_pixel, strm->pixels_per_block, strm->pixels_per_scanline, strm->image_pixels, &strm->msg);
    if (rv == 0)
        return SZ_INIT_ERROR;

    return SZ_OK;
}

int
SZ_Compress(sz_stream *strm, int flush)
{
    sz_stream *sz;
    sz_hidden_data *hidden;
    int bytes_per_pixel;
    long image_size;
    long size_in;
    long size_out;
    long output_bytes;

    if (strm == SZ_NULL || strm->next_in == SZ_NULL)
        return SZ_STREAM_ERROR;

    bytes_per_pixel = (strm->bits_per_pixel+7) >> 3;
    if (bytes_per_pixel == 3)
        bytes_per_pixel = 4;

    hidden = (sz_hidden_data *) strm->hidden;
    if (hidden->image_in == SZ_NULL)
        {
        image_size = strm->image_pixels * bytes_per_pixel;
        hidden->image_in = (char *) malloc(image_size);
        hidden->next_in = hidden->image_in;
        hidden->avail_in = image_size;
        }

    if (hidden->image_out == SZ_NULL)
        {
        image_size = (long)(strm->image_pixels * bytes_per_pixel * 1.75);
        hidden->image_out = (char *) malloc(image_size);
        hidden->next_out = hidden->image_out;
        hidden->avail_out = 0;
        }

    if (strm->state == SZ_INPUT_IMAGE)
        {
        /*** store input data in buffer ***/
        size_in = hidden->avail_in < strm->avail_in ? hidden->avail_in : strm->avail_in;
        memcpy(hidden->next_in, strm->next_in, size_in);
        hidden->next_in += size_in;
        hidden->avail_in -= size_in;

        strm->next_in += size_in;
        strm->avail_in -= size_in;
        strm->total_in += size_in;

        if (hidden->avail_in == 0)
            {
            sz = strm;
            output_bytes = szip_compress_memory(sz->options_mask, sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, hidden->image_in, sz->image_pixels, hidden->image_out);
            if (output_bytes < 0)
                return output_bytes;
#if 0
            printf("compress_memory: output_bytes=%ld\n", output_bytes);
#endif
            hidden->avail_out = output_bytes;
            sz->state = SZ_OUTPUT_IMAGE;
            if (flush == SZ_NO_FLUSH)
                return SZ_OK;
            }
        }

    if (strm->state == SZ_OUTPUT_IMAGE)
        {
        /*** read output data in buffer ***/
        size_out = hidden->avail_out < strm->avail_out ? hidden->avail_out : strm->avail_out;
        memcpy(strm->next_out, hidden->next_out, size_out);
        hidden->next_out += size_out;
        hidden->avail_out -= size_out;

        strm->next_out += size_out;
        strm->avail_out -= size_out;
        strm->total_out += size_out;

        if (hidden->avail_out == 0)
            strm->state = SZ_FINISH;
        }

    if (strm->state == SZ_FINISH)
        return SZ_STREAM_END;

    return SZ_OK;
}

int
SZ_CompressEnd(sz_stream *strm)
{
    sz_hidden_data *hidden;

    if (strm == SZ_NULL)
        return SZ_STREAM_ERROR;

    strm->avail_in = 0;
    strm->avail_out = 0;

    strm->next_in = 0;
    strm->next_out = 0;
    if (strm->hidden)
        {
        hidden = strm->hidden;
        if (hidden->image_in)
            {
            free(hidden->image_in);
            hidden->image_in = 0;
            }

        if (hidden->image_out)
            {
            free(hidden->image_out);
            hidden->image_out = 0;
            }
        }

    strm->state = SZ_FINISH;

    return SZ_OK;
}

int
SZ_DecompressInit(sz_stream *strm)
{
    int rv;

    if (strm == SZ_NULL)
        return SZ_STREAM_ERROR;

    strm->msg = SZ_NULL;
    strm->hidden = 0;
    strm->state = SZ_INPUT_IMAGE;

    if (strm->hidden == SZ_NULL)
        {
        strm->hidden = (sz_hidden_data *) malloc(sizeof(sz_hidden_data));
        if (strm->hidden == SZ_NULL)
            return SZ_MEM_ERROR;

        memset(strm->hidden, 0, sizeof(sz_hidden_data));
        }

    rv = szip_check_params(strm->bits_per_pixel, strm->pixels_per_block, strm->pixels_per_scanline, strm->image_pixels, &strm->msg);
    if (rv == 0)
        return SZ_INIT_ERROR;

    return SZ_OK;
}

int
SZ_Decompress(sz_stream *strm, int flush)
{
    sz_stream *sz;
    sz_hidden_data *hidden;
    int bytes_per_pixel;
    long image_size;
    long size_in;
    long size_out;
    long output_bytes;

    if (strm == SZ_NULL || strm->next_out == SZ_NULL)
        return SZ_STREAM_ERROR;

    bytes_per_pixel = (strm->bits_per_pixel+7) >> 3;
    if (bytes_per_pixel == 3)
        bytes_per_pixel = 4;

    hidden = (sz_hidden_data *) strm->hidden;
    if (hidden->image_in == SZ_NULL)
        {
        image_size = (long)(strm->image_pixels * bytes_per_pixel * 1.75);
        hidden->image_in = (char *) malloc(image_size);
        hidden->next_in = hidden->image_in;
        hidden->avail_in = image_size;
        }

    if (hidden->image_out == SZ_NULL)
        {
        image_size = (long)(strm->image_pixels * bytes_per_pixel * 1.0);
        hidden->image_out = (char *) malloc(image_size);
        hidden->next_out = hidden->image_out;
        hidden->avail_out = 0;
        }

    if (strm->state == SZ_INPUT_IMAGE)
        {
        /*** store input data in buffer ***/
        size_in = hidden->avail_in < strm->avail_in ? hidden->avail_in : strm->avail_in;
        memcpy(hidden->next_in, strm->next_in, size_in);
        hidden->next_in += size_in;
        hidden->avail_in -= size_in;

        strm->next_in += size_in;
        strm->avail_in -= size_in;
        strm->total_in += size_in;

        if (hidden->avail_in == 0 || flush == SZ_FINISH)
            {
            sz = strm;
            size_in = hidden->next_in - hidden->image_in;
            output_bytes = szip_uncompress_memory(sz->options_mask, sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, hidden->image_in, size_in, hidden->image_out, strm->image_pixels);
            if (output_bytes < 0)
                return output_bytes;

            hidden->avail_out = output_bytes;
            sz->state = SZ_OUTPUT_IMAGE;
            }
        }

    if (strm->state == SZ_OUTPUT_IMAGE)
        {
        /*** read output data in buffer ***/
        size_out = hidden->avail_out < strm->avail_out ? hidden->avail_out : strm->avail_out;
        memcpy(strm->next_out, hidden->next_out, size_out);
        hidden->next_out += size_out;
        hidden->avail_out -= size_out;

        strm->next_out += size_out;
        strm->avail_out -= size_out;
        strm->total_out += size_out;

        if (hidden->avail_out == 0)
            strm->state = SZ_FINISH;
        }

    if (strm->state == SZ_FINISH)
        return SZ_STREAM_END;

    return SZ_OK;
}

int
SZ_DecompressEnd(sz_stream *strm)
{
    sz_hidden_data *hidden;

    if (strm == SZ_NULL)
        return SZ_STREAM_ERROR;

    strm->avail_in = 0;
    strm->avail_out = 0;

    strm->next_in = 0;
    strm->next_out = 0;
    if (strm->hidden)
        {
        hidden = strm->hidden;
        if (hidden->image_in)
            {
            free(hidden->image_in);
            hidden->image_in = 0;
            }

        if (hidden->image_out)
            {
            free(hidden->image_out);
            hidden->image_out = 0;
            }
        }

    strm->state = SZ_FINISH;

    return SZ_OK;
}

extern int szip_output_buffer_full;

static SZ_com_t sz_default_param = { SZ_RAW_OPTION_MASK | SZ_ALLOW_K13_OPTION_MASK, 8, 16, 256 };

int
SZ_BufftoBuffCompress(void *dest, size_t *destLen, const void *source, size_t sourceLen, SZ_com_t *param)
{
    SZ_com_t *sz;
    char *image_out;
    char *msg;
    int bytes_per_pixel;
    int rv;
    long out_size;
    long pixels;
    long output_bytes;

    if (szip_allow_encoding == 0) {
        return SZ_NO_ENCODER_ERROR;
    }
    sz = param;
    if (sz == 0)
        sz = &sz_default_param;

    rv = szip_check_params(sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, sz->pixels_per_scanline, &msg);
    if (rv == 0)
        return SZ_PARAM_ERROR;

    bytes_per_pixel = (sz->bits_per_pixel+7) >> 3;
    if (bytes_per_pixel == 3)
        bytes_per_pixel = 4;

    pixels = (sourceLen+bytes_per_pixel-1)/bytes_per_pixel;
    out_size = (long)(sourceLen * 2.00);

    if (*destLen >= out_size)
        image_out = dest;
    else
        {
        image_out = (char *) malloc(out_size);
        if (image_out == 0)
            return SZ_MEM_ERROR;
        }

    output_bytes = szip_compress_memory(sz->options_mask, sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, source, pixels, image_out);
    if (output_bytes < 0)
        {
        if (image_out != dest)
            free(image_out);

        return output_bytes;
        }

    rv = SZ_OK;
    if (*destLen >= output_bytes)
        *destLen = output_bytes;
    else
        rv = SZ_OUTBUFF_FULL;

    if (image_out != dest)
        {
        memcpy(dest, image_out, *destLen);
        free(image_out);
        }

    return rv;
}

int
SZ_BufftoBuffDecompress(void *dest, size_t *destLen, const void *source, size_t sourceLen, SZ_com_t *param)
{
    SZ_com_t *sz;
    char *msg;
    int bytes_per_pixel;
    int rv;
    long pixels;
    long output_bytes;

    sz = param;
    if (sz == 0)
        sz = &sz_default_param;

    rv = szip_check_params(sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, sz->pixels_per_scanline, &msg);
    if (rv == 0)
        return SZ_PARAM_ERROR;

    bytes_per_pixel = (sz->bits_per_pixel+7) >> 3;
    if (bytes_per_pixel == 3)
        bytes_per_pixel = 4;

    pixels = *destLen/bytes_per_pixel;

    output_bytes = szip_uncompress_memory(sz->options_mask, sz->bits_per_pixel, sz->pixels_per_block, sz->pixels_per_scanline, source, sourceLen, dest, pixels);
    if (output_bytes < 0)
        return output_bytes;

    rv = SZ_OK;
    if (szip_output_buffer_full)
        rv = SZ_OUTBUFF_FULL;
    else
        *destLen = output_bytes;

    return rv;
}

int
SZ_encoder_enabled() {
    return(szip_allow_encoding);
}
