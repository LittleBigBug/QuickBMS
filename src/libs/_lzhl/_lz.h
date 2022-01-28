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

#define LZPOS UINT32
#define LZBUFMASK ( (LZBUFSIZE) - 1 )

#define LZTABLEINT UINT16
typedef LZTABLEINT LZTableItem;

#define LZHASH UINT32

//*****************************************************************************

class LZBuffer
    {
    protected:
    BYTE* buf;
    LZPOS bufPos;

    protected:
    inline LZBuffer();
    inline ~LZBuffer();

    protected:
    inline static int _wrap( LZPOS pos );
    inline static int _distance( int diff );

    inline void _toBuf( BYTE );
    inline void _toBuf( const BYTE*, size_t sz );
    inline void _bufCpy( BYTE* dst, int pos, size_t sz );
    inline int _nMatch( int pos, const BYTE* p, int nLimit );
    };

//*****************************************************************************

class LZHLCompressor : private LZBuffer
    {
    private:
    LZHLEncoderStat stat;
    LZTableItem* table;

    public:
    LZHLCompressor();
    ~LZHLCompressor();

    public:
    static size_t calcMaxBuf( size_t rawSz )
        {
        return LZHLEncoder::calcMaxBuf( rawSz );
        }
    size_t compress( BYTE* dst, const BYTE* src, size_t sz );

    private:
    void _wrapTable();
    LZHASH _updateTable( LZHASH hash, const BYTE* src, LZPOS pos, int len );
    };

class LZHLDecompressor : private LZBuffer, private LZHLDecoderStat
    {
    private:
    UINT32 bits;
    int nBits;

    public:
    LZHLDecompressor();
    ~LZHLDecompressor();
    BOOL decompress( BYTE* dst, size_t* dstSz, const BYTE* src, size_t* srcSz );

    private:
    inline int _get( const BYTE*& src, const BYTE* srcEnd, int n );
    };

//*****************************************************************************

inline LZBuffer::LZBuffer()
    {
    buf= new BYTE[ LZBUFSIZE ];
    bufPos = 0;
    }

inline LZBuffer::~LZBuffer()
    {
    delete [] buf;
    }

inline /* static */ int LZBuffer::_wrap( LZPOS pos )
    {
    return pos & LZBUFMASK;
    }

inline /* static */ int LZBuffer::_distance( int diff )
    {
    return diff & LZBUFMASK;
    }

inline void LZBuffer::_toBuf( BYTE c )
    {
    buf[ _wrap( bufPos++ ) ] = c;
    }

inline void LZBuffer::_toBuf( const BYTE* src, size_t sz )
    {
    assert( sz < LZBUFSIZE );
    int begin = _wrap( bufPos );
    int end = begin + sz;
    if( end > LZBUFSIZE )
        {
        size_t left = LZBUFSIZE - begin;
        memcpy( buf + begin, src, left );
        memcpy( buf, src + left, sz - left );
        }
    else
        memcpy( buf + begin, src, sz );
    bufPos += sz;
    }

inline void LZBuffer::_bufCpy( BYTE* dst, int pos, size_t sz )
    {
    assert( sz < LZBUFSIZE );
    int begin = _wrap( pos );
    int end = begin + sz;
    if( end > LZBUFSIZE )
        {
        size_t left = LZBUFSIZE - begin;
        memcpy( dst, buf + begin, left );
        memcpy( dst + left, buf, sz - left );
        }
    else
        memcpy( dst, buf + begin, sz );
    }
