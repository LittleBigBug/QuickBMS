/*
 *  TEST single-file compressor for LZHL algorithm
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Warning: this program is only a demonstration for LZHL algorithm usage;
 *           only single-file compression supported
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include "lzhl.h"

#ifndef min
#define min( a, b ) ( (a) < (b) ? (a) : (b) )
#endif
#ifndef max
#define max( a, b ) ( (a) > (b) ? (a) : (b) )
#endif

#define BLOCKSIZE 4000000

void usage()
    {
    printf( "Usage: LZHL {+|-} <input_file> <output_file>\n" );
    }

int main( int argc, char** argv )
	{
    if( argc < 4 )
        {
        usage();
        return 1;
        }
    if( *argv[ 1 ] == '+' )
        {
        FILE* f;
        int i, rawLen, compLen;
        unsigned char* rawBlock;
        unsigned char* compBlock;
        clock_t start, finish;
        LZHL_CHANDLE comp;
        
        f = fopen( argv[ 2 ], "rb" );
        if( !f )
            abort();
        fseek( f, 0, SEEK_END );
        rawLen = ftell( f );
        fseek( f, 0, SEEK_SET );
        rawBlock = (unsigned char*)malloc( rawLen );
        compBlock = (unsigned char*)malloc( LZHLCompressorCalcMaxBuf( rawLen ) );
        fread( rawBlock, 1, rawLen, f );
        fclose( f );

        start = clock();
        comp = LZHLCreateCompressor();
        compLen = 0;

        for( i=0; i < rawLen ; i += BLOCKSIZE )
            {
            int l = min( BLOCKSIZE, rawLen - i );
            int lComp = LZHLCompress( comp, compBlock + compLen, rawBlock + i, l );
            compLen += lComp;
            }
        LZHLDestroyCompressor( comp );
        finish = clock();
        printf( "%d ticks\n", finish - start );

        f = fopen( argv[ 3 ], "wb" );
        if( !f )
            abort();

        // rawLen is stored as a byte sequence to avoid problems 
        // with little_endian/big_endian
        for( i=0; i < 4 ; ++i )
            fputc( (unsigned char)(rawLen >> i*8), f );
        fwrite( compBlock, 1, compLen, f );
        fclose( f );
        }
    else if( *argv[ 1 ] == '-' )
        {
        FILE* f;
        int i, fileLen, rawLen, compLen;
        size_t srcSz, dstSz;
        unsigned char* rawBlock;
        unsigned char* compBlock;
        clock_t start, finish;
        LZHL_DHANDLE decomp;
        
        f = fopen( argv[ 2 ], "rb" );
        if( !f )
            abort();

        fseek( f, 0, SEEK_END );
        fileLen = ftell( f );
        fseek( f, 0, SEEK_SET );
        if( fileLen < 4 )
            abort();

        rawLen = 0;
        for( i=0; i < 4 ; ++i )
            rawLen |= (fgetc( f ) << i*8);
        compLen = fileLen - 4;

        rawBlock = (unsigned char*)malloc( rawLen );
        compBlock = (unsigned char*)malloc( fileLen );
        if( !rawBlock || !compBlock )
            abort();
        fread( compBlock, 1, fileLen, f );
        fclose( f );

        start = clock();
        srcSz = compLen;
        dstSz = rawLen;
        decomp = LZHLCreateDecompressor();
        for(;;)
            {
            int Ok = LZHLDecompress( decomp, rawBlock + rawLen - dstSz, &dstSz, compBlock + compLen - srcSz, &srcSz );
            if( !Ok )
                abort();
            if( srcSz == 0 )
                break;
            }
        LZHLDestroyDecompressor( decomp );
        finish = clock();
        printf( "%d ticks\n", finish - start );

        f = fopen( argv[ 3 ], "wb" );
        if( !f )
            abort();
        fwrite( rawBlock, 1, rawLen, f );
        fclose( f );
        }
    else
        {
        usage();
        return 1;
        }


    return 0;
	}
