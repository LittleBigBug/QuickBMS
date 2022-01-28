/* ctwmath: contains the data constants, type definitions and functions used for the mathematics
   in the CTW algorithm. These functions are used by the encoder and decoder in CTWENCDEC. */

#define MAXCOUNTS 256	/* MAXCOUNTS = 2^M		  max. value of counters in nodes + 1, M=8 */
#define LOGENTRIES 512	/* LOGENTRIES = 2*MAXCOUNTS	  number of entries in log-table */
#define ACCURACY 128	/* ACCURACY   = 2^(M-1)		  accuracy of table elements */
#define JACENTRIES 1152	/* JACENTRIES = ACCURACY*(M+1)	  number of entries in Jacobian table */

/* A CTWRecord contains the information for the tree nodes. The first field contains the counts
   for the number of zeros (cnt[0]) and ones (cnt[1]), which are both stored as an 8 bit value
   (so max 255). As soon as one of these counts reaches 2^M (256) both are divided by two (leads
   to redundancy increase). The second field contains the logarithm of  beta, a 16 bit value
   (short int). Beta is some kind of switch that controls the mixture between the incoming
   (external) quotient of weighted probabilies and the (internal) quotient of estimated 
   probabilities. */
struct CTWRecord {
  unsigned char cnt[2];
  short int     logbeta;
};

/* A CTWProb contains information on the probablity of the most probable symbol. The first field
   (logpwroot) contains the logarithm of the weighted probability on the most probable symbol in
   the root of the CTW tree, which will be used for the arithmetic coder. The second field
   (predsymb) contains the most probable symbol (0 or 1). */
struct CTWProb {
  int      logpwroot;
  boolean  predsymb;
};

extern struct CTWRecord CTW_DATA_ZERO;	/* contains all zeros, initialized in ctwmath.c */


/* function prototypes ====================================================================== */

/* CTW_data_from_one_count returns a CTWRecord with cnt[bit]=1, cnt[1-bit]=0 and logbeta=0. */
struct CTWRecord CTW_data_from_one_count(unsigned int bit);

/* CTWProcess calculates the logarithm of the weighted probability on the most probable symbol
   in the root of the CTW tree, which will be used for the arithmetic coder, and returns this
   probability and the most probable symbol in ctwprob. It also does a dummy-0 and a dummy-1
   update on the CTW tree, which is returned in dummy0info[] and dummy1info[]. During these
   dummy updates the new values for the counts of zeros and ones as well as the new value for
   logarithm of beta are calculated for each node along the current path (identified by curdepth
   and ctwinfo[]. */
void CTWProcess(int curdepth, struct CTWRecord ctwinfo[],
                struct CTWProb *ctwprob,
                struct CTWRecord dummy0info[],
                struct CTWRecord dummy1info[]);
