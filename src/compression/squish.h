/*
 * Copyright (c) 2013 Andrew G. Crowell (Overkill / Bananattack)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of squish nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SQUISH CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQUISH_H
#define SQUISH_H

#include <stdint.h>
#include <stddef.h>

/*
 * Pack raw source data into the given destination buffer.
 *
 * Arguments:
 *      src - pointer to source buffer.
 *      src_length - size of the source buffer.
 *      dest - pointer to destination buffer.
 *      dest_length - size of the destination buffer.
 * Return:
 *      On success, returns the length of the compressed buffer.
 *      On failure, returns 0.
 */
size_t squish_pack(const uint8_t *src, size_t src_length, uint8_t *dest, size_t dest_length);

/*
 * Unpack previously packed source data into the given destination buffer.
 * Arguments:
 *      src - pointer to source buffer.
 *      src_length - size of the source buffer.
 *      dest - pointer to destination buffer.
 *      dest_length - size of the destination buffer.
 * Return:
 *      On success, returns the length of the decompressed buffer.
 *      On failure, returns 0.
 */
size_t squish_unpack(const uint8_t *src, size_t src_length, uint8_t *dest, size_t dest_length);

#endif
