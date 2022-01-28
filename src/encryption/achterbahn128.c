#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              achterbahn128_ECRYPT_ctx
#define ECRYPT_init             achterbahn128_ECRYPT_init
#define ECRYPT_keysetup         achterbahn128_ECRYPT_keysetup
#define ECRYPT_ivsetup          achterbahn128_ECRYPT_ivsetup
#define ECRYPT_process_bytes    achterbahn128_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    achterbahn128_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    achterbahn128_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  achterbahn128_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   achterbahn128_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   achterbahn128_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   achterbahn128_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   achterbahn128_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   achterbahn128_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   achterbahn128_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks achterbahn128_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/achterbahn/128-80/ref/achterbahn-128-80.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(achterbahn128_ECRYPT)
