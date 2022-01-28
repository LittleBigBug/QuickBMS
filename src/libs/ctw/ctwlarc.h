/* CTWLARC contains the arithmetic encoder and decoder. These are a normal arithmetic encoder
   and decoder, with the difference that they accept block probablities instead of conditional
   probabilities, which has the advantage that the arithmetic encoder and decoder do not have to
   multiply to subdivide the source interval in a 0-interval and a 1-interval. This arithmetic
   encoder and decoder are NOT part of the CTW algorithm, but for those interested more
   information on this arithmetic encoder and decoder can be found in [EIDMA ch. 6]. */

#define PPBUFSIZ 2048	/* Size of the push and pull buffer */
#define ARENTRIES 4096	/* ARENTRIES = 2^K         number of entries in log and exp tables */
#define ACCUMSIZE 13	/* ACCUMSIZE = K + 1       number of bits in accumulator */
#define ACCUMMASK 8191	/* ACCUMMASK = 2^(K+1) - 1 mask to get value of accumulator */
#define DELAYREGSIZE 24	/* DELAYREGSIZE = 24       number of bits in delay register */
#define DELAYREGMASK 0x00FFFFFF	/* DELAYREGMASK = 2^24 - 1 */

/* function prototypes ====================================================================== */
/* arencinit: initialize arithmetic encoder. Encoder will write to CodeFile */
void    arencinit(unsigned char *CodeFile, unsigned int CodeSize);

/* arenc: encodes bit symbsmall. instep is a measure of the logarithm of the probability of 
   the most probable symbol */
void    arenc(int instep, boolean symbsmall);

/* arencexit: terminates the encoding process. Bits in the accumulator and delay register are
   stored, and the size of the encoded file in bits is returned */
void    arencexit(unsigned int *codelength);

/* ardecinit: initialize arithmetic decoder. Decoder will read from CodeFile */
void    ardecinit(unsigned char *CodeFile, unsigned int CodeSize);

/* ardec: decodes one bit, using instep as measure of the logarithm of the probability of the
   most probable symbol */
boolean ardec(int instep);

/* ardecexit: terminates the decoding process. Size of the compressed file in bits is returned */
void    ardecexit(unsigned int *codelength);
