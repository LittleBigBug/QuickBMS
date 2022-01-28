#include "Portable.h"
#include "LZMADecoder.h"

#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }

namespace NCompress {
namespace NLZMA {

HRESULT CDecoder::SetDictionarySize(INT aDictionarySize)
{
  if (aDictionarySize > (1 << kDicLogSizeMax))
    return E_INVALIDARG;
  
  INT aWindowReservSize = MyMax(aDictionarySize, INT(1 << 21));

  if (m_DictionarySize != aDictionarySize)
  {
    m_OutWindowStream.Create(aDictionarySize, kMatchMaxLen, aWindowReservSize);
    m_DictionarySize = aDictionarySize;
  }
  return S_OK;
}

HRESULT CDecoder::SetLiteralProperties(
    INT aLiteralPosStateBits, INT aLiteralContextBits)
{
  if (aLiteralPosStateBits > 8)
    return E_INVALIDARG;
  if (aLiteralContextBits > 8)
    return E_INVALIDARG;
  m_LiteralDecoder.Create(aLiteralPosStateBits, aLiteralContextBits);
  return S_OK;
}

HRESULT CDecoder::SetPosBitsProperties(INT aNumPosStateBits)
{
  if (aNumPosStateBits > NLength::kNumPosStatesBitsMax)
    return E_INVALIDARG;
  INT aNumPosStates = 1 << aNumPosStateBits;
  m_LenDecoder.Create(aNumPosStates);
  m_RepMatchLenDecoder.Create(aNumPosStates);
  m_PosStateMask = aNumPosStates - 1;
  return S_OK;
}

CDecoder::CDecoder():
  m_DictionarySize((INT)-1)
{
  Create();
}

HRESULT CDecoder::Create()
{
  for(int i = 0; i < kNumPosModels; i++)
  {
    RETURN_E_OUTOFMEMORY_IF_FALSE(
        m_PosDecoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]));
  }
  return S_OK;
}


HRESULT CDecoder::Init(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream)
{
  m_RangeDecoder.Init(anInStream);

  m_OutWindowStream.Init(anOutStream);

  int i;
  for(i = 0; i < kNumStates; i++)
  {
    for (INT j = 0; j <= m_PosStateMask; j++)
    {
      m_MainChoiceDecoders[i][j].Init();
      m_MatchRepShortChoiceDecoders[i][j].Init();
    }
    m_MatchChoiceDecoders[i].Init();
    m_MatchRepChoiceDecoders[i].Init();
    m_MatchRep1ChoiceDecoders[i].Init();
    m_MatchRep2ChoiceDecoders[i].Init();
  }
  
  m_LiteralDecoder.Init();
   
  for (i = 0; i < kNumLenToPosStates; i++)
    m_PosSlotDecoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    m_PosDecoders[i].Init();
  
  m_LenDecoder.Init();
  m_RepMatchLenDecoder.Init();

  m_PosAlignDecoder.Init();
  return S_OK;

}

HRESULT CDecoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;

  Init(anInStream, anOutStream);

  CState aState;
  aState.Init();
  bool aPeviousIsMatch = false;
  BYTE aPreviousByte = 0;
  INT aRepDistances[kNumRepDistances];
  for(int i = 0 ; i < kNumRepDistances; i++)
    aRepDistances[i] = 0;

  UINT64 aNowPos64 = 0;
  UINT64 aSize = *anOutSize;
  while(aNowPos64 < aSize)
  {
    UINT64 aNext = MyMin(aNowPos64 + (1 << 18), aSize);
    while(aNowPos64 < aNext)
    {
      INT aPosState = INT(aNowPos64) & m_PosStateMask;
      if (m_MainChoiceDecoders[aState.m_Index][aPosState].Decode(&m_RangeDecoder) == kMainChoiceLiteralIndex)
      {
        aState.UpdateChar();
        if(aPeviousIsMatch)
        {
          BYTE aMatchByte = m_OutWindowStream.GetOneByte(0 - aRepDistances[0] - 1);
          aPreviousByte = m_LiteralDecoder.DecodeWithMatchByte(&m_RangeDecoder, 
              INT(aNowPos64), aPreviousByte, aMatchByte);
          aPeviousIsMatch = false;
        }
        else
          aPreviousByte = m_LiteralDecoder.DecodeNormal(&m_RangeDecoder, 
              INT(aNowPos64), aPreviousByte);
        m_OutWindowStream.PutOneByte(aPreviousByte);
        aNowPos64++;
      }
      else             
      {
        aPeviousIsMatch = true;
        INT aDistance, aLen;
        if(m_MatchChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 
            kMatchChoiceRepetitionIndex)
        {
          if(m_MatchRepChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
          {
            if(m_MatchRepShortChoiceDecoders[aState.m_Index][aPosState].Decode(&m_RangeDecoder) == 0)
            {
              aState.UpdateShortRep();
              aPreviousByte = m_OutWindowStream.GetOneByte(0 - aRepDistances[0] - 1);
              m_OutWindowStream.PutOneByte(aPreviousByte);
              aNowPos64++;
              continue;
            }
            aDistance = aRepDistances[0];
          }
          else
          {
            if(m_MatchRep1ChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
            {
              aDistance = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
            }
            else 
            {
              if (m_MatchRep2ChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
              {
                aDistance = aRepDistances[2];
              }
              else
              {
                aDistance = aRepDistances[3];
                aRepDistances[3] = aRepDistances[2];
              }
              aRepDistances[2] = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
            }
            aRepDistances[0] = aDistance;
          }
          aLen = m_RepMatchLenDecoder.Decode(&m_RangeDecoder, aPosState) + kMatchMinLen;
          aState.UpdateRep();
        }
        else
        {
          aLen = kMatchMinLen + m_LenDecoder.Decode(&m_RangeDecoder, aPosState);
          aState.UpdateMatch();
          INT aPosSlot = m_PosSlotDecoder[GetLenToPosState(aLen)].Decode(&m_RangeDecoder);
          if (aPosSlot >= kStartPosModelIndex)
          {
            aDistance = kDistStart[aPosSlot];
            if (aPosSlot < kEndPosModelIndex)
              aDistance += m_PosDecoders[aPosSlot - kStartPosModelIndex].Decode(&m_RangeDecoder);
            else
            {
              aDistance += (m_RangeDecoder.DecodeDirectBits(kDistDirectBits[aPosSlot] - 
                  kNumAlignBits) << kNumAlignBits);
              aDistance += m_PosAlignDecoder.Decode(&m_RangeDecoder);
            }
          }
          else
            aDistance = aPosSlot;

          
          aRepDistances[3] = aRepDistances[2];
          aRepDistances[2] = aRepDistances[1];
          aRepDistances[1] = aRepDistances[0];
          
          aRepDistances[0] = aDistance;
        }
        if (aDistance >= aNowPos64)
          throw E_INVALIDDATA;
        m_OutWindowStream.CopyBackBlock(aDistance, aLen);
        aNowPos64 += aLen;
        aPreviousByte = m_OutWindowStream.GetOneByte(0 - 1);
      }
    }
  }
  return Flush();
}

HRESULT CDecoder::Code(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize)
{
  try {
     return CodeReal(anInStream, anOutStream, anInSize, anOutSize);
  } catch (HRESULT& e) {
     return e;
  } catch (...) {
     return E_FAIL;
  }
}

HRESULT CDecoder::ReadCoderProperties(ISequentialInStream *anInStream)
{
  INT aNumPosStateBits;
  INT aLiteralPosStateBits;
  INT aLiteralContextBits;
  INT aDictionarySize;

  INT aProcessesedSize;

  BYTE aByte;
  RETURN_IF_NOT_S_OK(anInStream->Read(&aByte, sizeof(aByte), &aProcessesedSize));
  if (aProcessesedSize != sizeof(aByte))
    return E_INVALIDARG;

  aLiteralContextBits = aByte % 9;
  BYTE aRemainder = aByte / 9;
  aLiteralPosStateBits = aRemainder % 5;
  aNumPosStateBits = aRemainder / 5;

  RETURN_IF_NOT_S_OK(anInStream->Read(&aDictionarySize, sizeof(aDictionarySize), &aProcessesedSize));
  if (aProcessesedSize != sizeof(aDictionarySize))
    return E_INVALIDARG;

  RETURN_IF_NOT_S_OK(SetDictionarySize(aDictionarySize));
  RETURN_IF_NOT_S_OK(SetLiteralProperties(aLiteralPosStateBits, aLiteralContextBits));
  RETURN_IF_NOT_S_OK(SetPosBitsProperties(aNumPosStateBits));

  return S_OK;
}

}}
