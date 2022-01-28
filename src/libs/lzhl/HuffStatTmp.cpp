#include "HuffStatTmp.hpp"
#include <cassert>

inline int cmpHuffStatTmpStruct( const HuffStatTmpStruct& a,
                                 const HuffStatTmpStruct& b )
{
  int cmp = b.n - a.n;
  return ( cmp ? cmp : b.i - a.i );
}

inline int operator<( const HuffStatTmpStruct& a,
                      const HuffStatTmpStruct& b )
{
  return cmpHuffStatTmpStruct( a, b ) < 0;
}

//static int __cdecl _cmpStat( const void* a_, const void* b_ ) {
//  HuffStatTmpStruct* a = (HuffStatTmpStruct*)a_;
//  HuffStatTmpStruct* b = (HuffStatTmpStruct*)b_;
//  return cmpHuffStatTmpStruct( *a, *b );
//}

void shellSort( HuffStatTmpStruct* a, int N ) {
  int i, j;
  HuffStatTmpStruct v;

  /*
     for ( int h = 1; h <= N/9; h = ( 3 * h + 1 ) )// determine the value for h
     ;
   */

  assert( 13 <= N / 9 );
  assert( 40 > N / 9 );

  int h = 40;

  for ( ; h > 0; h /= 3 ) {       // h = 40, 13, 4, 1
    for ( i = h + 1; i <= N; ++i ) {
      v = a[i];
      j = i;

      while ( ( j > h ) && ( v < a[j - h] ) ) {
        a[j] = a[j-h];
        j -= h;
      }

      a[j] = v;
    }
  }
}
