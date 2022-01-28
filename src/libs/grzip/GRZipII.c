/*-------------------------------------------------*/
/* GRZipII/libGRZip compressor           GRZipII.c */
/* GRZipII file to file compressor                 */
/*-------------------------------------------------*/

/*--
  This file is a part of GRZipII and/or libGRZip, a program
  and library for lossless, block-sorting data compression.

  Copyright (C) 2002-2004 Grebnov Ilya. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  Grebnov Ilya, Ivanovo, Russian Federation.
  Ilya.Grebnov@magicssoft.ru, http://magicssoft.ru/

  This program is based on (at least) the work of:
  Juergen Abel, Jon L. Bentley, Edgar Binder,
  Charles Bloom, Mike Burrows, Andrey Cadach,
  Damien Debin, Sebastian Deorowicz, Peter Fenwick,
  George Plechanov, Michael Schindler, Robert Sedgewick,
  Julian Seward, David Wheeler, Vadim Yoockin.

  Normal compression mode:
    Compression     memory use : [9-11]*BlockLen + 1Mb
    Decompression   memory use : 7*BlockLen      + 1Mb
  Fast compression mode:
    Compression     memory use : 7*BlockLen      + 1Mb
    Decompression   memory use : 7.125*BlockLen  + 1Mb

  For more information on these sources, see the manual.
--*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libgrzip.c"

uint8  GRZipIISign[12]={0x47,0x52,0x5A,0x69,0x70,0x49,0x49,0x0,0x2,0x4,':',')'};

sint32 BlockSize=5*1024*1024;
sint32 FastBWTMode=1;
sint32 DeltaFilter=0;
sint32 UseLZP=1;
sint32 AdaptativeBSize=0;
sint32 Mode=GRZ_Compression_BWT+GRZ_Compression_WFC;
sint32 LZPHTS=GRZ_LZP_HTS15;
sint32 LZPMML=GRZ_LZP_MML32;

void ShowAllSwitches(void)
{
  fprintf(stdout,"Usage:GRZipII <e|d> InputFile OutputFile <Switches>\n\n");
  fprintf(stdout,"<Switches>\n");
  fprintf(stdout,"  -b<size> Maximum block size(e.g. -b1m, -b1024k)\n");
  fprintf(stdout,"           (default -b5m or -b5120k, maximum -b8m)\n");
  fprintf(stdout,"  -m<mode> Compression algorithms, default: -m1\n");
  fprintf(stdout,"           -m1 LZP + BWT + WFC + EC\n");
  fprintf(stdout,"           -m2 LZP + BWT + MTF + EC\n");
  fprintf(stdout,"           -m3 LZP + ST4 + WFC + EC\n");
  fprintf(stdout,"           -m4 LZP + ST4 + MTF + EC\n");
  fprintf(stdout,"  -L<size> LZP Hash table size[10-18], 4*2^Size bytes, default: -L15\n");
  fprintf(stdout,"  -M<size> LZP Minimum Matched Len [2,5,8...,47], default: -M32\n");
  fprintf(stdout,"  -S       Use alternative BWT Sorting algorithm\n");
  fprintf(stdout,"           (faster for repetitive blocks)\n");
  fprintf(stdout,"  -a       Enable Adaptative block size reduction\n");
  fprintf(stdout,"  -l       Disable LZP preprocessing\n");
  fprintf(stdout,"  -d       Enable Delta filter\n");
  fprintf(stdout,"  -p       Disable all Preprocessing techniques\n");
  fprintf(stdout,"  -h       Show all Switches\n");
  exit(0);
}

void ShowUsage(void)
{
  fprintf(stdout,"Usage:GRZipII <e|d> InputFile OutputFile <Switches>\n\n");
  fprintf(stdout,"<Switches>\n");
  fprintf(stdout,"  -b<size> Maximum block size(e.g. -b1m, -b1024k)\n");
  fprintf(stdout,"           (default -b5m or -b5120k, maximum -b8m)\n");
  fprintf(stdout,"  -m<mode> Compression algorithms, default: -m1\n");
  fprintf(stdout,"           -m1 LZP + BWT + WFC + EC\n");
  fprintf(stdout,"           -m2 LZP + BWT + MTF + EC\n");
  fprintf(stdout,"           -m3 LZP + ST4 + WFC + EC\n");
  fprintf(stdout,"           -m4 LZP + ST4 + MTF + EC\n");
  fprintf(stdout,"  -a       Enable Adaptative block size reduction\n");
  fprintf(stdout,"  -l       Disable LZP preprocessing\n");
  fprintf(stdout,"  -p       Disable all Preprocessing techniques\n");
  fprintf(stdout,"  -h       Show all Switches\n");
  exit(0);
}

void Options(char * s)
{
 while (*(++s)!='\0')
   switch (*s)
   {
     case 'b':
      {
        char *ss=++s;
        while ((*s>='0')&&(*s<='9')) s++;
        if (*s=='m')
        {
           BlockSize=atoi(ss)*1024*1024;
           break;
        }
        if (*s=='k')
        {
           BlockSize=atoi(ss)*1024;
           break;
        }
        ShowUsage();
       }
     case 'm':
      {
        char *ss=++s;
        while ((*s>='0')&&(*s<='9')) s++;
        s--;
        switch (atoi(ss))
        {
          case 1:{Mode=GRZ_Compression_BWT+GRZ_Compression_WFC;break;}
          case 2:{Mode=GRZ_Compression_BWT+GRZ_Compression_MTF;break;}
          case 3:{Mode=GRZ_Compression_ST4+GRZ_Compression_WFC;break;}
          case 4:{Mode=GRZ_Compression_ST4+GRZ_Compression_MTF;break;}
          default:ShowUsage();
        }
        break;
      }
     case 'L':
      {
        char *ss=++s;
        while ((*s>='0')&&(*s<='9')) s++;
        s--;
        switch (atoi(ss))
        {
          case 10:{LZPHTS=GRZ_LZP_HTS10;break;}
          case 11:{LZPHTS=GRZ_LZP_HTS11;break;}
          case 12:{LZPHTS=GRZ_LZP_HTS12;break;}
          case 13:{LZPHTS=GRZ_LZP_HTS13;break;}
          case 14:{LZPHTS=GRZ_LZP_HTS14;break;}
          case 15:{LZPHTS=GRZ_LZP_HTS15;break;}
          case 16:{LZPHTS=GRZ_LZP_HTS16;break;}
          case 17:{LZPHTS=GRZ_LZP_HTS17;break;}
          case 18:{LZPHTS=GRZ_LZP_HTS18;break;}
          default:ShowUsage();
        }
        break;
      }
     case 'M':
      {
        char *ss=++s;
        while ((*s>='0')&&(*s<='9')) s++;
        s--;
        switch (atoi(ss))
        {
          case 2 :{LZPMML=GRZ_LZP_MML2;break;}
          case 5 :{LZPMML=GRZ_LZP_MML5;break;}
          case 8 :{LZPMML=GRZ_LZP_MML8;break;}
          case 11:{LZPMML=GRZ_LZP_MML11;break;}
          case 14:{LZPMML=GRZ_LZP_MML14;break;}
          case 17:{LZPMML=GRZ_LZP_MML17;break;}
          case 20:{LZPMML=GRZ_LZP_MML20;break;}
          case 23:{LZPMML=GRZ_LZP_MML23;break;}
          case 26:{LZPMML=GRZ_LZP_MML26;break;}
          case 29:{LZPMML=GRZ_LZP_MML29;break;}
          case 32:{LZPMML=GRZ_LZP_MML32;break;}
          case 35:{LZPMML=GRZ_LZP_MML35;break;}
          case 38:{LZPMML=GRZ_LZP_MML38;break;}
          case 41:{LZPMML=GRZ_LZP_MML41;break;}
          case 44:{LZPMML=GRZ_LZP_MML44;break;}
          case 47:{LZPMML=GRZ_LZP_MML47;break;}
          default:ShowUsage();
        }
        break;
      }
     case 'h': {ShowAllSwitches();break;}
     case 'S': {FastBWTMode=0;break;}
     case 'l': {UseLZP=0;break;}
     case 'a': {AdaptativeBSize=1;break;}
     case 'd': {DeltaFilter=1;break;}
     case 'p': {DeltaFilter=0;UseLZP=0;AdaptativeBSize=0;break;}
     default : ShowUsage();
   }
}

void MakeMode(void)
{
 if ((BlockSize>8*1024*1024)||(BlockSize<1024)) ShowUsage();
 if (BlockSize>GRZ_MaxBlockSize) BlockSize=GRZ_MaxBlockSize;
 if (UseLZP)
   Mode=Mode+LZPMML+LZPHTS;
 else
   Mode+=GRZ_Disable_LZP;
 if (FastBWTMode)
   Mode+=GRZ_BWTSorting_Fast;
 else
   Mode+=GRZ_BWTSorting_Strong;
 if (DeltaFilter)
   Mode+=GRZ_Enable_DeltaFlt;
 else
   Mode+=GRZ_Disable_DeltaFlt;
}

sint32 CLine(sint32 argc,char ** argv)
{
  sint32 Result=0,i;
  if (argc==1) ShowUsage();
  if (*argv[1]=='-') {Options(argv[1]);ShowUsage();};
  if (argc<4) ShowUsage();
  if (strlen(argv[1])!=1) ShowUsage();
  switch (*argv[1])
  {
    case 'e': {Result=1;break; }
    case 'd': {Result=2;break; }
    default : ShowUsage();
  }
  for (i=4;i<argc;i++)
    if (*argv[i]=='-')
      Options(argv[i]);
    else
      ShowUsage();
  MakeMode();
  return (Result);
}

void Compression(char ** argv)
{
  FILE * FInput=fopen(argv[2],"rb");
  if (FInput==NULL)
  {
    fprintf(stderr,"Can't open input file %s!\n",argv[2]);
    return ;
  }
  FILE * FOutput=fopen(argv[3],"wb");
  if (FOutput==NULL)
  {
    fclose(FInput);
    fprintf(stderr,"Can't create output file %s!\n",argv[3]);
    return ;
  }
  fwrite(GRZipIISign,1,sizeof(GRZipIISign),FOutput);

  if (fseek(FInput,0,SEEK_END))
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }
  long RealSize=ftell(FInput);
  if (RealSize==-1)
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }

  if (fseek(FInput,0,SEEK_SET))
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }

  uint8 * Input=(uint8 *)malloc(BlockSize+1024);
  if (Input==NULL)
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"Not enough memory!\n");
    return ;
  }
  uint8 * Output=(uint8 *)malloc(BlockSize+1024);
  if (Output==NULL)
  {
    free(Input); fclose(FOutput); fclose(FInput);
    fprintf(stderr,"Not enough memory!\n");
    return ;
  }

  clock_t BeginTime = clock();

  while ((feof(FInput)==0)&&(ftell(FInput)!=RealSize))
  {
    fprintf(stdout,"\r");
    double Tmp=ftell(FInput);Tmp=(100*Tmp)/RealSize;
    fprintf(stdout,"Compressing %.55s(%02d%%)",argv[2],(sint32)Tmp);fflush(stdout);

    sint32 NumRead=fread(Input,1,BlockSize,FInput);

    if (AdaptativeBSize)
    {
       sint32 NewSize=GRZip_GetAdaptativeBlockSize(Input,NumRead);
       if (NewSize!=NumRead)
       {
         sint32 NewPos=ftell(FInput)+NewSize-NumRead;
         fseek(FInput,NewPos,SEEK_SET);
         NumRead=NewSize;
       }
    }

    sint32 Size=GRZip_CompressBlock(Input,NumRead,Output,Mode);

    fwrite(Output,1,Size,FOutput);
  }

  fprintf(stdout,"\r                                                                          \r");fflush(stdout);

  double TotTime=((double)(clock()-BeginTime))/CLOCKS_PER_SEC;
  fprintf(stdout,"Compressed %ld into %ld in %.3f seconds.\n",RealSize,ftell(FOutput),TotTime);

  free(Input);free(Output);
  fclose(FOutput);fclose(FInput);
}

void Decompression(char ** argv)
{
  uint8 BlockSign[28];

  FILE * FInput=fopen(argv[2],"rb");
  if (FInput==NULL)
  {
    fprintf(stderr,"Can't open input file %s!\n",argv[2]);
    return ;
  }
  FILE * FOutput=fopen(argv[3],"wb");
  if (FOutput==NULL)
  {
    fclose(FInput);
    fprintf(stderr,"Can't create output file %s!\n",argv[3]);
    return ;
  }
  uint8 SignTest[sizeof(GRZipIISign)];

  if (fread(SignTest,1,sizeof(GRZipIISign),FInput)!=sizeof(GRZipIISign))
  {
    fclose(FInput);fclose(FOutput);
    fprintf(stderr,"This's not GRZip archive!\n");
    return;
  }

  if (memcmp(SignTest,GRZipIISign,sizeof(GRZipIISign))!=0)
  {
    fclose(FInput);fclose(FOutput);
    fprintf(stderr,"This's not GRZip archive!\n");
    return;
  }

  if (fseek(FInput,0,SEEK_END))
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }
  long FileSize=ftell(FInput);
  if (FileSize==-1)
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }
  if (fseek(FInput,sizeof(GRZipIISign),SEEK_SET))
  {
    fclose(FOutput); fclose(FInput);
    fprintf(stderr,"IO error on file %s!\n",argv[2]);
    return ;
  }

  clock_t BeginTime = clock();

  while ((feof(FInput)==0)&&(ftell(FInput)!=FileSize))
  {
    fprintf(stdout,"\r");
    double Tmp=ftell(FInput);Tmp=(100*Tmp)/FileSize;
    fprintf(stdout,"Decompressing %.55s(%02d%%)",argv[2],(sint32)Tmp);fflush(stdout);

    sint32 NumRead=fread(BlockSign,1,28,FInput);
    if (NumRead!=28)
    {
      fclose(FOutput); fclose(FInput);
      fprintf(stderr,"Unexpected end of file %s!\n",argv[2]);
      return ;
    }

    if (GRZip_CheckBlockSign(BlockSign,28)!=GRZ_NO_ERROR)
    {
      fclose(FOutput); fclose(FInput);
      fprintf(stderr,"CRC check failed!\n");
      return ;
    }

    uint8 * Input=(uint8 *)malloc(1024+(*(sint32 *)(BlockSign+16)));
    if (Input==NULL)
    {
      fclose(FOutput); fclose(FInput);
      fprintf(stderr,"Not enough memory!\n");
      return ;
    }

    memcpy(Input,BlockSign,28);

    uint8 * Output=(uint8 *)malloc(1024+(*(sint32 *)(Input)));
    if (Output==NULL)
    {
      free(Input);fclose(FOutput); fclose(FInput);
      fprintf(stderr,"Not enough memory!\n");
      return ;
    }

    NumRead=fread(Input+28,1,*(sint32 *)(Input+16),FInput);
    if (NumRead!=*(sint32 *)(Input+16))
    {
      free(Input); free(Output); fclose(FOutput); fclose(FInput);
      fprintf(stderr,"Unexpected end of file %s!\n",argv[2]);
      return ;
    }

    sint32 Size=GRZip_DecompressBlock(Input,NumRead+28,Output);

    if (Size==GRZ_NOT_ENOUGH_MEMORY)
    {
      free(Input); free(Output); fclose(FOutput); fclose(FInput);
      fprintf(stderr,"Not enough memory!\n");
      return ;
    }

    if (Size<0)
    {
      free(Input); free(Output); fclose(FOutput); fclose(FInput);
      fprintf(stderr,"CRC check failed!\n");
      return ;
    }

    fwrite(Output,1,Size,FOutput);

    free(Input); free(Output);

  }

  fprintf(stdout,"\r                                                                          \r");fflush(stdout);

  double TotTime=((double)(clock()-BeginTime))/CLOCKS_PER_SEC;
  fprintf(stdout,"Decompressed %ld into %ld in %.3f seconds.\n",FileSize,ftell(FOutput),TotTime);

  fclose(FOutput);fclose(FInput);
}

int main(int argc,char * argv[])
{
  fprintf(stderr,"This is GRZipII, yet another BSLDCA compressor. Version 0.2.4, 12-Feb-2004.\n");
  fprintf(stderr,"CopyRight (c) 2002-2004 by Grebnov Ilya <Ilya.Grebnov@magicssoft.ru>.\n\n");
  switch (CLine(argc, argv))
  {
    case 1: {Compression(argv);break;}
    case 2: {Decompression(argv);break;}
  }
  return 0;
}

/*-------------------------------------------------*/
/* End                                   GRZipII.c */
/*-------------------------------------------------*/
