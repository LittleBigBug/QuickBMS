unsigned long prs_compress(void* source,void* dest,unsigned long size);
/* compresses data using the PRS scheme.
 * This function is not based on Sega's compression routine; it was written
 * by Fuzziqer Software. It's not as efficient as Sega's, but it compresses
 * rather well.
 *
 * Arguments:
 *     void* source - data to be compressed
 *     void* dest - buffer for compressed data
 *     unsigned long size - size of the uncompressed data
 *
 * Return value: size of the compressed data
 *
 * Notes:
 *     There's no way to tell exactly how large the compressed data will be;
 *     it is recommended that the destination buffer be 9/8 the size of the
 *     source buffer (yes, larger), although it is highly unlikely that the
 *     compressed data will be larger than the uncompressed data. */

unsigned long prs_decompress(void* source,void* dest);
/* decompresses data that was compressed using the PRS scheme.
 * This function was reverse-engineered from Sega's Phantasy Star Online
 * Episode III.
 *
 * Arguments:
 *     void* source - data to be decompressed
 *     void* dest - buffer for decompressed data
 *
 * Return value: size of the decompressed data
 *
 * Notes:
 *     Do not call this function without making sure the destination buffer can
 *     hold the required amount of data. Use the following function to check
 *     the size of the decompressed data. */
 
unsigned long prs_decompress_size(void* source);
/* checks the original size of data that was compressed using PRS.
 * This function was reverse-engineered from Sega's Phantasy Star Online
 * Episode III.
 *
 * Arguments:
 *     void* source - data to check
 *
 * Return value: size of the decompressed data */
