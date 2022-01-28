#pragma once

#define __declspec(X)
#ifdef WIN32
#include <windows.h>
#else
typedef union _LARGE_INTEGER {
  struct {
    long LowPart;
    long  HighPart;
  };
  struct {
    long LowPart;
    long  HighPart;
  } u;
  long long QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;
typedef long long   __int64;
#endif

class Stopwatch
{
public:
	__declspec(dllexport) Stopwatch();

	__declspec(dllexport) void Start();

	__declspec(dllexport) __int64 Elapsed() const;

private:
	LARGE_INTEGER m_liPerfFreq;
	LARGE_INTEGER m_liPerfStart;
};