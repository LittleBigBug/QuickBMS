#include "LZHLDecompressor.hpp"
#include <cassert>

LZHLDecompressor::LZHLDecompressor() {
  nBits = 0;
  bits = 0;
}

LZHLDecompressor::~LZHLDecompressor() { }

inline int LZHLDecompressor::_get( const uint8_t*& src, const uint8_t* srcEnd, int n )
{
  assert( n <= 8 );
  if ( nBits < n ) {
    if ( src >= srcEnd ) {
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

bool LZHLDecompressor::decompress( uint8_t* dst, size_t* dstSz, const uint8_t* src, size_t* srcSz )
{
  uint8_t* startDst = dst;
  const uint8_t* startSrc = src;
  const uint8_t* endSrc = src + *srcSz;
  const uint8_t* endDst = dst + *dstSz;
  nBits = 0;

  for (;;) {
    int grp = _get( src, endSrc, 4 );
    if ( grp < 0 ) {
      return false;
    }

    Group& group = groupTable[ grp ];

    HUFFINT symbol;
    int nBits = group.nBits;

    if ( nBits == 0 ) {
      symbol = symbolTable[ group.pos ];
    } else {
      assert( nBits <= 8 );
      int got = _get( src, endSrc, nBits );

      if ( got < 0 ) {
        return false;
      }

      int pos = group.pos + got;

      if ( pos >= NHUFFSYMBOLS ) {
        return false;
      }

      symbol = symbolTable[ pos ];
    }

    assert( symbol < NHUFFSYMBOLS );
    ++stat[ symbol ];

    int matchOver;

    if ( symbol < 256 ) {
      if ( dst >= endDst ) {
        return false;
      }

      *dst++ = (uint8_t)symbol;
      _toBuf( (uint8_t)symbol );
      continue; //forever

    } else if ( symbol == NHUFFSYMBOLS - 2 ) {
      HuffStatTmpStruct s[ NHUFFSYMBOLS ];
      makeSortedTmp( s );

      for ( int i=0; i < NHUFFSYMBOLS ; ++i )
        symbolTable[ i ] = s[ i ].i;

      int lastNBits = 0;
      int pos = 0;

      for ( int i=0; i < 16 ; ++i ) {

        int n;
        for ( n=0 ;; ++n )
          if ( _get( src, endSrc, 1 ) )
            break;

        lastNBits += n;
        groupTable[ i ].nBits = lastNBits;
        groupTable[ i ].pos   = pos;

        pos += 1 << lastNBits;
      }

      assert( pos < NHUFFSYMBOLS + 255 );
      continue;  //forever


    } else if ( symbol == NHUFFSYMBOLS - 1 )
      break;    //forever

    static struct MatchOverItem {
      int nExtraBits;
      int base;
    } _matchOverTable[] = {
      { 1,   8 },
      { 2,  10 },
      { 3,  14 },
      { 4,  22 },
      { 5,  38 },
      { 6,  70 },
      { 7, 134 },
      { 8, 262 }
    };

    if ( symbol < 256 + 8 ) {
      matchOver = symbol - 256;
    } else {
      MatchOverItem* item = &_matchOverTable[ symbol - 256 - 8 ];
      int extra = _get( src, endSrc, item->nExtraBits );

      if ( extra < 0 ) {
        return false;
      }

      matchOver = item->base + extra;
    }

    int dispPrefix = _get( src, endSrc, 3 );

    if ( dispPrefix < 0 ) {
      return false;
    }

    static struct DispItem {
      int nBits;
      int disp;
    } _dispTable[] = {
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

    if ( nBits > 8 ) {
      nBits -= 8;
      disp |= _get( src, endSrc, 8 ) << nBits;
    }

    assert( nBits <= 8 );
    int got = _get( src, endSrc, nBits );

    if ( got < 0 ) {
      return false;
    }

    disp |= got;
    disp += item->disp << (LZBUFBITS - 7);
    assert( disp >=0 && disp < LZBUFSIZE );

    int matchLen = matchOver + LZMIN;

    if ( dst + matchLen > endDst ) {
      return false;
    }

    int pos = bufPos - disp;
    if ( matchLen < disp ) {
      _bufCpy( dst, pos, matchLen );
    } else {
      _bufCpy( dst, pos, disp );

      for ( int i=0; i < matchLen - disp; ++i ) {
        dst[ i + disp ] = dst[ i ];
      }
    }

    _toBuf( dst, matchLen );
    dst += matchLen;

  } // forever

  if ( dstSz )
    *dstSz = dst - startDst;

  if ( srcSz )
    *srcSz = src - startSrc;

  return true;
}
