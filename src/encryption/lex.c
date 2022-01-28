#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              lex_ECRYPT_ctx
#define ECRYPT_init             lex_ECRYPT_init
#define ECRYPT_keysetup         lex_ECRYPT_keysetup
#define ECRYPT_ivsetup          lex_ECRYPT_ivsetup
#define ECRYPT_process_bytes    lex_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    lex_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    lex_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  lex_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   lex_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   lex_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   lex_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   lex_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   lex_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   lex_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks lex_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/lex/v2/lex.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(lex_ECRYPT)
