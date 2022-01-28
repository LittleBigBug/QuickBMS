#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/PKLib/pklib.h"

typedef unsigned int    DWORD;


// from bwapi https://github.com/bwapi/bwapi

typedef struct __Param
{
  char  *pCompressedData;
  DWORD dwReadPos;
  char  *pDecompressedData;
  DWORD dwWritePos;
  DWORD dwMaxRead;
  DWORD dwMaxWrite;
} _Param;
static unsigned int PKEXPORT read_buf(char *buf, unsigned int *size, void *_param)
{
  _Param *param = (_Param*)_param;

  DWORD dwSize = param->dwMaxRead - param->dwReadPos;
  if ( dwSize > *size )
    dwSize = *size;
  memcpy(buf, &param->pCompressedData[param->dwReadPos], dwSize);
  param->dwReadPos += dwSize;
  return dwSize;
}
static void PKEXPORT write_buf(char *buf, unsigned int *size, void *_param)
{
  _Param *param = (_Param*)_param;
  if ( param->dwWritePos + *size <= param->dwMaxWrite )
    memcpy(&param->pDecompressedData[param->dwWritePos], buf, *size);
  param->dwWritePos += *size;
}



int pkware_dcl_explode(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   bWorkBuff[EXP_BUFFER_SIZE];
    memset(bWorkBuff, 0, sizeof(bWorkBuff));

    _Param params;
    memset(&params, 0, sizeof(params));
    params.pCompressedData    = in;
    params.dwMaxRead          = insz;
    params.pDecompressedData  = out;
    params.dwMaxWrite         = outsz;

    if ( explode(&read_buf, &write_buf, bWorkBuff, &params) ) return -1;

    return params.dwWritePos;
}



int pkware_dcl_implode(unsigned char *in, int insz, unsigned char *out, int outsz) {
    unsigned char   bWorkBuff[CMP_BUFFER_SIZE];
    memset(bWorkBuff, 0, sizeof(bWorkBuff));

    _Param params;
    memset(&params, 0, sizeof(params));
    params.pCompressedData    = in;
    params.dwMaxRead          = insz;
    params.pDecompressedData  = out;
    params.dwMaxWrite         = outsz;

    unsigned int dwType = CMP_BINARY;
    unsigned int dwImplSize = 0x400;
    if ( implode(&read_buf, &write_buf, bWorkBuff, &params, &dwType, &dwImplSize) ) return -1;

    return params.dwWritePos;
}
