#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              mickey128_ECRYPT_ctx
#define ECRYPT_init             mickey128_ECRYPT_init
#define ECRYPT_keysetup         mickey128_ECRYPT_keysetup
#define ECRYPT_ivsetup          mickey128_ECRYPT_ivsetup
#define ECRYPT_process_bytes    mickey128_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    mickey128_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    mickey128_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  mickey128_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   mickey128_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   mickey128_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   mickey128_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   mickey128_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   mickey128_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   mickey128_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks mickey128_ECRYPT_keystream_blocks

#define R_Mask      mickey128_R_Mask
#define COMP0       mickey128_COMP0
#define COMP1       mickey128_COMP1
#define FB0         mickey128_FB0
#define FB1         mickey128_FB1
#define CLOCK_R     mickey128_CLOCK_R
#define CLOCK_S     mickey128_CLOCK_S
#define CLOCK_KG    mickey128_CLOCK_KG

#include "../libs/ecrypt/submissions/mickey/v2-128/bit-by-bit/mickey-128-v2.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(mickey128_ECRYPT)
