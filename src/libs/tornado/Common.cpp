// modified by Luigi Auriemma

#define _WIN32_WINNT 0x0500
#include <stdlib.h>
#include "Common.h"
#include "Compression.h"

// Для обработки ошибок во вложенных процедурах - longjmp сигнализирует процедуре верхнего уровня о произошедшей ошибке
int jmpready = FALSE;
jmp_buf jumper;

bool AllocTopDown = true;

// ****************************************************************************
// MEMORY ALLOCATION **********************************************************
// ****************************************************************************

// #define _SZ_ALLOC_DEBUG
/* use _SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef _SZ_ALLOC_DEBUG
#include <stdio.h>
int g_allocCount = 0;
int g_allocCountMid = 0;
int g_allocCountBig = 0;
#define alloc_debug_printf(x) fprintf x
#else
#define alloc_debug_printf(x)
#endif

void *MyAlloc(size_t size) throw()
{
  if (size == 0)
    return 0;
  alloc_debug_printf((stderr, "  Alloc %10d bytes; count = %10d\n", size, g_allocCount++));
  return ::malloc(size);
}

void MyFree(void *address) throw()
{
  if (address != 0)
    alloc_debug_printf((stderr, "  Free; count = %10d\n", --g_allocCount));

  ::free(address);
}

int compress_all_at_once = 1;
