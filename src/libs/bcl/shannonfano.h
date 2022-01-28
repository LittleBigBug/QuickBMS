/*************************************************************************
* Name:        shannonfano.h
* Author:      Marcus Geelnard
* Description: Shannon-Fano coder/decoder interface.
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

#ifndef _shannonfano_h_
#define _shannonfano_h_

#ifdef __cplusplus
extern "C" {
#endif


/*************************************************************************
* Function prototypes
*************************************************************************/

int SF_Compress( unsigned char *in, unsigned char *out,
                 unsigned int insize );
void SF_Uncompress( unsigned char *in, unsigned char *out,
                    unsigned int insize, unsigned int outsize );


#ifdef __cplusplus
}
#endif

#endif /* _shannonfano_h_ */
