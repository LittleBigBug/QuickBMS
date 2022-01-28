/* ecrypt-zk-crypt.h */

/* 
 * Header file for the ZK-Crypt stream cipher
 *
 * All rights reserved to FortressGB Ltd.
 *
 * Author: Vladimir Rivkin
 */

/* ------------------------------------------------------------------------------ */
/* ZK-Crypt internal definitions                                                  */
/* ------------------------------------------------------------------------------ */

#define MODE_FB_ENABLE      (1<<0)  /* feedback enable */
#define MODE_FB_A           (1<<1)  /* feedback mode A */
#define MODE_FB_B           (1<<2)  /* feedback mode B */

#define MODE_BRAWNIAN       (1<<3)  /* brawnian enable */

#define ENABLE_SINGLE_TIER  (1<<4)  /* enable single tier select by decoder 10S */
#define TOP_TIER_ALWAYS     (1<<5)  /* force top tier for debugging purpose 10S */
#define MID_TIER_ALWAYS     (1<<6)  /* force mid tier for debugging purpose 10S */
#define BOT_TIER_ALWAYS     (1<<7)  /* force bot tier for debugging purpose 10S */

#define ENABLE_ODDNS        (1<<8)  /* enable ODDN permutations 10S */

#define SAMPLE              (1<<9)  /* sample immunizer & feedback */

/* ------------------------------------------------------------------------------ */

#define DEF_LSFR_13     0
#define DEF_LSFR_14     1
#define DEF_LSFR_15     2
#define DEF_LSFR_17     3
#define DEF_LSFR_18     4
#define DEF_LSFR_19     5

#define SLIP_LEFT   (1<<0)  
#define SLIP_RIGHT  (1<<1)
#define CLOCK       (1<<2)	/* enable nLSFR iteration */
#define LOAD        (1<<4)	/* enable loading cipher word */
#define BROWN       (1<<5)	/* enable tier reversed pseudo brownian motion bits */

#define TOP_ODDN    (1<<0)
#define MID_ODDN    (1<<1)
#define BOT_ODDN    (1<<2)
#define ODDN4       (1<<3)

#define TOP_ODDN_MASK    ((1<<2)+(1<<5)+(1<<8)+(1<<11)+(1<<14)+(1<<17)+(1<<20)+(1<<23)+(1<<26))
#define MID_ODDN_MASK    ((1<<0)+(1<<1)+(1<<3)+(1<<6)+(1<<9)+(1<<12)+(1<<15)+(1<<18)+(1<<21)+(1<<24)+(1<<27)+(1<<29)+(1<<30))
#define BOT_ODDN_MASK    ((1<<7)+(1<<10)+(1<<13)+(1<<16)+(1<<19)+(1<<22)+(1<<25)+(1<<28)+(1<<31))
#define ODDN4_MASK       (1<<4)

#define VECTOR_A    (1<<0)
#define VECTOR_B    (1<<1)
#define VECTOR_C    (1<<2)
#define VECTOR_D    (1<<3)

#define RNG_OPERATION           0
#define STREAM_CIPHER_OPERATION 1
#define HASH_OPERATION          2

#define MAC_SIGNATURE_LEN_BITS      (160)
#define MAC_SIGNATURE_LEN_BYTES     (MAC_SIGNATURE_LEN_BITS>>3)
#define MAC_SIGNATURE_LEN_32B_WORDS (MAC_SIGNATURE_LEN_BYTES>>2) 


/* ------------------------------------------------------------------------------ */
/* ZK-Crypt external functions                                                    */
/* ------------------------------------------------------------------------------ */
/*
Note:
        ZK-Crypt is ECRYPT API compatible, so in order to find information about 
        the standard ECRYPT API functions please see file "ecrypt-sync.h".
        Here we give our proprietary API for the ZK-Crypt Hash Function.
        Usage:
        1. Allocate 20 bytes for a digest.
        2. Call the function.
        3, The output digest will be at the  digest buffer.
Assumption:
        1.This is a stand alone function. It does not requires key and iv setups.
          It sets the key and iv equal to zero.
        2.THE MESSAGE LENGTH IN BYTES MUST BE DIVISIBLE BY FOUR. 
Reference:
        For further explanations see "Using ZK-Crypt - Protocols and Strategy" by 
        Carmi Gressel, Ran Granot, Gabi Vago.
*/

void ECRYPT_create_digest(
  ECRYPT_ctx* ctx,            /* inner parameters structure                         */
  const u8* msg,              /* pointer to the hashed message                      */
  u8* digest,                 /* pointer to the preallocated 160 bit digest         */
  u32* header_words,          /* the header 32-bit word x_hdr                       */
  u32 tail_word,              /* the tail 32-bit word x_t                           */
  u32 msglen );               /* message length in bytes.MUST BE DIVISIBLE BY FOUR! */ 



/* ------------------------------------------------------------------------------ */
/* ZK-Crypt internal functions                                                    */
/* ------------------------------------------------------------------------------ */

void	ZK_CRYPT_StartAlgorithm (ECRYPT_ctx* ctx);

u32     ZK_CRYPT_NextRandom     (ECRYPT_ctx* ctx);

void	nLSFR_iteration (u32 cipher_word, u32 feedback_word, int enable_cipher,
                         int slip, int index,
                         ECRYPT_ctx* ctx);

u32	    tier_shift      (u32 input);

void	top_tier        (ECRYPT_ctx* ctx);
void	mid_tier        (ECRYPT_ctx* ctx);
void	bot_tier        (ECRYPT_ctx* ctx);

u32	    majority_2_3    (u32 a, u32 b, u32 c);

void    nLSFR_bank      (ECRYPT_ctx* ctx);

void    hash_mix        (const u8 *table,
                         ECRYPT_ctx* ctx);
void    Hash_matrix     (ECRYPT_ctx* ctx);

u32	    ROL3_func       (u32 input);
u32	    ROR1_func       (u32 input);

void    Immunizer       (ECRYPT_ctx* ctx);

void    cypher_machine  (ECRYPT_ctx* ctx);

void	top_brown       (ECRYPT_ctx* ctx);
void	top_slip        (ECRYPT_ctx* ctx);
void	top_controller  (ECRYPT_ctx* ctx);

void	mid_brown       (ECRYPT_ctx* ctx);
void	mid_slip        (ECRYPT_ctx* ctx);
void	mid_controller  (ECRYPT_ctx* ctx);

void	bot_brown       (ECRYPT_ctx* ctx);
void	bot_slip        (ECRYPT_ctx* ctx);
void	bot_controller  (ECRYPT_ctx* ctx);

int     p_random_clock_generator(ECRYPT_ctx* ctx); 

void    johnson_counter (ECRYPT_ctx* ctx);

void	tier_controller (ECRYPT_ctx* ctx);

void    system_reset    (ECRYPT_ctx* ctx);

void    init_preload    (ECRYPT_ctx* ctx); 
  
void    internal_reset  (ECRYPT_ctx* ctx);

/* ------------------------------------------------------------------------------ */
