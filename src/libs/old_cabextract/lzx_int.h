
/* generic stuff */
//#define CAB(x) (decomp_state.x)
//#define ZIP(x) (decomp_state.methods.zip.x)
//#define QTM(x) (decomp_state.methods.qtm.x)
#define LZX(x) (decomp_state.methods.lzx.x)
#define DECR_OK           (0)
#define DECR_DATAFORMAT   (1)
#define DECR_ILLEGALDATA  (2)
#define DECR_NOMEMORY     (3)
#define DECR_CHECKSUM     (4)
#define DECR_INPUT        (5)
#define DECR_OUTPUT       (6)

/* CAB data blocks are <= 32768 bytes in uncompressed form. Uncompressed
 * blocks have zero growth. MSZIP guarantees that it won't grow above
 * uncompressed size by more than 12 bytes. LZX guarantees it won't grow
 * more than 6144 bytes.
 */
#define CAB_BLOCKMAX (32768)
#define CAB_INPUTMAX (CAB_BLOCKMAX+6144)

/* LZX stuff */

/* some constants defined by the LZX specification */
#define LZX_MIN_MATCH                (2)
#define LZX_MAX_MATCH                (257)
#define LZX_NUM_CHARS                (256)
#define LZX_BLOCKTYPE_INVALID        (0)   /* also blocktypes 4-7 invalid */
#define LZX_BLOCKTYPE_VERBATIM       (1)
#define LZX_BLOCKTYPE_ALIGNED        (2)
#define LZX_BLOCKTYPE_UNCOMPRESSED   (3)
#define LZX_PRETREE_NUM_ELEMENTS     (20)
#define LZX_ALIGNED_NUM_ELEMENTS     (8)   /* aligned offset tree #elements */
#define LZX_NUM_PRIMARY_LENGTHS      (7)   /* this one missing from spec! */
#define LZX_NUM_SECONDARY_LENGTHS    (249) /* length tree #elements */

/* LZX huffman defines: tweak tablebits as desired */
#define LZX_PRETREE_MAXSYMBOLS  (LZX_PRETREE_NUM_ELEMENTS)
#define LZX_PRETREE_TABLEBITS   (6)
#define LZX_MAINTREE_MAXSYMBOLS (LZX_NUM_CHARS + 50*8)
#define LZX_MAINTREE_TABLEBITS  (12)
#define LZX_LENGTH_MAXSYMBOLS   (LZX_NUM_SECONDARY_LENGTHS+1)
#define LZX_LENGTH_TABLEBITS    (12)
#define LZX_ALIGNED_MAXSYMBOLS  (LZX_ALIGNED_NUM_ELEMENTS)
#define LZX_ALIGNED_TABLEBITS   (7)

#define LZX_LENTABLE_SAFETY (64) /* we allow length table decoding overruns */

#define LZX_DECLARE_TABLE(tbl) \
  UWORD tbl##_table[(1<<LZX_##tbl##_TABLEBITS) + (LZX_##tbl##_MAXSYMBOLS<<1)];\
  UBYTE tbl##_len  [LZX_##tbl##_MAXSYMBOLS + LZX_LENTABLE_SAFETY]

struct LZXstate {
    UBYTE *window;         /* the actual decoding window              */
    ULONG window_size;     /* window size (32Kb through 2Mb)          */
    ULONG actual_size;     /* window size when it was first allocated */
    ULONG window_posn;     /* current offset within the window        */
    ULONG R0, R1, R2;      /* for the LRU offset system               */
    UWORD main_elements;   /* number of main tree elements            */
    int   header_read;     /* have we started decoding at all yet?    */
    UWORD block_type;      /* type of this block                      */
    ULONG block_length;    /* uncompressed length of this block       */
    ULONG block_remaining; /* uncompressed bytes still left to decode */
    ULONG frames_read;     /* the number of CFDATA blocks processed   */
    LONG  intel_filesize;  /* magic header value used for transform   */
    LONG  intel_curpos;    /* current offset in transform space       */
    int   intel_started;   /* have we seen any translatable data yet? */

    LZX_DECLARE_TABLE(PRETREE);
    LZX_DECLARE_TABLE(MAINTREE);
    LZX_DECLARE_TABLE(LENGTH);
    LZX_DECLARE_TABLE(ALIGNED);
};


struct {
  struct folder *current; /* current folder we're extracting from  */
  ULONG offset;           /* uncompressed offset within folder     */
  UBYTE *outpos;          /* (high level) start of data to use up  */
  UWORD outlen;           /* (high level) amount of data to use up */
  UWORD split;            /* at which split in current folder?     */
  int (*decompress)(int, int); /* the chosen compression func      */
  UBYTE inbuf[CAB_INPUTMAX+2]; /* +2 for lzx bitbuffer overflows!  */
  UBYTE outbuf[CAB_BLOCKMAX];
  union {
    //    struct ZIPstate zip;
    //    struct QTMstate qtm;
    struct LZXstate lzx;
  } methods;
} decomp_state;

