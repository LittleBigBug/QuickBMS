/* $Id: nls.h 444 2006-05-17 07:41:23Z mwp $ */
/* nls: NLS stream cipher and Mundja MAC header files */

/*
THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND AGAINST
INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _NLS_DEFINED
#define _NLS_DEFINED 1

#ifndef ECRYPT_API
#include <limits.h>
#if __STDC_VERSION__ >= 199901
#include <stdint.h>
#endif
#endif

/*
 * NLS maintains a counter that perturbs the state of the register
 * at long intervals to ensure that short cycles are impossibile.
 * This counter makes no difference to the output of the cipher for
 * the first F16 words (approximately 1/4 MegaByte).
 * If NLS is only going to be used for packets shorter than this,
 * setting the following define to 0  will generate faster and simpler code.
 */
#ifndef NLS_LONG_OUTPUT
#define NLS_LONG_OUTPUT 1
#endif

#define N 17
#define NMAC 8
#define WORDSIZE 32

#ifdef ECRYPT_API
#define WORD u32
#define UCHAR u8
#else
#if __STDC_VERSION__ >= 199901
#define WORD uint32_t
#define WORD_MAX UINT32_MAX
#elif UINT_MAX >= 0xffffffff
#define WORD unsigned int
#define WORD_MAX UINT_MAX
#else
#define WORD unsigned long
#define WORD_MAX ULONG_MAX
#endif
#define UCHAR unsigned char
#endif

#if NLS_LONG_OUTPUT
#define F16 0x10001ul
#endif /*NLS_LONG_OUTPUT*/
#define MACKONST 8

#ifdef ECRYPT_API
#define ROTL ROTL32
#define ROTR ROTR32
#else
#if WORD_MAX == 0xffffffff
#define ROTL(w,x) (((w) << (x))|((w) >> (32 - (x))))
#define ROTR(w,x) (((w) >> (x))|((w) << (32 - (x))))
#else
#define ROTL(w,x) (((w) << (x))|(((w) & 0xffffffff) >> (32 - (x))))
#define ROTR(w,x) ((((w) & 0xffffffff) >> (x))|((w) << (32 - (x))))
#endif
#endif

typedef struct {
    WORD	R[N];		/* Working storage for the shift register */
    WORD	M[NMAC];	/* Working storage for MAC accumulation */
    WORD	CRC[NMAC];	/* Working storage for CRC accumulation */
    WORD	initR[N];	/* saved register contents */ 
    WORD	konst;		/* key dependent constant */
    WORD	sbuf;		/* partial word encryption buffer */
    WORD	mbuf;		/* partial word MAC buffer */
    int		nbuf;		/* number of part-word stream bits buffered */
#if NLS_LONG_OUTPUT
    WORD	CtrModF16;	/* Multiprecision counter, modulo F16 */
    WORD	CtrMod232;	/* Multiprecision counter, LSW */
#endif /*NLS_LONG_OUTPUT*/
} nls_ctx;

/* interface definitions */
void nls_key(nls_ctx *c, UCHAR key[], int keylen); 	/* set key */
void nls_nonce(nls_ctx *c, UCHAR nonce[], int nlen);	/* set Init Vector */
void nls_stream(nls_ctx *c, UCHAR *buf, int nbytes);	/* stream cipher */
void nls_maconly(nls_ctx *c, UCHAR *buf, int nbytes);	/* accumulate MAC */
void nls_encrypt(nls_ctx *c, UCHAR *buf, int nbytes);	/* encrypt + MAC */
void nls_decrypt(nls_ctx *c, UCHAR *buf, int nbytes);	/* decrypt + MAC */
void nls_finish(nls_ctx *c, UCHAR *buf, int nbytes);	/* finalise MAC */

#endif /* _NLS_DEFINED */
