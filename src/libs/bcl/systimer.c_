/*************************************************************************
* Name:        systimer.c
* Author:      Marcus Geelnard
* Description: System dependent high resolution timer functions.
*
* This module is used for timing purposes. It is not part of the Basic
* Compression Library, but used by the test programs. The module provides
* two simple functions:
*
*  InitTimer() - Initializes the timer.
*  GetTime()   - Returns the number of seconds since InitTimer was called.
*
* The code has been tested under Linux, Windows and DOS (DJGPP).
*
*-------------------------------------------------------------------------
* Copyright (c) 2003-2006 Marcus Geelnard
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would
*    be appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not
*    be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source
*    distribution.
*
* Marcus Geelnard
* marcus.geelnard at home.se
*************************************************************************/

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

 /* Windows */
 #include <windows.h>
 #define WINDOWS_TIMER
 typedef struct {
   double  period;
   __int64 t0;
 } systimer_t;

#elif defined(__DJGPP__)

 /* DOS/DJGPP */
 #include <time.h>
 #define DOS_TIMER
 typedef struct {
   uclock_t t0;
 } systimer_t;

#else

 /* Unix (Linux etc) */
 #include <stdlib.h>
 #include <sys/time.h>
 #define GTOD_TIMER
 typedef struct {
   struct timeval t0;
 } systimer_t;

#endif

/* Global timer resource */
static systimer_t global_timer;



/*************************************************************************
* InitTimer()
*************************************************************************/

void InitTimer( void )
{
#if defined(WINDOWS_TIMER)

  __int64 freq;

  global_timer.period = 0.0;
  if( QueryPerformanceFrequency( (LARGE_INTEGER *)&freq ) )
  {
    global_timer.period = 1.0 / (double) freq;
    QueryPerformanceCounter( (LARGE_INTEGER *)&global_timer.t0 );
  }
  else
    global_timer.t0 = (__int64) GetTickCount();

#elif defined(DOS_TIMER)

  global_timer.t0 = uclock();

#elif defined(GTOD_TIMER)

  gettimeofday( &global_timer.t0, NULL );

#endif
}


/*************************************************************************
* GetTime()
*************************************************************************/

double GetTime( void )
{
#if defined(WINDOWS_TIMER)

  __int64 t;

  if( global_timer.period > 0.0 )
  {
    QueryPerformanceCounter( (LARGE_INTEGER *)&t );
    return global_timer.period * (double) (t - global_timer.t0);
  }
  else
    return 0.001 * (double) (GetTickCount() - (int) global_timer.t0);

#elif defined(DOS_TIMER)

  return (1.0 / UCLOCKS_PER_SEC) * (double) (uclock() - global_timer.t0);

#elif defined(GTOD_TIMER)

  struct timeval tv;

  gettimeofday( &tv, NULL );

  tv.tv_sec -= global_timer.t0.tv_sec;
  tv.tv_usec -= global_timer.t0.tv_usec;
  if( tv.tv_usec < 0 )
  {
    --tv.tv_sec;
    tv.tv_usec += 1000000;
  }

  return (double) tv.tv_sec + 0.000001 * (double) tv.tv_usec;

#else

  return 0.0;

#endif
}
