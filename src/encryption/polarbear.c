#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              polarbear_ECRYPT_ctx
#define ECRYPT_init             polarbear_ECRYPT_init
#define ECRYPT_keysetup         polarbear_ECRYPT_keysetup
#define ECRYPT_ivsetup          polarbear_ECRYPT_ivsetup
#define ECRYPT_process_bytes    polarbear_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    polarbear_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    polarbear_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  polarbear_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   polarbear_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   polarbear_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   polarbear_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   polarbear_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   polarbear_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   polarbear_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks polarbear_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/polarbear/v2/polar-bear.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(polarbear_ECRYPT)
