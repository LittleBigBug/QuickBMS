// modified by Luigi Auriemma
/*
Written 2005 By Brendan G Bohannon

I release this under the public domain. 
However, I do requst mention in any derived source if applicable.


Thought:
LZBSS: a modified, vaguely LZSS like algo.
Normal extension 'lzb'.

Will use a 64kB rotating buffer, and a rotating "codespace" for encoding.

0..127: direct references into the rotating codespace
128: escape code, 8 bit char value
129: 8 bit length, 8 bit relative offset
130..191: lengths 2..63, 8 bit offset
192: reserved
193: 8 bit length, 16 bit offset
194..255: 2..63, 16 bit offset

Worst case: 50% inflation.

The codespace will rotate, each time an escape is used all codes in the
codespace will be shifteded down by 1 value, and the new value (127) will
refer to the escaped character.

Implementation:
Ring buffer;
Linked lists;
Incremental state.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LZBSS_IOBUFSZ	262144

//typedef unsigned char byte;
//typedef unsigned short ushort;

typedef struct {
byte *winbuf;
ushort winpos;

ushort *linkbuf;
ushort *linkidx;

short codebuf[256];	//buffer reflecting the current codespace
short rcodebuf[256];	//reverse codespace
byte codebufpos;

byte *ibuf, *obuf;	//current buffer pointers
byte *ibufe, *obufe;	//current buffer limits
}LZBSS_State;

LZBSS_State *LZBSS_NewDecodeState()
{
	LZBSS_State *tmp;
	int i;

	tmp=malloc(sizeof(LZBSS_State));
	memset(tmp, 0, sizeof(LZBSS_State));

	tmp->winbuf=malloc(65536);
	memset(tmp->winbuf, 0, 65536);

	for(i=0; i<256; i++)
		tmp->codebuf[i]=i;

	return(tmp);
}

//encode data using current state:
//return value: 0=OK, 1=need input, 2=need output, -1=stream error
int LZBSS_DecodeState(LZBSS_State *state, int end)
{
	int i, j, k, bi, bl;
	ushort si;

	while(1)
	{
		i=state->ibufe-state->ibuf;
		if(i<=8)
		{
			if(!end)return(1);
			if(!i)return(0);
		}

		if((state->obuf+320)>=state->obufe)
			return(2);

		i=*state->ibuf++;

		if(i>128)
		{
			if(i>=194)
			{
				bl=i-192;
				bi=*state->ibuf++;
				bi|=(*state->ibuf++)<<8;

//				bi=state->ibuf[0]|(state->ibuf[1]<<8);
//				state->ibuf+=2;
			}else if(i>=192)
			{
				if(i==193)
				{
					bl=(*state->ibuf++)+64;
					bi=*state->ibuf++;
					bi|=(*state->ibuf++)<<8;
				}else 
				{
					return(-1);
				}
			}else if(i>=130)
			{
				bl=i-128;
				bi=*state->ibuf++;
			}else if(i==129)
			{
				bl=(*state->ibuf++)+64;
				bi=*state->ibuf++;
			}

			si=state->winpos-bi;
			while(bl--)
			{
				k=state->winbuf[si++];
				state->winbuf[state->winpos++]=k;
				*state->obuf++=k;
			}
		}else
		{
			if(i==128)
			{
				j=*state->ibuf++;

				state->codebufpos++;
				state->codebuf[(byte)
					(state->codebufpos+127)]=j;

				*state->obuf++=j;
				state->winbuf[state->winpos++]=j;

				continue;
			}

			j=state->codebuf[(byte)(state->codebufpos+i)];
//			j=state->codebuf[(state->codebufpos+i)&0xFF];
			*state->obuf++=j;

			state->winbuf[state->winpos++]=j;
		}
	}
	return(0);
}

int LZBSS_fread(unsigned char *dst, int size, unsigned char **src, unsigned char *srcl) {
    if(size > (srcl - *src)) size = srcl - *src;
    memcpy(dst, *src, size);
    (*src) += size;
    return(size);
}

int LZBSS_fwrite(unsigned char *src, int size, unsigned char **dst, unsigned char *dstl) {
    if(size > (dstl - *dst)) size = dstl - *dst;
    memcpy(*dst, src, size);
    (*dst) += size;
    return(size);
}

int LZBSS_unpack(unsigned char *in, int insz, unsigned char *out, int outsz) {
	LZBSS_State *state;
	byte *ibuf, *obuf;
	int i, j, k, eof;
	unsigned int /*crc,*/ sz, sz2;
	//float ft, ft1;

    unsigned char   *inl, *outl;
    inl = in + insz;
    outl = out + outsz;

    state=LZBSS_NewDecodeState();

    ibuf=malloc(LZBSS_IOBUFSZ);
    obuf=malloc(LZBSS_IOBUFSZ);

    i=LZBSS_fread(ibuf, LZBSS_IOBUFSZ, &in, inl);
    eof=i<LZBSS_IOBUFSZ;
    state->ibuf=ibuf;
    state->obuf=obuf;
    state->ibufe=ibuf+i;
    state->obufe=obuf+LZBSS_IOBUFSZ;

    //crc=1;
    sz=0;
    sz2=i;

    while(1)
    {
        i=LZBSS_DecodeState(state, eof);

        if(i==0)break;

        if(i==1)
        {
            if(eof)break;

            j=state->ibufe-state->ibuf;
            if(j)memcpy(ibuf, state->ibuf, j);
            k=LZBSS_fread(ibuf+j, LZBSS_IOBUFSZ-j, &in, inl);
            eof=(j+k)<LZBSS_IOBUFSZ;

            state->ibuf=ibuf;
            state->ibufe=ibuf+j+k;
            sz2+=k;

            continue;
        }

        if(i==2)
        {
            j=state->obuf-obuf;
            LZBSS_fwrite(obuf, j, &out, outl);

            //crc=PDRLELZ_DataAdler32(obuf, j, crc);
            sz+=j;

            state->obuf=obuf;
            state->obufe=obuf+LZBSS_IOBUFSZ;

            continue;
        }

        if(!i)break;
    }

    j=state->obuf-obuf;
    if(j)
    {
        LZBSS_fwrite(obuf, j, &out, outl);
        //crc=PDRLELZ_DataAdler32(obuf, j, crc);
        sz+=j;
    }

    free(state);
    free(ibuf);
    free(obuf);
	return(sz);
}
