#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              moustique_ECRYPT_ctx
#define ECRYPT_init             moustique_ECRYPT_init
#define ECRYPT_keysetup         moustique_ECRYPT_keysetup
#define ECRYPT_ivsetup          moustique_ECRYPT_ivsetup
#define ECRYPT_process_bytes    moustique_ECRYPT_process_bytes
#define ECRYPT_decrypt_bytes    moustique_ECRYPT_decrypt_bytes
#define ECRYPT_encrypt_bytes    moustique_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes(...)//  moustique_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   moustique_ECRYPT_process_packet
#define ECRYPT_encrypt_packet   moustique_ECRYPT_encrypt_packet
#define ECRYPT_decrypt_packet   moustique_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   moustique_ECRYPT_process_blocks
#define ECRYPT_encrypt_blocks   moustique_ECRYPT_encrypt_blocks
#define ECRYPT_decrypt_blocks   moustique_ECRYPT_decrypt_blocks
#define ECRYPT_keystream_blocks moustique_ECRYPT_keystream_blocks

#define init_ctx        moustique_init_ctx
#define byte_to_bits    moustique_byte_to_bits
#define bits_to_byte    moustique_bits_to_byte
#define iterate         moustique_iterate
#define encrypt_bit     moustique_encrypt_bit
#define decrypt_bit     moustique_decrypt_bit
#define encrypt_byte    moustique_encrypt_byte
#define decrypt_byte    moustique_decrypt_byte

#include "../libs/ecrypt/submissions/mosquito/moustique/moustique.c"

#include "ecrypt.h"
ECRYPT_FUNCTION_PREVIOUS(moustique_ECRYPT)
