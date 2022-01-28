#ifndef __RIJNDAEL_H
#define __RIJNDAEL_H

#include "kirk_engine.h"

#define AES_KEY_LEN_128	(128)
#define AES_KEY_LEN_192	(192)
#define AES_KEY_LEN_256	(256)

#define AES_BUFFER_SIZE (16)

#define AES_MAXKEYBITS	(256)
#define AES_MAXKEYBYTES	(AES_MAXKEYBITS/8)
/* for 256-bit keys, fewer for less */
#define AES_MAXROUNDS	14
#define pwuAESContextBuffer rijndael_ctx

/*  The structure for key information */
typedef struct 
{
	int	enc_only;		/* context contains only encrypt schedule */
	int	Nr;			/* key-length-dependent number of rounds */
	u32	ek[4*(AES_MAXROUNDS + 1)];	/* encrypt key schedule */
	u32	dk[4*(AES_MAXROUNDS + 1)];	/* decrypt key schedule */
} rijndael_ctx;

typedef struct 
{
	int	enc_only;		/* context contains only encrypt schedule */
	int	Nr;			/* key-length-dependent number of rounds */
	u32	ek[4*(AES_MAXROUNDS + 1)];	/* encrypt key schedule */
	u32	dk[4*(AES_MAXROUNDS + 1)];	/* decrypt key schedule */
} AES_ctx;

int libkirk_rijndael_set_key(rijndael_ctx *, const u8 *, int);
int	libkirk_rijndael_set_key_enc_only(rijndael_ctx *, const u8 *, int);
void libkirk_rijndael_decrypt(rijndael_ctx *, const u8 *, u8 *);
void libkirk_rijndael_encrypt(rijndael_ctx *, const u8 *, u8 *);

int libkirk_AES_set_key(AES_ctx *ctx, const u8 *key, int bits);
void libkirk_AES_encrypt(AES_ctx *ctx, const u8 *src, u8 *dst);
void libkirk_AES_decrypt(AES_ctx *ctx, const u8 *src, u8 *dst);
void libkirk_AES_cbc_encrypt(AES_ctx *ctx, u8 *src, u8 *dst, int size);
void libkirk_AES_cbc_decrypt(AES_ctx *ctx, u8 *src, u8 *dst, int size);
void libkirk_AES_CMAC(AES_ctx *ctx, unsigned char *input, int length, unsigned char *mac);

int	libkirk_rijndaelKeySetupEnc(unsigned int [], const unsigned char [], int);
int	libkirk_rijndaelKeySetupDec(unsigned int [], const unsigned char [], int);
void libkirk_rijndaelEncrypt(const unsigned int [], int, const unsigned char [],
	    unsigned char []);
		
typedef struct SHA1Context
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */
    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */
    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */
    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

/*
 *  Function Prototypes
 */
void libkirk_SHA1Reset(SHA1Context *);
int libkirk_SHA1Result(SHA1Context *);
void libkirk_SHA1Input( SHA1Context *,
                const unsigned char *,
                unsigned);

#endif /* __RIJNDAEL_H */
