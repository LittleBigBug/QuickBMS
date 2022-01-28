/* $Id: nlsref.c 444 2006-05-17 07:41:23Z mwp $ */
/* nls: NLS stream cipher and Mundja MAC -- reference implementation */
/* This is "tweaked" to support non-word-multiple keys and
 * variable "Konst" to address Joe Cho's attack.
 */
 
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

/* interface and SBox */
#include <stdlib.h>
#include <string.h>
#include "nls.h"
#include "nlssbox.h"
#include "nlsmultab.h"

/*
 * FOLD is how many register cycles need to be performed after combining the
 * last byte of key and non-linear feedback, before every byte depends on every
 * byte of the key. This depends on the feedback and nonlinear functions, and
 * on where they are combined into the register.
 */
#define FOLD N		/* how many iterations of folding to do */
#define INITKONST 0x6996c53a /* value of KONST to use during key loading */
#define KEYP 15		/* where to insert key words */
#define FOLDP 4		/* where to insert extra non-linear feedback during keying */
#if NLS_LONG_OUTPUT
#define CTRP 2		/* where to insert counter to avoid small cycles */
#endif /*NLS_LONG_OUTPUT*/

/* some useful macros -- machine independent little-endian */
#define Byte(x,i) ((UCHAR)(((x) >> (8*(i))) & 0xFF))
#define BYTE2WORD(b) ( \
	(((WORD)(b)[3] & 0xFF)<<24) | \
	(((WORD)(b)[2] & 0xFF)<<16) | \
	(((WORD)(b)[1] & 0xFF)<<8) | \
	(((WORD)(b)[0] & 0xFF)) \
)
#define WORD2BYTE(w, b) { \
	(b)[3] = Byte(w,3); \
	(b)[2] = Byte(w,2); \
	(b)[1] = Byte(w,1); \
	(b)[0] = Byte(w,0); \
}
#define XORWORD(w, b) { \
	(b)[3] ^= Byte(w,3); \
	(b)[2] ^= Byte(w,2); \
	(b)[1] ^= Byte(w,1); \
	(b)[0] ^= Byte(w,0); \
}

#if NLS_LONG_OUTPUT
#define ZEROCOUNTER(c)	c->CtrModF16 = c->CtrMod232 = 0
#else
#define ZEROCOUNTER(c)	/* nothing */
#endif /*NLS_LONG_OUTPUT*/

/* Return a non-linear function of some parts of the register. Note that most
 * of the nonlinearity is actually in the feedback function.
 */
static WORD
nltap(nls_ctx *c)
{
    return (c->R[0] + c->R[16]) ^ (c->R[1] + c->R[13]) ^ (c->R[6] + c->konst);
}

/* cycle the contents of the shift register
 */
static void
cycle(nls_ctx *c)
{
    WORD	t;
    int		i;

    /* nonlinear feedback function */
    t = ROTL(c->R[0], 19) + ROTL(c->R[15], 9) + c->konst;
    t ^= Sbox[(t >> 24) & 0xFF];
    t ^= c->R[4];
    /* shift register */
    for (i = 1; i < N; ++i)
	c->R[i-1] = c->R[i];
    c->R[N-1] = t;
#if NLS_LONG_OUTPUT
    /* increment counter */
    ++c->CtrModF16;
    /* mix counter into register every so often */
    if (c->CtrModF16 == F16) {
	c->CtrMod232 += c->CtrModF16;
	c->R[CTRP] += c->CtrMod232;
	c->CtrModF16 = 0;
	c->konst = nltap(c); /* TWEAK */
	cycle(c); /* TWEAK */
    }
#endif /*NLS_LONG_OUTPUT*/
}

/* The nls MAC function is modelled after the round function of SHA-256.
 * The following lines establish aliases for the MAC accumulator, just
 * so that the definition of that function looks more like FIPS-180-2.
 */
#define A c->M[0]
#define B c->M[1]
#define C c->M[2]
#define D c->M[3]
#define E c->M[4]
#define F c->M[5]
#define G c->M[6]
#define H c->M[7]
#define SIGMA0(x) (ROTR((x), 2) ^ ROTR((x), 13) ^ ROTR((x), 22))
#define SIGMA1(x) (ROTR((x), 6) ^ ROTR((x), 11) ^ ROTR((x), 25))
#define CHOOSE(x,y,z) (((x) & (y)) ^ ((~(x)) & (z)))
#define MAJORITY(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

/* Accumulate a nonlinear function of a register word and an input word for MAC.
 * Except for the added S-Box and SOBER LFSR input instead of constants,
 * this is exactly a round of SHA-256.
 */
static void
shafunc(nls_ctx *c, WORD i)
{
    WORD	t1;

    t1 = H + c->R[MACKONST] + i;
    t1 ^= Sbox[(t1 >> 24) & 0xFF];
    t1 += SIGMA1(E) + CHOOSE(E, F, G);
    H = G;
    G = F;
    F = E;
    E = D + t1;
    t1 += SIGMA0(A) + MAJORITY(A, B, C);
    D = C;
    C = B;
    B = A;
    A = t1;
}

/* Accumulate a CRC of input words, later to be fed into MAC.
 */
static void
crcfunc(nls_ctx *c, WORD i)
{
    WORD	t1;
    int		j;

    /* Accumulate CRC of input */
    t1 = (c->CRC[0] << 8) ^ Multab[(c->CRC[0] >> 24) & 0xFF] ^ c->CRC[5] ^ i;
    for (j = 1; j < NMAC; ++j)
	c->CRC[j-1] = c->CRC[j];
    c->CRC[NMAC-1] = t1;
}

/* Normal MAC word processing: do both SHA and CRC.
 */
static void
macfunc(nls_ctx *c, WORD i)
{
    crcfunc(c, i);
    shafunc(c, i);
}

/* initialise to known state
 */
static void
nls_initstate(nls_ctx *c)
{
    int		i;

    /* Register initialised to Fibonacci numbers; Counter zeroed. */
    c->R[0] = 1;
    c->R[1] = 1;
    for (i = 2; i < N; ++i)
	c->R[i] = c->R[i-1] + c->R[i-2];
    c->konst = INITKONST;
    ZEROCOUNTER(c);
}

/* Save the current register state
 */
static void
nls_savestate(nls_ctx *c)
{
    int		i;

    for (i = 0; i < N; ++i)
	c->initR[i] = c->R[i];
}

/* initialise to previously saved register state
 */
static void
nls_reloadstate(nls_ctx *c)
{
    int		i;

    for (i = 0; i < N; ++i)
	c->R[i] = c->initR[i];
    ZEROCOUNTER(c);
}

/* Initialise "konst"
 * Tweak -- since Konst now changes regularly, there is no reason
 * to avoid a zero high byte.
 */
static void
nls_genkonst(nls_ctx *c)
{
    cycle(c);
    c->konst = nltap(c);
}

/* Load key material into the register
 */
#define ADDKEY(k) \
	c->R[KEYP] += (k);

#define XORNL(nl) \
	c->R[FOLDP] ^= (nl);

/* nonlinear diffusion of register for key and MAC */
static void
nls_diffuse(nls_ctx *c)
{
    int		i;

    for (i = 0; i < FOLD; ++i)
    {
	cycle(c);
	XORNL(nltap(c));
    }
}

/* common actions for loading key material */
/* Tweak: allow non-word-multiple key and nonce material.
 */
static void
nls_loadkey(nls_ctx *c, UCHAR key[], int keylen)
{
    int		i, j;
    WORD	k;
    UCHAR	xtra[4];

    /* start folding in key */
    for (i = 0; i < (keylen & ~0x3); i += 4)
    {
	k = BYTE2WORD(&key[i]);
	ADDKEY(k);
        cycle(c);
	XORNL(nltap(c));
    }

    /* if there were any extra key bytes, zero pad to a word */
    if (i < keylen) {
	for (j = 0 /* i unchanged */; i < keylen; ++i)
	    xtra[j++] = key[i];
	for (/* j unchanged */; j < 4; ++j)
	    xtra[j] = 0;
	k = BYTE2WORD(xtra);
	ADDKEY(k);
        cycle(c);
	XORNL(nltap(c));
    }

    /* also fold in the length of the key */
    ADDKEY(keylen);

    /* now diffuse */
    nls_diffuse(c);
}

/* initialise MAC related registers
 */
void
nls_macinit(nls_ctx *c)
{
    int    i;
    
    for (i = 0; i < NMAC; ++i) {
	c->M[i] = c->R[i];
	c->CRC[i] = c->R[i+NMAC];
    }
}

/* Published "key" interface
 */
void
nls_key(nls_ctx *c, UCHAR key[], int keylen)
{
    nls_initstate(c);
    nls_loadkey(c, key, keylen);
    nls_genkonst(c);
    nls_savestate(c);
    nls_macinit(c);
    c->nbuf = 0;
    ZEROCOUNTER(c);
}

/* Published "IV" interface
 */
void
nls_nonce(nls_ctx *c, UCHAR nonce[], int noncelen)
{
    nls_reloadstate(c);
    c->konst = INITKONST; /* TWEAK */
    nls_loadkey(c, nonce, noncelen);
    nls_genkonst(c); /* TWEAK */
    nls_macinit(c);
    c->nbuf = 0;
    ZEROCOUNTER(c);
}

/* XOR pseudo-random bytes into buffer
 * Note: doesn't play well with MAC functions.
 */
void
nls_stream(nls_ctx *c, UCHAR *buf, int nbytes)
{
    UCHAR       *endbuf;
    WORD	t = 0;

    /* handle any previously buffered bytes */
    while (c->nbuf != 0 && nbytes != 0) {
	*buf++ ^= c->sbuf & 0xFF;
	c->sbuf >>= 8;
	c->nbuf -= 8;
	--nbytes;
    }

    /* handle whole words */
    endbuf = &buf[nbytes & ~((WORD)0x03)];
    while (buf < endbuf)
    {
	cycle(c);
	t = nltap(c);
	XORWORD(t, buf);
	buf += 4;
    }

    /* handle any trailing bytes */
    nbytes &= 0x03;
    if (nbytes != 0) {
	cycle(c);
	c->sbuf = nltap(c);
	c->nbuf = 32;
	while (c->nbuf != 0 && nbytes != 0) {
	    *buf++ ^= c->sbuf & 0xFF;
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* accumulate words into MAC without encryption
 * Note that plaintext is accumulated for MAC.
 */
void
nls_maconly(nls_ctx *c, UCHAR *buf, int nbytes)
{
    UCHAR       *endbuf;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= (*buf++) << (32 - c->nbuf);
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* not a whole word yet */
	    return;
	/* LFSR already cycled */
	macfunc(c, c->mbuf);
    }

    /* handle whole words */
    endbuf = &buf[nbytes & ~((WORD)0x03)];
    while (buf < endbuf)
    {
	cycle(c);
	macfunc(c, BYTE2WORD(buf));
	buf += 4;
    }

    /* handle any trailing bytes */
    nbytes &= 0x03;
    if (nbytes != 0) {
	cycle(c);
	c->sbuf = nltap(c);
	c->mbuf = 0;
	c->nbuf = 32;
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= (*buf++) << (32 - c->nbuf);
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* Combined MAC and encryption.
 * Note that plaintext is accumulated for MAC.
 */
void
nls_encrypt(nls_ctx *c, UCHAR *buf, int nbytes)
{
    UCHAR       *endbuf;
    WORD	t = 0;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= *buf << (32 - c->nbuf);
	    *buf ^= (c->sbuf >> (32 - c->nbuf)) & 0xFF;
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* not a whole word yet */
	    return;
	/* LFSR already cycled */
	macfunc(c, c->mbuf);
    }

    /* handle whole words */
    endbuf = &buf[nbytes & ~((WORD)0x03)];
    while (buf < endbuf)
    {
	cycle(c);
	t = BYTE2WORD(buf);
	macfunc(c, t);
	t ^= nltap(c);
	WORD2BYTE(t, buf);
	buf += 4;
    }

    /* handle any trailing bytes */
    nbytes &= 0x03;
    if (nbytes != 0) {
	cycle(c);
	c->sbuf = nltap(c);
	c->mbuf = 0;
	c->nbuf = 32;
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= *buf << (32 - c->nbuf);
	    *buf ^= (c->sbuf >> (32 - c->nbuf)) & 0xFF;
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* Combined MAC and decryption.
 * Note that plaintext is accumulated for MAC.
 */
void
nls_decrypt(nls_ctx *c, UCHAR *buf, int nbytes)
{
    UCHAR       *endbuf;
    WORD	t = 0;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    *buf ^= (c->sbuf >> (32 - c->nbuf)) & 0xFF;
	    c->mbuf ^= *buf << (32 - c->nbuf);
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* not a whole word yet */
	    return;
	/* LFSR already cycled */
	macfunc(c, c->mbuf);
    }

    /* handle whole words */
    endbuf = &buf[nbytes & ~((WORD)0x03)];
    while (buf < endbuf)
    {
	cycle(c);
	t = nltap(c);
	t ^= BYTE2WORD(buf);
	macfunc(c, t);
	WORD2BYTE(t, buf);
	buf += 4;
    }

    /* handle any trailing bytes */
    nbytes &= 0x03;
    if (nbytes != 0) {
	cycle(c);
	c->sbuf = nltap(c);
	c->mbuf = 0;
	c->nbuf = 32;
	while (c->nbuf != 0 && nbytes != 0) {
	    *buf ^= (c->sbuf >> (32 - c->nbuf)) & 0xFF;
	    c->mbuf ^= *buf << (32 - c->nbuf);
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* Having accumulated a MAC, finish processing and return it.
 * Note that any unprocessed bytes are treated as if
 * they were encrypted zero bytes, so plaintext (zero) is accumulated.
 */
void
nls_finish(nls_ctx *c, UCHAR *buf, int nbytes)
{
    int		i;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	/* LFSR already cycled */
	macfunc(c, c->mbuf);
    }
    
    /* perturb the MAC to mark end of input.
     * Note that only the SHA part is updated, not the CRC. This is an
     * action that can't be duplicated by passing in plaintext, hence
     * defeating any kind of extension attack.
     */
    cycle(c);
    shafunc(c, INITKONST + (c->nbuf << 24));
    c->nbuf = 0;

    /* now add the CRC to the MAC like input material */
    for (i = 0; i < NMAC; ++i) {
        cycle(c);
	crcfunc(c, 0);
	shafunc(c, c->CRC[7]);
    }

    /* continue that process, producing output from the MAC buffer */
    while (nbytes > 0) {
        cycle(c);
	crcfunc(c, 0);
	shafunc(c, c->CRC[7]);
	if (nbytes >= 4) {
	    WORD2BYTE(A, buf);
	    nbytes -= 4;
	    buf += 4;
	}
	else {
	    for (i = 0; i < nbytes; ++i)
		buf[i] = Byte(A, i);
	    break;
	}
    }
}
