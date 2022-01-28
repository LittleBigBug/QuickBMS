#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HSEL.h"



extern "C" void hsel_crypt(unsigned char *key, unsigned char *data, int size, int do_encrypt, char *options) {

    // keep them static for using the function easily without context
    static CHSEL_STREAM *m_hStream = NULL;
	static HselInit eninit = {0};

    // initialization
    if(key) {
        if(m_hStream) delete m_hStream;
        m_hStream = new(CHSEL_STREAM);

        // the following comes from Crypt_example.cpp but it's wrong for decryption because key and encryption are random
        eninit.iEncryptType		=	HSEL_ENCRYPTTYPE_RAND;
        eninit.iDesCount		=	HSEL_DES_TRIPLE;
        eninit.iCustomize		=	HSEL_KEY_TYPE_DEFAULT;
        eninit.iSwapFlag		=	HSEL_SWAP_FLAG_ON;

        // so let's set something default
        eninit.iCustomize		=	HSEL_KEY_TYPE_CUSTOMIZE;
        eninit.iEncryptType		=	HSEL_ENCRYPTTYPE_1;

        if(options) {
            if(strstr(options, "HSEL_DES_SINGLE")) eninit.iDesCount = HSEL_DES_SINGLE;
            if(strstr(options, "HSEL_DES_TRIPLE")) eninit.iDesCount = HSEL_DES_TRIPLE;
            if(strstr(options, "HSEL_ENCRYPTTYPE_RAND")) eninit.iEncryptType = HSEL_ENCRYPTTYPE_RAND;
            if(strstr(options, "HSEL_ENCRYPTTYPE_1")) eninit.iEncryptType = HSEL_ENCRYPTTYPE_1;
            if(strstr(options, "HSEL_ENCRYPTTYPE_2")) eninit.iEncryptType = HSEL_ENCRYPTTYPE_2;
            if(strstr(options, "HSEL_ENCRYPTTYPE_3")) eninit.iEncryptType = HSEL_ENCRYPTTYPE_3;
            if(strstr(options, "HSEL_ENCRYPTTYPE_4")) eninit.iEncryptType = HSEL_ENCRYPTTYPE_4;
            if(strstr(options, "HSEL_SWAP_FLAG_ON")) eninit.iSwapFlag = HSEL_SWAP_FLAG_ON;
            if(strstr(options, "HSEL_SWAP_FLAG_OFF")) eninit.iSwapFlag = HSEL_SWAP_FLAG_OFF;
            if(strstr(options, "HSEL_KEY_TYPE_DEFAULT")) eninit.iCustomize = HSEL_KEY_TYPE_DEFAULT;
            if(strstr(options, "HSEL_KEY_TYPE_CUSTOMIZE")) eninit.iCustomize = HSEL_KEY_TYPE_CUSTOMIZE;
        }

        memcpy(&eninit.Keys, key, sizeof(eninit.Keys));

        m_hStream->Initial(eninit);
    }

    // encryption/decryption
    if(data) {
        if(do_encrypt) m_hStream->Encrypt((char *)data, size);
        else           m_hStream->Decrypt((char *)data, size);
    }
}

