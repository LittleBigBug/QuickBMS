/* $Id: sss.h 388 2005-04-28 21:04:09Z mwp $ */
/* SSS; Self-Synchronous SOBER */

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

#ifndef _SSS_DEFINED
#define _SSS_DEFINED 1

#define N 17
#define MAXKEY 128/8
#define WORDBITS 16
#define WORDBYTES (WORDBITS >> 3)
#define WORD unsigned short
#define UCHAR unsigned char

/* the key dependent S-box can be either precomputed or calculated on the fly */
#define SBOX_PRECOMP 1

typedef struct {
    WORD	R[N];		/* Working storage for the shift register */
    WORD	CRC[N];		/* working storage for the CRC register */
#if SBOX_PRECOMP
    WORD	SBox[256];	/* key dependent SBox */
#else
    UCHAR	key[MAXKEY];	/* copy of key */
    int		keylen;		/* length of key in bytes */
#endif /*SBOX_PRECOMP*/
    WORD	sbuf;		/* partial word stream buffer */
    WORD	cbuf;		/* partial word ciphertext buffer */
    WORD	mbuf;		/* partial word MAC input buffer */
    int		nbuf;		/* number of part-word stream bits buffered */
} sss_ctx;

#if SBOX_PRECOMP
/* Run a word through the SBox, assuming it is precalculated */
#define S(c,w) ((c->SBox[HIGHBYTE(w)]) ^ (w))
#else
#define S(c,w) Sfunc((c)->key, (c)->keylen, (WORD)(w))
#endif /*SBOX_PRECOMP*/

#define HIGHBYTE(w) (((w) >> (WORDBITS - 8)) & 0xFF)
#define LOWMASK 0x00FF /* all the word except the high byte */
#define ROTL(w,x) (((w) << (x))|((w) >> (16 - (x))))
#define ROTR(w,x) (((w) >> (x))|((w) << (16 - (x))))

/* interface definitions */
void sss_key(sss_ctx *c, UCHAR key[], int keylen); 	/* set key */
void sss_nonce(sss_ctx *c, UCHAR nonce[], int nlen);	/* set nonce */
void sss_enconly(sss_ctx *c, UCHAR *buf, int nbytes);	/* stream encryption */
void sss_deconly(sss_ctx *c, UCHAR *buf, int nbytes);	/* stream decryption */
void sss_maconly(sss_ctx *c, UCHAR *buf, int nbytes);	/* accumulate MAC */
void sss_encrypt(sss_ctx *c, UCHAR *buf, int nbytes);	/* encrypt + MAC */
void sss_decrypt(sss_ctx *c, UCHAR *buf, int nbytes);	/* decrypt + MAC */
void sss_finish(sss_ctx *c, UCHAR *buf, int nbytes);	/* finalise MAC */

#endif /* _SSS_DEFINED */
