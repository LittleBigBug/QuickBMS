/*
 *  LZH-Light algorithm implementation v 1.0
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
 *
 */

#ifndef LZHLINTERNAL
#error This file is for LZHL internal use only
#endif

#include "_lzhl.h"
#define HUFFRECALCSTAT( s ) ( (s) >> 1 )

//*****************************************************************************

#define HUFFINT   INT16
#define HUFFUINT UINT16
#define NHUFFSYMBOLS ( 256 + 16 + 2 )

struct HuffStatTmpStruct
    {
    HUFFINT i;
    HUFFINT n;
    };

class HuffStat
    {
    public:
    HUFFINT* stat;

    public:
    HuffStat();
    ~HuffStat();

    protected:
    int makeSortedTmp( HuffStatTmpStruct* );
    };

class LZHLEncoderStat : public HuffStat
    {
    public:
    struct Symbol { HUFFINT nBits; HUFFUINT code; };

    public:
    int nextStat;

    static Symbol symbolTable0[];
    Symbol* symbolTable;

    public:
    LZHLEncoderStat();
    ~LZHLEncoderStat();

    public:
    void calcStat( int* groups );

    private:
    inline static void _addGroup( int* groups, int group, int nBits );
    };

class LZHLEncoder
    {
    public:
    enum { maxMatchOver = 517, maxRaw = 64 };

    private:
    LZHLEncoderStat* stat;
    HUFFINT* sstat;
    int& nextStat;

    BYTE* dst;
    BYTE* dstBegin;
    UINT32 bits;
    int nBits;

    public:
    static size_t calcMaxBuf( size_t rawSz )
        {
        return rawSz + ( rawSz >> 1 ) + 32;
        }

    public:
    inline LZHLEncoder( LZHLEncoderStat* stat_, BYTE* dst_ );
    inline ~LZHLEncoder();
    size_t flush();

    void putRaw( const BYTE* src, size_t sz );
    void putMatch( const BYTE* src, size_t nRaw, size_t matchOver, size_t disp );

    private:
    void _callStat();

    inline void _put( UINT16 symbol );
    inline void _put( UINT16 symbol, int codeBits, UINT32 code );
    inline void _putBits( int codeBits, UINT32 code );
    };

class LZHLDecoderStat : public HuffStat
    {
    public:
    struct Group { int nBits; int pos; };

    public:
    static Group groupTable0[];
    Group groupTable[ 16 ];

    static HUFFINT symbolTable0[];
    HUFFINT* symbolTable;

    public:
    LZHLDecoderStat();
    ~LZHLDecoderStat();
    };

//*****************************************************************************

inline LZHLEncoder::LZHLEncoder( LZHLEncoderStat* stat_, BYTE* dst_ )
: stat( stat_ ),
  sstat( stat_->stat ),
  nextStat( stat_->nextStat )
    {
    dst = dstBegin = dst_;
    bits = 0;
    nBits = 0;
    }

inline LZHLEncoder::~LZHLEncoder()
    {
    }

inline void LZHLEncoder::_putBits( int codeBits, UINT32 code )
    {
    assert( codeBits <= 16 );
    bits |= ( code << ( 32 - nBits - codeBits ) );
    nBits += codeBits;
    if( nBits >= 16 )
        {
        *dst++ = (BYTE)( bits >> 24 );
        *dst++ = (BYTE)( bits >> 16 );
        nBits -= 16;
        bits <<= 16;
        }
    }

inline void LZHLEncoder::_put( UINT16 symbol )
    {
    assert( symbol < NHUFFSYMBOLS );
    if( --nextStat <= 0 )
        _callStat();

    ++sstat[ symbol ];

    LZHLEncoderStat::Symbol* item = &stat->symbolTable[ symbol ];
    assert( item->nBits >= 0 );

    _putBits( item->nBits, item->code );
    }

inline void LZHLEncoder::_put( UINT16 symbol, int codeBits, UINT32 code )
    {
    assert( symbol < NHUFFSYMBOLS );
    assert( codeBits <= 4 );
    if( --nextStat <= 0 )
        _callStat();

    ++sstat[ symbol ];

    LZHLEncoderStat::Symbol* item = &stat->symbolTable[ symbol ];
    assert( item->nBits >= 0 );

    int nBits = item->nBits;
    _putBits( nBits + codeBits, ( item->code << codeBits ) | code );
    }
