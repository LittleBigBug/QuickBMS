// modified by Luigi Auriemma

// Common definitions, used by various parts of FreeArc project
#ifndef FREEARC_COMMON_H
#define FREEARC_COMMON_H

#define FREEARC_DECOMPRESS_ONLY
#ifdef WIN32
    #define FREEARC_UNIX
#else
    #define FREEARC_UNIX
#endif
#define FREEARC_INTEL_BYTE_ORDER
//#undef __cplusplus
#define FREEARC_NO_TIMING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <setjmp.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64__)
#define FREEARC_64BIT
#endif

#ifdef FREEARC_WIN
#  include <windows.h>
#  include <direct.h>
#  include <float.h>
#  ifndef __GNUC__
#    define logb       _logb
#  endif
#  define strcasecmp stricmp
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
** Базовые определения FREEARC ************************************************
******************************************************************************/
#if !defined(FREEARC_WIN) && !defined(FREEARC_UNIX)
#error "You must define OS!"
#endif

#if defined(FREEARC_INTEL_BYTE_ORDER)
#if _BIG_ENDIAN
#error "You're compiling for Motorola byte order, but FREEARC_INTEL_BYTE_ORDER was defined."
#endif
#elif defined(FREEARC_MOTOROLA_BYTE_ORDER)
#if _M_IX86 || __i386 || __x86_64
#error "You're compiling for Intel byte order, but FREEARC_MOTOROLA_BYTE_ORDER was defined."
#endif
#else
#error "You must define byte order!"
#endif

#ifdef FREEARC_WIN
#define WINDOWS_ONLY
#else
#define WINDOWS_ONLY(_)
#endif

#ifdef FREEARC_UNIX
#define UNIX_ONLY
#else
#define UNIX_ONLY(_)
#endif

#ifdef FREEARC_WIN
#define UNSUPPORTED_PATH_DELIMITERS   "/"
#define PATH_DELIMITER                '\\'
#define STR_PATH_DELIMITER            "\\"
#else
#define UNSUPPORTED_PATH_DELIMITERS   ":\\"
#define PATH_DELIMITER                '/'
#define STR_PATH_DELIMITER            "/"
#endif

#define  DIRECTORY_DELIMITERS         "/\\"
#define  ALL_PATH_DELIMITERS          ":/\\"


/******************************************************************************
** Синонимы для простых типов, используемых в программе ***********************
******************************************************************************/
typedef unsigned long        ulong;
typedef unsigned int         uint,   UINT;
typedef unsigned short int   ushort;
typedef unsigned char        uchar;
#ifdef __GNUC__
#include <stdint.h>
typedef          uint64_t    uint64;
typedef          uint32_t    uint32;
typedef          uint16_t    uint16;
typedef          uint8_t     uint8,  byte, BYTE;
typedef          int64_t     sint64, int64;
typedef          int32_t     sint32, int32;
typedef          int16_t     sint16, int16;
typedef          int8_t      sint8,  int8;
#else
typedef          __int64     sint64, int64;
typedef unsigned __int64     uint64;
typedef          __int32     sint32, int32;
typedef unsigned __int32     uint32;
typedef          __int16     sint16, int16;
typedef unsigned __int16     uint16;
typedef          __int8      sint8,  int8;
typedef unsigned __int8      uint8,  byte, BYTE;
#endif

typedef size_t               MemSize;          // объём памяти
typedef uint64               LongMemSize;
#define MEMSIZE_MAX          UINT_MAX
#ifdef FREEARC_WIN
typedef int64                FILESIZE;         // размер файла
#else
typedef off_t                FILESIZE;
#endif
typedef char*                FILENAME;         // имя файла


/******************************************************************************
** Коды ошибок ****************************************************************
******************************************************************************/
#define FREEARC_OK                               0     /* ALL RIGHT */
#define FREEARC_ERRCODE_GENERAL                  (-1)  /* Some error when (de)compressing */
#define FREEARC_ERRCODE_INVALID_COMPRESSOR       (-2)  /* Invalid compression method or parameters */
#define FREEARC_ERRCODE_ONLY_DECOMPRESS          (-3)  /* Program was compiled with FREEARC_DECOMPRESS_ONLY, so don't try to use compress() */
#define FREEARC_ERRCODE_OUTBLOCK_TOO_SMALL       (-4)  /* Output block size in (de)compressMem is not enough for all output data */
#define FREEARC_ERRCODE_NOT_ENOUGH_MEMORY        (-5)  /* Can't allocate memory needed for (de)compression */
#define FREEARC_ERRCODE_READ                     (-6)  /* Error when reading data */
#define FREEARC_ERRCODE_BAD_COMPRESSED_DATA      (-7)  /* Data can't be decompressed */
#define FREEARC_ERRCODE_NOT_IMPLEMENTED          (-8)  /* Requested feature isn't supported */
#define FREEARC_ERRCODE_NO_MORE_DATA_REQUIRED    (-9)  /* Required part of data was already decompressed */
#define FREEARC_ERRCODE_OPERATION_TERMINATED    (-10)  /* Operation terminated by user */
#define FREEARC_ERRCODE_WRITE                   (-11)  /* Error when writing data */
#define FREEARC_ERRCODE_BAD_CRC                 (-12)  /* File failed CRC check */
#define FREEARC_ERRCODE_BAD_PASSWORD            (-13)  /* Password/keyfile failed checkcode test */
#define FREEARC_ERRCODE_BAD_HEADERS             (-14)  /* Archive headers are corrupted */
#define FREEARC_ERRCODE_INTERNAL                (-15)  /* It should never happen: implementation error. Please report this bug to developers! */


/******************************************************************************
** Стандартные определения ****************************************************
******************************************************************************/
#define make4byte(a,b,c,d)       ((a)+256*((b)+256*((c)+256*(((uint32)d)))))
#define iterate(num, statement)  {for( int i=0; i<(num); i++) {statement;}}
#define iterate_var(i, num)      for( int i=0; i<(num); i++)
#define iterate_array(i, array)  for( int i=0; i<(array).size; i++)
#ifndef TRUE
#define TRUE                     1
#endif
#ifndef FALSE
#define FALSE                    0
#endif

#define PATH_CHARS               ":/\\"
#define t_str_end(str)           (_tcsrchr (str,'\0'))
#define t_last_char(str)         (t_str_end(str) [-1])
#define t_strequ(a,b)            (_tcscmp((a),(b))==EQUAL)
#define is_path_char(c)          in_set(c, PATH_CHARS)
#define in_set(c, set)           (strchr (set, c ) != NULL)
#define in_set0(c, set)          (memchr (set, c, sizeof(set) ) != 0)
#define str_end(str)             (strchr (str,'\0'))
#define last_char(str)           (str_end(str) [-1])
#define strequ(a,b)              (strcmp((a),(b))==EQUAL)
#define namecmp                  strcasecmp
#define nameequ(s1,s2)           (namecmp(s1,s2)==EQUAL)
#define start_with(str,with)     (strncmp((str),(with),strlen(with))==EQUAL? (str)+strlen(with) : NULL)
#define end_with(str,with)       (nameequ (str_end(str)-strlen(with), with))
#define find_extension(str)      (find_extension_in_entry (drop_dirname(str)))
#define mymax(a,b)               ((a)>(b)? (a) : (b))
#define mymin(a,b)               ((a)<(b)? (a) : (b))
#define inrange(x,a,b)           ((a)<=(x) && (x)<(b))
#define elements(arr)            (sizeof(arr)/sizeof(*arr))
#define endof(arr)               ((arr)+elements(arr))
#define zeroArray(arr)           (memset (arr, 0, sizeof(arr)))
#define EQUAL                    0   /* result of strcmp/memcmp for equal strings */


// ****************************************************************************
// FILE OPERATIONS ************************************************************
// ****************************************************************************

#define MY_FILENAME_MAX      65536              /* maximum length of filename */
#define MAX_PATH_COMPONENTS  256                /* maximum number of directories in filename */

#ifdef FREEARC_WIN

#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <share.h>
#ifndef __GNUC__
#define file_seek(stream,pos)                   (_fseeki64(stream, (pos), SEEK_SET))
#define file_seek_cur(stream,pos)               (_fseeki64(stream, (pos), SEEK_CUR))
#else
#define file_seek(stream,pos)                   (fseeko64(stream, (pos), SEEK_SET))
#define file_seek_cur(stream,pos)               (fseeko64(stream, (pos), SEEK_CUR))
#endif
#define set_flen(stream,new_size)               (chsize( file_no(stream), new_size ))
#define get_flen(stream)                        (_filelengthi64(fileno(stream)))
#define myeof(file)                             (feof(file))
#define get_ftime(stream,tstamp)                getftime( file_no(stream), (struct ftime *) &tstamp )
#define set_ftime(stream,tstamp)                setftime( file_no(stream), (struct ftime *) &tstamp )
#define set_binary_mode(file)                   setmode(fileno(file),O_BINARY)

// Number of 100 nanosecond units from 01.01.1601 to 01.01.1970
#define EPOCH_BIAS    116444736000000000ull

static inline void WINAPI UnixTimeToFileTime( time_t time, FILETIME* ft )
{
  *(uint64*)ft = EPOCH_BIAS + time * 10000000ull;
}


#endif // FREEARC_WIN


#ifdef FREEARC_UNIX

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#ifdef WIN32
#else
#define __cdecl
#endif
#define SSIZE_T ssize_t

typedef char  TCHAR;
#define _T
#define _tcscmp         strcmp
#define _tcschr         strchr
#define	_tcsrchr	strrchr
#define _tcscpy         strcpy
#define _stprintf       sprintf
#define _tcslen         strlen
#define	_tcslwr		strlwr
#define _tstat          stat
#define _stat           stat
#define _trmdir         rmdir
#define _trename        rename
#define _tremove        remove
#define _taccess        access

typedef int (*FARPROC) (void);


#endif // FREEARC_UNIX


// ****************************************************************************
// ****************************************************************************
// ****************************************************************************

#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)


#if (GCC_VERSION >= 304)
#  define prefetch(var)   __builtin_prefetch(&(var))
#elif 1
#include <xmmintrin.h>
#  define prefetch(var)   (_mm_prefetch((char*)&(var),_MM_HINT_T0))
#else
#  define prefetch(var)   ((void)0)
#endif

#define CACHE_ROW 64  /* size of typical CPU cache line: from long data only every 64'th byte should be prefetched */

#ifdef FREEARC_INTEL_BYTE_ORDER

// Read unsigned 16/24/32-bit value at given address
#define value16(p)               (*(uint16*)(p))
#define value24(p)               (*(uint32*)(p) & 0xffffff)
#define value32(p)               (*(uint32*)(p))
#define value64(p)               (*(uint64*)(p))
// Write unsigned 16/24/32-bit value to given address
#define setvalue16(p,x)          (*(uint16*)(p) = (x))
#define setvalue24(p,x)          (*(uint32*)(p) = ((x) & 0xffffff) + (*(uint32*)(p) & 0xff000000))
#define setvalue32(p,x)          (*(uint32*)(p) = (x))
#define setvalue64(p,x)          (*(uint64*)(p) = (x))

#elif FREEARC_MOTOROLA_BYTE_ORDER
// routines for non-little-endian cpus, written by Joachim Henke
#if _ARCH_PPC
#if __GNUC__ == 4 && __GNUC_MINOR__ > 0 || __GNUC__ > 4
#define PPC_MCONSTR "Z"
#else
#define PPC_MCONSTR "Q"
#endif
#define PPC_LBRX(s,p,x)   __asm__ ("l"  s "brx %0,%y1" : "=r" (x) : PPC_MCONSTR (*p))
#define PPC_STBRX(s,p,x)  __asm__ ("st" s "brx %1,%y0" : "=" PPC_MCONSTR (*p) : "r" (x))
#endif

static inline uint16 value16 (void *p)
{
  uint16 x;
#if _ARCH_PPC
  uint16 *m = (uint16 *)p;
  PPC_LBRX("h", m, x);
#else
  uint8 *m = (uint8 *)p;
  x = m[0] + (m[1] << 8);
#endif
  return x;
}

static inline uint32 value24 (void *p)
{
  uint32 x;
#if __GNUC__ == 4 && __GNUC_MINOR__ > 2 || __GNUC__ > 4
  uint32 *m = (uint32 *)p;
  x = __builtin_bswap32(*m) & 0xffffff;
#elif _ARCH_PPC
  uint32 *m = (uint32 *)p;
  PPC_LBRX("w", m, x);
  x &= 0xffffff;
#else
  uint8 *m = (uint8 *)p;
  x = m[0] + (m[1] << 8) + (m[2] << 16);
#endif
  return x;
}

static inline uint32 value32 (void *p)
{
  uint32 x;
#if __GNUC__ == 4 && __GNUC_MINOR__ > 2 || __GNUC__ > 4
  uint32 *m = (uint32 *)p;
  x = __builtin_bswap32(*m);
#elif _ARCH_PPC
  uint32 *m = (uint32 *)p;
  PPC_LBRX("w", m, x);
#else
  uint8 *m = (uint8 *)p;
  x = m[0] + (m[1] << 8) + (m[2] << 16) + (m[3] << 24);
#endif
  return x;
}

static inline uint64 value64 (void *p)
{
  uint64 x;
#if _ARCH_PPC && __PPU__
  uint64 *m = (uint64 *)p;
  PPC_LBRX("d", m, x);
#else
  uint32 *m = (uint32 *)p;
  x = value32(m) + ((uint64)value32(m + 1) << 32);
#endif
  return x;
}

static inline void setvalue16 (void *p, uint16 x)
{
#if _ARCH_PPC
  uint16 *m = (uint16 *)p;
  PPC_STBRX("h", m, x);
#else
  uint8 *m = (uint8 *)p;
  m[0] = x;
  m[1] = x >> 8;
#endif
}

static inline void setvalue24 (void *p, uint32 x)
{
  uint8 *m = (uint8 *)p;
  m[0] = x;
  m[1] = x >> 8;
  m[2] = x >> 16;
}

static inline void setvalue32 (void *p, uint32 x)
{
#if __GNUC__ == 4 && __GNUC_MINOR__ > 2 || __GNUC__ > 4
  uint32 *m = (uint32 *)p;
  *m = __builtin_bswap32(x);
#elif _ARCH_PPC
  uint32 *m = (uint32 *)p;
  PPC_STBRX("w", m, x);
#else
  uint8 *m = (uint8 *)p;
  m[0] = x;
  m[1] = x >> 8;
  m[2] = x >> 16;
  m[3] = x >> 24;
#endif
}

static inline void setvalue64 (void *p, uint64 x)
{
#if _ARCH_PPC && __PPU__
  uint64 *m = (uint64 *)p;
  PPC_STBRX("d", m, x);
#else
  uint32 *m = (uint32 *)p;
  setvalue32(m, x);
  setvalue32(m + 1, x >> 32);
#endif
}

#endif //FREEARC_MOTOROLA_BYTE_ORDER


static inline uint16 value16b (void *p)
{
#if defined(FREEARC_INTEL_BYTE_ORDER) && defined(_MSC_VER)
  return _byteswap_ushort(value16(p));
#else
  uint8 *m = (uint8 *)p;
  return (m[0] << 8) + m[1];
#endif
}

static inline uint32 value32b (void *p)
{
#if defined(FREEARC_INTEL_BYTE_ORDER) && defined(_MSC_VER)
  return _byteswap_ulong(value32(p));
#elif __GNUC__ == 4 && __GNUC_MINOR__ > 2 || __GNUC__ > 4
  return __builtin_bswap32(value32(p));
#else
  uint8 *m = (uint8 *)p;
  return (m[0] << 24) + (m[1] << 16) + (m[2] << 8) + m[3];
#endif
}

static inline void setvalue16b (void *p, uint32 x)
{
#if defined(FREEARC_INTEL_BYTE_ORDER) && defined(_MSC_VER)
  uint16 *m = (uint16 *)p;
  *m = _byteswap_ushort(x);
#else
  uint8 *m = (uint8 *)p;
  m[0] = x >> 8;
  m[1] = x;
#endif
}

static inline void setvalue32b (void *p, uint32 x)
{
#if defined(FREEARC_INTEL_BYTE_ORDER) && defined(_MSC_VER)
  uint32 *m = (uint32 *)p;
  *m = _byteswap_ulong(x);
#elif __GNUC__ == 4 && __GNUC_MINOR__ > 2 || __GNUC__ > 4
  uint32 *m = (uint32 *)p;
  *m = __builtin_bswap32(x);
#else
  uint8 *m = (uint8 *)p;
  m[0] = x >> 24;
  m[1] = x >> 16;
  m[2] = x >> 8;
  m[3] = x;
#endif
}



// Check for equality
#define val16equ(p,q)             (*(uint16*)(p) == *(uint16*)(q))
#define val24equ(p,q)             (   value24(p) ==    value24(q))
#define val32equ(p,q)             (*(uint32*)(p) == *(uint32*)(q))
#define val64equ(p,q)             (*(uint64*)(p) == *(uint64*)(q))

// Free memory block and set pointer to NULL
#ifndef FreeAndNil
#define FreeAndNil(p)            ((p) && (free(p),    (p)=NULL))
#define BigFreeAndNil(p)         ((p) && (BigFree(p), (p)=NULL))
#endif

// Exit code used to indicate serious problems in FreeArc utilities
#define FREEARC_EXIT_ERROR 2

// Переменные, используемые для сигнализации об ошибках из глубоко вложеных процедур
extern int jmpready;
extern jmp_buf jumper;

// Процедура сообщения о неожиданных ошибочных ситуациях
#ifndef CHECK
#  if defined(FREEARC_WIN) && defined(FREEARC_GUI)
#    define CHECK(e,a,b)           {if (!(a))  {if (jmpready) longjmp(jumper,1);  char *s=(char*)malloc_msg(MY_FILENAME_MAX*4);  WCHAR *utf16=(WCHAR*) malloc_msg(MY_FILENAME_MAX*4);  sprintf b;  utf8_to_utf16(s,utf16);  MessageBoxW(NULL, utf16, L"Error encountered", MB_ICONERROR);  ON_CHECK_FAIL();  exit(FREEARC_EXIT_ERROR);}}
#  elif defined(FREEARC_WIN)
#    define CHECK(e,a,b)           {if (!(a))  {if (jmpready) longjmp(jumper,1);  char *s=(char*)malloc_msg(MY_FILENAME_MAX*4),  *oem=(char*)malloc_msg(MY_FILENAME_MAX*4);  sprintf b;  utf8_to_oem(s,oem);  printf("\n%s",oem);  ON_CHECK_FAIL();  exit(FREEARC_EXIT_ERROR);}}
#  else
#    define CHECK(e,a,b)           {if (!(a))  {if (jmpready) longjmp(jumper,1);  char s[MY_FILENAME_MAX*4];  sprintf b;  printf("\n%s",s);  ON_CHECK_FAIL();  exit(FREEARC_EXIT_ERROR);}}
#  endif
#endif

#ifndef ON_CHECK_FAIL
#define ON_CHECK_FAIL()
#endif

// Устанавливает Jump Point с переходом на метку label
#define SET_JMP_POINT_GOTO(label)                                                      \
{                                                                                      \
  if (!jmpready && setjmp(jumper) != 0)                                                \
    /* Сюда мы попадём при возникновении ошибки в одной из вызываемых процедур */      \
    {jmpready = FALSE; goto label;}                                                    \
  jmpready = TRUE;                                                                     \
}

// Устанавливает Jump Point с кодом возврата retcode
#define SET_JMP_POINT(retcode)                                                         \
{                                                                                      \
  if (!jmpready && setjmp(jumper) != 0)                                                \
    /* Сюда мы попадём при возникновении ошибки в одной из вызываемых процедур */      \
    {jmpready = FALSE; return retcode;}                                                \
  jmpready = TRUE;                                                                     \
}

// Снимает Jump Point
#define RESET_JMP_POINT()                                                              \
{                                                                                      \
  jmpready = FALSE;                                                                    \
}


// Include statements marked as debug(..)  only if we enabled debugging
#ifdef DEBUG
#define debug(stmt)  stmt
#else
#define debug(stmt)  ((void)0)
#endif

// Include statements marked with stat_only(..)  only if we enabled gathering stats
#ifdef STAT
#define stat_only(stmt)  stmt
#else
#define stat_only(stmt)  ((void)0)
#endif

// Define default parameter value only when compiled as C++
#ifdef __cplusplus
#define DEFAULT(x,n) x=n
#else
#define DEFAULT(x,n) x
#endif

#define then


// ****************************************************************************
// MEMORY ALLOCATION **********************************************************
// ****************************************************************************
#ifndef __cplusplus
#define throw()
#endif

void *MyAlloc(size_t size) throw();
void MyFree(void *address) throw();
extern bool AllocTopDown;
#ifdef FREEARC_WIN
enum LPType {DEFAULT, FORCE, DISABLE, TRY};
extern LPType DefaultLargePageMode;
void *MidAlloc(size_t size) throw();
void MidFree(void *address) throw();
void *BigAlloc (int64 size, LPType LargePageMode=DEFAULT) throw();
void BigFree(void *address) throw();
#else
#define MidAlloc(size) MyAlloc(size)
#define MidFree(address) MyFree(address)
#define BigAlloc(size) MyAlloc(size)
#define BigFree(address) MyFree(address)
#endif // !FREEARC_WIN


// Round first number *down* to divisible by second one
static inline MemSize roundDown (MemSize a, MemSize b)
{
  return b>1? (a/b)*b : a;
}

// Round first number *up* to divisible by second one
static inline MemSize roundUp (MemSize a, MemSize b)
{
  return (a!=0 && b>1)? roundDown(a-1,b)+b : a;
}


// Whole part of number's binary logarithm (logb) - please ensure that n > 0
static inline int lb (MemSize n)
{
    int result;
#if __INTEL_COMPILER || (_MSC_VER >= 1400)
#if defined(__x86_64) || defined(_M_X64)
    _BitScanReverse64((DWORD *)&result, n);
#else
    _BitScanReverse((DWORD *)&result, n);
#endif
#elif __GNUC__ == 3 && __GNUC_MINOR__ > 3 || __GNUC__ > 3
#if defined(__x86_64) || defined(_M_X64)
     result = __builtin_clzll(n) ^ (8 * sizeof(unsigned long long) - 1);
#else
     result = __builtin_clz(n)   ^ (8 * sizeof(unsigned int) - 1);
#endif
#else
    result = 0;
#if defined(__x86_64) || defined(_M_X64)
    if (n > 0xffffffffu)  result  = 32, n >>= 32;
#endif
    if (n > 0xffff)       result += 16, n >>= 16;
    if (n > 0xff)         result +=  8, n >>= 8;
    if (n > 0xf)          result +=  4, n >>= 4;
    if (n > 0x3)          result +=  2, n >>= 2;
    if (n > 0x1)          result +=  1;
#endif
    return result;
}

#if __INTEL_COMPILER || (_MSC_VER && _MSC_VER<1800)
static inline double log2  (double x)  {return log(x)/log(2.);}
static inline double round (double x)  {return (x > 0.0) ? floor(x + 0.5) : ceil(x - 0.5);}
#endif

// Эта процедура округляет число к ближайшей сверху степени
// базы, например f(13,2)=16
static inline MemSize roundup_to_power_of (MemSize n, MemSize base)
{
    MemSize result = base;
    if (!n)
        return 0;
    if (!(--n))
        return 1;
    if (base == 2)
        result <<= lb(n);
    else
        while (n /= base)
            result *= base;
    return result;
}

// Эта процедура округляет число к ближайшей снизу степени
// базы, например f(13,2)=8
static inline MemSize rounddown_to_power_of (MemSize n, MemSize base)
{
    MemSize result = 1;
    if (!n)
        return 1;
    if (base == 2)
        result <<= lb(n);
    else
        while (n /= base)
            result *= base;
    return result;
}

// Эта процедура округляет число к логарифмически ближайшей степени
// базы, например f(9,2)=8  f(15,2)=16
static inline MemSize round_to_nearest_power_of (MemSize n, MemSize base)
{
    MemSize result;
    uint64 nn = ((uint64)n)*n/base;
    if (nn==0)  return 1;
    for (result=base; (nn/=base*base) != 0; result *= base);
    return result;
}

#ifdef __cplusplus

}       // extern "C"

#endif  // __cplusplus

#endif  // FREEARC_COMMON_H
