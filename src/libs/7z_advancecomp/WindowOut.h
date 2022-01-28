#ifndef __STREAM_WINDOWOUT_H
#define __STREAM_WINDOWOUT_H

#include "IInOutStreams.h"

namespace NStream {
namespace NWindow {

// m_KeepSizeBefore: how mach BYTEs must be in buffer before m_Pos;
// m_KeepSizeAfter: how mach BYTEs must be in buffer after m_Pos;
// m_KeepSizeReserv: how mach BYTEs must be in buffer for Moving Reserv; 
//                    must be >= aKeepSizeAfter; // test it

class COut
{
  BYTE  *m_Buffer;
  INT m_Pos;
  INT m_PosLimit;
  INT m_KeepSizeBefore;
  INT m_KeepSizeAfter;
  INT m_KeepSizeReserv;
  INT m_StreamPos;

  INT m_WindowSize;
  INT m_MoveFrom;

  ISequentialOutStream *m_Stream;

  virtual void MoveBlockBackward();
public:
  COut(): m_Buffer(0), m_Stream(0) {}
  virtual ~COut();
  void Create(INT aKeepSizeBefore,
      INT aKeepSizeAfter, INT aKeepSizeReserv = (1<<17));
  void SetWindowSize(INT aWindowSize);

  void Init(ISequentialOutStream *aStream, bool aSolid = false);
  HRESULT Flush();
  
  INT GetCurPos() const { return m_Pos; }
  const BYTE *GetPointerToCurrentPos() const { return m_Buffer + m_Pos;};

  void CopyBackBlock(INT aDistance, INT aLen)
  {
    if (m_Pos >= m_PosLimit)
      MoveBlockBackward();  
    BYTE *p = m_Buffer + m_Pos;
    aDistance++;
    for(INT i = 0; i < aLen; i++)
      p[i] = p[i - aDistance];
    m_Pos += aLen;
  }

  void PutOneByte(BYTE aByte)
  {
    if (m_Pos >= m_PosLimit)
      MoveBlockBackward();  
    m_Buffer[m_Pos++] = aByte;
  }

  BYTE GetOneByte(INT anIndex) const
  {
    return m_Buffer[m_Pos + anIndex];
  }

  BYTE *GetBuffer() const { return m_Buffer; }
};

}}

#endif
