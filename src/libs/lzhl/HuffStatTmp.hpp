#ifndef  LZHL_HuffStatTmp_HPP
#define  LZHL_HuffStatTmp_HPP

#include "LZHMacro.hpp"

class HuffStatTmpStruct
{
public:
  HUFFINT i;
  HUFFINT n;
};

void shellSort( HuffStatTmpStruct* a, int N );

#endif
