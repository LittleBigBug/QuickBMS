#ifndef  LZHL_LZHLEncoderStat_HPP
#define  LZHL_LZHLEncoderStat_HPP

#include "HuffStat.hpp"
#include "LZHMacro.hpp"

class LZHLEncoderStat : public HuffStat {
public:
  struct Symbol {
    HUFFINT  nBits;
    HUFFUINT code;
  };

public:
  LZHLEncoderStat();
  ~LZHLEncoderStat();

public:
  void calcStat( HUFFINT* groups );

private:
  inline static void _addGroup( HUFFINT* groups, HUFFINT group, HUFFINT nBits );

public:
  int nextStat;

  static Symbol symbolTable0[];
  Symbol* symbolTable;
};

#endif
