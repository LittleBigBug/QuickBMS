//#include "lucifer.h"

/* LUCIFER is a cryptographic algorithm developed by IBM in the early
 *	seventies.  It was a predecessor of the DES, and is much simpler
 *	than that algorithm.  In particular, it has only two substitution
 *	boxes.  It does, however, use a 128 bit key and operates on
 *	sixteen unsigned char data blocks...
 *
 *	This implementation of LUCIFER was crafted by Graven Cyphers at the
 *	University of Toronto, Canada, with programming assistance from
 *	Richard Outerbridge.  It is based on the FORTRAN routines which
 *	concluded Arthur Sorkin's article "LUCIFER: A Cryptographic Algorithm",
 *	CRYPTOLOGIA, Volume 8, Number 1, January 1984, pp22-42.  The interested
 *	reader should refer to that article rather than this program for more
 *	details on LUCIFER.
 *
 *	These routines bear little resemblance to the actual LUCIFER algorithm,
 *	which has been severely twisted in the interests of speed.  They do
 *	perform the same transformations, and are believed to be UNIX portable.
 *	The package was developed for use on UNIX-like systems lacking crypto
 *	facilities.  They are not very fast, but the cipher is very strong.
 *	The routines in this file are suitable for use as a subroutine library
 *	after the fashion of crypt(3).  When linked together with applications
 *	routines they can also provide a high-level cryptographic system.
 *
 *	-DENHANCE : modify LUCIFER by changing the key schedule and performing
 *		an "autokeyed" encryption.  These may improve the algorithm.
 */

#ifndef DE
#define DE	1	/* for separate compilation	*/
#endif

static unsigned char Dps[64] = { /* Diffusion Pattern schedule	*/
	4,16,32,2,1,8,64,128,	128,4,16,32,2,1,8,64,
	64,128,4,16,32,2,1,8,	8,64,128,4,16,32,2,1,
	1,8,64,128,4,16,32,2,	2,1,8,64,128,4,16,32,
	32,2,1,8,64,128,4,16,	16,32,2,1,8,64,128,4	};

/* Precomputed S&P Boxes, Two Varieties */
static unsigned char TCB0[256] = {
	 87, 21,117, 54, 23, 55, 20, 84,116,118, 22, 53, 85,119, 52, 86,
	223,157,253,190,159,191,156,220,252,254,158,189,221,255,188,222,
	207,141,237,174,143,175,140,204,236,238,142,173,205,239,172,206,
	211,145,241,178,147,179,144,208,240,242,146,177,209,243,176,210,
	215,149,245,182,151,183,148,212,244,246,150,181,213,247,180,214,
	 95, 29,125, 62, 31, 63, 28, 92,124,126, 30, 61, 93,127, 60, 94,
	219,153,249,186,155,187,152,216,248,250,154,185,217,251,184,218,
	 67,  1, 97, 34,  3, 35,  0, 64, 96, 98,  2, 33, 65, 99, 32, 66,
	195,129,225,162,131,163,128,192,224,226,130,161,193,227,160,194,
	199,133,229,166,135,167,132,196,228,230,134,165,197,231,164,198,
	203,137,233,170,139,171,136,200,232,234,138,169,201,235,168,202,
	 75,  9,105, 42, 11, 43,  8, 72,104,106, 10, 41, 73,107, 40, 74,
	 91, 25,121, 58, 27, 59, 24, 88,120,122, 26, 57, 89,123, 56, 90,
	 71,  5,101, 38,  7, 39,  4, 68,100,102,  6, 37, 69,103, 36, 70,
	 79, 13,109, 46, 15, 47, 12, 76,108,110, 14, 45, 77,111, 44, 78,
	 83, 17,113, 50, 19, 51, 16, 80,112,114, 18, 49, 81,115, 48, 82 };

static unsigned char TCB1[256] = {
	 87,223,207,211,215, 95,219, 67,195,199,203, 75, 91, 71, 79, 83,
	 21,157,141,145,149, 29,153,  1,129,133,137,  9, 25,  5, 13, 17,
	117,253,237,241,245,125,249, 97,225,229,233,105,121,101,109,113,
	 54,190,174,178,182, 62,186, 34,162,166,170, 42, 58, 38, 46, 50,
	 23,159,143,147,151, 31,155,  3,131,135,139, 11, 27,  7, 15, 19,
	 55,191,175,179,183, 63,187, 35,163,167,171, 43, 59, 39, 47, 51,
	 20,156,140,144,148, 28,152,  0,128,132,136,  8, 24,  4, 12, 16,
	 84,220,204,208,212, 92,216, 64,192,196,200, 72, 88, 68, 76, 80,
	116,252,236,240,244,124,248, 96,224,228,232,104,120,100,108,112,
	118,254,238,242,246,126,250, 98,226,230,234,106,122,102,110,114,
	 22,158,142,146,150, 30,154,  2,130,134,138, 10, 26,  6, 14, 18,
	 53,189,173,177,181, 61,185, 33,161,165,169, 41, 57, 37, 45, 49,
	 85,221,205,209,213, 93,217, 65,193,197,201, 73, 89, 69, 77, 81,
	119,255,239,243,247,127,251, 99,227,231,235,107,123,103,111,115,
	 52,188,172,176,180, 60,184, 32,160,164,168, 40, 56, 36, 44, 48,
	 86,222,206,210,214, 94,218, 66,194,198,202, 74, 90, 70, 78, 82 };

static unsigned char Key[16], Pkey[128];
static int P[8] = { 3,5,0,4,2,1,7,6 };
static int Smask[16] = { 128,64,32,16,8,4,2,1 };

void lucifer(bytes)
unsigned char *bytes;	/* points to a 16-byte array	*/
{
	register unsigned char *cp, *sp, *dp;
	register int val, *sbs, tcb, j, i;
	unsigned char *h0, *h1, *kc, *ks;

	h0 = bytes;	/* the "lower" half	*/
	h1 = &bytes[8]; /* the "upper" half	*/
	kc = Pkey;
	ks = Key;

	for( i = 0; i < 16; i++ ) {
		tcb = *ks++;
		sbs = Smask;
		dp = Dps;
		sp = &h0[8];
#ifdef ENHANCE
		for( j = 0, cp = h1; j < 8; j++ ) tcb ^= *cp++;
#endif
		for( j = 0; j < 8; j++ ) {
			if( tcb & *sbs++ ) val = TCB1[h1[j] & 0377];
			else val = TCB0[h1[j] & 0377];
			val ^= *kc++;
			for( cp = h0; cp < sp; ) *cp++ ^= (val & *dp++);
			}

		/* swap (virtual) halves	*/
		cp = h0;
		h0 = h1;
		h1 = cp;
		}

	/* REALLY swap halves	*/
	dp = bytes;
	cp = &bytes[8];
	for( sp = cp; dp < sp; dp++, cp++ ) {
		val = *dp;
		*dp = *cp;
		*cp = val;
		}
	return;
	} 

void lucifer_loadkey(keystr, edf)	/* precomputes the key schedules	*/
unsigned char *keystr;
register int edf;
{
	register unsigned char *ep, *cp, *pp;
	register int kc, i, j;
	unsigned char kk[16], pk[16];
	cp = kk;
	pp = pk;
	ep = &kk[16];
	while( cp < ep ) {
		*cp++ = *keystr;
		for( *pp = i = 0; i < 8; i++ )
			if( *keystr & Smask[i] ) *pp |= Smask[P[i]];
		keystr++;
		pp++;
		}
	cp = Key;
	pp = Pkey;
	kc = (edf == DE) ? 8 : 0;
	for( i = 0; i < 16; i++ ) {
		if( edf == DE ) kc = (kc + 1) & 017;
#ifdef ENHANCE
		*cp++ = kk[( (kc == 0) ? 15 : (kc - 1) )];
#else
		*cp++ = kk[kc];
#endif 
		for( j = 0; j < 8; j++ ) {
			*pp++ = pk[kc];
			if( j < 7 || (edf == DE) ) kc = (kc + 1)&017;
			}
		}
	return;
	}

/* lucifer cks # < /dev/null 
 *	: 16 bytes	: 32186510 6acf6094 87953eba 196f5a75 :
 *	(-DENHANCE)	: 378cfd5b bd54a07b 28513809 624e6071 :
 *			(rwo/8412.03.18:10/V5.0)		*/
/************************ lucifer *******************************/
