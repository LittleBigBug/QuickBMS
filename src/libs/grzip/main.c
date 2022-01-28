/*----------------------------------------------*/
/* GRZipII/libGRZip compressor           main.c */
/* GRZipII file to file compressor              */
/*----------------------------------------------*/

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

#include <utime.h>

uint8  GRZipIISign[12]={0x47,0x52,0x5A,0x69,0x70,0x49,0x49,0x0,0x2,0x4,':',')'};

sint32 BlockSize=5*1024*1024;
sint32 FastBWTMode=1;
sint32 DeltaFilter=0;
sint32 UseCompression=1;
sint32 Verbose=-1;
sint32 UseLZP=1;
sint32 AdaptativeBSize=0;
sint32 ForceOverwrite=0;
sint32 Keep=0;
sint32 AreFiles=0;
sint32 NumFiles=0;
sint32 StdIn=0;
sint32 StdOut=0;
sint32 Mode=GRZ_Compression_BWT+GRZ_Compression_WFC;
sint32 LZPHTS=GRZ_LZP_HTS15;
sint32 LZPMML=GRZ_LZP_MML32;

char** Files=NULL;
char * FileOut=NULL;

void ShowVersion(FILE *fd)
{
    fprintf(fd, _("This is grzip, yet another BSLDCA compressor. Version 0.3.0, 31-Jan-2007.\n"));
    fprintf(fd, _("Copyright (c) 2002-2004 by Grebnov Ilya <Ilya.Grebnov@magicssoft.ru>.\n"));
    fprintf(fd, _("Enhanced version by <Jean-Pierre.Demailly@ujf-grenoble.fr>, Jan 2007.\n\n"));
}

void ShowUsage(FILE *fd)
{
  ShowVersion(fd);
  fprintf(fd, _("Usage: grzip <Switches> InputFile(s)\n\n"));
  fprintf(fd, _("Switches:\n"));
  fprintf(fd, _("  -i       All further args are input files\n"));
  fprintf(fd, _("  -f       Force overwrite of output file\n"));   
  fprintf(fd, _("  -k       Keep input file after operation\n"));   
  fprintf(fd, _("  -r       Remove input file after operation\n"));      
  fprintf(fd, _("  -c       Use stdout as output\n"));
  fprintf(fd, _("  -d       Decompression (compression is default)\n"));
  fprintf(fd, _("           equivalent to using 'grunzip' instead of 'grzip' and\n"));
  fprintf(fd, _("           using 'grzcat' is equivalent to using 'grunzip -c -q'\n"));
  fprintf(fd, _("  -z       Compression (this is the default)\n"));   
  fprintf(fd, _("  -o<outf> Name of output file\n"));
  fprintf(fd, _("  -b<size> Maximum block size (e.g. -b1m, -b1024k)\n"));
  fprintf(fd, _("           (default -b5m or -b5120k, maximum -b8m)\n"));
  fprintf(fd, _("  -m<mode> Compression algorithms, default: -m1\n"));
  fprintf(fd, _("           -m1 LZP + BWT + WFC + EC\n"));
  fprintf(fd, _("           -m2 LZP + BWT + MTF + EC\n"));
  fprintf(fd, _("           -m3 LZP + ST4 + WFC + EC\n"));
  fprintf(fd, _("           -m4 LZP + ST4 + MTF + EC\n"));
  fprintf(fd, _("  -L<size> LZP Hash table size [10-18], 4*2^Size bytes, default: -L15\n"));
  fprintf(fd, _("  -M<size> LZP Minimum Matched Len [2,5,8...,47], default: -M32\n"));
  fprintf(fd, _("  -S       Use alternative BWT Sorting algorithm\n"));
  fprintf(fd, _("           (faster for repetitive blocks)\n"));
  fprintf(fd, _("  -D       Enable Delta filter\n"));
  fprintf(fd, _("  -a       Enable Adaptative block size reduction\n"));
  fprintf(fd, _("  -l       Disable LZP preprocessing\n"));
  fprintf(fd, _("  -p       Disable all Preprocessing techniques\n"));
  fprintf(fd, _("  -q       Run quietly/silently\n"));
  fprintf(fd, _("  -v       Set verbose mode\n"));
  fprintf(fd, _("  -V       Show version\n"));
  fprintf(fd, _("  -h       Show all Switches\n"));
  exit(0);
}

void MakeStdin()   
{
  NumFiles=1;
  StdIn=1;
  Files=(char **)malloc(sizeof(char *));
  Files[0] = "<stdin>";
}

void Options(int argc,char * argv[])
{
char *s, *ss=NULL;
sint32 i;

if (!strcmp(argv[0], "grunzip")) UseCompression=0;
if (!strcmp(argv[0], "grzcat"))
  {
    UseCompression=0;
    StdOut=1;
    Verbose=0;
  }
     
for (i=1; i<argc; i++)
   {
   s = argv[i];
   if (*s!='-' || AreFiles)
   {
     if (StdIn) ShowUsage(stderr);
     ++NumFiles;
     Files = (char **)realloc(Files, NumFiles*sizeof(char *));
     Files[NumFiles-1]=argv[i];
     continue;
   } 
   else
   {
     ++s;
     if (*s!='\0' && index("bimoLM", *s) && s[1]=='\0' && i<argc-1) 
       ss=argv[++i];
     else
       ss=s+1;
   }
   if (*s=='\0')
   {
     if (!StdIn)
       MakeStdin();
     continue;
   }  
   switch (*s)
   {
     case 'b':
      { 
        if (!*ss) ShowUsage(stderr); else s=ss; 
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
        ShowUsage(stderr);
       }
     case 'm':
      {
        if (!*ss) ShowUsage(stderr); else s=ss;
        while ((*s>='0')&&(*s<='9')) s++;
        s--;
        switch (atoi(ss))
        {
          case 1:{Mode=GRZ_Compression_BWT+GRZ_Compression_WFC;break;}
          case 2:{Mode=GRZ_Compression_BWT+GRZ_Compression_MTF;break;}
          case 3:{Mode=GRZ_Compression_ST4+GRZ_Compression_WFC;break;}
          case 4:{Mode=GRZ_Compression_ST4+GRZ_Compression_MTF;break;}
          default:ShowUsage(stderr);
        }
        break;
      }
     case 'o':
      {
        if (!*ss || StdOut) ShowUsage(stderr);
        if (FileOut) free(FileOut);
        FileOut = strdup(ss);
        break;
      }
     case 'L':
      {
        if (!*ss) ShowUsage(stderr); else s=ss;
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
          default:ShowUsage(stderr);
        }
        break;
      }
     case 'M':
      {
        if (!*ss) ShowUsage(stderr); else s=ss;
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
          default:ShowUsage(stderr);
        }
        break;
      }
     case 'h': {ShowUsage(stdout);break;}
     case 'l': {UseLZP=0;break;}
     case 'a': {AdaptativeBSize=1;break;}
     case 'f': {ForceOverwrite=1;break;}      
     case 'i': {if (Files) ShowUsage(stderr);AreFiles=1;break;}
     case 'k': {Keep=1;break;}
     case 'r': {Keep=0;break;}      
     case 'c': {if (FileOut) ShowUsage(stderr);StdOut=1;break;}
     case 'd': {UseCompression=0;break;}
     case 'z': {UseCompression=1;break;}      
     case 'p': {DeltaFilter=0;UseLZP=0;AdaptativeBSize=0;break;}
     case 'q': {Verbose=0;break;}            
     case 'v': {Verbose=1;break;}
     case 'D': {DeltaFilter=1;break;}      
     case 'S': {FastBWTMode=0;break;}
     case 'V': {ShowVersion(stdout);exit(0);}      
     default : {ShowUsage(stderr);}
   }
 }
}

void MakeMode(void)
{
 if ((BlockSize>8*1024*1024)||(BlockSize<1024)) ShowUsage(stderr);
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

void Compression(char * InputFile)
{
FILE * FInput=NULL;
FILE * FOutput=NULL;
char * OutputFile=NULL;
sint32 l= strlen(InputFile);
long FileSize=0;
long ReadSize=0;
struct stat buf;
struct utimbuf timebuf;

  if (l>=4 && !strcmp(InputFile+(l-4),".grz"))
  {
    fprintf(stderr, _("grzip: input file %s has suffix .grz\n"), InputFile);
    fprintf(stderr, _("Will not try to compress it !!\n\n"));
    return;
  }

  if (StdIn)
    FInput = stdin;
  else
  {
    FInput=fopen(InputFile,"rb");
    if (FInput==NULL)
    {
      fprintf(stderr, _("grzip: cannot open input file %s!\n"), InputFile);
      return;
    }

    if (stat(InputFile, &buf) || !S_ISREG(buf.st_mode))
    {
      fprintf(stderr, _("grzip: %s is not a regular file. Ignored\n"), 
              InputFile);
      return;
    }
  }

  if (StdOut)
    FOutput=stdout;
  else
  {
    if (FileOut)
      OutputFile=FileOut;
    else
      OutputFile=malloc(l+5);
    if (!OutputFile) 
    {
      fprintf(stderr, _("grzip: available memory seems to be exhausted !!\n"));
      exit(-1);
    }
    if (!FileOut) 
    {
      strcpy(OutputFile, InputFile);
      strcpy(OutputFile+l, ".grz");
    }
    if (!ForceOverwrite)
    {
      if (stat(OutputFile, &buf)==0)
      {
	fprintf(stderr, _("File %s already exists.\n"), OutputFile);
	fprintf(stderr, _("Use -f option to force overwriting\n"));	  
	return;
      }
    }
    FOutput=fopen(OutputFile,"wb");
  } 
   
  if (FOutput==NULL)
  {
    fclose(FInput);
    fprintf(stderr, _("Cannot create output file %s!\n"), OutputFile);
    return;
  }  

  if (Verbose)
    fprintf(stderr, "grzip: %s --> %s\n", InputFile, 
            OutputFile?OutputFile:"<stdout>");

  fwrite(GRZipIISign,1,sizeof(GRZipIISign),FOutput);

  if (!StdIn) 
  {
    if (fseek(FInput,0,SEEK_END))
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
    FileSize=ftell(FInput);
    if (FileSize==-1)
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
  
    if (fseek(FInput,0,SEEK_SET))
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
  }

  uint8 * Input=(uint8 *)malloc(BlockSize+1024);
  if (Input==NULL)
  {
    if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
    fprintf(stderr, _("grzip: not enough memory!\n"));
    return ;
  }
  uint8 * Output=(uint8 *)malloc(BlockSize+1024);
  if (Output==NULL)
  {
    free(Input);
    if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
    fprintf(stderr, _("grzip: not enough memory!\n"));
    return ;
  }

  clock_t BeginTime = clock();

  /*  while ((feof(FInput)==0)&&(ftell(FInput)!=FileSize)) */
  while (!feof(FInput))
  {
    if (!StdIn)
    {
      if (Verbose)
        fprintf(stderr, "\r");
      double Tmp=ftell(FInput);Tmp=(100*Tmp)/FileSize;
      if (Verbose) {
        fprintf(stderr, _("Compressing %.55s (%02d%%)"), InputFile,(sint32)Tmp);
        fflush(stderr);
      }
    }
    sint32 NumRead=fread(Input,1,BlockSize,FInput);
    if (AdaptativeBSize)
    {
       sint32 NewSize=GRZip_GetAdaptativeBlockSize((uint8 *)Input,NumRead);
       if (NewSize!=NumRead)
       {
         long NewPos=ftell(FInput)+(long)(NewSize-NumRead);
         fseek(FInput,NewPos,SEEK_SET);
         NumRead=NewSize;
       }
    }
    ReadSize += (long) NumRead;

    sint32 Size=GRZip_CompressBlock(Input,NumRead,Output,Mode);
    
    fwrite(Output,1,Size,FOutput);
  }
  
  fprintf(stderr, "\r                                                                          \r");fflush(stderr);

  double TotTime=((double)(clock()-BeginTime))/CLOCKS_PER_SEC;
  if (Verbose)
    fprintf(stderr, 
       _("Compressed %ld Bytes into %ld Bytes in %.3f seconds.\n"),    
       ReadSize, ftell(FOutput),TotTime);

  if (OutputFile && !StdIn && !StdOut)
  {    
    timebuf.modtime=buf.st_mtime;
    timebuf.actime=buf.st_atime;     
    utime(OutputFile, &timebuf);
  }
   
  free(Input);free(Output);
  if (FInput!=stdin) fclose(FInput);
  if (FOutput!=stdout) fclose(FOutput);
  if (OutputFile) free(OutputFile);
     
  if (!StdIn && !StdOut && !Keep)
    unlink(InputFile);
}

void Decompression(char * InputFile)
{
FILE * FInput=NULL;
FILE * FOutput=NULL;
char * OutputFile=NULL;
sint32 l= strlen(InputFile);
long FileSize=0;
long OutSize=0;
long ReadSize=0;
uint8 BlockSign[28];
uint8 SignTest[sizeof(GRZipIISign)];
struct stat buf;
struct utimbuf timebuf;   

  if (StdIn)
    FInput = stdin;
  else
  {
    if (l<4 || strcmp(InputFile+(l-4),".grz"))
    {
      fprintf(stderr, _("grzip: input file %s does not have suffix .grz\n"), InputFile);
      fprintf(stderr, _("Will not try to decompress it !!\n"));
      return;
    }

    FInput=fopen(InputFile,"rb");
    if (FInput==NULL)
    {
      fprintf(stderr, _("grzip: cannot open input file %s!\n"), InputFile);
      return;
    }

    if (stat(InputFile, &buf) || !S_ISREG(buf.st_mode))
    {
      fprintf(stderr, _("grzip: %s is not a regular file. Ignored\n"), 
              InputFile);
      return;
    }
  }

  if (StdOut)
    FOutput=stdout;
  else
  {
    if (FileOut)
      OutputFile=FileOut;
    else
    {
      OutputFile = malloc(l+5);
      if (!OutputFile) 
      {
        fprintf(stderr, _("grzip: available memory seems to be exhausted !!\n"));
        exit(-1);
      }
      strcpy(OutputFile, InputFile);
      OutputFile[l-4] = '\0';
    }
    if (!ForceOverwrite)
    {
       struct stat buf;
       if (stat(OutputFile, &buf)==0)
       {
	  fprintf(stderr, _("grzip: file %s already exists.\n"), OutputFile);
	  fprintf(stderr, _("Use -f option to force overwriting\n"));	  
	  return;
       }
    }
    FOutput=fopen(OutputFile,"wb");
  } 
   
  if (FOutput==NULL)
  {
    fclose(FInput);
    fprintf(stderr, _("grzip: cannot create output file %s!\n"), OutputFile);
    return;
  }  

  if (Verbose)
    fprintf(stderr, _("grzip: %s --> %s\n"), InputFile, 
            OutputFile?OutputFile:"<stdout>");
  
  if (fread(SignTest,1,sizeof(GRZipIISign),FInput)!=sizeof(GRZipIISign))
  {
    if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
    fprintf(stderr, _("This is not a grzip archive!\n"));
    return;
  }

  ReadSize=sizeof(GRZipIISign);

  if (memcmp(SignTest,GRZipIISign,sizeof(GRZipIISign))!=0)
  {
    if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
    fprintf(stderr, _("This is not a grzip archive!\n"));
    return;
  }

  if (!StdIn)
  {
    if (fseek(FInput,0,SEEK_END))
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
    FileSize=ftell(FInput);
    if (FileSize==-1)
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
    if (fseek(FInput,sizeof(GRZipIISign),SEEK_SET))
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: IO error on file %s!\n"), InputFile);
      return ;
    }
  }

  clock_t BeginTime = clock();

  while (!feof(FInput))
  {
    if (!StdIn)
    {
      if (ftell(FInput)==FileSize) break;
      if (Verbose)
        fprintf(stderr, "\r");
      double Tmp=ftell(FInput);Tmp=(100*Tmp)/FileSize;
      if (Verbose) {
        fprintf(stderr, _("Decompressing %.55s (%02d%%)"), InputFile,(sint32)Tmp);
        fflush(stderr);
      }
    }

    sint32 NumRead=fread(BlockSign,1,28,FInput);
    ReadSize += (long) NumRead;

    if (NumRead!=28)
    {
      if (FOutput!=stdout) fclose(FOutput); 
      if (StdIn) break;
      fclose(FInput);
      fprintf(stderr, _("grzip: unexpected end of file %s!\n"), InputFile);
      return ;
    }

    if (GRZip_CheckBlockSign(BlockSign,28)!=GRZ_NO_ERROR)
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: CRC check failed!\n"));      
      return ;
    }

    uint8 * Input=(uint8 *)malloc(1024+(*(sint32 *)(BlockSign+16)));
    if (Input==NULL)
    {
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: available memory seems to be exhausted !!\n"));
      return ;
    }

    memcpy(Input,BlockSign,28);

    uint8 * Output=(uint8 *)malloc(1024+(*(sint32 *)(Input)));
    if (Output==NULL)
    {
      free(Input);
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: available memory seems to be exhausted !!\n"));
      return ;
    }    

    NumRead=fread(Input+28,1,*(sint32 *)(Input+16),FInput);
    ReadSize += (long) NumRead;

    if (NumRead!=*(sint32 *)(Input+16))
    {
      free(Input); free(Output);
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: unexpected end of file %s!\n"), InputFile);
      return ;
    }

    sint32 Size=GRZip_DecompressBlock(Input,NumRead+28,Output);
    OutSize += (long)Size;

    if (Size==GRZ_NOT_ENOUGH_MEMORY)
    {
      free(Input); free(Output);
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: available memory seems to be exhausted !!\n"));
      return ;
    }

    if (Size<0)
    {
      free(Input); free(Output);
      if (FOutput!=stdout) fclose(FOutput); fclose(FInput);
      fprintf(stderr, _("grzip: CRC check failed!\n"));
      return ;
    }

    fwrite(Output,1,Size,FOutput);
    free(Input); free(Output);

  }
  if (Verbose)
  {
    fprintf(stderr, "\r                                                                          \r");
    fflush(stderr);
  }

  double TotTime=((double)(clock()-BeginTime))/CLOCKS_PER_SEC;
  if (Verbose)
    fprintf(stderr, _("Decompressed %ld Bytes into %ld Bytes in %.3f seconds.\n"), ReadSize,OutSize,TotTime);

  if (OutputFile && !StdIn && !StdOut)
  {    
    timebuf.modtime=buf.st_mtime;
    timebuf.actime=buf.st_atime;     
    utime(OutputFile, &timebuf);
  }   
   
  if (FInput!=stdin) fclose(FInput);
  if (FOutput!=stdout) fclose(FOutput);
  if (OutputFile) free(OutputFile);

  if (!StdIn && !StdOut && !Keep)
    unlink(InputFile);
}

int main(int argc,char * argv[])
{
  int i;

#ifdef ENABLE_NLS
#define PACKAGE "grzip"
  setlocale (LC_ALL, "");
  /* This is not for gettext but all i18n software should have this line. */

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  Options(argc, argv);

  if (!NumFiles) 
  {
    MakeStdin();
    if (!FileOut) StdOut=1;
    if (Verbose==-1) Verbose=0;
  }

  if (StdOut && NumFiles>1) {
    fprintf(stderr, _("grzip: cannot use <stdout> as output with several input files !!\n"));
    exit(-1);
  }

  MakeMode();

  if (Verbose) ShowVersion(stderr);

  for (i=0; i<NumFiles; i++)
  {
    if (UseCompression)
      Compression(Files[i]);
    else
      Decompression(Files[i]);
  }

  return 0;
}

/*----------------------------------------------*/
/* End                                   main.c */
/*----------------------------------------------*/
