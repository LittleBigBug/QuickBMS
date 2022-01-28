/*************************************************************************
* Name:        rice.h
* Author:      Marcus Geelnard
* Description: Rice coder/decoder interface.
* Reentrant:   Yes
*-------------------------------------------------------------------------
* Copyright (c) 2003-2006 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/

#ifndef _rice_h_
#define _rice_h_

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************
* Supported binary formats
*************************************************************************/

/* These formats have the same endianity as the machine on which the
   (de)coder is running on */
#define RICE_FMT_INT8   1  /* signed 8-bit integer    */
#define RICE_FMT_UINT8  2  /* unsigned 8-bit integer  */
#define RICE_FMT_INT16  3  /* signed 16-bit integer   */
#define RICE_FMT_UINT16 4  /* unsigned 16-bit integer */
#define RICE_FMT_INT32  7  /* signed 32-bit integer   */
#define RICE_FMT_UINT32 8  /* unsigned 32-bit integer */


/*************************************************************************
* Function prototypes
*************************************************************************/

int Rice_Compress( void *in, void *out, unsigned int insize, int format );
void Rice_Uncompress( void *in, void *out, unsigned int insize,
                      unsigned int outsize, int format );


#ifdef __cplusplus
}
#endif

#endif /* _rice_h_ */
