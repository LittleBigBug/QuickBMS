#ifndef AZO_DECODER_MATCHCODE_H
#define AZO_DECODER_MATCHCODE_H

#include "DictionaryTableD.h"
#include "DistanceCodeD.h"
#include "LengthCodeD.h"

namespace AZO {
namespace Decoder {

class MatchCode
{
public:
    MatchCode(const byte* buf, u_int size) : 
        dictTable_(buf, size) {}
    
    void Code(EntropyCode& entropy, u_int pos, 
              u_int& dist, u_int& length);

private:    
    DictionaryTable    dictTable_;
    DistanceCode       distProb_;
    LengthCode         lenProb_;
};

} //namespaces Decoder
} //namespaces AZO

#include "MatchCodeD.cpp"

#endif /*AZO_DECODER_MATCHCODE_H*/
