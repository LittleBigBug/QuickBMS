#include "LZHLCompressor.hpp"
#include <cassert>
#include <algorithm>

inline LZHASH _calcHash( const uint8_t* src )
{
  LZHASH hash = 0;
  const uint8_t* pEnd = src + LZMATCH;
  for( const uint8_t* p = src; p < pEnd ; )
  {
    UPDATE_HASH( hash, *p++ );
  }
  return hash;
}

LZHLCompressor::LZHLCompressor() {
  table = new LZTableItem[ LZTABLESIZE ];
  for ( int i=0; i < LZTABLESIZE ; ++i ) {
    table[ i ] = (LZTABLEINT)(-1);
  }
}

LZHLCompressor::~LZHLCompressor() {
  delete [] table;
}

inline LZHASH LZHLCompressor::_updateTable( LZHASH hash, const uint8_t* src, LZPOS pos, ptrdiff_t len )
{
  if ( len <= 0 )
    return 0;

  if ( len > LZSKIPHASH ) {
    ++src;
    hash = 0;
    const uint8_t* pEnd = src + len + LZMATCH;

    for ( const uint8_t* p=src+len; p < pEnd ; ) {
      UPDATE_HASH( hash, *p++ );
    }

    return hash;
  }

  UPDATE_HASH_EX( hash, src );
  ++src;

  for ( int i=0; i < len ; ++i ) {
    table[ HASH_POS( hash ) ] = (LZTableItem)_wrap( pos + i );
    UPDATE_HASH_EX( hash, src + i );
  }

  return hash;
}

size_t LZHLCompressor::compress( uint8_t* dst, const uint8_t* src, size_t sz ) {
  LZHLEncoder coder( &stat, dst );
  // (unused) const uint8_t* srcBegin = src;
  const uint8_t* srcEnd = src + sz;

  LZHASH hash = 0;

  if ( sz >= LZMATCH ) {
    const uint8_t* pEnd = src + LZMATCH;

    for ( const uint8_t* p=src; p < pEnd ; ) {
      UPDATE_HASH( hash, *p++ );
    }
  }

  for (;;) {
    ptrdiff_t srcLeft = srcEnd - src;
    if ( srcLeft < LZMATCH ) {
      if ( srcLeft ) {
        _toBuf( src, srcLeft );
        coder.putRaw( src, srcLeft );
      }

      break;  //forever
    }

    ptrdiff_t nRaw = 0;
    ptrdiff_t maxRaw = std::min( srcLeft - LZMATCH, (ptrdiff_t)LZHLEncoder::maxRaw );

#ifdef LZLAZYMATCH
    int    lazyMatchLen = 0;
    int    lazyMatchHashPos = 0;
    LZPOS  lazyMatchBufPos = 0;
    ptrdiff_t    lazyMatchNRaw = 0;
    LZHASH lazyMatchHash = 0;
    bool   lazyForceMatch = false;
#endif
    for (;;) {
      LZHASH hash2 = HASH_POS( hash );

      LZPOS hashPos = table[ hash2 ];
      LZPOS wrapBufPos = _wrap( bufPos );
      table[ hash2 ] = (LZTableItem)wrapBufPos;

      int matchLen = 0;
      if ( hashPos != (LZTABLEINT)(-1) && hashPos != wrapBufPos )
      {
        int matchLimit = std::min( std::min( _distance( wrapBufPos - hashPos ), (int)(srcLeft - nRaw) ), LZMIN + LZHLEncoder::maxMatchOver );
        matchLen = _nMatch( hashPos, src + nRaw, matchLimit );

#ifdef LZOVERLAP
        if ( _wrap( hashPos + matchLen ) == wrapBufPos )
        {
          assert( matchLen != 0 );
          ptrdiff_t xtraMatchLimit = std::min( LZMIN + (ptrdiff_t)LZHLEncoder::maxMatchOver - matchLen, srcLeft - nRaw - matchLen );
          int xtraMatch;
          for ( xtraMatch = 0; xtraMatch < xtraMatchLimit ; ++xtraMatch )
          {
            if ( src[ nRaw + xtraMatch ] != src[ nRaw + xtraMatch + matchLen ] )
              break;//for ( xtraMatch )
          }

          matchLen += xtraMatch;
        }
#endif

#ifdef LZBACKWARDMATCH
        if ( matchLen >= LZMIN - 1 )//to ensure that buf will be overwritten
        {
          int xtraMatchLimit = (int)std::min( LZMIN + LZHLEncoder::maxMatchOver - (ptrdiff_t)matchLen, nRaw );
          int d = (int)_distance( bufPos - hashPos );
          xtraMatchLimit = std::min( std::min( xtraMatchLimit, d - matchLen ), LZBUFSIZE - d );
          int xtraMatch;
          for ( xtraMatch = 0; xtraMatch < xtraMatchLimit ; ++xtraMatch )
          {
            if ( buf[ _wrap( hashPos - xtraMatch - 1 ) ] != src[ nRaw - xtraMatch - 1 ] )
              break;//for ( xtraMatch )
          }

          if ( xtraMatch > 0 ) {
            assert( matchLen + xtraMatch >= LZMIN );
            assert( matchLen + xtraMatch <= _distance( bufPos - hashPos ) );

            nRaw -= xtraMatch;
            bufPos -= xtraMatch;
            hashPos -= xtraMatch;
            matchLen += xtraMatch;
            wrapBufPos = _wrap( bufPos );
            hash = _calcHash( src + nRaw );

#ifdef LZLAZYMATCH
            lazyForceMatch = true;
#endif
          }
        }
#endif
      }

#ifdef LZLAZYMATCH
      if ( lazyMatchLen >= LZMIN ) {
        if ( matchLen > lazyMatchLen ) {
          coder.putMatch( src, nRaw, matchLen - LZMIN, _distance( wrapBufPos - hashPos ) );
          hash = _updateTable( hash, src + nRaw, bufPos + 1, std::min( (ptrdiff_t)matchLen - 1, srcEnd - (src + nRaw + 1) - LZMATCH ) );
          _toBuf( src + nRaw, matchLen );
          src += nRaw + matchLen;
          break;//for ( nRaw )

        } else {
          nRaw = lazyMatchNRaw;
          bufPos = lazyMatchBufPos;

          hash = lazyMatchHash;
          UPDATE_HASH_EX( hash, src + nRaw );
          coder.putMatch( src, nRaw, lazyMatchLen - LZMIN, _distance( bufPos - lazyMatchHashPos ) );
          hash = _updateTable( hash, src + nRaw + 1, bufPos + 2, std::min( (ptrdiff_t)lazyMatchLen - 2, srcEnd - (src + nRaw + 2) - LZMATCH ) );
          _toBuf( src + nRaw, lazyMatchLen );
          src += nRaw + lazyMatchLen;

          break;//for ( nRaw )
        }
      }
#endif

      if ( matchLen >= LZMIN ) {

#ifdef LZLAZYMATCH
        if ( !lazyForceMatch ) {
          lazyMatchLen = matchLen;
          lazyMatchHashPos = hashPos;
          lazyMatchNRaw = nRaw;
          lazyMatchBufPos = bufPos;
          lazyMatchHash = hash;
        } else
#endif
        {
          coder.putMatch( src, nRaw, matchLen - LZMIN, _distance( wrapBufPos - hashPos ) );
          hash = _updateTable( hash, src + nRaw, bufPos + 1, std::min( (ptrdiff_t)matchLen - 1, srcEnd - (src + nRaw + 1) - LZMATCH ) );
          _toBuf( src + nRaw, matchLen );
          src += nRaw + matchLen;

          break;//for ( nRaw )
        }
      }

#ifdef LZLAZYMATCH
      assert( !lazyForceMatch );
#endif

      if ( nRaw + 1 > maxRaw )
      {
#ifdef LZLAZYMATCH
        if ( lazyMatchLen >= LZMIN )
        {
          coder.putMatch( src, nRaw, lazyMatchLen - LZMIN, _distance( bufPos - lazyMatchHashPos ) );
          hash = _updateTable( hash, src + nRaw, bufPos + 1, std::min( (ptrdiff_t)lazyMatchLen - 1, srcEnd - (src + nRaw + 1) - LZMATCH ) );
          _toBuf( src + nRaw, lazyMatchLen );
          src += nRaw + lazyMatchLen;
          break;//for ( nRaw )
        }
#endif

        if ( nRaw + LZMATCH >= srcLeft && srcLeft <= LZHLEncoder::maxRaw )
        {
          _toBuf( src + nRaw, srcLeft - nRaw );
          nRaw = srcLeft;
        }

        coder.putRaw( src, nRaw );
        src += nRaw;
        break;//for ( nRaw )
      }

      UPDATE_HASH_EX( hash, src + nRaw );
      _toBuf( src[ nRaw++ ] );
    }//for ( nRaw )
  }//forever

  return coder.flush();
}
