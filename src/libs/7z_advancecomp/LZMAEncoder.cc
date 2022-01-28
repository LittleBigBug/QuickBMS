#include "Portable.h"
#include "LZMAEncoder.h"

#include "BinTree2Main.h"

using namespace NCompression;
using namespace NArithmetic;

#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }

namespace NCompress {
namespace NLZMA {

BYTE g_FastPos[1024];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    int c = 0;
    const int kFastSlots = 20;
    c = 0;
    for (BYTE aSlotFast = 0; aSlotFast < kFastSlots; aSlotFast++)
    {
      INT k = (1 << kDistDirectBits[aSlotFast]);
      for (INT j = 0; j < k; j++, c++)
        g_FastPos[c] = aSlotFast;
    }
  }
} g_FastPosInit;

const int kDefaultDictionaryLogSize = 20;
const int kNumFastBytesDefault = 0x20;

CEncoder::CEncoder():
  m_DictionarySize(1 << kDefaultDictionaryLogSize),
  m_DictionarySizePrev(INT(-1)),
  m_NumFastBytes(kNumFastBytesDefault),
  m_NumFastBytesPrev(INT(-1)),
  m_DistTableSize(kDefaultDictionaryLogSize * 2),
  m_PosStateBits(2),
  m_PosStateMask(4 - 1),
  m_LiteralPosStateBits(0),
  m_LiteralContextBits(3)
{
  m_MaxMode = false;
  m_FastMode = false;
  m_PosAlignEncoder.Create(kNumAlignBits);
  for(int i = 0; i < kNumPosModels; i++)
    m_PosEncoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]);
}

HRESULT CEncoder::Create()
{
  if (m_DictionarySize == m_DictionarySizePrev && m_NumFastBytesPrev == m_NumFastBytes)
    return S_OK;
  RETURN_IF_NOT_S_OK(m_MatchFinder.Create(m_DictionarySize, kNumOpts, m_NumFastBytes, 
      kMatchMaxLen - m_NumFastBytes));
  m_DictionarySizePrev = m_DictionarySize;
  m_NumFastBytesPrev = m_NumFastBytes;
  m_LiteralEncoder.Create(m_LiteralPosStateBits, m_LiteralContextBits);
  m_LenEncoder.Create(1 << m_PosStateBits);
  m_RepMatchLenEncoder.Create(1 << m_PosStateBits);
  return S_OK;
}

HRESULT CEncoder::SetEncoderAlgorithm(INT A) {
  INT aMaximize = A;
  if (aMaximize > 2)
    return E_INVALIDARG;

  m_FastMode = (aMaximize == 0);
  m_MaxMode = (aMaximize >= 2);

  return S_OK;
}

HRESULT CEncoder::SetEncoderNumFastBytes(INT A) {
  INT aNumFastBytes = A;
  if(aNumFastBytes < 2 || aNumFastBytes > kMatchMaxLen)
    return E_INVALIDARG;

  m_NumFastBytes = aNumFastBytes;

  return S_OK;
}

HRESULT CEncoder::SetDictionarySize(INT aDictionarySize)
{
  if (aDictionarySize > INT(1 << kDicLogSizeMax))
    return E_INVALIDARG;
  m_DictionarySize = aDictionarySize;
  INT aDicLogSize;
  for(aDicLogSize = 0; aDicLogSize < kDicLogSizeMax; aDicLogSize++)
    if (aDictionarySize <= (INT(1) << aDicLogSize))
      break;
  m_DistTableSize = aDicLogSize * 2;
  return S_OK;
}

HRESULT CEncoder::SetLiteralProperties(INT aLiteralPosStateBits, INT aLiteralContextBits)
{
  if (aLiteralPosStateBits > kNumLitPosStatesBitsEncodingMax)
    return E_INVALIDARG;
  if (aLiteralContextBits > kNumLitContextBitsMax)
    return E_INVALIDARG;
  m_LiteralPosStateBits = aLiteralPosStateBits;
  m_LiteralContextBits = aLiteralContextBits;
  return S_OK;
}

HRESULT CEncoder::SetPosBitsProperties(INT aNumPosStateBits)
{
  if (aNumPosStateBits > NLength::kNumPosStatesBitsEncodingMax)
    return E_INVALIDARG;
  m_PosStateBits = aNumPosStateBits;
  m_PosStateMask = (1 << m_PosStateBits) - 1;
  return S_OK;
}


HRESULT CEncoder::WriteCoderProperties(ISequentialOutStream *anOutStream)
{ 
  BYTE aByte = (m_PosStateBits * 5 + m_LiteralPosStateBits) * 9 + m_LiteralContextBits;
  INT aProcessedSize;
  HRESULT aResult = anOutStream->Write(&aByte, sizeof(aByte), &aProcessedSize);
  if (aResult != S_OK)
     return aResult;
  if (aProcessedSize != sizeof(aByte))
     return E_FAIL;
  aResult = anOutStream->Write(&m_DictionarySize, sizeof(m_DictionarySize), &aProcessedSize);
    if (aResult != S_OK)
     return aResult;
  if (aProcessedSize != sizeof(m_DictionarySize))
     return E_FAIL;
  return S_OK;
}

HRESULT CEncoder::Init(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream)
{
  CBaseCoder::Init();

  RETURN_IF_NOT_S_OK(m_MatchFinder.Init(anInStream));
  m_RangeEncoder.Init(anOutStream);

  int i;
  for(i = 0; i < kNumStates; i++)
  {
    for (INT j = 0; j <= m_PosStateMask; j++)
    {
      m_MainChoiceEncoders[i][j].Init();
      m_MatchRepShortChoiceEncoders[i][j].Init();
    }
    m_MatchChoiceEncoders[i].Init();
    m_MatchRepChoiceEncoders[i].Init();
    m_MatchRep1ChoiceEncoders[i].Init();
    m_MatchRep2ChoiceEncoders[i].Init();
  }

  m_LiteralEncoder.Init();

  for(i = 0; i < kNumLenToPosStates; i++)
    m_PosSlotEncoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    m_PosEncoders[i].Init();

  m_LenEncoder.Init();
  m_RepMatchLenEncoder.Init();

  m_PosAlignEncoder.Init();

  m_LongestMatchWasFound = false;
  m_OptimumEndIndex = 0;
  m_OptimumCurrentIndex = 0;
  m_AdditionalOffset = 0;

  return S_OK;
}

void CEncoder::MovePos(INT aNum)
{
  for (;aNum > 0; aNum--)
  {
    m_MatchFinder.DummyLongestMatch();
    HRESULT aResult = m_MatchFinder.MovePos();
    if (aResult != S_OK)
      throw aResult;
    m_AdditionalOffset++;
  }
}

INT CEncoder::Backward(INT &aBackRes, INT aCur)
{
  m_OptimumEndIndex = aCur;
  INT aPosMem = m_Optimum[aCur].PosPrev;
  INT aBackMem = m_Optimum[aCur].BackPrev;
  do
  {
    if (m_Optimum[aCur].Prev1IsChar)
    {
      m_Optimum[aPosMem].MakeAsChar();
      m_Optimum[aPosMem].PosPrev = aPosMem - 1;
      if (m_Optimum[aCur].Prev2)
      {
        m_Optimum[aPosMem - 1].Prev1IsChar = false;
        m_Optimum[aPosMem - 1].PosPrev = m_Optimum[aCur].PosPrev2;
        m_Optimum[aPosMem - 1].BackPrev = m_Optimum[aCur].BackPrev2;
      }
    }
    INT aPosPrev = aPosMem;
    INT aBackCur = aBackMem;

    aBackMem = m_Optimum[aPosPrev].BackPrev;
    aPosMem = m_Optimum[aPosPrev].PosPrev;

    m_Optimum[aPosPrev].BackPrev = aBackCur;
    m_Optimum[aPosPrev].PosPrev = aCur;
    aCur = aPosPrev;
  }
  while(aCur > 0);
  aBackRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex  = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex; 
}

INT CEncoder::GetOptimum(INT &aBackRes, INT aPosition)
{
  if(m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    INT aLen = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    aBackRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return aLen;
  }
  m_OptimumCurrentIndex = 0;
  m_OptimumEndIndex = 0; // test it;
  
  INT aLenMain;
  if (!m_LongestMatchWasFound)
    aLenMain = ReadMatchDistances();
  else
  {
    aLenMain = m_LongestMatchLength;
    m_LongestMatchWasFound = false;
  }


  INT aReps[kNumRepDistances];
  INT aRepLens[kNumRepDistances];
  INT RepMaxIndex = 0;
  int i;
  for(i = 0; i < kNumRepDistances; i++)
  {
    aReps[i] = m_RepDistances[i];
    aRepLens[i] = m_MatchFinder.GetMatchLen(0 - 1, aReps[i], kMatchMaxLen);
    if (i == 0 || aRepLens[i] > aRepLens[RepMaxIndex])
      RepMaxIndex = i;
  }
  if(aRepLens[RepMaxIndex] > m_NumFastBytes)
  {
    aBackRes = RepMaxIndex;
    MovePos(aRepLens[RepMaxIndex] - 1);
    return aRepLens[RepMaxIndex];
  }

  if(aLenMain > m_NumFastBytes)
  {
    INT aBackMain = (aLenMain < m_NumFastBytes) ? m_MatchDistances[aLenMain] :
        m_MatchDistances[m_NumFastBytes];
    aBackRes = aBackMain + kNumRepDistances; 
    MovePos(aLenMain - 1);
    return aLenMain;
  }
  BYTE aCurrentByte = m_MatchFinder.GetIndexByte(0 - 1);

  m_Optimum[0].State = m_State;

  BYTE aMatchByte;
  
  aMatchByte = m_MatchFinder.GetIndexByte(0 - m_RepDistances[0] - 1 - 1);

  INT aPosState = (aPosition & m_PosStateMask);

  m_Optimum[1].Price = m_MainChoiceEncoders[m_State.m_Index][aPosState].GetPrice(kMainChoiceLiteralIndex) + 
      m_LiteralEncoder.GetPrice(aPosition, m_PreviousByte, m_PeviousIsMatch, aMatchByte, aCurrentByte);
  m_Optimum[1].MakeAsChar();

  m_Optimum[1].PosPrev = 0;

  for (i = 0; i < kNumRepDistances; i++)
    m_Optimum[0].Backs[i] = aReps[i];

  INT aMatchPrice = m_MainChoiceEncoders[m_State.m_Index][aPosState].GetPrice(kMainChoiceMatchIndex);
  INT aRepMatchPrice = aMatchPrice + 
      m_MatchChoiceEncoders[m_State.m_Index].GetPrice(kMatchChoiceRepetitionIndex);

  if(aMatchByte == aCurrentByte)
  {
    INT aShortRepPrice = aRepMatchPrice + GetRepLen1Price(m_State, aPosState);
    if(aShortRepPrice < m_Optimum[1].Price)
    {
      m_Optimum[1].Price = aShortRepPrice;
      m_Optimum[1].MakeAsShortRep();
    }
  }
  if(aLenMain < 2)
  {
    aBackRes = m_Optimum[1].BackPrev;
    return 1;
  }

  
  INT aNormalMatchPrice = aMatchPrice + 
      m_MatchChoiceEncoders[m_State.m_Index].GetPrice(kMatchChoiceDistanceIndex);

  if (aLenMain <= aRepLens[RepMaxIndex])
    aLenMain = 0;

  INT aLen;
  for(aLen = 2; aLen <= aLenMain; aLen++)
  {
    m_Optimum[aLen].PosPrev = 0;
    m_Optimum[aLen].BackPrev = m_MatchDistances[aLen] + kNumRepDistances;
    m_Optimum[aLen].Price = aNormalMatchPrice + 
        GetPosLenPrice(m_MatchDistances[aLen], aLen, aPosState);
    m_Optimum[aLen].Prev1IsChar = false;
  }

  if (aLenMain < aRepLens[RepMaxIndex])
    aLenMain = aRepLens[RepMaxIndex];

  for (; aLen <= aLenMain; aLen++)
    m_Optimum[aLen].Price = kIfinityPrice;

  for(i = 0; i < kNumRepDistances; i++)
  {
    unsigned aRepLen = aRepLens[i];
    for(INT aLenTest = 2; aLenTest <= aRepLen; aLenTest++)
    {
      INT aCurAndLenPrice = aRepMatchPrice + GetRepPrice(i, aLenTest, m_State, aPosState);
      COptimal &anOptimum = m_Optimum[aLenTest];
      if (aCurAndLenPrice < anOptimum.Price) 
      {
        anOptimum.Price = aCurAndLenPrice;
        anOptimum.PosPrev = 0;
        anOptimum.BackPrev = i;
        anOptimum.Prev1IsChar = false;
      }
    }
  }

  INT aCur = 0;
  INT aLenEnd = aLenMain;

  while(true)
  {
    aCur++;
    if(aCur == aLenEnd)  
      return Backward(aBackRes, aCur);
    aPosition++;
    INT aPosPrev = m_Optimum[aCur].PosPrev;
    CState aState;
    if (m_Optimum[aCur].Prev1IsChar)
    {
      aPosPrev--;
      if (m_Optimum[aCur].Prev2)
      {
        aState = m_Optimum[m_Optimum[aCur].PosPrev2].State;
        if (m_Optimum[aCur].BackPrev2 < kNumRepDistances)
          aState.UpdateRep();
        else
          aState.UpdateMatch();
      }
      else
        aState = m_Optimum[aPosPrev].State;
      aState.UpdateChar();
    }
    else
      aState = m_Optimum[aPosPrev].State;
    bool aPrevWasMatch;
    if (aPosPrev == aCur - 1)
    {
      if (m_Optimum[aCur].IsShortRep())
      {
        aPrevWasMatch = true;
        aState.UpdateShortRep();
      }
      else
      {
        aPrevWasMatch = false;
        aState.UpdateChar();
      }
      /*
      if (m_Optimum[aCur].Prev1IsChar)
        for(int i = 0; i < kNumRepDistances; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i];
      */
    }
    else
    {
      aPrevWasMatch = true;
      INT aPos;
      if (m_Optimum[aCur].Prev1IsChar && m_Optimum[aCur].Prev2)
      {
        aPosPrev = m_Optimum[aCur].PosPrev2;
        aPos = m_Optimum[aCur].BackPrev2;
        aState.UpdateRep();
      }
      else
      {
        aPos = m_Optimum[aCur].BackPrev;
        if (aPos < kNumRepDistances)
          aState.UpdateRep();
        else
          aState.UpdateMatch();
      }
      if (aPos < kNumRepDistances)
      {
        aReps[0] = m_Optimum[aPosPrev].Backs[aPos];
    		INT i;
        for(i = 1; i <= aPos; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i - 1];
        for(; i < kNumRepDistances; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i];
      }
      else
      {
        aReps[0] = (aPos - kNumRepDistances);
        for(INT i = 1; i < kNumRepDistances; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i - 1];
      }
    }
    m_Optimum[aCur].State = aState;
    for(INT i = 0; i < kNumRepDistances; i++)
      m_Optimum[aCur].Backs[i] = aReps[i];
    INT aNewLen = ReadMatchDistances();
    if(aNewLen > m_NumFastBytes)
    {
      m_LongestMatchLength = aNewLen;
      m_LongestMatchWasFound = true;
      return Backward(aBackRes, aCur);
    }
    INT aCurPrice = m_Optimum[aCur].Price; 
    const BYTE *aData = m_MatchFinder.GetPointerToCurrentPos() - 1;
    BYTE aCurrentByte = *aData;
    BYTE aMatchByte = aData[0 - aReps[0] - 1];

    INT aPosState = (aPosition & m_PosStateMask);

    INT aCurAnd1Price = aCurPrice +
        m_MainChoiceEncoders[aState.m_Index][aPosState].GetPrice(kMainChoiceLiteralIndex) +
        m_LiteralEncoder.GetPrice(aPosition, aData[-1], aPrevWasMatch, aMatchByte, aCurrentByte);

    COptimal &aNextOptimum = m_Optimum[aCur + 1];

    bool aNextIsChar = false;
    if (aCurAnd1Price < aNextOptimum.Price) 
    {
      aNextOptimum.Price = aCurAnd1Price;
      aNextOptimum.PosPrev = aCur;
      aNextOptimum.MakeAsChar();
      aNextIsChar = true;
    }

    INT aMatchPrice = aCurPrice + m_MainChoiceEncoders[aState.m_Index][aPosState].GetPrice(kMainChoiceMatchIndex);
    INT aRepMatchPrice = aMatchPrice + m_MatchChoiceEncoders[aState.m_Index].GetPrice(kMatchChoiceRepetitionIndex);
    
    if(aMatchByte == aCurrentByte &&
        !(aNextOptimum.PosPrev < aCur && aNextOptimum.BackPrev == 0))
    {
      INT aShortRepPrice = aRepMatchPrice + GetRepLen1Price(aState, aPosState);
      if(aShortRepPrice <= aNextOptimum.Price)
      {
        aNextOptimum.Price = aShortRepPrice;
        aNextOptimum.PosPrev = aCur;
        aNextOptimum.MakeAsShortRep();
      }
    }

    INT aNumAvailableBytes = m_MatchFinder.GetNumAvailableBytes() + 1;
    aNumAvailableBytes = MyMin(kNumOpts - 1 - aCur, aNumAvailableBytes);

    if (aNumAvailableBytes < 2)
      continue;
    if (aNumAvailableBytes > m_NumFastBytes)
      aNumAvailableBytes = m_NumFastBytes;
    if (aNumAvailableBytes >= 3 && !aNextIsChar)
    {
      INT aBackOffset = aReps[0] + 1;
      INT aTemp;
      for (aTemp = 1; aTemp < aNumAvailableBytes; aTemp++)
        if (aData[aTemp] != aData[aTemp - aBackOffset])
          break;
      INT aLenTest2 = aTemp - 1;
      if (aLenTest2 >= 2)
      {
        CState aState2 = aState;
        aState2.UpdateChar();
        INT aPosStateNext = (aPosition + 1) & m_PosStateMask;
        INT aNextRepMatchPrice = aCurAnd1Price + 
            m_MainChoiceEncoders[aState2.m_Index][aPosStateNext].GetPrice(kMainChoiceMatchIndex) +
            m_MatchChoiceEncoders[aState2.m_Index].GetPrice(kMatchChoiceRepetitionIndex);
        {
          while(aLenEnd < aCur + 1 + aLenTest2)
            m_Optimum[++aLenEnd].Price = kIfinityPrice;
          INT aCurAndLenPrice = aNextRepMatchPrice + GetRepPrice(
              0, aLenTest2, aState2, aPosStateNext);
          COptimal &anOptimum = m_Optimum[aCur + 1 + aLenTest2];
          if (aCurAndLenPrice < anOptimum.Price) 
          {
            anOptimum.Price = aCurAndLenPrice;
            anOptimum.PosPrev = aCur + 1;
            anOptimum.BackPrev = 0;
            anOptimum.Prev1IsChar = true;
            anOptimum.Prev2 = false;
          }
        }
      }
    }
    for(INT aRepIndex = 0; aRepIndex < kNumRepDistances; aRepIndex++)
    {
      INT aBackOffset = aReps[aRepIndex] + 1;
      INT aLenTest;
      for (aLenTest = 0; aLenTest < aNumAvailableBytes; aLenTest++)
        if (aData[aLenTest] != aData[aLenTest - aBackOffset])
          break;
      for(; aLenTest >= 2; aLenTest--)
      {
        while(aLenEnd < aCur + aLenTest)
          m_Optimum[++aLenEnd].Price = kIfinityPrice;
        INT aCurAndLenPrice = aRepMatchPrice + GetRepPrice(aRepIndex, aLenTest, aState, aPosState);
        COptimal &anOptimum = m_Optimum[aCur + aLenTest];
        if (aCurAndLenPrice < anOptimum.Price) 
        {
          anOptimum.Price = aCurAndLenPrice;
          anOptimum.PosPrev = aCur;
          anOptimum.BackPrev = aRepIndex;
          anOptimum.Prev1IsChar = false;
        }
      }
    }
    
    if (aNewLen > aNumAvailableBytes)
      aNewLen = aNumAvailableBytes;
    if (aNewLen >= 2)
    {
      if (aNewLen == 2 && m_MatchDistances[2] >= 0x80)
        continue;
      INT aNormalMatchPrice = aMatchPrice + 
        m_MatchChoiceEncoders[aState.m_Index].GetPrice(kMatchChoiceDistanceIndex);
      while(aLenEnd < aCur + aNewLen)
        m_Optimum[++aLenEnd].Price = kIfinityPrice;

      for(INT aLenTest = aNewLen; aLenTest >= 2; aLenTest--)
      {
        INT aCurBack = m_MatchDistances[aLenTest];
        INT aCurAndLenPrice = aNormalMatchPrice + GetPosLenPrice(aCurBack, aLenTest, aPosState);
        COptimal &anOptimum = m_Optimum[aCur + aLenTest];
        if (aCurAndLenPrice < anOptimum.Price) 
        {
          anOptimum.Price = aCurAndLenPrice;
          anOptimum.PosPrev = aCur;
          anOptimum.BackPrev = aCurBack + kNumRepDistances;
          anOptimum.Prev1IsChar = false;
        }

        if (m_MaxMode)
        {
          INT aBackOffset = aCurBack + 1;
          INT aTemp;
          for (aTemp = aLenTest + 1; aTemp < aNumAvailableBytes; aTemp++)
            if (aData[aTemp] != aData[aTemp - aBackOffset])
              break;
          INT aLenTest2 = aTemp - (aLenTest + 1);
          if (aLenTest2 >= 2)
          {
            CState aState2 = aState;
            aState2.UpdateMatch();
            INT aPosStateNext = (aPosition + aLenTest) & m_PosStateMask;
            INT aCurAndLenCharPrice = aCurAndLenPrice + 
                m_MainChoiceEncoders[aState2.m_Index][aPosStateNext].GetPrice(kMainChoiceLiteralIndex) +
                m_LiteralEncoder.GetPrice(aPosition + aLenTest, aData[aLenTest - 1], 
                true, aData[aLenTest - aBackOffset], aData[aLenTest]);
            aState2.UpdateChar();
            aPosStateNext = (aPosition + aLenTest + 1) & m_PosStateMask;
            INT aNextMatchPrice = aCurAndLenCharPrice + m_MainChoiceEncoders[aState2.m_Index][aPosStateNext].GetPrice(kMainChoiceMatchIndex);
            INT aNextRepMatchPrice = aNextMatchPrice + m_MatchChoiceEncoders[aState2.m_Index].GetPrice(kMatchChoiceRepetitionIndex);
            
            {
              INT anOffset = aLenTest + 1 + aLenTest2;
              while(aLenEnd < aCur + anOffset)
                m_Optimum[++aLenEnd].Price = kIfinityPrice;
              INT aCurAndLenPrice = aNextRepMatchPrice + GetRepPrice(
                  0, aLenTest2, aState2, aPosStateNext);
              COptimal &anOptimum = m_Optimum[aCur + anOffset];
              if (aCurAndLenPrice < anOptimum.Price) 
              {
                anOptimum.Price = aCurAndLenPrice;
                anOptimum.PosPrev = aCur + aLenTest + 1;
                anOptimum.BackPrev = 0;
                anOptimum.Prev1IsChar = true;
                anOptimum.Prev2 = true;
                anOptimum.PosPrev2 = aCur;
                anOptimum.BackPrev2 = aCurBack + kNumRepDistances;
              }
            }
          }
        }
      }
    }
  }
}

static bool inline ChangePair(INT aSmall, INT aBig)
{
  const int kDif = 7;
  return (aSmall < (INT(1) << (32-kDif)) && aBig >= (aSmall << kDif));
}


INT CEncoder::GetOptimumFast(INT &aBackRes, INT aPosition)
{
  INT aLenMain;
  if (!m_LongestMatchWasFound)
    aLenMain = ReadMatchDistances();
  else
  {
    aLenMain = m_LongestMatchLength;
    m_LongestMatchWasFound = false;
  }
  INT aRepLens[kNumRepDistances];
  INT RepMaxIndex = 0;
  for(int i = 0; i < kNumRepDistances; i++)
  {
    aRepLens[i] = m_MatchFinder.GetMatchLen(0 - 1, m_RepDistances[i], kMatchMaxLen);
    if (i == 0 || aRepLens[i] > aRepLens[RepMaxIndex])
      RepMaxIndex = i;
  }
  if(aRepLens[RepMaxIndex] >= m_NumFastBytes)
  {
    aBackRes = RepMaxIndex;
    MovePos(aRepLens[RepMaxIndex] - 1);
    return aRepLens[RepMaxIndex];
  }
  if(aLenMain >= m_NumFastBytes)
  {
    aBackRes = m_MatchDistances[m_NumFastBytes] + kNumRepDistances; 
    MovePos(aLenMain - 1);
    return aLenMain;
  }
  while (aLenMain > 2)
  {
    if (!ChangePair(m_MatchDistances[aLenMain - 1], 
        m_MatchDistances[aLenMain]))
      break;
    aLenMain--;
  }
  if (aLenMain == 2 && m_MatchDistances[2] >= 0x80)
    aLenMain = 1;

  INT aBackMain = m_MatchDistances[aLenMain];
  if (aRepLens[RepMaxIndex] >= 2)
  {
    if (aRepLens[RepMaxIndex] + 1 >= aLenMain || 
        aRepLens[RepMaxIndex] + 2 >= aLenMain && (aBackMain > (1<<12)))
    {
      aBackRes = RepMaxIndex;
      MovePos(aRepLens[RepMaxIndex] - 1);
      return aRepLens[RepMaxIndex];
    }
  }
  

  if (aLenMain >= 2)
  {
    m_LongestMatchLength = ReadMatchDistances();
    if (m_LongestMatchLength >= 2 &&
      (
        (m_LongestMatchLength >= aLenMain && 
          m_MatchDistances[aLenMain] < aBackMain) || 
        m_LongestMatchLength == aLenMain + 1 && 
          !ChangePair(aBackMain, m_MatchDistances[m_LongestMatchLength]) ||
        m_LongestMatchLength > aLenMain + 1 ||
        m_LongestMatchLength + 1 >= aLenMain && 
          ChangePair(m_MatchDistances[aLenMain - 1], aBackMain)
      )
      )
    {
      m_LongestMatchWasFound = true;
      aBackRes = INT(-1);
      return 1;
    }
    for(int i = 0; i < kNumRepDistances; i++)
    {
      INT aRepLen = m_MatchFinder.GetMatchLen(0 - 1, m_RepDistances[i], kMatchMaxLen);
      if (aRepLen >= 2 && aRepLen + 1 >= aLenMain)
      {
        m_LongestMatchWasFound = true;
        aBackRes = INT(-1);
        return 1;
      }
    }
    aBackRes = aBackMain + kNumRepDistances; 
    MovePos(aLenMain - 2);
    return aLenMain;
  }
  aBackRes = INT(-1);
  return 1;
}

HRESULT CEncoder::Flush()
{
  m_RangeEncoder.FlushData();
  return m_RangeEncoder.FlushStream();
}

HRESULT CEncoder::CodeReal(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize)
{
  RETURN_IF_NOT_S_OK(Create());
  Init(anInStream, anOutStream);

  if (m_MatchFinder.GetNumAvailableBytes() == 0)
    return Flush();

  if (!m_FastMode)
  {
    FillPosSlotPrices();
    FillDistancesPrices();
    FillAlignPrices();
  }

  m_LenEncoder.SetTableSize(m_NumFastBytes);
  m_LenEncoder.UpdateTables();
  m_RepMatchLenEncoder.SetTableSize(m_NumFastBytes);
  m_RepMatchLenEncoder.UpdateTables();

  UINT64 aLastPosSlotFillingPos = 0;

  UINT64 aNowPos64 = 0;
  ReadMatchDistances();
  INT aPosState = INT(aNowPos64) & m_PosStateMask;
  m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceLiteralIndex);
  m_State.UpdateChar();
  BYTE aByte = m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset);
  m_LiteralEncoder.Encode(&m_RangeEncoder, INT(aNowPos64), m_PreviousByte, false, 0, aByte);
  m_PreviousByte = aByte;
  m_AdditionalOffset--;
  aNowPos64++;
  if (m_MatchFinder.GetNumAvailableBytes() == 0)
    return Flush();
  while(true)
  {
    INT aPos;
    INT aPosState = INT(aNowPos64) & m_PosStateMask;

    INT aLen;
    if (m_FastMode)
      aLen = GetOptimumFast(aPos, INT(aNowPos64));
    else
      aLen = GetOptimum(aPos, INT(aNowPos64));

    if(aLen == 1 && aPos == (-1))
    {
      m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceLiteralIndex);
      m_State.UpdateChar();
      BYTE aMatchByte;
      if(m_PeviousIsMatch)
        aMatchByte = m_MatchFinder.GetIndexByte(0 - m_RepDistances[0] - 1 - m_AdditionalOffset);
      BYTE aByte = m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset);
      m_LiteralEncoder.Encode(&m_RangeEncoder, INT(aNowPos64), m_PreviousByte, m_PeviousIsMatch, aMatchByte, aByte);
      m_PreviousByte = aByte;
      m_PeviousIsMatch = false;
    }
    else
    {
      m_PeviousIsMatch = true;
      m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceMatchIndex);
      if(aPos < kNumRepDistances)
      {
        m_MatchChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, kMatchChoiceRepetitionIndex);
        if(aPos == 0)
        {
          m_MatchRepChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 0);
          if(aLen == 1)
            m_MatchRepShortChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, 0);
          else
            m_MatchRepShortChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, 1);
        }
        else
        {
          m_MatchRepChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 1);
          if (aPos == 1)
            m_MatchRep1ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 0);
          else
          {
            m_MatchRep1ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 1);
            m_MatchRep2ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, aPos - 2);
          }
        }
        if (aLen == 1)
          m_State.UpdateShortRep();
        else
        {
          m_RepMatchLenEncoder.Encode(&m_RangeEncoder, aLen - kMatchMinLen, aPosState);
          m_State.UpdateRep();
        }


        INT aDistance = m_RepDistances[aPos];
        if (aPos != 0)
        {
          for(INT i = aPos; i >= 1; i--)
            m_RepDistances[i] = m_RepDistances[i - 1];
          m_RepDistances[0] = aDistance;
        }
      }
      else
      {
        m_MatchChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, kMatchChoiceDistanceIndex);
        m_State.UpdateMatch();
        m_LenEncoder.Encode(&m_RangeEncoder, aLen - kMatchMinLen, aPosState);
        aPos -= kNumRepDistances;
        INT aPosSlot = GetPosSlot(aPos);
        INT aLenToPosState = GetLenToPosState(aLen);
        m_PosSlotEncoder[aLenToPosState].Encode(&m_RangeEncoder, aPosSlot);
        
        INT aFooterBits = kDistDirectBits[aPosSlot];
        INT aPosReduced = aPos - kDistStart[aPosSlot];
        if (aPosSlot >= kStartPosModelIndex)
        {
          if (aPosSlot < kEndPosModelIndex)
            m_PosEncoders[aPosSlot - kStartPosModelIndex].Encode(&m_RangeEncoder, aPosReduced);
          else
          {
            m_RangeEncoder.EncodeDirectBits(aPosReduced >> kNumAlignBits, aFooterBits - kNumAlignBits);
            m_PosAlignEncoder.Encode(&m_RangeEncoder, aPosReduced & kAlignMask);
            if (!m_FastMode)
              if (--m_AlignPriceCount == 0)
                FillAlignPrices();
          }
        }
        INT aDistance = aPos;
        for(INT i = kNumRepDistances - 1; i >= 1; i--)
          m_RepDistances[i] = m_RepDistances[i - 1];
        m_RepDistances[0] = aDistance;
      }
      m_PreviousByte = m_MatchFinder.GetIndexByte(aLen - 1 - m_AdditionalOffset);
    }
    m_AdditionalOffset -= aLen;
    aNowPos64 += aLen;
    if (!m_FastMode)
      if (aNowPos64 - aLastPosSlotFillingPos >= (1 << 9))
      {
        FillPosSlotPrices();
        FillDistancesPrices();
        aLastPosSlotFillingPos = aNowPos64;
      }
    if (m_AdditionalOffset == 0 && m_MatchFinder.GetNumAvailableBytes() == 0)
      return Flush();
  }
}

HRESULT CEncoder::Code(ISequentialInStream *anInStream, ISequentialOutStream *anOutStream, const UINT64 *anInSize)
{
  try {
    return CodeReal(anInStream, anOutStream, anInSize);
  } catch (HRESULT& e) {
    return e;
  } catch (...) {
    return E_FAIL;
  }
}
  
void CEncoder::FillPosSlotPrices()
{
  for (int aLenToPosState = 0; aLenToPosState < kNumLenToPosStates; aLenToPosState++)
  {
    INT aPosSlot;
    for (aPosSlot = 0; aPosSlot < kEndPosModelIndex && aPosSlot < m_DistTableSize; aPosSlot++)
      m_PosSlotPrices[aLenToPosState][aPosSlot] = m_PosSlotEncoder[aLenToPosState].GetPrice(aPosSlot);
    for (; aPosSlot < m_DistTableSize; aPosSlot++)
      m_PosSlotPrices[aLenToPosState][aPosSlot] = m_PosSlotEncoder[aLenToPosState].GetPrice(aPosSlot) + 
          ((kDistDirectBits[aPosSlot] - kNumAlignBits) << kNumBitPriceShiftBits);
  }
}

void CEncoder::FillDistancesPrices()
{
  for (int aLenToPosState = 0; aLenToPosState < kNumLenToPosStates; aLenToPosState++)
  {
    INT i;
    for (i = 0; i < kStartPosModelIndex; i++)
      m_DistancesPrices[aLenToPosState][i] = m_PosSlotPrices[aLenToPosState][i];
    for (; i < kNumFullDistances; i++)
    { 
      INT aPosSlot = GetPosSlot(i);
      m_DistancesPrices[aLenToPosState][i] = m_PosSlotPrices[aLenToPosState][aPosSlot] +
          m_PosEncoders[aPosSlot - kStartPosModelIndex].GetPrice(i - kDistStart[aPosSlot]);
    }
  }
}

void CEncoder::FillAlignPrices()
{
  for (int i = 0; i < kAlignTableSize; i++)
    m_AlignPrices[i] = m_PosAlignEncoder.GetPrice(i);
  m_AlignPriceCount = kAlignTableSize;
}

}}
