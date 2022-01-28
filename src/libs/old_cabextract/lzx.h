/* Using standard integer types is necessary for AMD64 support */
#ifdef __STDC_VERSION__
#include <stdint.h>

#if !defined(INT8_MAX)
typedef int_least8_t    int8_t;
#endif
#if !defined(INT16_MAX)
typedef int_least16_t    int16_t;
#endif
#if !defined(INT32_MAX)
typedef int_least32_t    int32_t;
#endif
#if !defined(UINT8_MAX)
typedef uint_least32_t    uint8_t;
#endif
#if !defined(UINT16_MAX)
typedef uint_least16_t    uint16_t;
#endif
#if !defined(UINT32_MAX)
typedef uint_least32_t    uint32_t;
#endif

#elif defined _XOPEN_VERSION && _XOPEN_VERSION >= 500
#    include <inttypes.h>
#else 
typedef signed char        int8_t;
typedef signed short int   int16_t;
typedef signed int         int32_t;

typedef unsigned char      uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int       uint32_t;
#endif

typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int8_t BYTE;
typedef int16_t WORD;
typedef int32_t LONG;

int LZXinit(int window);
int LZXdecompress(UBYTE *inbuf, UBYTE *outbuf, ULONG inlen, ULONG outlen);
void LZXreset(void);
