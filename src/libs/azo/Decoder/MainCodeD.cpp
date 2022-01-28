#ifndef AZO_DECODER_MAINCODE_CPP
#define AZO_DECODER_MAINCODE_CPP

#include "MainCodeD.h"
#include "BlockCodeD.h"
#include "../Common/x86Filter.h"

namespace AZO {
namespace Decoder {

MainCode::MainCode() : 
    compressSize_(0), setSizeInfo_(false)
{
}

MainCode::~MainCode()
{
    if(coderObj_)
    {
        reinterpret_cast<BlockCode*>(coderObj_)->~BlockCode();
        Allocator::Free(coderObj_);
    }
}

int MainCode::ReadBlock(const byte* in, u_int inSize, byte* out, u_int outSize)
{
    if(inSize + COMPRESSION_REDUCE_MIN_SIZE > outSize)
    {
        //no compression
        if(inSize == outSize) {
            std::memcpy(out, in, static_cast<size_t>(inSize));
            return AZO_OK;
        } else {
            return AZO_DATA_ERROR;
        }
    }
    else
    {
	    EntropyCode entropy(in, inSize);

        if(coderObj_ == NULL) {
            u_int s = sizeof(BlockCode);
            coderObj_ = Allocator::Alloc(s);
        }

        BlockCode& block = *new (coderObj_)BlockCode(out, outSize);

        int ret = block.Code(entropy);
        if(entropy.GetSize() != inSize)
        {
            ASSERT(ret != AZO_OK);
            ret = AZO_DATA_ERROR;
        }

        return ret;
    }
}

int MainCode::Code(const byte*& in, u_int& inAvailSize, 
                   byte*& out, u_int& outAvailSize)
{
    int ret(AZO_OK);

    for(;;)
    {
        if(init_ == false) {
            const byte* inPtr = GetInBuffer(in, inAvailSize, MAIN_HEAD_SIZE);            
            if(inPtr == NULL)
                break;
            
            if(inPtr[0] != '0' + AZO_PRIVATE_VERSION) {
                ret = AZO_DATA_ERROR_VERSON;
                break;
            }
            useFilter_ = inPtr[1] & 1 ? true : false;
            
            RemoveInBuffer(in, inAvailSize, MAIN_HEAD_SIZE);

            init_ = true;

            continue;
        }
   
        if(setSizeInfo_ == false)
        {
            const byte* inPtr = GetInBuffer(in, inAvailSize, BLOCK_HEAD_SIZE);            
            if(inPtr == NULL)
                break;

            ReadNumber(blockSize_, inPtr, BLOCK_SIZE_SIZE);
            ReadNumber(compressSize_, inPtr+BLOCK_SIZE_SIZE, BLOCK_SIZE_SIZE);

            u_int checkSize(0);
            ReadNumber(checkSize, inPtr+BLOCK_SIZE_SIZE*2, BLOCK_SIZE_SIZE);

            RemoveInBuffer(in, inAvailSize, BLOCK_HEAD_SIZE);

            if(blockSize_ < compressSize_ || (blockSize_^compressSize_) != checkSize) {
                ret = AZO_DATA_ERROR_BLOCKSIZE;
                break;
            }

            setSizeInfo_ = true;

            continue;
        }

        if(finish_)
        {
            if(OutBufferUseSize() == 0) {
                ret = AZO_STREAM_END;
            } else {
                GetOutBuffer(out, outAvailSize, 0);
            }
            break;
        }
    
        if(blockSize_ && compressSize_)
        {
            const byte* inPtr = GetInBuffer(in, inAvailSize, compressSize_);
            byte* outPtr = GetOutBuffer(out, outAvailSize, blockSize_);
            if(inPtr == NULL || outPtr == NULL)
                break;

            ret = ReadBlock(inPtr, compressSize_, outPtr, blockSize_);

            if(ret >= AZO_OK) {
                if(useFilter_) {
                    x86Filter(outPtr, blockSize_, false);
                }

                setSizeInfo_ = false;
                RemoveInBuffer(in, inAvailSize, compressSize_);
                RemoveOutBuffer(out, outAvailSize, blockSize_);
            } else {
                break;
            }
        }
        else
        {
            finish_ = true;
        }
    }

    return ret;
}

} //namespaces Decoder
} //namespaces AZO

#endif /*AZO_DECODER_MAINCODE_CPP*/
