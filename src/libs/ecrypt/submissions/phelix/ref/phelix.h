#ifndef _PHELIX_H_
#define _PHELIX_H_
/*
** Phelix header file
**
** Public domain code.  Author:  Doug Whiting, Hifn, 2005
*/

enum
    {
    PHELIX_NONCE_SIZE   =   128,    /* size of nonce        in bits */
    PHELIX_MAC_SIZE     =   128,    /* size of full mac tag in bits */
    PHELIX_KEY_SIZE     =   256     /* size of full key     in bits */
    };

/* parameter list for the all-in-one call */
#define  U08P  u08b *                    /* shortcut name */
#define PhelixPacketParms                                   \
            PhelixContext *ctx,                             \
            const U08P nonce,                               \
            const U08P aad,           u32b aadLen,          \
            const U08P src, U08P dst, u32b msgLen, U08P mac


#ifdef ECRYPT_API                   /* are we compiling for ECRYPT? */
        /********* remap Phelix names to ECRYPT names ***************/
#define u32b u32                    /* Phelix uses u32b, u08b       */
#define u08b u8

#define PHELIX_CIPHER_NAME             "ECRYPT-Phelix"

#define PHELIX_MAXKEYSIZE               ECRYPT_MAXKEYSIZE
#define PHELIX_KEYSIZE                  ECRYPT_KEYSIZE
#define PHELIX_MAXNONCESIZE             ECRYPT_MAXIVSIZE
#define PHELIX_NONCESIZE                ECRYPT_IVSIZE
#define PHELIX_MAXMACSIZE               ECRYPT_MAXMACSIZE
#define PHELIX_MACSIZE                  ECRYPT_MACSIZE

#define PhelixContext                   ECRYPT_AE_ctx

#define PhelixInit                      ECRYPT_init
#define PhelixSetupKey                  ECRYPT_AE_keysetup
#define PhelixSetupNonce                ECRYPT_AE_ivsetup
#define PhelixProcessAAD                ECRYPT_AE_authenticate_bytes
#define PhelixEncryptBytes              ECRYPT_AE_encrypt_bytes
#define PhelixDecryptBytes              ECRYPT_AE_decrypt_bytes
#define PhelixFinalize                  ECRYPT_AE_finalize

#define PhelixEncryptPacket             ECRYPT_AE_encrypt_packet
#define PhelixDecryptPacket             ECRYPT_AE_decrypt_packet
#define PhelixProcessPacket_CodeSize    ECRYPT_AE_process_packet_CodeSize

#else   /********* straight Phelix (not ECRYPT) *************/
#include "platform.h"

#define PHELIX_CIPHER_NAME "Phelix"

#define PHELIX_MAXKEYSIZE   PHELIX_KEY_SIZE
#define PHELIX_KEYSIZE(i)   (8*(i))             /* allow all byte key sizes */

#define PHELIX_MAXNONCESIZE PHELIX_NONCE_SIZE
#define PHELIX_NONCESIZE(i) PHELIX_NONCE_SIZE   /* just one nonce size allowed */

#define PHELIX_MAXMACSIZE   PHELIX_MAC_SIZE
#define PHELIX_MACSIZE(i)   ((i)+1)             /* allow all nonzero tag sizes */

/******** the common definitions and prototypes ********/

typedef struct          /* keep all cipher state                    */
    {       
    struct  /* key schedule (doesn't change after SetupNonce)       */
        {
        u32b    keySize;    /* initial key size, in bits: 0..256    */
        u32b    macSize;    /* mac tag     size, in bits: 0..128    */
        u32b    X_1_bump;   /* 4*(keySize/8)+256*(macSize mod 128)  */
        u32b    X_0[8];     /* processed working key material       */
        u32b    X_1[8];
        } ks;   /* ks --> "key schedule" */

    struct  /* internal cipher state (varies during block) */
        {
        u32b    oldZ[4];    /* previous four Z_4 values for output  */
        u32b    Z[5];       /* 5 internal state words (160 bits)    */
        u32b    i;          /* block number                         */
        u32b    aadLen[2];  /* 64-bit aadLen counter (LSW first)    */
        u32b    msgLen;     /* low 32 bits of msgLen                */
        u32b    aadXor;     /* aadXor constant                      */
        } cs;   /* cs --> "cipher state" */
    } PhelixContext;

#if 0
void PhelixProcessPacket(int action,PhelixPacketParms); /* "all-in-one" call */
#else
void PhelixEncryptPacket(PhelixPacketParms); 
void PhelixDecryptPacket(PhelixPacketParms); 
#endif

/* Phelix function prototypes, given definition of PhelixContext    */
void PhelixInit(void);
void PhelixSetupKey    (PhelixContext *ctx,const U08P keyPtr,u32b keySize,u32b ivSize,u32b macSize);
void PhelixSetupNonce  (PhelixContext *ctx,const U08P noncePtr);
void PhelixProcessAAD  (PhelixContext *ctx,const U08P aad,u32b aadLen);
void PhelixEncryptBytes(PhelixContext *ctx,const U08P plaintext ,U08P ciphertext,u32b msgLen);
void PhelixDecryptBytes(PhelixContext *ctx,const U08P ciphertext,U08P plaintext ,u32b msgLen);
void PhelixFinalize    (PhelixContext *ctx,U08P mac);

/*
* Note: The ECRYPT_API outputs the truncated MAC and does no comparison.
*
*       The Phelix API outputs the full MAC in the case of encryption,
*       and for decryption treats the input MAC as "const" and does a
*       compare and returns a value. Note that the ASM implementation
*       follows only the Phelix API w.r.t. the MAC.
*/
#endif

u32b PhelixProcessPacket_CodeSize(void);   /* size of all-in-1    code, in bytes */
u32b PhelixIncremental_CodeSize(void);     /* size of incremental code, in bytes */

const char *PhelixCompiler_Name(void);     /* name of compiler/assembler used for Phelix */

#endif /* ifndef _PHELIX_H_ */
