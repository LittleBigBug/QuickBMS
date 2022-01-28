#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
using std::min;
using std::max;

#define Fatal(...) exit(1)
#define FATAL(X,Y) if(X) Fatal(Y)
#define MEM_OWN(X)  X

typedef int8_t      int8;
typedef uint8_t     u_int8;
typedef int16_t     int16;
typedef uint16_t    u_int16;
typedef int32_t     int32;
typedef uint32_t    u_int32;
#define NEW new
