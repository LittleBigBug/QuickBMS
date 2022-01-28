/* ctw-settings.h : contains struct and functions definitions for keeping settings.
   Also contains definition of boolean, byte and pathname */

/* definitions of max/min allowed values for certain settings */
#define MAX_TREEDEPTH   12
#define MIN_TREEDEPTH   1
#define MAX_MAXNRNODES  16777216
#define MIN_MAXNRNODES  1024
#define MAX_MAXNRTRIES  32
#define MIN_MAXNRTRIES  1
#define MAX_FILEBUFSIZE 16777216
#define MIN_FILEBUFSIZE 512
#define MAX_MAXLOGBETA  16384
#define MIN_MAXLOGBETA  1

#define false 0
#define true  1

typedef unsigned char boolean;
typedef unsigned char byte;
typedef char pathname[FILENAME_MAX];


/* CTWsettings struct: contains all runtime changable settings that are used by the algorithm */
struct CTWsettings
{
  /* settings used in ctwtree: */
  unsigned int treedepth;    /* maximum depth of the trees, excluding the root node */
  unsigned int maxnrnodes;   /* max. number of nodes in the tree array (always a power of 2) */
  unsigned int maxnrtries;   /* how many entries in hashing table are tried to get the right
 			 	node before searching operation fails */
  unsigned int filebufsize;  /* actual file buffer size */
  unsigned int maxfilebufsize; /* maximum file buffer size (is always a power of 2) */
  boolean strictpruning;     /* indicates if the strict pruning method is used or not */

  /* settings used in ctwmath: */
  int maxlogbeta;	     /* maximum value of logbeta (is always a power of 2) */
  boolean rootweighting;     /* if true, weighting is performed at the root node */
  boolean use_zeroredundancy;/* if true, the zero-redundancy estimator is used instead of KT-est */
};

extern struct CTWsettings settings;


/* init_CTWsettings() puts all default values in "settings" */
void init_CTWsettings();

/* check_CTWsettings() checks validity of all settings in "settings". outputs errors tot stderr.
   returns true if all settings are correct, and false otherwise */
boolean check_CTWsettings();

/* print_CTWsettings prints all settings to out */
void print_CTWsettings(FILE *out);
