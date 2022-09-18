// modified by Luigi Auriemma

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//#include <Python.h>
//#include <malloc.h>
//#include <memory.h>
// Decode file (LUA & other compressed files)
// Input : Data.file.input : input buffer
//         Data.filesize   : input buffer size
// Output: Data.file.output: output buffer (CMemFile)

// New generation FSH/QFS decompressor/compressor
// Version 1.22 - copyright (c) Denis Auroux 1998-2002
// auroux@math.polytechnique.fr

//typedef char BOOL;
//typedef unsigned int UINT ;

static void qfs_mmemcpy(unsigned char *dest,unsigned char *src,int len) /* LZ-compatible memcopy */
{
  while (len--) *(dest++)=*(src++);
}

unsigned char *qfs_uncompress_data( unsigned char *inbuf,int *buflen )
{
  unsigned char *outbuf;
  unsigned char packcode;
  int a,b,c,len,offset;
  int inlen,outlen,inpos,outpos;
  
  /* length of data */
  inlen=*buflen;
  outlen=(inbuf[2]<<16)+(inbuf[3]<<8)+inbuf[4];
  outbuf=(unsigned char*)malloc(outlen);
  if (outbuf==NULL) 
  { 
    //AfxMessageBox("Insufficient memory."); 
	free(outbuf);
    return NULL; 
  }
  
  /* position in file */
  if (inbuf[0]&0x01) inpos=8; else inpos=5;
  outpos=0;
  
  /* main decoding loop */
  while ((inpos<inlen)&&(inbuf[inpos]<0xFC))
  {
	

    packcode=inbuf[inpos];
    a=inbuf[inpos+1];
    b=inbuf[inpos+2];
    
    if (!(packcode&0x80)) {
      len=packcode&3;
      qfs_mmemcpy(outbuf+outpos,inbuf+inpos+2,len);
      inpos+=len+2;
      outpos+=len;
      len=((packcode&0x1c)>>2)+3;
      offset=((packcode>>5)<<8)+a+1;
      qfs_mmemcpy(outbuf+outpos,outbuf+outpos-offset,len);
      outpos+=len;
    }
    else if (!(packcode&0x40)) {
      len=(a>>6)&3; 
      qfs_mmemcpy(outbuf+outpos,inbuf+inpos+3,len);
      inpos+=len+3;
      outpos+=len;
      len=(packcode&0x3f)+4;
      offset=(a&0x3f)*256+b+1;
      qfs_mmemcpy(outbuf+outpos,outbuf+outpos-offset,len);
      outpos+=len;
    }  
    else if (!(packcode&0x20)) {
      c=inbuf[inpos+3];
      len=packcode&3; 
      qfs_mmemcpy(outbuf+outpos,inbuf+inpos+4,len);
      inpos+=len+4;
      outpos+=len;
      len=((packcode>>2)&3)*256+c+5;
      offset=((packcode&0x10)<<12)+256*a+b+1;
      qfs_mmemcpy(outbuf+outpos,outbuf+outpos-offset,len);
      outpos+=len;
    }  
    else {
      len=(packcode&0x1f)*4+4;
      qfs_mmemcpy(outbuf+outpos,inbuf+inpos+1,len);
      inpos+=len+1;
      outpos+=len;
    }
  }
  
  /* trailing bytes */
  if ((inpos<inlen)&&(outpos<outlen)) {
    qfs_mmemcpy(outbuf+outpos,inbuf+inpos+1,inbuf[inpos]&3);
    outpos+=inbuf[inpos]&3;
  }
  
  if (outpos!=outlen) 
  {
	  //printf("Warning: bad length ? %d instead of %d\n",outpos,outlen);
  }
  *buflen=outlen;
  return outbuf;
}



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/* compressing a QFS file */
//fixed /* note: inbuf should have at least 1028 bytes beyond buflen */

/* QFS compression quality factor */
#define QFS_MAXITER 0x80    // by Luigi Auriemma

void qfs_compress_data(unsigned char *inbuf,int *buflen,unsigned char *outbuf)
{

#define QFS_WINDOW_LEN (1<<17)
#define QFS_WINDOW_MASK (QFS_WINDOW_LEN-1)
  
  unsigned char *inrd,*inref,*incmp;
  int *rev_similar; /* where is the previous occurrence */
  int **rev_last;   /* idem */
  int offs,len,bestoffs=0,bestlen,lastwrot,i;
  int inpos,inlen,outpos;
  int *x;  

  inlen=*buflen;
  inpos=0;
  inrd=inbuf;
  rev_similar=(int *)malloc(4*QFS_WINDOW_LEN);
  rev_last=(int **)malloc(256*sizeof(int *));
  if (rev_last) rev_last[0]=(int *)malloc(65536*4);
  
  if ((outbuf==NULL)||(rev_similar==NULL)||
      (rev_last==NULL)||(rev_last[0]==NULL))
    { 
	//	TRACE("Insufficient memory.\n"); 
		*buflen = 0;
		printf( "Insufficient memory.\n"); 
		return; 
	}
  for (i=1;i<256;i++) rev_last[i]=rev_last[i-1]+256;
  memset(rev_last[0],0xff,65536*4);
  memset(rev_similar,0xff,4*QFS_WINDOW_LEN);
  
  outbuf[0]=0x10; outbuf[1]=0xFB;
  outbuf[2]=inlen>>16; outbuf[3]=(inlen>>8)&255; outbuf[4]=inlen&255;
  outpos=5;
  lastwrot=0;
  
  /* main encoding loop */
  for (inpos=0,inrd=inbuf;inpos<inlen;inpos++,inrd++) {
    if ((inpos&0x3fff)==0) 
	{ /*putchar('.'); fflush(stdout); */
	}
    /* adjust occurrence tables */
    x=rev_last[*inrd]+(inrd[1]);
    offs=rev_similar[inpos&QFS_WINDOW_MASK]=*x;
    *x=inpos;
    /* if this has already been compressed, skip ahead */
    if (inpos<lastwrot) continue;

    /* else look for a redundancy */
    bestlen=0; i=0;
    while ((offs>=0)&&(inpos-offs<QFS_WINDOW_LEN)&&(i++<QFS_MAXITER)) {
      len=2; incmp=inrd+2; inref=inbuf+offs+2;
      while ((incmp<(inbuf+inlen))&&(inref<(inbuf+inlen)) && (*(incmp++)==*(inref++))&&(len<1028)) len++;
      if (len>bestlen) { bestlen=len; bestoffs=inpos-offs; }
      offs=rev_similar[offs&QFS_WINDOW_MASK];
    }
    
    /* check if redundancy is good enough */
    if (bestlen>inlen-inpos) bestlen=inpos-inlen;
    if (bestlen<=2) bestlen=0;
    if ((bestlen==3)&&(bestoffs>1024)) bestlen=0;
    if ((bestlen==4)&&(bestoffs>16384)) bestlen=0;

    /* update compressed data */
    if (bestlen) {
      while (inpos-lastwrot>=4) {
        len=(inpos-lastwrot)/4-1;
        if (len>0x1B) len=0x1B;
        outbuf[outpos++]=0xE0+len;
        len=4*len+4;
        memcpy(outbuf+outpos,inbuf+lastwrot,len);
        lastwrot+=len;
        outpos+=len;
      }
      len=inpos-lastwrot;
      if ((bestlen<=10)&&(bestoffs<=1024)) {
        outbuf[outpos++]=(((bestoffs-1)>>8)<<5)+((bestlen-3)<<2)+len;
        outbuf[outpos++]=(bestoffs-1)&0xff;
        while (len--) outbuf[outpos++]=inbuf[lastwrot++];
        lastwrot+=bestlen;
      }
      else if ((bestlen<=67)&&(bestoffs<=16384)) {
        outbuf[outpos++]=0x80+(bestlen-4);
        outbuf[outpos++]=(len<<6)+((bestoffs-1)>>8);
        outbuf[outpos++]=(bestoffs-1)&0xff;
        while (len--) outbuf[outpos++]=inbuf[lastwrot++];
        lastwrot+=bestlen;
      }
      else if ((bestlen<=1028)&&(bestoffs<QFS_WINDOW_LEN)) {
        bestoffs--;
        outbuf[outpos++]=0xC0+((bestoffs>>16)<<4)+(((bestlen-5)>>8)<<2)+len;
        outbuf[outpos++]=(bestoffs>>8)&0xff;
        outbuf[outpos++]=bestoffs&0xff;
        outbuf[outpos++]=(bestlen-5)&0xff;
        while (len--) outbuf[outpos++]=inbuf[lastwrot++];
        lastwrot+=bestlen;
      }
    }
  }
  
  /* end stuff */
  inpos=inlen;
  while (inpos-lastwrot>=4) {
    len=(inpos-lastwrot)/4-1;
    if (len>0x1B) len=0x1B;
    outbuf[outpos++]=0xE0+len;
    len=4*len+4;
    memcpy(outbuf+outpos,inbuf+lastwrot,len);
    lastwrot+=len;
    outpos+=len;
  }
  len=inpos-lastwrot;
  outbuf[outpos++]=0xFC+len;
  while (len--) outbuf[outpos++]=inbuf[lastwrot++];

  if (lastwrot!=inlen) 
  {
    printf("Something strange happened at the end of compression!\n");
	*buflen = 0;
    return;
  }
  if (rev_similar) free(rev_similar);
  if (rev_last) { free(rev_last[0]); free(rev_last); }
  *buflen=outpos;
}
