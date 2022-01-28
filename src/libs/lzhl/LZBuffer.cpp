#include "LZBuffer.hpp"
#include <cassert>
#include <cstring>

LZBuffer::LZBuffer()
{
  buf = new uint8_t[ LZBUFSIZE ];
  bufPos = 0;
}

LZBuffer::~LZBuffer()
{
  delete [] buf;
}

LZPOS LZBuffer::_wrap( LZPOS pos )
{
  return ( pos & LZBUFMASK );
}

int LZBuffer::_distance( int diff )
{
  return ( diff & LZBUFMASK );
}

void LZBuffer::_toBuf( uint8_t c )
{
  buf[ _wrap( bufPos++ ) ] = c;
}

void LZBuffer::_toBuf( const uint8_t* src, size_t sz )
{
  assert( sz < LZBUFSIZE );
  LZPOS begin = _wrap( bufPos );
  LZPOS end = begin + (LZPOS)sz;

  if ( end > LZBUFSIZE )
  {
    size_t left = LZBUFSIZE - begin;
    memcpy( buf + begin, src, left );
    memcpy( buf, src + left, sz - left );
  }
  else
  {
    memcpy( buf + begin, src, sz );
  }

  bufPos += sz;
}

void LZBuffer::_bufCpy( uint8_t* dst, LZPOS pos, size_t sz )
{
  assert( sz < LZBUFSIZE );
  LZPOS begin = _wrap( pos );
  LZPOS end = begin + (LZPOS)sz;

  if ( end > LZBUFSIZE )
  {
    size_t left = LZBUFSIZE - begin;
    memcpy( dst, buf + begin, left );
    memcpy( dst + left, buf, sz - left );
  }
  else
  {
    memcpy( dst, buf + begin, sz );
  }
}

LZPOS LZBuffer::_nMatch( LZPOS pos, const uint8_t* p, LZPOS nLimit )
{
  assert( nLimit < LZBUFSIZE );
  LZPOS begin = pos;
  if ( LZBUFSIZE - begin >= nLimit )
  {
    for ( LZPOS i = 0; i < nLimit ; i++ )
      if ( buf[ begin + i ] != p[ i ] )
        return i;

    return nLimit;

  }
  else
  {
    for ( LZPOS i = begin; i < LZBUFSIZE ; i++ )
      if ( buf[ i ] != p[ i - begin ] )
        return i - begin;

    LZPOS shift = LZBUFSIZE - begin;
    LZPOS n = nLimit - shift;

    for( LZPOS i = 0; i < n ; i++ )
      if( buf[ i ] != p[ shift + i ] )
        return shift + i;

    return nLimit;
  }
}
