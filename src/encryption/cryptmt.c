#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              cryptmt_ECRYPT_ctx
#define ECRYPT_init             cryptmt_ECRYPT_init
#define ECRYPT_keysetup         cryptmt_ECRYPT_keysetup
#define ECRYPT_ivsetup          cryptmt_ECRYPT_ivsetup
#define ECRYPT_process_bytes    cryptmt_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    cryptmt_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    cryptmt_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  cryptmt_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   cryptmt_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   cryptmt_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   cryptmt_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   cryptmt_ECRYPT_process_blocks
#define ECRYPT_encrypt_blocks   cryptmt_ECRYPT_encrypt_blocks
#define ECRYPT_decrypt_blocks   cryptmt_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks cryptmt_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/cryptmt/v3/cryptmt-v3.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(cryptmt_ECRYPT)
