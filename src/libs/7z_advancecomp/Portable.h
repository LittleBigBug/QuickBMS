#ifndef __PORTABLE_H
#define __PORTABLE_H

#include <string.h>
#include <stdint.h>

typedef signed char INT8;
typedef unsigned char UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef uintptr_t UINT_PTR;
typedef int32_t INT;

typedef UINT8 BYTE;
typedef UINT16 WORD;
typedef UINT32 DWORD;

typedef int BOOL;
#define FALSE 0
#define TRUE 1

#define HRESULT int
#define S_OK 0
#define E_INVALIDARG -1
#define E_OUTOFMEMORY -2
#define E_FAIL -3
#define E_INTERNAL_ERROR -4
#define E_INVALIDDATA -5

template <class T> inline T MyMin(T a, T b) {
	return a < b ? a : b;
}

template <class T> inline T MyMax(T a, T b) {
	return a > b ? a : b;
}

#define RETURN_IF_NOT_S_OK(x) { HRESULT __aResult_ = (x); if(__aResult_ != S_OK) return __aResult_; }

#endif
