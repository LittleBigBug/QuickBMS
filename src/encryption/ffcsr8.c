#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              ffcsr8_ECRYPT_ctx
#define ECRYPT_init             ffcsr8_ECRYPT_init
#define ECRYPT_keysetup         ffcsr8_ECRYPT_keysetup
#define ECRYPT_ivsetup          ffcsr8_ECRYPT_ivsetup
#define ECRYPT_process_bytes    ffcsr8_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    ffcsr8_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    ffcsr8_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  ffcsr8_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   ffcsr8_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   ffcsr8_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   ffcsr8_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   ffcsr8_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   ffcsr8_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   ffcsr8_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks ffcsr8_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/f-fcsr/f-fcsr-8/f-fcsr-8.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(ffcsr8_ECRYPT)
