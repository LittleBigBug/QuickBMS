#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              mir1_ECRYPT_ctx
#define ECRYPT_init             mir1_ECRYPT_init
#define ECRYPT_keysetup         mir1_ECRYPT_keysetup
#define ECRYPT_ivsetup          mir1_ECRYPT_ivsetup
#define ECRYPT_process_bytes    mir1_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    mir1_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    mir1_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  mir1_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   mir1_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   mir1_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   mir1_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   mir1_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   mir1_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   mir1_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks mir1_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/mir-1/mir-1.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(mir1_ECRYPT)
