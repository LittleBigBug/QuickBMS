#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              sss_ECRYPT_ctx
#define ECRYPT_init             sss_ECRYPT_init
#define ECRYPT_keysetup         sss_ECRYPT_keysetup
#define ECRYPT_ivsetup          sss_ECRYPT_ivsetup
#define ECRYPT_process_bytes    sss_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    sss_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    sss_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  sss_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   sss_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   sss_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   sss_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   sss_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   sss_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   sss_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks sss_ECRYPT_keystream_blocks

#define ECRYPT_AE_keysetup              sss_ECRYPT_AE_keysetup
#define ECRYPT_AE_ivsetup               sss_ECRYPT_AE_ivsetup
#define ECRYPT_AE_encrypt_bytes         sss_ECRYPT_AE_encrypt_bytes
#define ECRYPT_AE_decrypt_bytes         sss_ECRYPT_AE_decrypt_bytes
#define ECRYPT_AE_authenticate_bytes    sss_ECRYPT_AE_authenticate_bytes
#define ECRYPT_AE_finalize              sss_ECRYPT_AE_finalize

#include "../libs/ecrypt/submissions/sss/unused/sssref.c"
#include "../libs/ecrypt/submissions/sss/sss.c"

#include "ecrypt.h"
ECRYPT_FUNCTION_PREVIOUS(sss_ECRYPT)
