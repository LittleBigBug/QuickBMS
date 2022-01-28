#ifndef  LZHL_HuffStat_HPP
#define  LZHL_HuffStat_HPP

#include "LZHMacro.hpp"
#include "HuffStatTmp.hpp"

class HuffStat {
public:
  HuffStat();
  virtual ~HuffStat();
public:
  HUFFINT* stat;

protected:
  int makeSortedTmp( HuffStatTmpStruct* );

};

#endif
