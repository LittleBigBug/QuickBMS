/* $Id: ssstest.c 388 2005-04-28 21:04:09Z mwp $ */
/*
 * Test harness for SSS
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

#include "sss.h"		/* interface definitions */

sss_ctx ctx;

/* testing and timing harness */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hexlib.h"

/* mostly for debugging, print the LFSR contents. */
int	v = 1; /* disables debug stuff */
void
printLFSR(const char *s, WORD R[])
{
    register int	i;

    if (!v) return;
    printf("%s\n", s);
    for (i = 0; i < N; ++i) {
	printf(" %04lx", (unsigned long)R[i]);
    }
    printf("\n");
}

/* test vectors */
UCHAR	*testkey = (UCHAR *)"test key 128bits";
UCHAR	*testframe = (UCHAR *)"\0\0\0\0";

#define TESTSIZE 20
#define INPUTSIZE 100
#define STREAMTEST 1000000
#define ITERATIONS 999999
char    *testout =
	"f0 94 31 e6 15 99 ec ed 55 e5 02 31 f2 5e 98 b3 f8 49 b7 fd";
char	*streamout =
	"0c ae 2e 83 d2 3d 1a c7 a5 ac 18 b5 05 58 de 32 a8 5f cc ae";
char	*macout =
	"f5 00 74 e9 d7 f9 14 47 28 0e 5d 96 fc a0 d3 68 b4 66 f7 4c";
char	*encdecmac =
	"13 7a 97 1e 09 9c 0e 76 27 1b 31 35 97 26 5a 09 7c 86 2f f7";
char	*zeros = 
	"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00";
char    *iterout =
	"c7 22 3e 7f d4 38 7c 20 ff 71 8f b5 87 30 00 a1 e1 2e e3 e5";
char	*nonceout = 
	"f0 84 95 a4 0c 9a ad 4c 19 7d f8 cc e9 a3 2e 24 b6 fe f9 0c";
char	*hellmac = 
	"0c 88 9d 9d b8 33 d5 9e 96 30 ae 14 6e 46 b0 cc 17 74 17 9f";

UCHAR	testbuf[STREAMTEST + INPUTSIZE];
UCHAR	macbuf[INPUTSIZE];
UCHAR	mac[TESTSIZE];
UCHAR	bigbuf[1024*1024];

void
test_sss(int quick)
{
    int			i;
    unsigned char	ha, he, hm, hc; /* hell test variables */
    extern int		keylen;

    /* basic test */
    memset(testbuf, '\0', sizeof testbuf);
    sss_key(&ctx, testkey, strlen((char *)testkey));
    sss_nonce(&ctx, testframe, 4);
    sss_enconly(&ctx, testbuf, INPUTSIZE);
    hexprint("one chunk", testbuf, TESTSIZE);
    hexcheck(testbuf, testout, TESTSIZE);

    /* now redo that test, byte-at-a-time */
    memset(testbuf, '\0', sizeof testbuf);
    sss_nonce(&ctx, testframe, 4);
    for (i = 0; i < INPUTSIZE; ++i)
	sss_enconly(&ctx, testbuf+i, 1);
    hexprint("single bytes", testbuf, TESTSIZE);
    hexcheck(testbuf, testout, TESTSIZE);

    /* generate and test more of the same stream */
    sss_enconly(&ctx, testbuf + INPUTSIZE, STREAMTEST);
    hexprint("STREAMTEST", testbuf + STREAMTEST, TESTSIZE);
    hexcheck(testbuf + STREAMTEST, streamout, TESTSIZE);

    /* decrypt that buffer */
    sss_nonce(&ctx, testframe, 4);
    sss_deconly(&ctx, testbuf, STREAMTEST + TESTSIZE);
    hexprint("DECRYPT", testbuf, TESTSIZE);
    hexcheck(testbuf, zeros, TESTSIZE);
    hexprint("LONG DECRYPT", testbuf + STREAMTEST, TESTSIZE);
    hexcheck(testbuf + STREAMTEST, zeros, TESTSIZE);

    /* generate and check a MAC of an empty buffer */
    memset(macbuf, '\0', sizeof macbuf);
    sss_nonce(&ctx, testframe, 4);
    sss_maconly(&ctx, macbuf, sizeof macbuf);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("MAC test", mac, sizeof mac);
    hexcheck(mac, macout, sizeof mac);

    /* now redo that test, byte-at-a-time */
    memset(macbuf, '\0', sizeof macbuf);
    sss_nonce(&ctx, testframe, 4);
    for (i = 0; i < sizeof macbuf; ++i)
	sss_maconly(&ctx, macbuf+i, 1);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("MAC bytes", mac, sizeof mac);
    hexcheck(mac, macout, sizeof mac);

    /* encrypt and MAC an empty buffer */
    memset(macbuf, '\0', sizeof macbuf);
    sss_nonce(&ctx, testframe, 4);
    sss_encrypt(&ctx, macbuf, sizeof macbuf);
    hexprint("MAC+enc test", macbuf, TESTSIZE);
    hexcheck(macbuf, testout, TESTSIZE);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("final MAC", mac, sizeof mac);
    hexcheck(mac, encdecmac, sizeof mac);

    /* now decrypt it and verify the MAC */
    sss_nonce(&ctx, testframe, 4);
    sss_decrypt(&ctx, macbuf, sizeof macbuf);
    hexprint("MAC+dec test", macbuf, TESTSIZE);
    hexcheck(macbuf, zeros, TESTSIZE);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("final MAC", mac, sizeof mac);
    hexcheck(mac, encdecmac, sizeof mac);

    /* redo those tests, byte-at-a-time */
    memset(macbuf, '\0', sizeof macbuf);
    sss_nonce(&ctx, testframe, 4);
    for (i = 0; i < sizeof macbuf; ++i)
	sss_encrypt(&ctx, macbuf+i, 1);
    hexprint("M+e bytes", macbuf, TESTSIZE);
    hexcheck(macbuf, testout, TESTSIZE);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("final MAC", mac, sizeof mac);
    hexcheck(mac, encdecmac, sizeof mac);
    sss_nonce(&ctx, testframe, 4);
    for (i = 0; i < sizeof macbuf; ++i)
	sss_decrypt(&ctx, macbuf+i, 1);
    hexprint("M+d bytes", macbuf, TESTSIZE);
    hexcheck(macbuf, zeros, TESTSIZE);
    sss_finish(&ctx, mac, sizeof mac);
    hexprint("final MAC", mac, sizeof mac);
    hexcheck(mac, encdecmac, sizeof mac);

    if (quick)
	return;

    /* test many times iterated */
    for (i = 0; i < ITERATIONS; ++i) {
	if (i % 500 == 0 && v)
	    printf("%7d\r", i), fflush(stdout);
	sss_key(&ctx, testbuf, MAXKEY);
	sss_enconly(&ctx, testbuf, TESTSIZE);
    }
    printf("1000000\n");
    hexprint("iterated", testbuf, TESTSIZE);
    hexcheck(testbuf, iterout, TESTSIZE);

    /* test many times iterated through the nonce */
    sss_key(&ctx, testkey, strlen((char *)testkey));
    sss_nonce(&ctx, NULL, 0);
    memset(testbuf, '\0', sizeof testbuf);
    sss_enconly(&ctx, testbuf, TESTSIZE);
    for (i = 0; i < ITERATIONS; ++i) {
	if (i % 500 == 0 && v)
	    printf("%7d\r", i), fflush(stdout);
	sss_nonce(&ctx, testbuf, 4);
	sss_enconly(&ctx, testbuf, 4);
    }
    printf("1000000\n");
    hexprint("nonce test", testbuf, TESTSIZE);
    hexcheck(testbuf, nonceout, TESTSIZE);

    /* now the test vector from hell --
     * Start with a large, zero'd buffer, and a MAC buffer.
     * Iterate 1000000 times encrypting/decrypting/macing under control
     * of the output.
     */
    sss_key(&ctx, testkey, strlen((char *)testkey));
    memset(testbuf, '\0', sizeof testbuf);
    memset(mac, '\0', TESTSIZE);
    for (i = 0; i < ITERATIONS+1; ++i) {
	if (i % 500 == 0 && v)
	    printf("%6d\r", i), fflush(stdout);
	hm = 5 + mac[0] % (TESTSIZE - 4);
	he = mac[1];
	ha = mac[2];
	hc = mac[3] & 0x03;
	switch (hc) {
	case 0: /* MAC only, then encrypt */
	    if (v > 1) printf("mac %3d, enc %3d: ", ha, he);
	    sss_maconly(&ctx, testbuf, ha);
	    sss_encrypt(&ctx, testbuf+ha, he);
	    break;
	case 1: /* Encrypt then MAC */
	    if (v > 1) printf("enc %3d, mac %3d: ", he, ha);
	    sss_encrypt(&ctx, testbuf, he);
	    sss_maconly(&ctx, testbuf+he, ha);
	    break;
	case 2: /* MAC only, then decrypt */
	    if (v > 1) printf("mac %3d, dec %3d: ", ha, he);
	    sss_maconly(&ctx, testbuf, ha);
	    sss_decrypt(&ctx, testbuf+ha, he);
	    break;
	case 3: /* decrypt then MAC */
	    if (v > 1) printf("dec %3d, mac %3d: ", he, ha);
	    sss_decrypt(&ctx, testbuf, he);
	    sss_maconly(&ctx, testbuf+he, ha);
	    break;
	}
	sss_finish(&ctx, mac, hm);
	if (v > 1) hexprint("MAC", macbuf, hm);
	sss_nonce(&ctx, mac, TESTSIZE);
    }
    printf("1000000\n");
    hexbulk(testbuf, 510);
    hexprint("hell MAC", mac, TESTSIZE);
    hexcheck(mac, hellmac, TESTSIZE);
}

#define BLOCKSIZE	1600	/* for MAC-style tests */
#define MACSIZE		8
/* Perform various timing tests
 */
void
time_sss(void)
{
    long	i;
    clock_t	t;
    WORD	k[4] = { 0, 0, 0, 0 };

    test_sss(1);
    sss_key(&ctx, testkey, strlen((char *)testkey));
    sss_nonce(&ctx, (unsigned char *)"", 0);

    /* test stream encryption speed */
    t = clock();
    for (i = 0; i < 200000000; ) {
	i += sizeof bigbuf;
	sss_enconly(&ctx, bigbuf, sizeof bigbuf);
    }
    t = clock() - t;
    printf("%f Mbyte per second single stream encryption\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0);

    /* test stream decryption speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	i += sizeof bigbuf;
	sss_deconly(&ctx, bigbuf, sizeof bigbuf);
    }
    t = clock() - t;
    printf("%f Mbyte per second single stream decryption\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0);

    /* test packet encryption speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	sss_nonce(&ctx, testframe, 4);
	sss_enconly(&ctx, bigbuf, BLOCKSIZE);
	i += BLOCKSIZE;
    }
    t = clock() - t;
    printf("%f Mbyte per second encrypt %d-byte blocks\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0,
	BLOCKSIZE, MACSIZE*8);

    /* test packet decryption speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	sss_nonce(&ctx, testframe, 4);
	sss_deconly(&ctx, bigbuf, BLOCKSIZE);
	i += BLOCKSIZE;
    }
    t = clock() - t;
    printf("%f Mbyte per second decrypt %d-byte blocks\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0,
	BLOCKSIZE, MACSIZE*8);

    /* test MAC generation speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	sss_nonce(&ctx, testframe, 4);
	sss_maconly(&ctx, bigbuf, BLOCKSIZE);
	sss_finish(&ctx, macbuf, MACSIZE);
	i += BLOCKSIZE;
    }
    t = clock() - t;
    printf("%f Mbyte per second MAC %d-byte blocks %d-bit MAC\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0,
	BLOCKSIZE, MACSIZE*8);

    /* test combined encryption speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	sss_nonce(&ctx, testframe, 4);
	sss_encrypt(&ctx, bigbuf, BLOCKSIZE);
	sss_finish(&ctx, macbuf, MACSIZE);
	i += BLOCKSIZE;
    }
    t = clock() - t;
    printf("%f Mbyte per second MAC and encrypt %d-byte blocks %d-bit MAC\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0,
	BLOCKSIZE, MACSIZE*8);

    /* test combined decryption speed */
    t = clock();
    for (i = 0; i < 20000000; ) {
	sss_nonce(&ctx, testframe, 4);
	sss_decrypt(&ctx, bigbuf, BLOCKSIZE);
	sss_finish(&ctx, macbuf, MACSIZE);
	i += BLOCKSIZE;
    }
    t = clock() - t;
    printf("%f Mbyte per second decrypt and MAC %d-byte blocks %d-bit MAC\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0,
	BLOCKSIZE, MACSIZE*8);

    /* test key setup time */
    t = clock();
    for (i = 0; i < 200000; ++i) {
	k[3] = (WORD)i;
	sss_key(&ctx, (UCHAR *)k, 16);
    }
    t = clock() - t;
    printf("%f million 128-bit keys per second\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0);

    /* test nonce setup time */
    t = clock();
    for (i = 0; i < 200000; ++i) {
	k[3] = (WORD)i;
	sss_nonce(&ctx, (UCHAR *)k, 16);
    }
    t = clock() - t;
    printf("%f million 128-bit nonces per second\n",
	(((double)i/((double)t / (double)CLOCKS_PER_SEC))) / 1000000.0);
}

int
main(int ac, char **av)
{
    int         n, i;
    int		vflag = 0;
    UCHAR	key[32], nonce[32];
    int         keysz, noncesz;
    extern int	keylen;
    extern WORD	K[];

    if (ac >= 2 && strcmp(av[1], "-verbose") == 0) {
	vflag = 1;
	v = vflag;
	++av, --ac;
    }
    if (ac == 2 && strcmp(av[1], "-test") == 0) {
        test_sss(0);
        return nerrors;
    }
    if (ac == 2 && strcmp(av[1], "-time") == 0) {
        time_sss();
        return 0;
    }

    if (ac >= 2)
        hexread(key, av[1], keysz = strlen(av[1]) / 2);
    else
        hexread(key, "0000000000000000", keysz = 8);
    if (ac >= 3)
        hexread(nonce, av[2], noncesz = strlen(av[2]) / 2);
    else
        noncesz = 0;
    sscanf(ac >= 4 ? av[3] : "1000000", "%d", &n);

    if ((keysz | noncesz) & 0x3) {
	fprintf(stderr, "Key and nonce must be multiple of 4 bytes\n");
	return 1;
    }
    sss_key(&ctx, key, keysz);
    sss_nonce(&ctx, nonce, noncesz);
    while (n > 0) {
	i = sizeof bigbuf;
	i = n > i ? i : n;
	memset(bigbuf, '\0', n);
	sss_enconly(&ctx, bigbuf, i);
	hexbulk(bigbuf, i);
	n -= i;
    }
    return 0;
}
