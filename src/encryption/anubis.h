
#ifndef PORTABLE_C__
#define PORTABLE_C__

/* 
 * Anubis-specific definitions: 
 */ 

#define MIN_N			 4 
#define MAX_N			10 
#define MIN_ROUNDS		(8 + MIN_N) 
#define MAX_ROUNDS		(8 + MAX_N) 
#define MIN_KEYSIZEB	(4*MIN_N) 
#define MAX_KEYSIZEB	(4*MAX_N) 
#define BLOCKSIZE		128 
#define BLOCKSIZEB		(BLOCKSIZE/8) 
 
/* 
 * The KEYSIZEB macro should be redefined for each allowed key size 
 * in order to use the ANUBIS test vector generator program. 
 * Valid sizes (in bytes) are 16, 20, 24, 28, 32, 36, and 40. 
 */ 
#define KEYSIZEB		16 
 
typedef struct ANUBISstruct { 
	int keyBits; /* this field must be initialized before the ANUBISkeysetup call */ 
	int R; 
	u32 roundKeyEnc[MAX_ROUNDS + 1][4]; 
	u32 roundKeyDec[MAX_ROUNDS + 1][4]; 
} ANUBISstruct; 
 
/** 
 * Create the Anubis key schedule for a given cipher key. 
 * Both encryption and decryption key schedules are generated. 
 *  
 * @param key			The 32N-bit cipher key. 
 * @param structpointer	Pointer to the structure that will hold the expanded key. 
 */ 
void ANUBISkeysetup(const unsigned char * const key, 
					struct ANUBISstruct * const structpointer); 
 
/** 
 * Encrypt a data block. 
 *  
 * @param	structpointer	the expanded key. 
 * @param	plaintext		the data block to be encrypted. 
 * @param	ciphertext		the encrypted data block. 
 */ 
void ANUBISencrypt(const struct ANUBISstruct * const structpointer, 
				   const unsigned char * const plaintext, 
				         unsigned char * const ciphertext); 
 
void ANUBISdecrypt(const struct ANUBISstruct * const structpointer, 
				   const unsigned char * const ciphertext, 
				         unsigned char * const plaintext); 
 
#endif   /* PORTABLE_C__ */

