/*
 * DictionaryTableD.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_DECODER_DICTIONARYTABLE_H
#define AZO_DECODER_DICTIONARYTABLE_H

#include "../Base/DictionaryTableB.h"
#include "SymbolCodeD.h"


namespace AZO {
namespace Decoder {

class DictionaryTable : public Base::DictionaryTable
{
public:
    DictionaryTable(const byte* buf, u_int size) : 
        Base::DictionaryTable(buf, size) {}
    
    bool Code(EntropyCode& entropy, u_int& pos, u_int& length);
    void Add(u_int pos, u_int length);

private:
    void Get(u_int idx, u_int& pos, u_int& len);

private:
    BoolState<> findState_;
    SymbolCode<u_int, DICTIONARY_SIZE, DICTIONARY_HISTORY_SIZE>   prob_;
};


} //namespaces Decoder
} //namespaces AZO

#include "DictionaryTableD.cpp"

#endif /*AZO_DECODER_DICTIONARYTABLE_H*/
