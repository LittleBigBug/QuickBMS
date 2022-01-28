// modified by Luigi Auriemma

#include <stdio.h>
#include <stdlib.h>
//#include <time.h>

static const int log2table[256]=
{
  -1,
  0,
  1,1,
  2,2,2,2,
  3,3,3,3,3,3,3,3,
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
  5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static int u8_log2(unsigned char x)
{
  return log2table[x];
}
static int u16_log2(unsigned short x)
{
  int z;
  if(x>>8)
    z=8+u8_log2(x>>8);
  else
    z=u8_log2(x);
  return z;
}
static int u32_log2(unsigned int x)
{
  int z;
  if(x>>16)
    z=16+u16_log2(x>>16);
  else
    z=u16_log2(x);
  return z;
}

//#pragma mark -

/* 簡易ビット入出力ルーチン */
static unsigned char *infile,*outfile;
static int buffer,bcount,outcount;
static void _initput(unsigned char *file)
{
  outfile=file;
  buffer=0;
  bcount=8;
  outcount=0;
}
static void _termput(void)
{
  if(bcount!=8)
  {
    outfile[outcount] = buffer; //fputc(buffer,outfile);
    outcount++;
  }
  //fclose(outfile);
}
static void _initget(unsigned char *file)
{
  infile=file;
  buffer = *infile++; //buffer=fgetc(infile);
  bcount=8;
}
static void _termget(void)
{
  //fclose(infile);
}
#define MASK(v,n) ((v)&((1U<<(n))-1U))
static void _putbits(unsigned int v,int n)
{
  while(bcount<=n)
  {
    n-=bcount;
    buffer|=MASK(v>>n,bcount);
    outfile[outcount] = buffer; //fputc(buffer,outfile);
    outcount++;
    buffer=0;
    bcount=8;
  }
  bcount-=n;
  buffer|=MASK(v,n)<<bcount;
}
static unsigned int _getbits(int n)
{
  unsigned int v;

  v=0;
  while(bcount<=n)
  {
    n-=bcount;
    v|=MASK(buffer,bcount)<<n;
    buffer = *infile++; //buffer=fgetc(infile);
    bcount=8;
  }
  bcount-=n;
  v|=MASK(buffer>>bcount,n);
  return v;
}

//#pragma mark -

/* CBT符号 */
static void encode_number(int number,int max)
{
  int log2size,hold;
  if(max==0)
    return;
  log2size=u32_log2(max)+1;
  hold=(1<<log2size)-(max+1);
  if(number<hold)
    log2size--;
  else
    number+=hold;
  _putbits(number,log2size);
}
static int decode_number(int max)
{
  int number;
  int log2size,hold;
  if(max==0)
    return 0;
  log2size=u32_log2(max)+1;
  hold=(1<<log2size)-(max+1);
  number=_getbits(log2size-1);
  if(hold<=number)
    number=((number<<1)|_getbits(1))-hold;
  return number;
}

//#pragma mark -

static void _shellsort(int *A,int *B,int k)
{
  int h,i,j,t;
  for(h=4;h<k;h=h*3+1){}
  do{
    h/=3;
    for(i=h;i<k;i++)
    {
      t=A[i];
      for(j=i-h;(0<=j)&&((B[t]<B[A[j]]) || ((B[t]==B[A[j]])&&(t<A[j])));j-=h)
        A[j+h]=A[j];
      A[j+h]=t;
    }
  }while(h!=1);
}

//#pragma mark -

static void encode_depthfirst(unsigned char *T,int low,int high,int *A,int *B,int k)
{
  if(k==1)
  {/* グループの記号が一種類だけならこれ以上の符号化は必要ない */
    return;
  }
  else
  {
    int C[256],D[256];
    int mid=(low+high-1)/2;
    int n=high-low+1,m=mid-low+1;
    int i,c,f,k1,k2;

    /* アルファベットを頻度順にソートする */
    _shellsort(A,B,k);

    /* tableを初期化 */
    /*
      A・・グループ・サブグループ(left)のアルファベット
      B・・グループ・サブグループ(left)の頻度表
      C・・サブグループ(right)のアルファベット
      D・・サブグループ(right)の頻度表
    */
    for(i=0;i<k;i++)
    {
      c=A[i];
      D[c]=B[c];
      B[c]=0;
    }

    /* サブグループ(left)の各アルファベットの頻度を求める */
    for(i=low;i<=mid;i++)
      B[T[i]]++;

    /* サブグループ(left)の頻度表を符号化
     * A[k-1]の頻度は符号化しなくて良い */
    for(i=0,k1=0,k2=0;(0<m)&&(n!=m)&&((i+1)<k);i++)
    {
      c=A[i];
      f=B[c];
      encode_number(f,(D[c]<m)?D[c]:m);
      n-=D[c];
      D[c]-=f;
      m-=f;
      if(0<f)
        A[k1++]=c;
      if(0<D[c])
        C[k2++]=c;
    }
    if(n==m)
    {/* 残りのアルファベットをサブグループ(left)にセット */
      for(;i<k;i++)
      {
        c=A[i];
        D[c]=0;
        A[k1++]=c;
      }
    }
    else if(m==0)
    {/* 残りのアルファベットをサブグループ(right)にセット */
      for(;i<k;i++)
        C[k2++]=A[i];
    }
    else
    {/* A[k-1]用の処理 */
      c=A[k-1];
      D[c]-=m;
      A[k1++]=c;
      if(0<D[c])
        C[k2++]=c;
    }

    /* グループを分割して符号化を続ける */
    encode_depthfirst(T,low,mid,A,B,k1);
    encode_depthfirst(T,mid+1,high,C,D,k2);
  }
}

static void encode(unsigned char *T,int n)
{
  int A[256],B[256];
  int i,j,k;

  /* 各アルファベットの出現頻度を求める */
  for(i=0;i<256;i++)
    B[i]=0;
  for(i=0;i<n;i++)
    B[T[i]]++;

  /* 出現頻度を符号化 */
  encode_number(n,(1<<30)-1);/* nは2^30-1まで */
  for(i=0,j=n,k=0;i<256;i++)
  {
    encode_number(B[i],j);
    if(0<B[i])
    {
      j-=B[i];
      A[k++]=i;
      if(j==0)
        break;
    }
  }

  /* 深さ優先で分割・符号化 */
  encode_depthfirst(T,0,n-1,A,B,k);
}

//#pragma mark -

static void decode_depthfirst(int low,int high,int *A,int *B,int k)
{
  if(k==1)
  {/* グループの記号が一種類だけならこれ以上の復号化は必要ない */
    int i,c;
    for(i=low,c=A[0];i<=high;i++)
      outfile[outcount++] = c; //fputc(c,outfile);/* 出力 */
  }
  else
  {
    int C[256],D[256];
    int mid=(low+high-1)/2;
    int n=high-low+1,m=mid-low+1;
    int i,c,f,k1,k2;

    /* アルファベットを頻度順にソートする */
    _shellsort(A,B,k);

    /* tableを初期化 */
    for(i=0;i<k;i++)
    {
      c=A[i];
      D[c]=B[c];
      B[c]=0;
    }

    /* サブグループ(left)の頻度表を復号化 */
    for(i=0,k1=0,k2=0;(0<m)&&(n!=m)&&((i+1)<k);i++)
    {
      c=A[i];
      f=decode_number((D[c]<m)?D[c]:m);
      B[c]=f;
      n-=D[c];
      D[c]-=f;
      m-=f;
      if(0<f)
        A[k1++]=c;
      if(0<D[c])
        C[k2++]=c;
    }
    if(n==m)
    {/* 残りのアルファベットをサブグループ(left)にセット */
      for(;i<k;i++)
      {
        c=A[i];
        B[c]=D[c];
        D[c]=0;
        A[k1++]=c;
      }
    }
    else if(m==0)
    {/* 残りのアルファベットをサブグループ(right)にセット */
      for(;i<k;i++)
        C[k2++]=A[i];
    }
    else
    {/* A[k-1]用の処理 */
      c=A[k-1];
      B[c]=m;
      D[c]-=m;
      A[k1++]=c;
      if(0<D[c])
        C[k2++]=c;
    }

    /* グループを分割して復号化を続ける */
    decode_depthfirst(low,mid,A,B,k1);
    decode_depthfirst(mid+1,high,C,D,k2);
  }
}

static void decode(void)
{
  int A[256],B[256];
  int i,j,k,n;

  /* 出現頻度を復号化 */
  n=decode_number((1<<30)-1);
  for(i=0,j=n,k=0;i<256;i++)
  {
    B[i]=decode_number(j);
    if(0<B[i])
    {
      j-=B[i];
      A[k++]=i;
      if(j==0)
        break;
    }
  }

  /* 深さ優先で分割・復号化 */
  decode_depthfirst(0,n-1,A,B,k);
}

int m99coder(unsigned char *_infile, int insz, unsigned char *_outfile, int dec_enc) {

/*
 * m99coder.c
 * Copyright (C) 2003-2004 yuta mori
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * yuu (yuta mori) :
 *   mailto:YIV01157@nifty.ne.jp
 *   http://homepage3.nifty.com/wpage/
 */

    infile  = _infile;
    outfile = _outfile;

    if(dec_enc) {
        _initput(outfile);
        encode(infile, insz);
        _termput();
    } else {
        _initget(infile);
        decode();
        _termget();
    }

    return outcount;
}

