#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
//#define ECRYPT_ctx              nls_ECRYPT_ctx
#define ECRYPT_init             nls_ECRYPT_init
#define ECRYPT_keysetup         nls_ECRYPT_keysetup
#define ECRYPT_ivsetup          nls_ECRYPT_ivsetup
#define ECRYPT_process_bytes    nls_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    nls_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    nls_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  nls_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   nls_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   nls_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   nls_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   nls_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   nls_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   nls_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks nls_ECRYPT_keystream_blocks

#define ECRYPT_AE_keysetup              nls_ECRYPT_AE_keysetup
#define ECRYPT_AE_ivsetup               nls_ECRYPT_AE_ivsetup
#define ECRYPT_AE_encrypt_bytes         nls_ECRYPT_AE_encrypt_bytes
#define ECRYPT_AE_decrypt_bytes         nls_ECRYPT_AE_decrypt_bytes
#define ECRYPT_AE_authenticate_bytes    nls_ECRYPT_AE_authenticate_bytes
#define ECRYPT_AE_finalize              nls_ECRYPT_AE_finalize

#include "../libs/ecrypt/submissions/nls/v2/sync/nls-v2.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(nls_ECRYPT)
