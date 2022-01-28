#ifndef __LZARITHMETIC_ENCODER_H
#define __LZARITHMETIC_ENCODER_H

#include "Portable.h"
#include "AriPrice.h"
#include "LZMA.h"
#include "LenCoder.h"
#include "LiteralCoder.h"
#include "AriConst.h"

// NOTE Here is choosen the MatchFinder
#include "BinTree2.h"
#define MATCH_FINDER NBT2::CMatchFinderBinTree

namespace NCompress {
namespace NLZMA {

struct COptimal
{
  CState State;

  bool Prev1IsChar;
  bool Prev2;

  INT PosPrev2;
  INT BackPrev2;     

  INT Price;    
  INT PosPrev;         // posNext;
  INT BackPrev;     
  INT Backs[kNumRepDistances];
  void MakeAsChar() { BackPrev = INT(-1); Prev1IsChar = false; }
  void MakeAsShortRep() { BackPrev = 0; ; Prev1IsChar = false; }
  bool IsShortRep() { return (BackPrev == 0); }
};


extern BYTE g_FastPos[1024];
inline INT GetPosSlot(INT aPos)
{
  if (aPos < (1 << 10))
    return g_FastPos[aPos];
  if (aPos < (1 << 19))
    return g_FastPos[aPos >> 9] + 18;
  return g_FastPos[aPos >> 18] + 36;
}

inline INT GetPosSlot2(INT aPos)
{
  if (aPos < (1 << 16))
    return g_FastPos[aPos >> 6] + 12;
  if (aPos < (1 << 25))
    return g_FastPos[aPos >> 15] + 30;
  return g_FastPos[aPos >> 24] + 48;
}

const int kIfinityPrice = 0xFFFFFFF;

typedef CMyBitEncoder<kNumMoveBitsForMainChoice> CMyBitEncoder2;

const int kNumOpts = 1 << 12;

class CEncoder : public CBaseCoder
{
  COptimal m_Optimum[kNumOpts];
public:
  MATCH_FINDER m_MatchFinder;
  CMyRangeEncoder m_RangeEncoder;
private:

  CMyBitEncoder2 m_MainChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];
  CMyBitEncoder2 m_MatchChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRepChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRep1ChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRep2ChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRepShortChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];

  CBitTreeEncoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> m_PosSlotEncoder[kNumLenToPosStates];

  CReverseBitTreeEncoder2<kNumMoveBitsForPosCoders> m_PosEncoders[kNumPosModels];
  CReverseBitTreeEncoder2<kNumMoveBitsForAlignCoders> m_PosAlignEncoder;
  
  NLength::CPriceTableEncoder m_LenEncoder;
  NLength::CPriceTableEncoder m_RepMatchLenEncoder;

  NLiteral::CEncoder m_LiteralEncoder;

  UINT32 m_MatchDistances[kMatchMaxLen + 1];

  bool m_FastMode;
  bool m_MaxMode;
  INT m_NumFastBytes;
  INT m_LongestMatchLength;    

  INT m_AdditionalOffset;

  INT m_OptimumEndIndex;
  INT m_OptimumCurrentIndex;

  bool m_LongestMatchWasFound;

  INT m_PosSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  
  INT m_DistancesPrices[kNumLenToPosStates][kNumFullDistances];

  INT m_AlignPrices[kAlignTableSize];
  INT m_AlignPriceCount;

  INT m_DistTableSize;

  INT m_PosStateBits;
  INT m_PosStateMask;
  INT m_LiteralPosStateBits;
  INT m_LiteralContextBits;

  INT m_DictionarySize;


  INT m_DictionarySizePrev;
  INT m_NumFastBytesPrev;

  
  INT ReadMatchDistances()
  {
    INT aLen = m_MatchFinder.GetLongestMatch(m_MatchDistances);
    if (aLen == m_NumFastBytes)
      aLen += m_MatchFinder.GetMatchLen(aLen, m_MatchDistances[aLen], 
          kMatchMaxLen - aLen);
    m_AdditionalOffset++;
    HRESULT aResult = m_MatchFinder.MovePos();
    if (aResult != S_OK)
      throw aResult;
    return aLen;
  }

  void MovePos(INT aNum);
  INT GetRepLen1Price(CState aState, INT aPosState) const
  {
    return m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(0) +
        m_MatchRepShortChoiceEncoders[aState.m_Index][aPosState].GetPrice(0);
  }
  INT GetRepPrice(INT aRepIndex, INT aLen, CState aState, INT aPosState) const
  {
    INT aPrice = m_RepMatchLenEncoder.GetPrice(aLen - kMatchMinLen, aPosState);
    if(aRepIndex == 0)
    {
      aPrice += m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(0);
      aPrice += m_MatchRepShortChoiceEncoders[aState.m_Index][aPosState].GetPrice(1);
    }
    else
    {
      aPrice += m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(1);
      if (aRepIndex == 1)
        aPrice += m_MatchRep1ChoiceEncoders[aState.m_Index].GetPrice(0);
      else
      {
        aPrice += m_MatchRep1ChoiceEncoders[aState.m_Index].GetPrice(1);
        aPrice += m_MatchRep2ChoiceEncoders[aState.m_Index].GetPrice(aRepIndex - 2);
      }
    }
    return aPrice;
  }
  INT GetPosLenPrice(INT aPos, INT aLen, INT aPosState) const
  {
    if (aLen == 2 && aPos >= 0x80)
      return kIfinityPrice;
    INT aPrice;
    INT aLenToPosState = GetLenToPosState(aLen);
    if (aPos < kNumFullDistances)
      aPrice = m_DistancesPrices[aLenToPosState][aPos];
    else
      aPrice = m_PosSlotPrices[aLenToPosState][GetPosSlot2(aPos)] + 
          m_AlignPrices[aPos & kAlignMask];
    return aPrice + m_LenEncoder.GetPrice(aLen - kMatchMinLen, aPosState);
  }

  INT Backward(INT &aBackRes, INT aCur);
  INT GetOptimum(INT &aBackRes, INT aPosition);
  INT GetOptimumFast(INT &aBackRes, INT aPosition);

  void FillPosSlotPrices();
  void FillDistancesPrices();
  void FillAlignPrices();
    
  HRESULT Flush();

  HRESULT Create();

  HRESULT CodeReal(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize);

  HRESULT Init(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream);

public:
  CEncoder();

  HRESULT SetEncoderAlgorithm(INT A);
  HRESULT SetEncoderNumFastBytes(INT A);
  HRESULT SetDictionarySize(INT aDictionarySize);
  HRESULT SetLiteralProperties(INT aLiteralPosStateBits, INT aLiteralContextBits);
  HRESULT SetPosBitsProperties(INT aNumPosStateBits);
  HRESULT Code(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize);
  HRESULT WriteCoderProperties(ISequentialOutStream *anOutStream);
};

}}

#endif
