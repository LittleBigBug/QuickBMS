/*
 *  LZH-Light algorithm implementation v 1.01
 *  Copyright (C) Sergey Ignatchenko 1998
 *  This  software  is  provided  "as is"  without  express  or implied 
 *  warranty.
 *
 *  Permission to use, copy, modify, distribute and sell this  software 
 *  for any purpose is hereby  granted  without  fee,  subject  to  the 
 *  following restrictions:
 *  1. this notice may not be removed or altered;
 *  2. altered source versions must be plainly marked as such, and must 
 *     not be misrepresented as being the original software.
 */

#define LZHLINTERNAL
#include "_huff.h"
#include "_lz.h"

HuffStat::HuffStat()
    {
    stat = new HUFFINT[ NHUFFSYMBOLS ];
    memset( stat, 0, sizeof(HUFFINT) * NHUFFSYMBOLS );
    }

HuffStat::~HuffStat()
    {
    delete [] stat;
    }

inline int cmpHuffStatTmpStruct( const HuffStatTmpStruct& a, const HuffStatTmpStruct& b )
    {
    int cmp = b.n - a.n;
    return cmp ? cmp : b.i - a.i;
    }

inline int operator <( const HuffStatTmpStruct& a, const HuffStatTmpStruct& b ) 
    {
    return cmpHuffStatTmpStruct( a, b ) < 0;
    }

static int /*__cdecl*/ _cmpStat( const void* a_, const void* b_ )
    {
    HuffStatTmpStruct* a = (HuffStatTmpStruct*)a_;
    HuffStatTmpStruct* b = (HuffStatTmpStruct*)b_;
    return cmpHuffStatTmpStruct( *a, *b );
    }

void shellSort( HuffStatTmpStruct* a, int N )
    {
    int i, j;
    HuffStatTmpStruct v;

    /*
    for ( int h = 1; h <= N/9; h = ( 3 * h + 1 ) )// determine the value for h
        ;
    */
    assert( 13 <= N / 9 );
    assert( 40 > N / 9 ); 
    int h = 40;

    for ( ; h > 0; h /= 3 )         // h = 40, 13, 4, 1
        {
        for ( i = h + 1; i <= N; ++i )
            {
            v = a[i];
            j = i;
            while ( ( j > h ) && ( v < a[j - h] ) )
                {
                a[j] = a[j-h];
                j -= h;
                }
            a[j] = v;
            }
        }
    }

int HuffStat::makeSortedTmp( HuffStatTmpStruct* s )
    {
    int total = 0;
    for( int j=0; j < NHUFFSYMBOLS ; ++j )
        {
        s[ j ].i = j;
        s[ j ].n = stat[ j ];
        total += stat[ j ];
        stat[ j ] = HUFFRECALCSTAT( stat[ j ] );
        }

    //qsort( s, NHUFFSYMBOLS, sizeof(HuffStatTmpStruct), _cmpStat );
    shellSort( s - 1, NHUFFSYMBOLS );
    return total;
    }

//*****************************************************************************

LZHLEncoderStat::LZHLEncoderStat()
    {
    nextStat = HUFFRECALCLEN;

    symbolTable = new Symbol[ NHUFFSYMBOLS ];
    memcpy( symbolTable, symbolTable0, sizeof(Symbol)*NHUFFSYMBOLS );
    }

LZHLEncoderStat::~LZHLEncoderStat()
    {
    delete [] symbolTable;
    }

LZHLEncoderStat::Symbol LZHLEncoderStat::symbolTable0[ NHUFFSYMBOLS ]
 =  {
    #include "henc.tbl"
    };

inline /* static */ void LZHLEncoderStat::_addGroup( int* groups, int group, int nBits )
    {
    assert( nBits <= 8 );

    //Bubble sort
    int j;
    for( j=group; j > 0 && nBits < groups[ j - 1 ] ; --j )
        groups[ j ] = groups[ j - 1 ];
    groups[ j ] = nBits;
    }

void LZHLEncoderStat::calcStat( int* groups )
    {
    HuffStatTmpStruct s[ NHUFFSYMBOLS ];
    int total = makeSortedTmp( s );

    nextStat = HUFFRECALCLEN;

    int pos = 0;
    int nTotal = 0;
    for( int group=0; group < 14 ; ++group )
        {
        int avgGroup = ( total - nTotal )/( 16 - group );
        int i = 0, n = 0, nn = 0;
        for( int nBits=0 ;; ++nBits )
            {
            int over = 0;
            int nItems = 1 << nBits;

            if( pos + i + nItems > NHUFFSYMBOLS )
                {
                nItems = NHUFFSYMBOLS - pos;
                over = 1;
                }

            for( ; i < nItems ; ++i )
                nn += s[ pos + i ].n;

            if( over || nBits >= 8 || nn > avgGroup )
                {
                if( nBits == 0 || abs( n - avgGroup ) > abs( nn - avgGroup ) )
                    n = nn;
                else
                    --nBits;

                _addGroup( groups, group, nBits );
                nTotal += n;
                pos += 1 << nBits;
                break;
                }
            else
                n = nn;
            }
        }

    int bestNBits = 0, bestNBits15 = 0;
    int best = 0x7FFFFFFF;
    int i = 0, nn = 0, left = 0;
    int nBits, nBits15;
    int j;
    for( j=pos; j < NHUFFSYMBOLS ; ++j )
        left += s[ j ].n;
    for( nBits = 0 ;; ++nBits )
        {
        int nItems = 1 << nBits;
        if( pos + i + nItems > NHUFFSYMBOLS )
            break;

        for( ; i < nItems ; ++i )
            nn += s[ pos + i ].n;

        int nItems15 = NHUFFSYMBOLS - ( pos + i );
        for( nBits15=0 ;; ++nBits15 )
            if( 1 << nBits15 >= nItems15 )
                break;
        
        assert( left >= nn );
        if( nBits <= 8 && nBits15 <= 8 )
            {
            int n = nn * nBits + ( left - nn ) * nBits15;
            if( n < best )
                {
                best = n;
                bestNBits = nBits;
                bestNBits15 = nBits15;
                }
            else
                break;//PERF optimization
            }
        }

    int pos15 = pos + ( 1 << bestNBits );
    _addGroup( groups, 14, bestNBits );
    _addGroup( groups, 15, bestNBits15 );

    pos = 0;
    for( j=0; j < 16 ; ++j )
        {
        int nBits = groups[ j ];

        int nItems = 1 << nBits;
        int maxK = min( nItems, NHUFFSYMBOLS - pos );
        for( int k=0; k < maxK ; ++k )
            {
            int symbol = s[ pos + k ].i;
            symbolTable[ symbol ].nBits = nBits + 4;
            symbolTable[ symbol ].code = ( j << nBits ) | k;
            }

        pos += 1 << nBits;
        }
    }

//*****************************************************************************

void LZHLEncoder::_callStat()
    {
    nextStat = 2;//to avoid recursion, >=2
    _put( NHUFFSYMBOLS - 2 );

    int groups[ 16 ];
    stat->calcStat( groups );

    int lastNBits = 0;
    for( int i=0; i < 16 ; ++i )
        {
        int nBits = groups[ i ];
        assert( nBits >= lastNBits && nBits <= 8 );
        int delta = nBits - lastNBits;
        lastNBits = nBits;
        _putBits( delta + 1, 1 );
        }
    }

void LZHLEncoder::putRaw( const BYTE* src, size_t sz )
    {
    for( const BYTE* srcEnd = src + sz; src < srcEnd ; ++src )
        _put( *src );
    }

void LZHLEncoder::putMatch( const BYTE* src, size_t nRaw, size_t matchOver, size_t disp )
    {
    assert( nRaw <= maxRaw );
    assert( matchOver <= maxMatchOver );
    assert( disp >= 0 && disp < LZBUFSIZE );

    putRaw( src, nRaw );

    struct MatchOverItem { int symbol; int nBits; UINT16 bits; };
    static MatchOverItem _matchOverTable[] =
    {
    { 264, 1, 0x00 },

    { 265, 2, 0x00 },
    { 265, 2, 0x02 },

    { 266, 3, 0x00 },
    { 266, 3, 0x02 },
    { 266, 3, 0x04 },
    { 266, 3, 0x06 },

    { 267, 4, 0x00 },
    { 267, 4, 0x02 },
    { 267, 4, 0x04 },
    { 267, 4, 0x06 },
    { 267, 4, 0x08 },
    { 267, 4, 0x0A },
    { 267, 4, 0x0C },
    { 267, 4, 0x0E },
    };

    if( matchOver < 8 )
        _put( 256 + matchOver );
    else if( matchOver < 38 )
        {
        matchOver -= 8;
        MatchOverItem* item = &_matchOverTable[ matchOver >> 1 /*/ 2*/ ];
        _put( item->symbol, item->nBits, item->bits | (matchOver & 0x01 /*% 2*/ ) );
        }
    else
        {
        matchOver -= 38;
        MatchOverItem* item = &_matchOverTable[ matchOver >> 5 /*/ 32*/ ];
        _put( item->symbol + 4 );
        _putBits( item->nBits + 4, ( item->bits << 4 ) | (matchOver & 0x1F /*% 32*/) );
        }

    static struct DispItem { int nBits; UINT16 bits; } _dispTable[] = {
    #include "hdisp.tbl"
    };

    #if LZBUFBITS < 8
    #error
    #endif

    DispItem* item = &_dispTable[ disp >> (LZBUFBITS - 7) ];
    int nBits = item->nBits + (LZBUFBITS - 7);
    UINT32 bits = ( ((UINT32)item->bits) << (LZBUFBITS - 7) ) | ( disp & ( ( 1 << (LZBUFBITS - 7) ) - 1 ) );
    #if LZBUFBITS >= 15
    if( nBits > 16 )
        {
        assert( nBits <= 32 );
        _putBits( nBits - 16, bits >> 16 );
        _putBits( 16, bits & 0xFFFF );
        }
    else
    #endif
        {
        assert( nBits <= 16 );
        _putBits( nBits, bits );
        }
    }

size_t LZHLEncoder::flush()
    {
    _put( NHUFFSYMBOLS - 1 );
    while( nBits > 0 )
        {
        *dst++ = (BYTE)( bits >> 24 );
        nBits -= 8;
        bits <<= 8;
        }

    return dst - dstBegin;
    }

//*****************************************************************************

LZHLDecoderStat::LZHLDecoderStat()
    {
    symbolTable = new HUFFINT[ NHUFFSYMBOLS ];
    memcpy( symbolTable, symbolTable0, sizeof(HUFFINT)*NHUFFSYMBOLS );

    memcpy( groupTable, groupTable0, sizeof(Group)*16 );
    }

LZHLDecoderStat::~LZHLDecoderStat()
    {
    delete [] symbolTable;
    }

LZHLDecoderStat::Group LZHLDecoderStat::groupTable0[ 16 ]
 =  {
    #include "hdec_g.tbl"
    };

HUFFINT LZHLDecoderStat::symbolTable0[ NHUFFSYMBOLS ]
 =  {
    #include "hdec_s.tbl"
    };

//*****************************************************************************

LZHLDecompressor::LZHLDecompressor()
    {
    nBits = 0;
    bits = 0;
    }

LZHLDecompressor::~LZHLDecompressor()
    {
    }

inline int LZHLDecompressor::_get( const BYTE*& src, const BYTE* srcEnd, int n )
    {
    assert( n <= 8 );
    if( nBits < n )
        {
        if( src >= srcEnd )
            {
            nBits = 0;
            return -1;
            }
        bits |= ( *src++ << ( 24 - nBits ) );
        nBits += 8;
        }

    int ret = bits >> ( 32 - n );
    bits <<= n;
    nBits -= n;
    return ret;
    }

BOOL LZHLDecompressor::decompress( BYTE* dst, size_t* dstSz, const BYTE* src, size_t* srcSz )
    {
    BYTE* startDst = dst;
    const BYTE* startSrc = src;
    const BYTE* endSrc = src + *srcSz;
    const BYTE* endDst = dst + *dstSz;
    nBits = 0;
    int i, n;
    for(;;)
        {
        int grp = _get( src, endSrc, 4 );
        if( grp < 0 )
            return FALSE;
        Group& group = groupTable[ grp ];

        int symbol;
        int nBits = group.nBits;
        if( nBits == 0 )
            symbol = symbolTable[ group.pos ];
        else
            {
            assert( nBits <= 8 );
            int got = _get( src, endSrc, nBits );
            if( got < 0 )
                return FALSE;
            int pos = group.pos + got;
            if( pos >= NHUFFSYMBOLS )
                return FALSE;
            symbol = symbolTable[ pos ];
            }

        assert( symbol < NHUFFSYMBOLS );
        ++stat[ symbol ];
        int matchOver;
        BOOL shift = FALSE;
        if( symbol < 256 )
            {
            if( dst >= endDst )
                return FALSE;
            *dst++ = (BYTE)symbol;
            _toBuf( symbol );
            continue;//forever
            }
        else if( symbol == NHUFFSYMBOLS - 2 )
            {
            HuffStatTmpStruct s[ NHUFFSYMBOLS ];
            makeSortedTmp( s );

            for( i=0; i < NHUFFSYMBOLS ; ++i )
                symbolTable[ i ] = s[ i ].i;

            int lastNBits = 0;
            int pos = 0;
            for( i=0; i < 16 ; ++i )
                {
                for( n=0 ;; ++n )
                    if( _get( src, endSrc, 1 ) )
                        break;
                lastNBits += n;

                groupTable[ i ].nBits = lastNBits;
                groupTable[ i ].pos = pos;
                pos += 1 << lastNBits;
                }
            assert( pos < NHUFFSYMBOLS + 255 );

            continue;//forever
            }
        else if( symbol == NHUFFSYMBOLS - 1 )
            break;//forever

        static struct MatchOverItem { int nExtraBits; int base; } _matchOverTable[] =
            {
            { 1,   8 },
            { 2,  10 },
            { 3,  14 },
            { 4,  22 },
            { 5,  38 },
            { 6,  70 },
            { 7, 134 },
            { 8, 262 }
            };
        if( symbol < 256 + 8 )
            matchOver = symbol - 256;
        else
            {
            MatchOverItem* item = &_matchOverTable[ symbol - 256 - 8 ];
            int extra = _get( src, endSrc, item->nExtraBits );
            if( extra < 0 )
                return FALSE;
            matchOver = item->base + extra;
            }

        int dispPrefix = _get( src, endSrc, 3 );
        if( dispPrefix < 0 )
            return FALSE;

        static struct DispItem { int nBits; int disp; } _dispTable[] =
            {
            { 0,  0 },
            { 0,  1 },
            { 1,  2 },
            { 2,  4 },
            { 3,  8 },
            { 4, 16 },
            { 5, 32 },
            { 6, 64 }
            };
        DispItem* item = &_dispTable[ dispPrefix ];
        nBits = item->nBits + LZBUFBITS - 7;

        int disp = 0;
        assert( nBits <= 16 );
        if( nBits > 8 )
            {
            nBits -= 8;
            disp |= _get( src, endSrc, 8 ) << nBits;
            }
        assert( nBits <= 8 );
        int got = _get( src, endSrc, nBits );
        if( got < 0 )
            return FALSE;
        disp |= got;

        disp += item->disp << (LZBUFBITS - 7);
        assert( disp >=0 && disp < LZBUFSIZE );

        int matchLen = matchOver + LZMIN;
        if( dst + matchLen > endDst )
            return FALSE;
        int pos = bufPos - disp;
        if( matchLen < disp )
            _bufCpy( dst, pos, matchLen );
        else
            {
            _bufCpy( dst, pos, disp );
            for( i=0; i < matchLen - disp; ++i )
                dst[ i + disp ] = dst[ i ];
            }
        _toBuf( dst, matchLen );
        dst += matchLen;
        }

    if( dstSz )
        *dstSz -= dst - startDst;
    if( srcSz )
        *srcSz -= src - startSrc;

	return TRUE;
    }
