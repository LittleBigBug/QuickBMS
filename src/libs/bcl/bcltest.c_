/*************************************************************************
* Name:        bcltest.c
* Author:      Marcus Geelnard
* Description: Basic Compression Library tester.
*
* This program can be used to test any of the compression algorithms in
* the Basic Compression Library. The program reads a file, which is
* compressed and decompressed and compared in order to detect any errors
* in the compression and/or decompression routines. Also, the compression
* ratio and speed is shown.
*
*-------------------------------------------------------------------------
* Copyright (c) 2003-2007 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/

/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Basic Compression Library */
#include "rle.h"
#include "shannonfano.h"
#include "huffman.h"
#include "rice.h"
#include "lz.h"

/* Timing */
#include "systimer.h"


/*************************************************************************
* GetFileSize()
*************************************************************************/

long GetFileSize( FILE *f )
{
  long pos, size;

  pos = ftell( f );
  fseek( f, 0, SEEK_END );
  size = ftell( f );
  fseek( f, pos, SEEK_SET );

  return size;
}


/*************************************************************************
* TestFile()
*************************************************************************/

int TestFile( char *name, int algo, int verbose )
{
  unsigned int  insize, outsize, bufsize, *work, k, err_count;
  unsigned char *in, *out, *buf;
  FILE          *f;
  double        t0, t_comp, t_uncomp;

  printf( "Testing %s...", name );

  /* Open input file */
  f = fopen( name, "rb" );
  if( !f )
  {
    printf( "unable to open!\n" );
    return 0;
  }

  /* Get input size */
  insize = GetFileSize( f );
  if( insize < 1 )
  {
    printf( "empty file!\n" );
    fclose( f );
    return 0;
  }

  /* Worst case output buffer size */
  bufsize = (insize*104+50)/100 + 384;

  /* Allocate memory */
  in = (unsigned char *) malloc( insize + 2*bufsize );
  if( !in )
  {
    printf( "out of memory!\n" );
    fclose( f );
    return 0;
  }

  /* Pointers to compression buffer and output memory */
  buf = &in[ insize ];
  out = &buf[ bufsize ];

  /* Read and close input file */
  fread( in, 1, insize, f );
  fclose( f );

  /* Compress and decompress */
  switch( algo )
  {
    case 1:
      t0 = GetTime();
      outsize = RLE_Compress( in, insize, buf, bufsize );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      RLE_Uncompress( buf, outsize, out, insize );
      t_uncomp = GetTime() - t0;
      break;
    case 2:
      t0 = GetTime();
      outsize = Huffman_Compress( in, buf, insize );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Huffman_Uncompress( buf, out, outsize, insize );
      t_uncomp = GetTime() - t0;
      break;
    case 3:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_UINT8 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_UINT8 );
      t_uncomp = GetTime() - t0;
      break;
    case 4:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_UINT16 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_UINT16 );
      t_uncomp = GetTime() - t0;
      break;
    case 5:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_UINT32 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_UINT32 );
      t_uncomp = GetTime() - t0;
      break;
    case 6:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_INT8 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_INT8 );
      t_uncomp = GetTime() - t0;
      break;
    case 7:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_INT16 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_INT16 );
      t_uncomp = GetTime() - t0;
      break;
    case 8:
      t0 = GetTime();
      outsize = Rice_Compress( in, buf, insize, RICE_FMT_INT32 );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      Rice_Uncompress( buf, out, outsize, insize, RICE_FMT_INT32 );
      t_uncomp = GetTime() - t0;
      break;
    case 9:
      t0 = GetTime();
      outsize = LZ_Compress( in, buf, insize );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      LZ_Uncompress( buf, out, outsize );
      t_uncomp = GetTime() - t0;
      break;
    case 10:
      work = malloc( sizeof(unsigned int) * (65536+insize) );
      if( work )
      {
        t0 = GetTime();
        outsize = LZ_CompressFast( in, buf, insize, work );
        t_comp = GetTime() - t0;
        free( work );
        t0 = GetTime();
        LZ_Uncompress( buf, out, outsize );
        t_uncomp = GetTime() - t0;
      }
      else
      {
        printf( "unable to allocate working buffer!\n" );
        t_comp = 0.0;
        t_uncomp = 0.0;
        outsize = 0;
      }
      break;
    case 11:
      t0 = GetTime();
      outsize = SF_Compress( in, buf, insize );
      t_comp = GetTime() - t0;
      t0 = GetTime();
      SF_Uncompress( buf, out, outsize, insize );
      t_uncomp = GetTime() - t0;
      break;
    default:
      /* Should never happen... */
      outsize = 0;
      t_comp = 0.0;
      t_uncomp = 0.0;
  }

  err_count = 0;
  if(outsize > 0)
  {
    /* Show compression result */
    if( verbose )
    {
      printf( "\n  Compression: %d/%d bytes (%.1f%%)", outsize, insize,
              100*(float)outsize/(float)insize );
    }

    /* Compare input / output data */
    for( k = 0; k < insize; ++ k )
    {
      if( in[ k ] != out[ k ] )
      {
        if( err_count == 0 ) printf( "\n" );
        if( err_count == 30 ) printf( "    ...\n" );
        else if( err_count < 30 )
        {
            printf( "    %d: %d != %d\n", k, out[ k ], in[ k ] );
        }
        ++ err_count;
      }
    }

    /* Did we have success? */
    if( err_count == 0 )
    {
      printf( " - OK!\n" );
      if( verbose )
      {
        printf( "    Compression speed: %.1f KB/s (%.2f ms)\n",
                (double) insize / (1024.0 * t_comp), 1000.0 * t_comp );
        printf( "    Uncompression speed: %.1f KB/s (%.2f ms)\n",
                (double) insize / (1024.0 * t_uncomp), 1000.0 * t_uncomp );
      }
    }
    else
    {
      printf( "    *******************************\n" );
      printf( "    ERROR: %d faulty bytes\n", err_count );
      printf( "    *******************************\n" );
    }
  }

  /* Free all memory */
  free( in );

  return (outsize > 0) && (err_count == 0);
}


/*************************************************************************
* Help()
*************************************************************************/

void Help( char *prgname )
{
  printf( "Usage: %s [-v] algo file\n\n", prgname );
  printf( "Options:\n" );
  printf( "  -v      Verbose\n" );
  printf( "algo can be one of the following:\n" );
  printf( "  rle     RLE Compression\n" );
  printf( "  sf      Shannon-Fano compression\n" );
  printf( "  huff    Huffman compression\n" );
  printf( "  lz      LZ77 Compression\n" );
  printf( "  lz_f    LZ77 Compression - fast method\n" );
  printf( "  rice8   Rice compresison of 8-bit data\n" );
  printf( "  rice16  Rice compresison of 16-bit data\n" );
  printf( "  rice32  Rice compresison of 32-bit data\n" );
  printf( "  rice8s  Rice compresison of 8-bit signed data\n" );
  printf( "  rice16s Rice compresison of 16-bit signed data\n" );
  printf( "  rice32s Rice compresison of 32-bit signed data\n\n" );
  printf( "file is the name of a file to be tested.\n" );
}


/*************************************************************************
* main()
*************************************************************************/

int main( int argc, char **argv )
{
  int     algo, arg, verbose;
  char    *filename;

  /* Check arguments */
  if( (argc < 3) || (argc > 4) )
  {
    Help( argv[ 0 ] );
    return 0;
  }
  arg = 1;

  /* Check for options */
  verbose = 0;
  if( strcmp( argv[arg], "-v" ) == 0 )
  {
    verbose = 1;
    ++ arg;
  }

  /* Get algo */
  algo = 0;
  if( strcmp( argv[arg], "rle" ) == 0 )     algo = 1;
  if( strcmp( argv[arg], "huff" ) == 0 )    algo = 2;
  if( strcmp( argv[arg], "rice8" ) == 0 )   algo = 3;
  if( strcmp( argv[arg], "rice16" ) == 0 )  algo = 4;
  if( strcmp( argv[arg], "rice32" ) == 0 )  algo = 5;
  if( strcmp( argv[arg], "rice8s" ) == 0 )  algo = 6;
  if( strcmp( argv[arg], "rice16s" ) == 0 ) algo = 7;
  if( strcmp( argv[arg], "rice32s" ) == 0 ) algo = 8;
  if( strcmp( argv[arg], "lz" ) == 0 )      algo = 9;
  if( strcmp( argv[arg], "lz_f" ) == 0 )    algo = 10;
  if( strcmp( argv[arg], "sf" ) == 0 )      algo = 11;
  if( !algo )
  {
    Help( argv[ 0 ] );
    return 0;
  }
  ++ arg;

  /* Get file name */
  filename = argv[arg];

  /* Initialize timer */
  InitTimer();

  /* Test file */
  TestFile( filename, algo, verbose );

  return 0;
}
