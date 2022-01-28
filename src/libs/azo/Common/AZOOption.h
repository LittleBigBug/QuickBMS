#ifndef AZO_AZOOPTION_H
#define AZO_AZOOPTION_H

#include "AZOType.h"

namespace AZO {

const u_int AZO_PRIVATE_VERSION         (1);

const u_int COMPRESSION_REDUCE_MIN_SIZE (8);

const u_int ALPHA_SIZE                  (1<<8);
const u_int ALPHA_HISTORY_SIZE          (1);

const u_int MATCH_LENGTH_SGAP		    (32);
const u_int MATCH_LENGTH_GAP			(8);
const u_int MATCH_DIST_SGAP			    (16);
const u_int MATCH_DIST_GAP			    (4);

const u_int MATCH_MIN_LENGTH			(2);
const u_int MATCH_MIN_DIST			    (1);

const u_int MATCH_LENGTH_CODE_SIZE      (1<<7);
const u_int MATCH_DIST_CODE_SIZE        (1<<7);

const u_int MATCH_HASH_LEVEL			(5);
const u_int MATCH_HASH_BITSIZE			(22);

const u_int DISTANCE_HISTORY_SIZE       (1<<1);

const u_int DICTIONARY_SIZE             (1<<7);
const u_int DICTIONARY_HISTORY_SIZE     (2);

const u_int ALPHACODE_PREDICT_SHIFT     (5);
const u_int LENGTHCODE_PREDICT_SHIFT    (4);

} //namespaces AZO

#endif /*AZO_AZOOPTION_H*/
