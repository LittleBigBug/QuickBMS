/*
 * libLZR.h
 * 
 * This file is used to de- and encode LZR streams (as used in the Sony PSP firmware)
 * libLZR Version 0.11 by BenHur - http://www.psp-programming.com/benhur
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#ifndef __LIBLZR_H__
#define __LIBLZR_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup libLZR Compression and Decompression Library
 *  @{
 */
#define LZR_ERROR_BUFFER_SIZE   0x80000104
#define LZR_ERROR_INPUT_STREAM  0x80000108
#define LZR_ERROR_MEM_ALLOC     0x800200D9
#define LZR_ERROR_UNSUPPORTED   0x80020325
#define LZR_ENCODE_UNCOMPRESSED 0xFF
#define LZR_DATATYPE_08BIT      0x05
#define LZR_DATATYPE_16BIT      0x06
#define LZR_DATATYPE_32BIT      0x07
#define LZR_DATATYPE_64BIT      0x08

/**
 * Decompress a LZR stream.
 *
 * @param out - Pointer to decompressed stream
 *
 * @param out_capacity - Size of allocated memory for decompressed stream
 *
 * @param in - Pointer to LZR stream
 *
 * @param in_end - Pointer to pointer to first byte AFTER the LZR stream (use NULL if not needed)
 *
 * @returns The number of decompressed bytes on success; A negative error code on failure
 */
int LZRDecompress(void *out, unsigned int out_capacity, void *in, void *in_end);

/**
 * Encode a LZR stream.
 *
 * @param out - Pointer to encoded stream
 *
 * @param out_capacity - Size of allocated memory for encoded stream
 *
 * @param in - Pointer to raw data stream
 *
 * @param in_length - Size of raw data stream
 *
 * @param type - Type of raw data stream (only LZR_ENCODE_UNCOMPRESSED is supported)
 *
 * @returns The number of decompressed bytes on success; A negative error code on failure
 */
int LZRCompress(void *out, unsigned int out_capacity, void *in, unsigned int in_length, char type);

/** @} */

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __LIBLZR_H__
