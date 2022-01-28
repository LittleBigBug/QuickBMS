#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libs/libcsc/Types.h"
#include "../libs/libcsc/csc_dec.h"

struct StdioSeqStream
{
    union {
        ISeqInStream is;
        ISeqOutStream os;
    };
    unsigned char   *f;
    unsigned char   *fl;
};

static
int libcsc_read(void *p, void *buf, size_t *size) {
    StdioSeqStream *sss = (StdioSeqStream *)p;
    if(*size > (sss->fl - sss->f)) *size = (sss->fl - sss->f);
    memcpy(buf, sss->f, *size);
    sss->f += *size;
    return 0;
}

static
size_t libcsc_write(void *p, const void *buf, size_t size) {
    StdioSeqStream *sss = (StdioSeqStream *)p;
    if(size > (sss->fl - sss->f)) size = (sss->fl - sss->f);    // return -1;
    memcpy(sss->f, buf, size);
    sss->f += size;
    return size;
}

static
int libcsc_show_progress(void *p, UInt64 insize, UInt64 outsize)
{
    return 0;
}

extern "C"
int csc_decompress(unsigned char *in, int insz, unsigned char *out, int outsz) {
    CSCProps p;
    CSCDec_ReadProperties(&p, in);
    in   += CSC_PROP_SIZE;
    insz -= CSC_PROP_SIZE;

    StdioSeqStream isss, osss;
    isss.f = in;
    isss.fl = in + insz;
    isss.is.Read = libcsc_read;
    osss.f = out;
    osss.fl = out + outsz;
    osss.os.Write = libcsc_write;
    ICompressProgress prog;
    prog.Progress = libcsc_show_progress;

    CSCDecHandle h = CSCDec_Create(&p, (ISeqInStream*)&isss, NULL);
    CSCDec_Decode(h, (ISeqOutStream*)&osss, &prog);
    CSCDec_Destroy(h);

    return osss.f - out;
}

