#include "LZHLDecoderStat.hpp"
#include <cstring>

LZHLDecoderStat::LZHLDecoderStat() {
  symbolTable = new HUFFINT[ NHUFFSYMBOLS ];
  memcpy( symbolTable, symbolTable0, sizeof(HUFFINT)*NHUFFSYMBOLS );
  memcpy( groupTable,  groupTable0,  sizeof(Group)*16 );
}

LZHLDecoderStat::~LZHLDecoderStat() {
  delete [] symbolTable;
}

LZHLDecoderStat::Group LZHLDecoderStat::groupTable0[ 16 ] =  {
  #include "Table/Hdec_g.tbl"
};

HUFFINT LZHLDecoderStat::symbolTable0[ NHUFFSYMBOLS ] =  {
  #include "Table/Hdec_s.tbl"
};
