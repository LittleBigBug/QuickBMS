#ifndef __STREAM_INBYTE_H
#define __STREAM_INBYTE_H

#include "IInOutStreams.h"

namespace NStream {

class CInByte
{
  UINT64 m_ProcessedSize;
  BYTE *m_BufferBase;
  INT m_BufferSize;
  BYTE *m_Buffer;
  BYTE *m_BufferLimit;
  ISequentialInStream* m_Stream;
  bool m_StreamWasExhausted;

  bool ReadBlock();

public:
  CInByte(INT aBufferSize = 0x100000);
  ~CInByte();
  
  void Init(ISequentialInStream *aStream);

  bool ReadByte(BYTE &aByte)
    {
      if(m_Buffer >= m_BufferLimit)
        if(!ReadBlock())
          return false;
      aByte = *m_Buffer++;
      return true;
    }
  BYTE ReadByte()
    {
      if(m_Buffer >= m_BufferLimit)
        if(!ReadBlock())
          return 0x0;
      return *m_Buffer++;
    }
  void ReadBytes(void *aData, INT aSize, INT &aProcessedSize)
    {
      for(aProcessedSize = 0; aProcessedSize < aSize; aProcessedSize++)
        if (!ReadByte(((BYTE *)aData)[aProcessedSize]))
          return;
    }
  bool ReadBytes(void *aData, INT aSize)
    {
      INT aProcessedSize;
      ReadBytes(aData, aSize, aProcessedSize);
      return (aProcessedSize == aSize);
    }
  UINT64 GetProcessedSize() const { return m_ProcessedSize + (m_Buffer - m_BufferBase); }
};

}

#endif
