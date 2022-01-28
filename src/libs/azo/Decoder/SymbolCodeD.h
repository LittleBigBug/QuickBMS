#ifndef AZO_DECODER_SYMBOLCODE_H
#define AZO_DECODER_SYMBOLCODE_H

#include "HistoryListD.h"
#include "EntropyBitProbD.h"

namespace AZO {
namespace Decoder {

template <typename T, u_int N, u_int HISTORY_N>
class SymbolCode
{
public:
    SymbolCode() {}
    
    T Code(EntropyCode& entropy);

private:
    HistoryList<T, HISTORY_N>   history_; 
    EntropyBitProb<N>           prob_;
};

} //namespaces Decoder
} //namespaces AZO

#include "SymbolCodeD.cpp"

#endif /*AZO_DECODER_SYMBOLCODE_H*/
