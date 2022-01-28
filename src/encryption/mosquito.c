#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              mosquito_ECRYPT_ctx
#define ECRYPT_init             mosquito_ECRYPT_init
#define ECRYPT_keysetup         mosquito_ECRYPT_keysetup
#define ECRYPT_ivsetup          mosquito_ECRYPT_ivsetup
#define ECRYPT_process_bytes    mosquito_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    mosquito_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    mosquito_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  mosquito_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   mosquito_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   mosquito_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   mosquito_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   mosquito_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   mosquito_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   mosquito_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks(...)// mosquito_ECRYPT_keystream_blocks

#define init_ctx        mosquito_init_ctx
#define byte_to_bits    mosquito_byte_to_bits
#define bits_to_byte    mosquito_bits_to_byte
#define iterate         mosquito_iterate
#define encrypt_bit     mosquito_encrypt_bit
#define decrypt_bit     mosquito_decrypt_bit
#define encrypt_byte    mosquito_encrypt_byte
#define decrypt_byte    mosquito_decrypt_byte

#include "../libs/ecrypt/submissions/mosquito/mosquito/mosquito.c"

#include "ecrypt.h"
ECRYPT_FUNCTION_PREVIOUS(mosquito_ECRYPT)

