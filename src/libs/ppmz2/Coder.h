#pragma once

#include <memory>
#include <string.h>
#include "PPMZ2.h"
#include "IntMath.h"

namespace Ppmz2
{
    class Coder
    {
		typedef std::auto_ptr<Ppmz> SafePpmz;
    public:

		Coder(void (*loggingCallback)(const std::string&))
			: _loggingCallback(loggingCallback)
		{}

        unsigned int Encode(unsigned char *rawBuffer, unsigned int rawLength, unsigned char **compressedBuffer, CodingOptions options)
        {
            unsigned char* paddedRawBuffer = new unsigned char[rawLength + (2 * PREAMBLE)];
            unsigned char* paddedCompBuffer = new unsigned char[(rawLength * 2) + (2 * PREAMBLE)];
            memset(paddedRawBuffer, ' ', rawLength + (2 * PREAMBLE));
            memcpy(paddedRawBuffer + PREAMBLE, rawBuffer, rawLength);
            memset(paddedCompBuffer, 0, (rawLength * 2) + (2 * PREAMBLE));

	        unsigned int ret = EncodeArrayPrivate(paddedRawBuffer + PREAMBLE, rawLength, paddedCompBuffer + PREAMBLE, options);
            if (ret > 0)
            {
                // for some reason this is underreported by up to 2 bytes
                ret += 2;
                *compressedBuffer = new unsigned char[ret];
                memcpy(*compressedBuffer, paddedCompBuffer + PREAMBLE, ret);
                delete[] paddedCompBuffer;
                delete[] paddedRawBuffer;
                return ret;
            }
            else
            {
                *compressedBuffer = NULL;
                return -1;
            }
        }

        bool Decode(unsigned char **rawBuffer, unsigned int rawLength, unsigned char *compBuf, CodingOptions options)
        {
            *rawBuffer = new unsigned char[rawLength];

            unsigned char* paddedRawBuf = new unsigned char[rawLength + (2 * PREAMBLE)];

            memset(paddedRawBuf, ' ', PREAMBLE);
            bool success = DecodeArrayPrivate(paddedRawBuf + PREAMBLE, rawLength, compBuf, options);
            memcpy(*rawBuffer, paddedRawBuf + PREAMBLE, rawLength);
            delete[] paddedRawBuf;
	        return success;
        }

        unsigned int Encode(
		        unsigned char *rawBuffer, unsigned int rawLength, unsigned char **compressedBuffer,
		        unsigned char *refBuf, unsigned int refLen, CodingOptions options)
        {
            *compressedBuffer = new unsigned char[(rawLength * 2) + 65536];

            int compressedLength = EncodeArrayWithPrecondition(rawBuffer, rawLength, *compressedBuffer, refBuf, refLen, options);
            return compressedLength;
        }

        bool Decode(
		        unsigned char **rawBuffer, unsigned int rawLength, unsigned char *compBuf,
		        unsigned char *refBuf,unsigned int refLen, CodingOptions options)
        {
            *rawBuffer = new unsigned char[rawLength];

            bool success = DecodeArrayWithPrecondition(*rawBuffer, rawLength, compBuf, refBuf, refLen, options);
            return success;
        }

    private:
        static const unsigned int PREAMBLE = 1024;
		void (*_loggingCallback)(const std::string&);

        unsigned int EncodeArrayPrivate(unsigned char *rawBuf, unsigned int rawLen, unsigned char *compBuf, CodingOptions options)
        {
			SafePpmz ppz(Ppmz2::Ppmz::Create(options, _loggingCallback));

	        unsigned int ret = ppz->EncodeArraySub(rawBuf, rawLen, compBuf);

	        return ret;
        }

        bool DecodeArrayPrivate(unsigned char* rawBuffer, unsigned int rawLength, unsigned char* compBuffer, CodingOptions options)
        {
	        SafePpmz ppz(Ppmz2::Ppmz::Create(options, _loggingCallback));

	        bool ret = ppz->DecodeArraySub(rawBuffer, rawLength, compBuffer);
        	
	        return ret;
        }

        unsigned int EncodeArrayWithPrecondition(
		        unsigned char *rawBuf, unsigned int rawLen,unsigned char *compBuf,
		        unsigned char *refBuf, unsigned int refLen, CodingOptions options)
        {
            unsigned char* paddedRawBuf = new unsigned char[(2 * PREAMBLE) + rawLen];
            unsigned char* paddedCompBuf = new unsigned char[max(rawLen,refLen)*2 + 65536 + PREAMBLE];
            unsigned char* paddedRefBuf = new unsigned char[(2 * PREAMBLE) + refLen];
        	
	        memset(paddedRawBuf, ' ', PREAMBLE);
	        memcpy(paddedRawBuf + PREAMBLE, rawBuf, rawLen);

            memset(paddedCompBuf, 0, PREAMBLE);

            memset(paddedRefBuf, ' ', PREAMBLE);
	        memcpy(paddedRefBuf + PREAMBLE, refBuf, refLen);
        	
	        unsigned int ret = EncodeArrayWithPreconditionUnsafe(paddedRawBuf + PREAMBLE, rawLen, paddedCompBuf + PREAMBLE, paddedRefBuf + PREAMBLE, refLen, options);
	        memcpy(compBuf, paddedCompBuf + PREAMBLE, ret + 2);
        	
	        delete[] paddedRawBuf;
            delete[] paddedCompBuf;
            delete[] paddedRefBuf;
	        return ret + 2;
        }

        unsigned int EncodeArrayWithPreconditionUnsafe(
		        unsigned char *rawBuf, unsigned int rawLen, unsigned char *compBuf,
		        unsigned char *refBuf, unsigned int refLen, CodingOptions options)
        {
			SafePpmz ppz(Ppmz2::Ppmz::Create(options, _loggingCallback));

	        ppz->EncodeArraySub(refBuf, refLen, compBuf);

	        ppz->ReInitCoder();

	        unsigned int ret = ppz->EncodeArraySub(rawBuf, rawLen, compBuf);

	        return ret;
        }

        bool DecodeArrayWithPrecondition(
		        unsigned char* rawBuf, unsigned int rawLen, unsigned char* compBuf,
		        unsigned char* refBuf, unsigned int refLen, CodingOptions options)
        {
	        // RawBuffer, Decoded buffer, Compressed Buffer, Condition Buffer
            unsigned char* paddedRawBuf = new unsigned char[(2 * PREAMBLE) + rawLen];
            unsigned char* paddedCompBuf = new unsigned char[max(rawLen,refLen)*2 + 65536 + PREAMBLE];
            unsigned char* paddedRefBuf = new unsigned char[(2 * PREAMBLE) + refLen];
        	
	        memset(paddedRawBuf, ' ', PREAMBLE);

            memset(paddedCompBuf, 0, PREAMBLE);

            memset(paddedRefBuf, ' ', PREAMBLE);
	        memcpy(paddedRefBuf + PREAMBLE, refBuf, refLen);
        	
            // We don't know how long the compressed buffer is, so copy the maximum value, or until we
	        // get an access violation exception
            // there is a slight security vulnerability here in that we may copy more memory than we should,
            // but it shouldn't be possible to use this as an attack vector
	        //__try
	        //{
		        memcpy(paddedCompBuf + PREAMBLE, compBuf, (max(rawLen,refLen)*2 + 65536));
	        //}
	        //__except (1){}
        	
	        bool ret = DecodeArrayWithPreconditionUnsafe(paddedRawBuf + PREAMBLE, rawLen, paddedCompBuf + PREAMBLE, paddedRefBuf + PREAMBLE, refLen, options);
	        memcpy(rawBuf, paddedRawBuf + PREAMBLE, rawLen);
            delete[] paddedRawBuf;
            delete[] paddedCompBuf;
            delete[] paddedRefBuf;
        	
	        return ret;
        }

        bool DecodeArrayWithPreconditionUnsafe(
		        unsigned char* rawBuf, unsigned int rawLen, unsigned char* compBuf,
		        unsigned char* refBuf, unsigned int refLen, CodingOptions options)
        {
			SafePpmz ppz(Ppmz2::Ppmz::Create(options, _loggingCallback));

	        ppz->EncodeArraySub(refBuf, refLen, rawBuf);

	        ppz->ReInitCoder();

	        bool ret = ppz->DecodeArraySub(rawBuf, rawLen, compBuf);

	        return ret;
        }
    };
}