/* $Id: sssfast.c 388 2005-04-28 21:04:09Z mwp $ */
/* SSS: Self-Synchronous SOBER -- fast implementation */

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

#include <stdlib.h>
#include "sss.h"
#include "ssssbox.h"
#include "sssmultab.h"

#define IS_LITTLE_ENDIAN 1	/* set to 1 for faster operation when appropriate */
#if IS_LITTLE_ENDIAN
/* Useful macros -- little endian words on a little endian machine */
#define BYTE2WORD(b) (*(WORD *)(b))
#define WORD2BYTE(w, b) ((*(WORD *)(b)) = w)
#define XORWORD(w, b) ((*(WORD *)(b)) ^= w)
#else
/* some useful macros -- machine independent little-endian 2-byte words */
#define B(x,i) ((UCHAR)(((x) >> (8*i)) & 0xFF))
#define BYTE2WORD(b) ( \
	(((WORD)((b)[1]) & 0xFF)<<8) | \
	(((WORD)((b)[0]) & 0xFF)) \
)
#define WORD2BYTE(w, b) { \
	(b)[1] = B(w,1); \
	(b)[0] = B(w,0); \
}
#define XORWORD(w, b) { \
	(b)[1] ^= B(w,1); \
	(b)[0] ^= B(w,0); \
}
#endif /* IS_LITTLE_ENDIAN */

/* give correct offset for the current position of the register,
 * where logically R[0] is at position "zero".
 */
#define OFF(zero, i) (((zero)+(i)) % N)

/* key-dependent Sbox function -- after the model used by Turing */
static WORD
Sfunc(UCHAR *key, int keylen, WORD w)
{
    register int	i;
    WORD		t;
    UCHAR		b;

    t = 0;
    b = HIGHBYTE(w);
    for (i = 0; i < keylen; ++i) {
	b = ftable[b ^ key[i]];
	t ^= ROTL(Qbox[b], i);
    }
    return ((b << (WORDBITS-8)) | (t & LOWMASK)) ^ (w & LOWMASK);
}

/* step the register */
/* After stepping, "zero" moves right one place */
#define CYCLE(R,z,ctxt) { \
	R[OFF(z+1,16)] = ctxt; \
	R[OFF(z+1,14)] += S(c, ROTR(ctxt, 8)); \
	R[OFF(z+1,12)] = S(c, R[OFF(z+1,12)]); \
	R[OFF(z+1,1)] = ROTR(R[OFF(z+1,1)], 8); \
}

/* cycle the contents of the shift register the slow way */
static void cycle(sss_ctx *c, WORD ctxt)
{
    register int    i;

    CYCLE(c->R, 0, ctxt);
    for (i = 0; i < N-1; ++i)
	c->R[i] = c->R[i+1];
    c->R[16] = ctxt;
}

/* Return a non-linear function of some parts of the register.
 * The positions of the state bytes form a maximal span full positive
 * difference set, and are 0, 1, 6, 13, 16.
 */
#define NLTAP(c,z) \
    t = c->R[OFF(z,0)] + c->R[OFF(z,16)]; \
    t = S(c,t) + c->R[OFF(z,1)] + c->R[OFF(z,6)] + c->R[OFF(z,13)]; \
    t = ROTR(t, 8); \
    t = S(c,t) ^ c->R[OFF(z,0)]; \

static WORD
nltap(sss_ctx *c)
{
    register WORD	t;

    /*t =*/ NLTAP(c, 0);
    return t;
}

/*
 * Add a MAC-text word to the CRC register.
 * When the transmission is plaintext, the corresponding ciphertext is
 * calculated and fed back into the stream cipher, and also accumulated
 * into the CRC. When the transmission is ciphertext, it is decrypted
 * as usual, and plaintext is accumulated into the CRC. Whatever goes into
 * the CRC is something the adversary didn't intercept.
 */
#define CRCCYCLE(R,z,w) { \
	R[OFF(z,0)] = w ^ (R[OFF(z,0)] << 8) ^ \
	    tab500F[(R[OFF(z,0)] >> 8) & 0xFF] ^ \
	    R[OFF(z,4)] ^ R[OFF(z,15)]; \
}

static void
crccycle(sss_ctx *c, WORD w)
{
    register int	i;

    CRCCYCLE(c->CRC, 0, w);
    w = c->CRC[0];
    for (i = 0; i < N-1; ++i)
	c->CRC[i] = c->CRC[i+1];
    c->CRC[N-1] = w;
}

/*
 * Set up key for SSS.
 * There are at least four different ways to go about this.
 * 1. Save the key for later and calculate Sbox entries on the fly.
 *    (This gives greatest key agility, smallest memory, slow execution.)
 * 2. Fill in a table of all possible Sbox entries at key setup time.
 *    (Less key agility, more memory, fastest streaming.)
 * 3. Fill in table entries when they are first used (lazy evaluation).
 *    (Most memory, good key agility, possible side-channel vulnerability.)
 * 4. Calculate the Sbox at build time, burn into ROM.
 *    (Most efficient, no key agility at all, good for embedded devices.)
 * We implement either 1 or 2, depending on SBOX_PRECOMP.
 */
#if SBOX_PRECOMP
/*
 * Precompute the key-dependent Sbox for later efficiency.
 */
void
sss_key(sss_ctx *c, UCHAR key[], int keylen)
{
    int	    i;

    if (keylen > MAXKEY)
	abort();
    for (i = 0; i < 256; ++i)
	c->SBox[i] = Sfunc(key, keylen, (WORD)(i << (WORDBITS-8)))
	    ^ (i << (WORDBITS-8));

    /* in case no nonce... */
    sss_nonce(c, (UCHAR *)0, 0);
}
#else
/*
 * Just save the key for later Sbox on-the-fly generation
 */
void
sss_key(sss_ctx *c, UCHAR key[], int keylen)
{
    int	    i;

    if ((c->keylen = keylen) > MAXKEY)
	abort();
    for (i = 0; i < keylen; ++i)
	c->key[i] = key[i];

    /* in case no nonce... */
    sss_nonce(c, (UCHAR *)0, 0);
}
#endif /*SBOX_PRECOMP*/

/*
 * Fold in the nonce.
 * Start the two registers as all zeros.
 * (TWEAKABILITY: values other than zero at this point yield distinct, but
 * equally secure, ciphers.)
 * Next treat nonce as ciphertext input (MAC and decrypt), until used up.
 * Then MAC N words of zeros (completely diffuses nonce).
 */
void
sss_nonce(sss_ctx *c, UCHAR nonce[], int nlen)
{
    int			i;
    static UCHAR	nb[WORDBYTES*N];

    if ((nlen % WORDBYTES) != 0)
	abort();
    /* first fill both registers with zeros */
    for (i = 0; i < N; ++i)
	c->R[i] = c->CRC[i] = 0;

    /* now process words of the nonce */
    for (i = 0; i < nlen; i += WORDBYTES) {
	nb[0] = nonce[i];
	nb[1] = nonce[i+1];
	sss_decrypt(c, nb, WORDBYTES);
    }

    /* now MAC N words of zeros */
    nb[0] = nb[1] = 0;
    sss_maconly(c, nb, N*WORDBYTES);

    c->nbuf = 0;
}

/* encryption.
 * (Don't mix with MAC operations)
 */
#define EROUND(z) \
    /* t = */ NLTAP(c,z); \
    t ^= BYTE2WORD(buf+(z*WORDBYTES)); \
    WORD2BYTE(t, buf+(z*WORDBYTES)); \
    CYCLE(c->R,z,t);
void
sss_enconly(sss_ctx *c, UCHAR *buf, int nbytes)
{
    WORD	t = 0;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    *buf ^= c->sbuf & 0xFF;
	    c->sbuf >>= 8;
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* still not a whole word yet */
	    return;
	/* Accrue that ciphertext word */
	cycle(c, c->cbuf);
    }

    /* do lots at a time, if there's enough to do */
    while (nbytes >= N*WORDBYTES)
    {
	EROUND(0);
	EROUND(1);
	EROUND(2);
	EROUND(3);
	EROUND(4);
	EROUND(5);
	EROUND(6);
	EROUND(7);
	EROUND(8);
	EROUND(9);
	EROUND(10);
	EROUND(11);
	EROUND(12);
	EROUND(13);
	EROUND(14);
	EROUND(15);
	EROUND(16);
	buf += WORDBYTES*N;
	nbytes -= N*WORDBYTES;
    }

    /* handle whole words */
    while (nbytes >= WORDBYTES)
    {
	t = nltap(c) ^ BYTE2WORD(buf);
	WORD2BYTE(t, buf);
	cycle(c, t);
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }

    /* handle any trailing bytes */
    if (nbytes != 0) {
	c->sbuf = nltap(c);
	c->cbuf = 0;
	c->nbuf = WORDBITS;
	while (c->nbuf != 0 && nbytes != 0) {
	    *buf ^= c->sbuf & 0xFF;
	    c->sbuf >>= 8;
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* decryption.
 * (Don't mix with MAC operations)
 */
#define DROUND(z) \
    /*t = */ NLTAP(c,z); \
    t2 = BYTE2WORD(buf+(z*WORDBYTES)); \
    CYCLE(c->R,z,t2); \
    t ^= t2; \
    WORD2BYTE(t, buf+(z*WORDBYTES));
void
sss_deconly(sss_ctx *c, UCHAR *buf, int nbytes)
{
    WORD	t = 0, t2 = 0;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* still not a whole word yet */
	    return;
	/* Accrue that ciphertext word */
	cycle(c, c->cbuf);
    }

    /* do lots at a time, if there's enough to do */
    while (nbytes >= N*WORDBYTES)
    {
	DROUND(0);
	DROUND(1);
	DROUND(2);
	DROUND(3);
	DROUND(4);
	DROUND(5);
	DROUND(6);
	DROUND(7);
	DROUND(8);
	DROUND(9);
	DROUND(10);
	DROUND(11);
	DROUND(12);
	DROUND(13);
	DROUND(14);
	DROUND(15);
	DROUND(16);
	buf += WORDBYTES*N;
	nbytes -= N*WORDBYTES;
    }

    /* handle whole words */
    while (nbytes >= WORDBYTES)
    {
	t = nltap(c);
	t2 = BYTE2WORD(buf);
	cycle(c, t2);
	t ^= t2;
	WORD2BYTE(t, buf);
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }

    /* handle any trailing bytes */
    if (nbytes != 0) {
	c->sbuf = nltap(c);
	c->cbuf = 0;
	c->nbuf = WORDBITS;
	while (c->nbuf != 0 && nbytes != 0) {
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* encrypt and MAC.
 * Note that plaintext is accumulated for MAC.
 */
#define EMROUND(z) \
    t2 = BYTE2WORD(buf+(z*WORDBYTES)); \
    CRCCYCLE(c->CRC,z,t2); \
    /* t = */ NLTAP(c,z); \
    t ^= t2; \
    CYCLE(c->R,z,t); \
    WORD2BYTE(t, buf+(z*WORDBYTES));
void
sss_encrypt(sss_ctx *c, UCHAR *buf, int nbytes)
{
    WORD	t, t2;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    c->sbuf >>= 8;
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* still not a whole word yet */
	    return;
	/* Accrue that ciphertext word */
	cycle(c, c->cbuf);
	crccycle(c, c->mbuf);
    }

    /* do lots at a time, if there's enough to do */
    while (nbytes >= N*WORDBYTES)
    {
	EMROUND(0);
	EMROUND(1);
	EMROUND(2);
	EMROUND(3);
	EMROUND(4);
	EMROUND(5);
	EMROUND(6);
	EMROUND(7);
	EMROUND(8);
	EMROUND(9);
	EMROUND(10);
	EMROUND(11);
	EMROUND(12);
	EMROUND(13);
	EMROUND(14);
	EMROUND(15);
	EMROUND(16);
	buf += WORDBYTES*N;
	nbytes -= N*WORDBYTES;
    }

    /* handle whole words */
    while (nbytes >= WORDBYTES)
    {
	t = BYTE2WORD(buf);
	crccycle(c, t);
	t ^= nltap(c);
	WORD2BYTE(t, buf);
	cycle(c, t);
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }

    /* handle any trailing bytes */
    if (nbytes != 0) {
	c->mbuf = 0;
	c->sbuf = nltap(c);
	c->cbuf = 0;
	c->nbuf = WORDBITS;
	while (c->nbuf != 0 && nbytes != 0) {
	    c->mbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	    ++buf;
	}
    }
}

/* decrypt and accumulate MAC.
 * Note that plaintext is accumulated for MAC.
 */
#define DMROUND(z) \
    t2 = BYTE2WORD(buf+(z*WORDBYTES)); \
    /* t = */ NLTAP(c,z); \
    t ^= t2; \
    CYCLE(c->R,z,t2); \
    CRCCYCLE(c->CRC,z,t); \
    WORD2BYTE(t, buf+(z*WORDBYTES));
void
sss_decrypt(sss_ctx *c, UCHAR *buf, int nbytes)
{
    WORD	t = 0, t2 = 0;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->mbuf ^= *buf << (WORDBITS - c->nbuf);
	    ++buf;
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* still not a whole word yet */
	    return;
	/* Accrue that ciphertext word */
	cycle(c, c->cbuf);
	crccycle(c, c->mbuf);
    }

    /* do lots at a time, if there's enough to do */
    while (nbytes >= N*WORDBYTES)
    {
	DMROUND(0);
	DMROUND(1);
	DMROUND(2);
	DMROUND(3);
	DMROUND(4);
	DMROUND(5);
	DMROUND(6);
	DMROUND(7);
	DMROUND(8);
	DMROUND(9);
	DMROUND(10);
	DMROUND(11);
	DMROUND(12);
	DMROUND(13);
	DMROUND(14);
	DMROUND(15);
	DMROUND(16);
	buf += WORDBYTES*N;
	nbytes -= N*WORDBYTES;
    }

    /* handle whole words */
    while (nbytes >= WORDBYTES)
    {
	t = BYTE2WORD(buf);
	t2 = nltap(c);
	cycle(c, t);
	t ^= t2;
	crccycle(c, t);
	WORD2BYTE(t, buf);
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }

    /* handle any trailing bytes */
    if (nbytes != 0) {
	c->sbuf = nltap(c);
	c->cbuf = 0;
	c->mbuf = 0;
	c->nbuf = WORDBITS;
	while (c->nbuf != 0 && nbytes != 0) {
	    c->cbuf ^= *buf << (WORDBITS - c->nbuf);
	    *buf ^= c->sbuf & 0xFF;
	    c->mbuf ^= *buf << (WORDBITS - c->nbuf);
	    c->sbuf >>= 8;
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* Accumulate words into MAC without encryption
 * Note that ciphertext is accumulated for both feedback and MAC.
 */
#define MROUND(z) \
    t2 = BYTE2WORD(buf+(z*WORDBYTES)); \
    /* t = */ NLTAP(c,z); \
    t ^= t2; \
    CYCLE(c->R,z,t); \
    CRCCYCLE(c->CRC,z,t);
void
sss_maconly(sss_ctx *c, UCHAR *buf, int nbytes)
{
    WORD	t, t2;
    UCHAR	ct;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0 && nbytes != 0) {
	    ct = *buf ^ (c->sbuf & 0xFF);
	    c->cbuf ^= ct << (WORDBITS - c->nbuf);
	    c->mbuf ^= ct << (WORDBITS - c->nbuf);
	    ++buf;
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	    --nbytes;
	}
	if (c->nbuf != 0) /* still not a whole word yet */
	    return;
	/* Accrue that ciphertext word */
	cycle(c, c->cbuf);
	crccycle(c, c->mbuf);
    }

    /* do lots at a time, if there's enough to do */
    while (nbytes >= N*WORDBYTES)
    {
	MROUND(0);
	MROUND(1);
	MROUND(2);
	MROUND(3);
	MROUND(4);
	MROUND(5);
	MROUND(6);
	MROUND(7);
	MROUND(8);
	MROUND(9);
	MROUND(10);
	MROUND(11);
	MROUND(12);
	MROUND(13);
	MROUND(14);
	MROUND(15);
	MROUND(16);
	buf += WORDBYTES*N;
	nbytes -= N*WORDBYTES;
    }


    /* handle whole words */
    while (nbytes >= WORDBYTES)
    {
	t = BYTE2WORD(buf) ^ nltap(c);
	cycle(c, t);
	crccycle(c, t);
	/* discard t */
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }

    /* handle any trailing bytes */
    if (nbytes != 0) {
	c->sbuf = nltap(c);
	c->cbuf = 0;
	c->mbuf = 0;
	c->nbuf = WORDBITS;
	while (c->nbuf != 0 && nbytes != 0) {
	    ct = *buf ^ (c->sbuf & 0xFF);
	    c->cbuf ^= ct << (WORDBITS - c->nbuf);
	    c->mbuf ^= ct << (WORDBITS - c->nbuf);
	    c->sbuf >>= 8;
	    ++buf;
	    c->nbuf -= 8;
	    --nbytes;
	}
    }
}

/* Having accumulated a MAC, finish processing and return it.
 * Note that any unprocessed bytes are treated as if
 * they were maconly zero bytes, so ciphertext is accumulated.
 */
void
sss_finish(sss_ctx *c, UCHAR *buf, int nbytes)
{
    int		i;
    WORD	w = c->nbuf;
    UCHAR	ct;

    /* handle any previously buffered bytes */
    if (c->nbuf != 0) {
	while (c->nbuf != 0) {
	    ct = c->sbuf & 0xFF; /* as if encrypted zero byte */
	    c->cbuf ^= ct << (WORDBITS - c->nbuf);
	    c->mbuf ^= ct << (WORDBITS - c->nbuf);
	    c->sbuf >>= 8;
	    c->nbuf -= 8;
	}
	/* Accrue that ciphertext and MAC word */
	cycle(c, c->cbuf);
	crccycle(c, c->mbuf);
    }
    
    /* Perturb only the stream cipher state to mark end of input,
     * by effectively encrypting a word with the previous number of
     * buffered bits in it.
     * Note that only the nonlinear part is updated, not the CRC.
     * Desynchronizing the registers this way is an
     * action that can't be duplicated by passing in plaintext, hence
     * defeating any kind of extension attack.
     */
    w ^= nltap(c);
    cycle(c, w);

    /* Now process the CRC words in reverse order as if they were plaintext */
    for (i = N; --i >= 0; i)
	cycle(c, (WORD)(c->CRC[i] ^ nltap(c)));

    /* To produce the MAC, just encrypt a stream of zeros */
    while (nbytes >= WORDBYTES) {
	w = nltap(c);
	cycle(c, w);
	WORD2BYTE(w, buf);
	buf += WORDBYTES;
	nbytes -= WORDBYTES;
    }
    if (nbytes > 0) {
	/* one last partial word... */
	w = nltap(c);
	while (nbytes > 0) {
	    *buf++ ^= w;
	    w >>= 8;
	    --nbytes;
	}
    }
}
