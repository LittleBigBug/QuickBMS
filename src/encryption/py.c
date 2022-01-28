#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ECRYPT_API
#define ECRYPT_ctx              py_ECRYPT_ctx
#define ECRYPT_init             py_ECRYPT_init
#define ECRYPT_keysetup         py_ECRYPT_keysetup
#define ECRYPT_ivsetup          py_ECRYPT_ivsetup
#define ECRYPT_process_bytes    py_ECRYPT_process_bytes
//#define ECRYPT_decrypt_bytes    py_ECRYPT_decrypt_bytes
//#define ECRYPT_encrypt_bytes    py_ECRYPT_encrypt_bytes
#define ECRYPT_keystream_bytes  py_ECRYPT_keystream_bytes
#define ECRYPT_process_packet   py_ECRYPT_process_packet
//#define ECRYPT_encrypt_packet   py_ECRYPT_encrypt_packet
//#define ECRYPT_decrypt_packet   py_ECRYPT_decrypt_packet
#define ECRYPT_process_blocks   py_ECRYPT_process_blocks
//#define ECRYPT_encrypt_blocks   py_ECRYPT_encrypt_blocks
//#define ECRYPT_decrypt_blocks   py_ECRYPT_decrypt_blocks
//#define ECRYPT_keystream_blocks py_ECRYPT_keystream_blocks

#define internal_permutation    py_internal_permutation

#include "../libs/ecrypt/submissions/py/py/py.c"

#include "ecrypt.h"
ECRYPT_FUNCTION(py_ECRYPT)
