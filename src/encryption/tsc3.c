#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              tsc3_ECRYPT_ctx
#define ECRYPT_init             tsc3_ECRYPT_init
#define ECRYPT_keysetup         tsc3_ECRYPT_keysetup
#define ECRYPT_ivsetup          tsc3_ECRYPT_ivsetup
#define ECRYPT_process_bytes    tsc3_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    tsc3_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    tsc3_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  tsc3_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   tsc3_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   tsc3_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   tsc3_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   tsc3_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   tsc3_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   tsc3_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks tsc3_ECRYPT_keystream_blocks

#define S       tsc3_S
#define tsc     tsc3_tsc

#include "../libs/ecrypt/submissions/tsc-3/tsc-3/ref/tsc-3.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(tsc3_ECRYPT)
