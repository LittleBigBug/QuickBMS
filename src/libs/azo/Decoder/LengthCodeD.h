#ifndef AZO_DECODER_LENGTHCODE_H
#define AZO_DECODER_LENGTHCODE_H

#include "PredictProbD.h"
#include "PredictProbD.h"

namespace AZO {
namespace Decoder {

class LengthCode
{
public:
    LengthCode() {}
    
    u_int Code(EntropyCode& entropy, uint8_t distCode);

private:    
    PredictProb<MATCH_DIST_CODE_SIZE, MATCH_LENGTH_CODE_SIZE, LENGTHCODE_PREDICT_SHIFT> prob_;    
};

} //namespaces Decoder
} //namespaces AZO

#include "LengthCodeD.cpp"

#endif /*AZO_DECODER_LENGTHCODE_H*/
