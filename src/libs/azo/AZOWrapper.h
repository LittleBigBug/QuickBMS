/*
 * AZOWrapper.h
 *
 *  Created on: 2010. 12. 16.
 *      Author: Yoonsik Oh(Boom)
 *
 *  Copyright(c) 2010-Present ESTsoft Corp. All rights reserved.
 *
 *  For conditions of distribution and use, see copyright notice in license.txt
 */

#ifndef AZO_AZOWRAPPER_H
#define AZO_AZOWRAPPER_H

#include "AZO.h"

class AZODecoder
{
public:
    AZODecoder() :
        handle_(NULL)
    {
        AZO_DecompressInit(&handle_);
    }

    ~AZODecoder()
    {
        AZO_DecompressEnd(handle_);
    }

    int Decompress(const char*& next_in, unsigned int& avail_in,
                char*& next_out, unsigned int& avail_out)
    {
        AZO_Stream stream;
        stream.next_in = next_in;
        stream.avail_in = avail_in;
        stream.next_out = next_out;
        stream.avail_out = avail_out;

        int ret = Decompress(stream);

        next_in = stream.next_in;
        avail_in = stream.avail_in;
        next_out = stream.next_out;
        avail_out = stream.avail_out;

        return ret;
    }

    int Decompress(AZO_Stream& stream)
    {
        return AZO_Decompress(handle_, &stream);
    }

private:
    AZO_HDECOMPRESS handle_;    
};

#endif /*AZO_AZOWRAPPER_H*/
