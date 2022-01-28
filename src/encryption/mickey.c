#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              mickey_ECRYPT_ctx
#define ECRYPT_init             mickey_ECRYPT_init
#define ECRYPT_keysetup         mickey_ECRYPT_keysetup
#define ECRYPT_ivsetup          mickey_ECRYPT_ivsetup
#define ECRYPT_process_bytes    mickey_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    mickey_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    mickey_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  mickey_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   mickey_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   mickey_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   mickey_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   mickey_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   mickey_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   mickey_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks mickey_ECRYPT_keystream_blocks

#define R_Mask      mickey_R_Mask
#define COMP0       mickey_COMP0
#define COMP1       mickey_COMP1
#define FB0         mickey_FB0
#define FB1         mickey_FB1
#define CLOCK_R     mickey_CLOCK_R
#define CLOCK_S     mickey_CLOCK_S
#define CLOCK_KG    mickey_CLOCK_KG

#include "../libs/ecrypt/submissions/mickey/v2/bit-by-bit/mickey-v2.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(mickey_ECRYPT)
