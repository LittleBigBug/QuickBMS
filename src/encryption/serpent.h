
/* Number of rounds per Serpent encrypt/decrypt operation.  */
#define ROUNDS 32

/* Magic number, used during generating of the subkeys.  */
#define PHI 0x9E3779B9

/* Serpent works on 128 bit blocks.  */
typedef u32 serpent_block_t[4];

/* Serpent key, provided by the user.  If the original key is shorter
   than 256 bits, it is padded.  */
typedef u32 serpent_key_t[8];

/* The key schedule consists of 33 128 bit subkeys.  */
typedef u32 serpent_subkeys_t[ROUNDS + 1][4];

/* A Serpent context.  */
typedef struct serpent_context
{
  serpent_subkeys_t keys;	/* Generated subkeys.  */
} serpent_context_t;

void
serpent_setkey_internal (serpent_context_t *context,
			 const byte *key, unsigned int key_length);
void
serpent_encrypt_internal (serpent_context_t *context,
			  const serpent_block_t input, serpent_block_t output);
void
serpent_decrypt_internal (serpent_context_t *context,
			  const serpent_block_t input, serpent_block_t output);

