#include "HuffStat.hpp"
#include <cstring>

HuffStat::HuffStat()
{
  stat = new HUFFINT[ NHUFFSYMBOLS ];
  memset( stat, 0, sizeof(HUFFINT) * NHUFFSYMBOLS );
}

HuffStat::~HuffStat()
{
  delete [] stat;
}

int HuffStat::makeSortedTmp( HuffStatTmpStruct* s )
{
  int total = 0;
  for( HUFFINT j = 0; j < NHUFFSYMBOLS ; j++ ) {
    s[ j ].i = j;
    s[ j ].n = stat[ j ];
    total += stat[ j ];
    stat[ j ] = HUFFRECALCSTAT( stat[ j ] );
  }

  //qsort( s, NHUFFSYMBOLS, sizeof(HuffStatTmpStruct), _cmpStat );
  shellSort( s - 1, NHUFFSYMBOLS );
  return total;
}
