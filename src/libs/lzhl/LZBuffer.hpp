#ifndef  LZHL_LZBuffer_HPP
#define  LZHL_LZBuffer_HPP

#include "LZHMacro.hpp"
#include <cstddef>

class LZBuffer {
public:
  LZBuffer();
  ~LZBuffer();

protected:
  static LZPOS _wrap( LZPOS pos );
  static int _distance( int diff );

  void _toBuf( uint8_t );
  void _toBuf( const uint8_t*, size_t sz );
  void _bufCpy( uint8_t* dst, LZPOS pos, size_t sz );
  LZPOS _nMatch( LZPOS pos, const uint8_t* p, LZPOS nLimit );

public:
  uint8_t* buf;
  LZPOS bufPos;
};

#endif
