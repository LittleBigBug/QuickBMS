/***************************************/
/* Set to 1 to allow szip encoding.    */
/* Set to 0 to prohibit szip encoding. */
/***************************************/
#include "SZconfig.h"

#ifndef HAVE_ENCODING
#define REMOVE_SZIP_ENCODER 1
#endif

#ifdef REMOVE_SZIP_ENCODER
int szip_allow_encoding = 0;
char * szip_encoder_status = "SZIP ENCODER DISABLED";
#else
int szip_allow_encoding = 1;
char * szip_encoder_status = "SZIP ENCODER ENABLED";
#endif
