#include "stdafx.h"

#include "Stopwatch.h"

#ifdef WIN32
#else
    #define QueryPerformanceFrequency(X)
    #define QueryPerformanceCounter(X)
#endif

Stopwatch::Stopwatch()
{
	QueryPerformanceFrequency(&m_liPerfFreq);
	Start();
}

void Stopwatch::Start()
{
	QueryPerformanceCounter(&m_liPerfStart);
}

__int64 Stopwatch::Elapsed() const
{
	LARGE_INTEGER liPerfNow;
	QueryPerformanceCounter(&liPerfNow);
	return liPerfNow.QuadPart - m_liPerfStart.QuadPart;
}