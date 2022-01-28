#ifndef LZH_H
#define LZH_H

#ifndef __TURBOC__
#define far
#endif

typedef unsigned char  uchar;   /*  8 bits or more */
typedef unsigned       uint;    /* 16 bits or more */
typedef unsigned short ushort;  /* 16 bits or more */
typedef unsigned long  ulong;   /* 32 bits or more */

#ifndef LZH_CHAR_BIT
    #define LZH_CHAR_BIT            8
#endif

#ifndef LZH_UCHAR_MAX
    #define LZH_UCHAR_MAX           255
#endif

#define LZH_BITBUFTYPE ushort

#define LZH_BITBUFSIZ (LZH_CHAR_BIT * sizeof (LZH_BITBUFTYPE))
#define LZH_DICBIT    13                              /* 12(-lh4-) or 13(-lh5-) */
#define LZH_DICSIZ (1U << LZH_DICBIT)
#define LZH_MAXMATCH 256                              /* formerly F (not more than UCHAR_MAX + 1) */
#define LZH_THRESHOLD  3                              /* choose optimal value */
#define LZH_NC (LZH_UCHAR_MAX + LZH_MAXMATCH + 2 - LZH_THRESHOLD) /* alphabet = {0, 1, 2, ..., NC - 1} */
#define LZH_CBIT 9                                    /* $\lfloor \log_2 NC \rfloor + 1$ */
#define LZH_CODE_BIT  16                              /* codeword length */

#define LZH_MAX_HASH_VAL (3 * LZH_DICSIZ + (LZH_DICSIZ / 512 + 1) * LZH_UCHAR_MAX)

typedef void far * void_far_pointer;
typedef int (*type_fnc_read) (void far *data, int n);
typedef int (*type_fnc_write) (void far *data, int n);
typedef void_far_pointer (*type_fnc_malloc) (unsigned n);
typedef void (*type_fnc_free) (void far *p);

int lzh_freeze (type_fnc_read   pfnc_read,
                type_fnc_write  pfnc_write,
		type_fnc_malloc pfnc_malloc,
		type_fnc_free   pfnc_free);

int lzh_melt (type_fnc_read   pfnc_read,
              type_fnc_write  pfnc_write,
	      type_fnc_malloc pfnc_malloc,
	      type_fnc_free   pfnc_free,
	      ulong origsize);

#endif /* ifndef LZH_H */
