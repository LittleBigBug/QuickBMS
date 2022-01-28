#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              fubuki_ECRYPT_ctx
#define ECRYPT_init             fubuki_ECRYPT_init
#define ECRYPT_keysetup         fubuki_ECRYPT_keysetup
#define ECRYPT_ivsetup          fubuki_ECRYPT_ivsetup
#define ECRYPT_process_bytes    fubuki_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    fubuki_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    fubuki_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  fubuki_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   fubuki_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   fubuki_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   fubuki_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   fubuki_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   fubuki_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   fubuki_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks fubuki_ECRYPT_keystream_blocks

#include "../libs/ecrypt/submissions/fubuki/fubuki.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(fubuki_ECRYPT)
