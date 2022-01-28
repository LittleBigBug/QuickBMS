#include "LZHLEncoder.hpp"
#include <cassert>

const int LZHLEncoder::maxRaw;
const int LZHLEncoder::maxMatchOver;

LZHLEncoder::LZHLEncoder( LZHLEncoderStat* stat_, uint8_t* dst_ )
: stat( stat_ ),
  sstat( stat_->stat ),
  nextStat( stat_->nextStat ),
  dst( dst_ ),
  dstBegin( dst_ ),
  bits( 0 ),
  nBits( 0 )
{ }

LZHLEncoder::~LZHLEncoder() { }

void LZHLEncoder::_callStat() {
  nextStat = 2;   // to avoid recursion, >=2

  _put( NHUFFSYMBOLS - 2 );

  HUFFINT groups[ 16 ];
  stat->calcStat( groups );

  int lastNBits = 0;

  for( int i=0; i < 16 ; ++i ) {
    int nBits = groups[ i ];
    assert( nBits >= lastNBits && nBits <= 8 );
    int delta = nBits - lastNBits;
    lastNBits = nBits;
    _putBits( delta + 1, 1 );
  }
}

void LZHLEncoder::putRaw( const uint8_t* src, size_t sz ) {
  for( const uint8_t* srcEnd = src + sz; src < srcEnd ; ++src ) {
    _put( *src );
  }
}

void LZHLEncoder::putMatch( const uint8_t* src, size_t nRaw, size_t matchOver, size_t disp )
{
  assert( nRaw <= maxRaw );
  assert( matchOver <= maxMatchOver );
  assert( disp >= 0 && disp < LZBUFSIZE );
  putRaw( src, nRaw );
  struct MatchOverItem { uint16_t symbol; int nBits; uint16_t bits; };

  static MatchOverItem _matchOverTable[] = {
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

  if ( matchOver < 8 ) {
    _put( 256 + (uint16_t)matchOver );

  } else if ( matchOver < 38 ) {
    matchOver -= 8;
    MatchOverItem* item = &_matchOverTable[ matchOver >> 1 ];
    _put( item->symbol, item->nBits, item->bits | (matchOver & 0x01) );

  } else {
    matchOver -= 38;
    MatchOverItem* item = &_matchOverTable[ matchOver >> 5 ];
    _put( item->symbol + 4 );
    _putBits( item->nBits + 4, ( item->bits << 4 ) | (matchOver & 0x1F) );
  }

  static struct DispItem { int nBits; uint16_t bits; } _dispTable[] = {
#include "Table/Hdisp.tbl"
  };

#if LZBUFBITS < 8
#error
#endif

  DispItem* item = &_dispTable[ disp >> (LZBUFBITS - 7) ];
  int nBits = item->nBits + (LZBUFBITS - 7);
  uint32_t bits = ( ((uint32_t)item->bits) << (LZBUFBITS - 7) ) | ( disp & ( ( 1 << (LZBUFBITS - 7) ) - 1 ) );

#if LZBUFBITS >= 15
  if ( nBits > 16 ) {
    assert( nBits <= 32 );
    _putBits( nBits - 16, bits >> 16 );
    _putBits( 16, bits & 0xFFFF );

  } else
#endif

  {
    assert( nBits <= 16 );
    _putBits( nBits, bits );
  }
}

size_t LZHLEncoder::flush() {
  _put( NHUFFSYMBOLS - 1 );

  while( nBits > 0 ) {
    *dst++ = (uint8_t)( bits >> 24 );
    nBits -= 8;
    bits <<= 8;
  }

  return dst - dstBegin;
}

void LZHLEncoder::_putBits( int codeBits, uint32_t code ) {
  assert( codeBits <= 16 );
  bits |= ( code << ( 32 - nBits - codeBits ) );
  nBits += codeBits;

  if ( nBits >= 16 ) {
    *dst++ = (uint8_t)( bits >> 24 );
    *dst++ = (uint8_t)( bits >> 16 );

    nBits -= 16;
    bits <<= 16;
  }
}

void LZHLEncoder::_put( uint16_t symbol ) {
  assert( symbol < NHUFFSYMBOLS );

  if ( --nextStat <= 0 ) {
    _callStat();
  }

  ++sstat[ symbol ];

  LZHLEncoderStat::Symbol* item = &stat->symbolTable[ symbol ];
  assert( item->nBits >= 0 );

  _putBits( item->nBits, item->code );
}

void LZHLEncoder::_put( uint16_t symbol, int codeBits, uint32_t code ) {
  assert( symbol < NHUFFSYMBOLS );
  assert( codeBits <= 4 );

  if ( --nextStat <= 0 ) {
    _callStat();
  }

  ++sstat[ symbol ];

  LZHLEncoderStat::Symbol* item = &stat->symbolTable[ symbol ];
  assert( item->nBits >= 0 );

  int nBits = item->nBits;
  _putBits( nBits + codeBits, ( item->code << codeBits ) | code );
}
