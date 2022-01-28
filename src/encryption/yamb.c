#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              yamb_ECRYPT_ctx
#define ECRYPT_init             yamb_ECRYPT_init
#define ECRYPT_keysetup         yamb_ECRYPT_keysetup
#define ECRYPT_ivsetup          yamb_ECRYPT_ivsetup
#define ECRYPT_process_bytes    yamb_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    yamb_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    yamb_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  yamb_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   yamb_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   yamb_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   yamb_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   yamb_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   yamb_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   yamb_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks yamb_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/yamb/yamb256/yamb.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(yamb_ECRYPT)
