#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              tsc4_ECRYPT_ctx
#define ECRYPT_init             tsc4_ECRYPT_init
#define ECRYPT_keysetup         tsc4_ECRYPT_keysetup
#define ECRYPT_ivsetup          tsc4_ECRYPT_ivsetup
#define ECRYPT_process_bytes    tsc4_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    tsc4_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    tsc4_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  tsc4_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   tsc4_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   tsc4_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   tsc4_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   tsc4_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   tsc4_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   tsc4_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks tsc4_ECRYPT_keystream_blocks

#define S       tsc4_S
#define tsc     tsc4_tsc

#include "../libs/ecrypt/submissions/tsc-3/tsc-4/tsc-4.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(tsc4_ECRYPT)
