#pragma once

#include <string>
#include <iostream>
#include "CodingMetrics.h"
#include "LocalOrderEstimation.h"

#define __declspec(X)
#ifdef WIN32
    // nothing to do
#else
    typedef long long   __int64;
#endif

const unsigned int PPMZ2_MaxContextLen	=32;
const unsigned int PPMZ2_SeedBytes		=8;
const unsigned int PPMZ2_SeedByte		=214;

const unsigned int PPMZ2_DetMegs		=4;
const unsigned int PPMZ2_TrieMegs		=72;

#define PPMZ2_Order 8

namespace Ppmz2
{
	// Forward defs to reduce header dependencies
	class ContextTrie;
	class ArithInfo;
	class See;
	class PpmDet;
	class Exclude;

	enum CodingOptions 
	{
		Default = 0,
		NoUpdate = 1,
		TextMode = 2
	};

    class Ppmz
    {
    private:
        Ppmz() {}

        ContextTrie* _contextTrie;
	    ArithInfo* _arithInfo;
        Exclude* _exclude;
        See* _see;
        PpmDet* _det;
        int _order;
        int _trieMegs;
        int _detMegs;
        CodingOptions _options;
        LocalOrderEstimation::LOEType _loeType;		
		void (*_loggingCallback)(const std::string&);

    public:
		__declspec(dllexport) static Ppmz* Create(int order, int trieMegs, int detMegs, CodingOptions options, LocalOrderEstimation::LOEType loeType, void (*loggingCallback)(const std::string&));

        static Ppmz* Create(CodingOptions options, void (*loggingCallback)(const std::string&))
        {
			return Create(8, 72, 4, options, /*LocalOrderEstimation::LOEType::LOETYPE_MPS*/(LocalOrderEstimation::LOEType)1, loggingCallback);
        }

        ~Ppmz()
        {
	        delete _exclude;
            delete _arithInfo;
            delete _see;
            delete _contextTrie;
	        delete _det;
        }

        __declspec(dllexport) unsigned int EncodeArraySub(unsigned char* rawBuf, unsigned int rawLen, unsigned char* compBuf);
		__declspec(dllexport) __int64 EncodeOrderMinusOne(int sym);
		__declspec(dllexport) __int64 Update(Context** contexts, int sym, unsigned long cntx, int order);
		__declspec(dllexport) int EncodeFromOrder(Context** contexts, int sym, unsigned long cntx, bool* useFull, CodingMetrics* metrics);
		__declspec(dllexport) bool DetEncode(unsigned char* rawPtr, unsigned char* rawBuf, int sym, Context** contexts, bool* useFull, __int64* codingTime);
		__declspec(dllexport) __int64 DetUpdate(unsigned char* rawPtr, unsigned char* rawBuf, int sym);		
		__declspec(dllexport) int DecodeOrderMinusOne(__int64* minusOneOrderTime);
		__declspec(dllexport) int DecodeFromOrder(Context** contexts, unsigned long cntx, bool* useFull, CodingMetrics* metrics, int* codedOrder);
		__declspec(dllexport) bool DetDecode(unsigned char* rawPtr, unsigned char* rawBuf, int* sym, Context** contexts, bool* useFull, __int64* codingTime);
		__declspec(dllexport) bool DecodeArraySub(unsigned char* rawBuf, unsigned int rawLength, unsigned char* compBuf);        
        __declspec(dllexport) void ReInitCoder();
		__declspec(dllexport) unsigned int DecodeOrderMinusOneText();
		__declspec(dllexport) void EncodeOrderMinusOne(unsigned int sym, unsigned int numChars);
		__declspec(dllexport) void EncodeOrderMinusOneText(unsigned int sym);
		__declspec(dllexport) unsigned int DecodeOrderMinusOne(unsigned int numChars);
		__declspec(dllexport) bool EncodeFromContext(Context* cntx, unsigned long index, int sym, bool* pUseFull);
		__declspec(dllexport) bool DecodeFromContext(Context* cntx, unsigned long index, int *psym, bool * pUseFull);

		__declspec(dllexport) void SafeLog(const std::string& message)
		{
			if (_loggingCallback)
				_loggingCallback(message);
		}

		__declspec(dllexport) void SafeLog(const std::string& message, const CodingMetrics& metrics)
		{
			if (_loggingCallback)
			{
				std::ostringstream stream;
				stream << message << " " << metrics.ToString() << std::endl;

				_loggingCallback(stream.str());
			}
		}
    };
}