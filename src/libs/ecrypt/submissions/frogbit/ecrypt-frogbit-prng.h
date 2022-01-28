/* ecrypt-frogbit-prng.h */

#if !defined(MTGFSR_F)

/*
 *  Make up for missing definitions if this file is compiled
 *  outside of the ECRYPT NoE Frogbit submission.
 */

#define FROGBIT10 (10)

/* Some MTGFSR parameters */

#define MTGFSR_F (2)
#define MTGFSR_siz (4)

typedef unsigned long u32;
typedef unsigned char u8;

typedef struct
{
 int ind;
 u32 bits[MTGFSR_siz];
 u32 table[1<<MTGFSR_F]; /* actually constant data is placed here */
} ECRYPT_AE_frogbit_prng;

extern void ECRYPT_init(void);
#endif

/* Some more MTGFSR parameters */

#define MTGFSR_N (3)
#define MTGFSR_T (22)
#define MTGFSR_t (2)

#define MTGFSR_N11 (49)
#define MTGFSR_N13 (176)

/**************** Start of parameters that can be modified ****************/

#define MTGFSR_M (1) /* Either 1 or 2 */

/**************** End of parameters that can be modified ****************/

/*
 * These parts of the key schedule:
 * "Nibble Distribution to First State Word in Each PRNG"
 * "Other State Words in Each PRNG"
 */
extern int ECRYPT_frogbit_key_shed(
  ECRYPT_AE_frogbit_prng *prng,
  int ivsize,
  const u8* key);

/*
 * The draw function for the Frogbit MTGFSR is implemented as a macro,
 * software purists would prefer an inline function.
 *
 * Parameter 1: a pointer to a ECRYPT_AE_frogbit_prng structure.
 * Parameter 2: a variable name that receives the PRNG output (least
 *              significant two bits).
 */
#define MTGFSR_DRAW(ctx,t) { \
  u32 temp; ECRYPT_AE_frogbit_prng *pt=(ctx); \
  pt->ind++; \
  temp=pt->bits[(pt->ind-MTGFSR_N)&(MTGFSR_siz-1)]; \
  pt->bits[pt->ind&(MTGFSR_siz-1)] \
      =(t) \
      = pt->bits[(pt->ind-MTGFSR_N+MTGFSR_M)&(MTGFSR_siz-1)] \
       ^(temp>>MTGFSR_F) \
       ^pt->table[temp&((1<<MTGFSR_F)-1)]; \
  /* (t)&=(1<<MTGFSR_t)-1; */ \
}

