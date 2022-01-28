#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              pomaranch_ECRYPT_ctx
#define ECRYPT_init             pomaranch_ECRYPT_init
#define ECRYPT_keysetup         pomaranch_ECRYPT_keysetup
#define ECRYPT_ivsetup          pomaranch_ECRYPT_ivsetup
#define ECRYPT_process_bytes    pomaranch_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    pomaranch_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    pomaranch_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  pomaranch_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   pomaranch_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   pomaranch_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   pomaranch_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   pomaranch_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   pomaranch_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   pomaranch_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks pomaranch_ECRYPT_keystream_blocks

#define S   pomaranch_S
#define F   pomaranch_F

#include "../libs/ecrypt/submissions/pomaranch/v1/pomaranch.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(pomaranch_ECRYPT)
