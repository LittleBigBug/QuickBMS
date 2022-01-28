/* ------------------------------------------------------------------------- *
 *
 *   Program:   A C H T E R B A H N  - 128 / 80
 *              Version 1.1
 *
 *   Authors:   Berndt M. Gammel, Email: gammel@matpack.de
 *              Rainer Goettfert, Email: rainer.goettfert@gmx.de
 *              Oliver Kniffler,  Email: oliver.kniffler@arcor.de
 *
 *  Language:   ANSI C99
 *
 *   Sources:   achterbahn.c
 *              ecrypt-sync.h
 *
 *  Includes:   ecrypt-portable.h,
 *              ecrypt-config.h,
 *              ecrypt-machine.h
 *
 *  Makefile:   Makefile
 *
 * Platforms:   This program has been tested on the following platforms:
 *              gcc 3.4.4, Cygwin, Windows 2000
 *              gcc 4.0.2, S.u.S.E. Linux 10.0
 *
 * Copyright:   (C) 2005-2006 by Berndt M. Gammel, Rainer Goettfert, 
 *                               and Oliver Kniffler
 *
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * if COMPILE_TEST_CODE is defined the implementation test code (main)
 * in this file will be compiled.
 * Preferably this flag is defined by the Makefile, not in the code.
 * ------------------------------------------------------------------------- */

/* #define COMPILE_TEST_CODE */

/* ------------------------------------------------------------------------- *
 * general include files
 * ------------------------------------------------------------------------- */

#include <assert.h>

#ifdef COMPILE_TEST_CODE
#include <stdio.h>
#include <string.h>
#endif

/* ------------------------------------------------------------------------- *
 * ECRYPT include file
 * ------------------------------------------------------------------------- */

#include "ecrypt-sync.h"

/* ------------------------------------------------------------------------- *
 * Achterbahn local definitions
 * ------------------------------------------------------------------------- */

enum { 
  /* truth values */
  false    = 0U,
  true     = 1U,

  /* unsigned constants */
  ZERO     = 0U,
  ONE      = 1U,       
         
  /* NLFSR lengths */
  A0_size  = 21U,              /* NLFSR A0:  n = 21 */
  A1_size  = 22U,              /* NLFSR A1:  n = 22 */
  A2_size  = 23U,              /* NLFSR A2:  n = 23 */
  A3_size  = 24U,              /* NLFSR A3:  n = 24 */
  A4_size  = 25U,              /* NLFSR A4:  n = 25 */
  A5_size  = 26U,              /* NLFSR A5:  n = 26 */
  A6_size  = 27U,              /* NLFSR A6:  n = 27 */
  A7_size  = 28U,              /* NLFSR A7:  n = 28 */
  A8_size  = 29U,              /* NLFSR A8:  n = 29 */
  A9_size  = 30U,              /* NLFSR A9:  n = 30 */
  A10_size = 31U,              /* NLFSR A10: n = 31 */
  A11_size = 32U,              /* NLFSR A11: n = 32 */
  A12_size = 33U,              /* NLFSR A12: n = 33 */
};

/* bit mask which is equal to the period */
const u32 A0_mask  = 0x001FFFFFU;      /* NLFSR A0:  n = 21 */
const u32 A1_mask  = 0x003FFFFFU;      /* NLFSR A1:  n = 22 */
const u32 A2_mask  = 0x007FFFFFU;      /* NLFSR A2:  n = 23 */
const u32 A3_mask  = 0x00FFFFFFU;      /* NLFSR A3:  n = 24 */
const u32 A4_mask  = 0x01FFFFFFU;      /* NLFSR A4:  n = 25 */
const u32 A5_mask  = 0x03FFFFFFU;      /* NLFSR A5:  n = 26 */
const u32 A6_mask  = 0x07FFFFFFU;      /* NLFSR A6:  n = 27 */
const u32 A7_mask  = 0x0FFFFFFFU;      /* NLFSR A7:  n = 28 */
const u32 A8_mask  = 0x1FFFFFFFU;      /* NLFSR A8:  n = 29 */
const u32 A9_mask  = 0x3FFFFFFFU;      /* NLFSR A9:  n = 30 */
const u32 A10_mask = 0x7FFFFFFFU;      /* NLFSR A10: n = 31 */
const u32 A11_mask = 0xFFFFFFFFU;      /* NLFSR A11: n = 32 */
const u64 A12_mask = 0x1FFFFFFFFULL;   /* NLFSR A12: n = 33 */

/* ------------------------------------------------------------------------- *
 * macros for notational convenience
 * ------------------------------------------------------------------------- */

/* Boolean functions of basic gates */
#define AND3(a,b,c) ((a)&(b)&(c))
#define AND2(a,b)   ((a)&(b))
#define XOR3(a,b,c) ((a)^(b)^(c))
#define XOR2(a,b)   ((a)^(b))
#define MUX3(a,b,c) (((c)&((a)^(b)))^(a))
#define MAJ3(a,b,c) (((a)&((b)^(c)))^((b)&(c)))

/* ------------------------------------------------------------------------- *
 * Functions to step each NLFSR by one clock cycle are implemented as macros.
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * NLFSR A0 (length 21, period = 2097151) 
 *
 * A0(x) = x0 + x15 + x2 + x3 + x4 + x5 + x6 + x8 + x11 + x1*x11 + x11*x2 
 *            + x12*x2 + x4*x6 x4*x7 + x5*x6 + + x1*x11*x2 + x1*x12*x2 
 *            + x1*x11*x9 + x10*x11*x9 + x1*x13*x2*x6 + x1*x11*x2*x9 
 *            + x10*x11*x2*x9 + x1*x12*x2*x9 + x10*x12*x2*x9 
 *            + x1*x13*x2*x6*x9 + x10*x13*x2*x6*x9
 *  
 * ------------------------------------------------------------------------- */

#define A0_cycle(x,feedin)\
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>15,\
                                        XOR3((x)>>3,(x)>>2,(x))),\
                                   XOR3(AND2((x)>>4,(x)>>7),\
                                        XOR3((x)>>5,(x)>>6,(x)>>8),\
                                        MUX3((x)>>4,(x)>>5,(x)>>6)),\
                                   MUX3(MUX3((x)>>11,(x)>>12,(x)>>2),\
                                        AND3((x)>>6,(x)>>2,(x)>>13),\
                                        MUX3((x)>>1,(x)>>10,(x)>>9)))\
                     )) << 20))

/* ------------------------------------------------------------------------- *
 * NLFSR A1 (length 22, period = 4194303)
 *
 * A1(x) = x0 + x1 + x5 + x6 + x8 + x13 + x15 + x1*x13 + x1*x11*x14 + x1*x3 
 *            + x1*x7 + x11*x5 + x12*x4 + x12*x6 + x7*x9 + x1*x10*x11*x12*x14 
 *            + x1*x11*x14*x4 + x1*x10*x11*x14*x4 + x1*x11*x14*x7 
 *            + x1*x11*x14*x7*x9
 * 
 * ------------------------------------------------------------------------- */

#define A1_cycle(x,feedin)\
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>15,\
                                        XOR3((x)>>8,(x)>>5,(x))),\
                                   XOR3(AND2((x)>>5,(x)>>11),\
                                        MUX3((x)>>13,(x)>>3,(x)>>1),\
                                        MUX3((x)>>6,(x)>>4,(x)>>12)),\
                                   MUX3(MUX3((x)>>1,(x)>>9,(x)>>7),\
                                        MUX3((x)>>4,(x)>>12,(x)>>10),\
                                        AND3((x)>>1,(x)>>11,(x)>>14)))\
                     )) << 21))

/* ------------------------------------------------------------------------- *
 * NLFSR A2 (length 23, period = 8388607)
 *
 * A2(x) = x0 + x4 + x5 + x13 + x16 + x11*x5 + x1*x6 + x12*x14 + x4*x6 
 *            + x1*x7 + x11*x8 + x7*x9 + x1*x10*x15*x9 + x1*x10*x11*x15*x9 
 *            + x1*x11*x15*x3*x9 + x1*x15*x5*x9 + x1*x11*x15*x5*x9 
 *            + x1*x11*x15*x8*x9
 *
 * ------------------------------------------------------------------------- */

#define A2_cycle(x,feedin)\
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>16,\
                                        XOR3((x)>>13,(x)>>4,(x))),\
                                   XOR3(AND2((x)>>12,(x)>>14),\
                                        MUX3((x)>>1,(x)>>9,(x)>>7),\
                                        MUX3((x)>>1,(x)>>4,(x)>>6)),\
                                   MUX3(MUX3((x)>>5,(x)>>8,(x)>>11),\
                                        MUX3((x)>>10,(x)>>3,(x)>>11),\
                                        AND3((x)>>1,(x)>>9,(x)>>15)))\
                     )) << 22))

/* ------------------------------------------------------------------------- *
 * NLFSR A3 (length 24, period = 16777215)
 *
 * A3(x) = x0 + x2 + x3 + x6 + x8 + x12 + x18 + x1*x11 + x1*x15 + x12*x13 
 *            + x13*x14 + x13*x2 + x13*x4 + x14*x2*x5 + x15*x6 + x1*x15*x2*x5 
 *            + x2*x5*x6 + x14*x2*x7 + x14*x5*x7 + x2*x6*x7 + x5*x6*x7 
 *            + x15*x2*x5*x6 + x1*x15*x2*x7 + x1*x15*x5*x7 + x14*x2*x5*x9 
 *            + x16*x2*x5*x9 + x15*x2*x6*x7 + x14*x2*x7*x9 + x14*x5*x7*x9 
 *            + x16*x2*x7*x9 + x15*x5*x6*x7 + x16*x5*x7*x9
 *
 * ------------------------------------------------------------------------- */

#define A3_cycle(x,feedin)\
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>18,\
                                        XOR3((x)>>8,(x)>>3,(x))),\
                                   XOR3(AND2((x)>>1,(x)>>11),\
                                        MUX3((x)>>2,(x)>>14,(x)>>13),\
                                        MUX3((x)>>12,(x)>>4,(x)>>13)),\
                                   MUX3(MUX3((x)>>6,(x)>>1,(x)>>15),\
                                        MUX3((x)>>14,(x)>>16,(x)>>9),\
                                        MAJ3((x)>>2,(x)>>5,(x)>>7)))\
                     )) << 23))

/* ------------------------------------------------------------------------- *
 * NLFSR A4 (length 25, period = 33554431)  
 *
 * A4(x) = x0 + x6 + x11 + x20 + x15*x17 + x14*x2*x3 + x12*x4 + x1*x5 + x14*x5
 *            + x3*x5 + x12*x13*x5 + x12*x14*x5 + x14*x2*x5 + x15*x17*x5 
 *            + x15*x8 + x16*x6 + x16*x7 + x17*x8 + x15*x5*x8 + x12*x13*x2*x3 
 *            + x12*x14*x2*x3 + x12*x13*x2*x5 + x12*x14*x2*x5 + x17*x5*x8 
 *            + x15*x17*x2*x3 + x15*x17*x2*x5 + x15*x2*x3*x8 + x17*x2*x3*x8 
 *            + x15*x2*x5*x8 + x17*x2*x5*x8
 * 
 * ------------------------------------------------------------------------- */

#define A4_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>20,\
                                        XOR3((x)>>11,(x)>>1,(x))),\
                                   XOR3(AND2((x)>>4,(x)>>12),\
                                        MUX3((x)>>1,(x)>>3,(x)>>5),\
                                        MUX3((x)>>6,(x)>>7,(x)>>16)),\
                                   MUX3(MAJ3((x)>>8,(x)>>15,(x)>>17),\
                                        MUX3((x)>>14,(x)>>13,(x)>>12),\
                                        MUX3((x)>>5,(x)>>3,(x)>>2)))\
                     )) << 24))

/* ------------------------------------------------------------------------- *
 * NLFSR A5 (length 26, period = 67108863) 
 *
 * A5(x) = x0 + x4 + x5 + x15 + x16 + x17 + x21 + x12*x13 + x10*x14*x15 
 *            + x13*x4 + x18*x2 + x10*x14*x3 + x14*x15*x3 + x2*x4 + x10*x15*x4 
 *            + x10*x3*x4 + x3*x6 + x10*x11*x15*x7 + x10*x11*x3*x7 
 *            + x10*x12*x13*x15 + x10*x12*x13*x3 + x10*x13*x15*x4 
 *            + x10*x14*x15*x7 + x10*x13*x3*x4 + x10*x14*x3*x7 + x11*x15*x3*x7 
 *            + x12*x13*x15*x3 + x13*x15*x3*x4 + x14*x15*x3*x7 + x15*x3*x4
 * 
 * ------------------------------------------------------------------------- */

#define A5_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>21,\
                                        XOR3((x)>>17,(x)>>16,(x)>>15)),\
                                   XOR3(XOR3((x)>>5,(x)>>4,(x)),\
                                        AND2((x)>>3,(x)>>6),\
                                        MUX3((x)>>4,(x)>>18,(x)>>2)),\
                                   MUX3(MUX3((x)>>4,(x)>>12,(x)>>13),\
                                        MUX3((x)>>14,(x)>>11,(x)>>7),\
                                        MAJ3((x)>>3,(x)>>10,(x)>>15)))\
                     )) << 25))

/* ------------------------------------------------------------------------- *
 * NLFSR A6 (length 27, period = 134217727)
 *
 * A6(x) = x0 + x3 + x4 + x15 + x25 + x1*x12 + x10*x13 + x10*x17 + x13*x14 
 *            + x1*x3 + x1*x8 + x17*x6 + x10*x11*x18*x5 + x10*x11*x13*x18*x5 
 *            + x11*x13*x14*x18*x5 + x11*x16*x17*x18*x5 
 *            + x11*x16*x18*x2*x5 + x11*x17*x18*x2*x5
 *
 * ------------------------------------------------------------------------- */

#define A6_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>25,\
                                        XOR3((x)>>15,(x)>>4,(x))),\
                                   XOR3(AND2((x)>>1,(x)>>12),\
                                        MUX3((x)>>10,(x)>>6,(x)>>17),\
                                        MUX3((x)>>3,(x)>>8,(x)>>1)),\
                                   MUX3(MUX3((x)>>10,(x)>>14,(x)>>13),\
                                        MAJ3((x)>>17,(x)>>2,(x)>>16),\
                                        AND3((x)>>18,(x)>>11,(x)>>5)))\
                     )) << 26))


/* ------------------------------------------------------------------------- *
 * NLFSR A7 (length 28, period = 268435455)
 *
 * A7(x) = x0 + x1 + x5 + x20 + x25 + x10*x15 + x10*x18 + x14*x16 + x1*x2 
 *            + x12*x4 + x16*x20 + x17*x2 + x18*x19*x7*x9 
 *            + x1*x13*x19*x2*x7*x9 + x10*x15*x19*x7*x9 + x10*x18*x19*x7*x9
 *
 * ------------------------------------------------------------------------- */

#define A7_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>25,\
                                        XOR3((x)>>18,(x)>>5,(x))),\
                                   XOR3(AND2((x)>>4,(x)>>12),\
                                        MUX3((x)>>1,(x)>>17,(x)>>2),\
                                        MUX3((x)>>20,(x)>>14,(x)>>16)),\
                                   MUX3(MUX3((x)>>18,(x)>>15,(x)>>10),\
                                        AND3((x)>>1,(x)>>2,(x)>>13),\
                                        AND3((x)>>7,(x)>>9,(x)>>19)))\
                     )) << 27))

/* ------------------------------------------------------------------------- *
 * NLFSR A8 (length 29, period = 536870911)
 *
 * A8(x) = x0 + x2 + x10 + x11 + x17 + x18 + x21 + x24 + x13*x14*x16 + x13*x19 
 *            + x13*x14*x19 + x13*x15*x19 + x1*x4 + x10*x21 + x21*x8 
 *            + x13*x14*x15*x16 + x13*x14*x15*x19 + x14*x15*x16*x6 
 *            + x14*x15*x19*x6 + x15*x19*x6 + x18*x8*x9 + x14*x16*x18*x8*x9 
 *            + x18*x19*x8*x9 + x14*x18*x19*x8*x9 
 *
 * ------------------------------------------------------------------------- */

#define A8_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>24,\
                                        XOR3((x)>>21,(x)>>18,(x)>>17)),\
                                   XOR3(AND2((x)>>1,(x)>>4),\
                                        XOR3((x)>>11,(x)>>2,(x)),\
                                        MUX3((x)>>10,(x)>>8,(x)>>21)),\
                                   MUX3(AND3((x)>>8,(x)>>18,(x)>>9),\
                                        MUX3((x)>>13,(x)>>6,(x)>>15),\
                                        MUX3((x)>>19,(x)>>16,(x)>>14)))\
                     )) << 28))

/* ------------------------------------------------------------------------- *
 * NLFSR A9 (length 30, period = 1073741823 )  
 *
 * A9(x) = x0 + x1 + x7 + x10 + x12 + x18 + x28 + x10*x12 + x10*x19 + x10*x22 
 *            + x14*x22 + x18*x4 + x2*x8 + x4*x7 + x1*x21*x3*x5 + x1*x21*x3*x8 
 *            + x1*x21*x5*x8 + x1*x3*x5*x9 + x1*x3*x8*x9 + x1*x5*x8*x9 
 *            + x18*x3*x4*x5 + x3*x4*x5*x7 + x3*x5*x7 + x18*x3*x4*x8 
 *            + x21*x3*x5*x9 + x21*x3*x8*x9 + x3*x7*x8 + x5*x7*x8 
 *            + x18*x4*x5*x8 + x21*x5*x8*x9 + x3*x4*x7*x8 + x4*x5*x7*x8
 *
 * ------------------------------------------------------------------------- */

#define A9_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>28,\
                                        XOR3((x)>>18,(x)>>1,(x))),\
                                   XOR3(AND2((x)>>2,(x)>>8),\
                                        MUX3((x)>>12,(x)>>19,(x)>>10),\
                                        MUX3((x)>>10,(x)>>14,(x)>>22)),\
                                   MUX3(MUX3((x)>>7,(x)>>18,(x)>>4),\
                                        MAJ3((x)>>21,(x)>>9,(x)>>1),\
                                        MAJ3((x)>>8,(x)>>5,(x)>>3)))\
                     )) << 29))

/* ------------------------------------------------------------------------- *
 * NLFSR A10 (length 31, period = 2147483647)
 *
 * A10(x) = x0 + x2 + x5 + x6 + x15 + x17 + x18 + x20 + x25 + x14*x19 + x12*x21 
 *             + x17*x21 + x20*x22 + x12*x19*x22 + x12*x22*x4 + x19*x22*x4 
 *             + x20*x21*x22 + x20*x21*x7 + x18*x8 + x20*x8 + x12*x19*x21*x22 
 *             + x12*x19*x21*x7 + x12*x21*x22*x4 + x12*x21*x4*x7 
 *             + x19*x21*x22*x4 + x19*x21*x4*x7 + x18*x21*x22*x8 + x18*x21*x7*x8 
 *             + x20*x21*x22*x8 + x20*x21*x7*x8 + x18*x22*x8 + x20*x22*x8
 *
 * ------------------------------------------------------------------------- */

#define A10_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>25,\
                                        XOR3((x)>>18,(x)>>15,(x)>>6)),\
                                   XOR3(XOR3((x)>>5,(x)>>2,(x)),\
                                        AND2((x)>>19,(x)>>14),\
                                        MUX3((x)>>17,(x)>>12,(x)>>21)),\
                                   MUX3(MUX3((x)>>20,(x)>>18,(x)>>8),\
                                        MAJ3((x)>>4,(x)>>12,(x)>>19),\
                                        MUX3((x)>>22,(x)>>7,(x)>>21)))\
                     )) << 30))

/* ------------------------------------------------------------------------- *
 * NLFSR A11 (length 32, period = 4294967295)
 *
 * A11(x) = x0 + x3 + x17 + x22 + x28 + x12*x8 + x13*x15 + x13*x2 + x19*x5 
 *             + x12*x13*x2 + x19*x7 + x13*x8 + x11*x12*x24*x4 + x12*x13*x8 
 *             + x12*x13*x2*x7 + x13*x14*x2*x7 + x11*x12*x24*x4*x7 
 *             + x11*x14*x24*x4*x7 + x12*x13*x7*x8 + x13*x14*x7*x8 
 *             + x12*x7*x8 + x14*x7*x8
 *
 * ------------------------------------------------------------------------- */

#define A11_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>28,\
                                        XOR3((x)>>22,(x)>>17,(x)>>8)),\
                                   XOR3(AND2((x)>>13,(x)>>15),\
                                        XOR3((x)>>5,(x)>>3,(x)),\
                                        MUX3((x)>>5,(x)>>7,(x)>>19)),\
                                   MUX3(MUX3((x)>>8,(x)>>2,(x)>>13),\
                                        AND3((x)>>4,(x)>>11,(x)>>24),\
                                        MUX3((x)>>12,(x)>>14,(x)>>7)))\
                     )) << 31))

/* ------------------------------------------------------------------------- *
 * NLFSR A12 (length 33, period = 8589934591)
 *
 * A12(x) = x0 + x2 + x7 + x9 + x10 + x15 + x23 + x25 + x30 + x12*x16 + x13*x15 
 *             + x13*x25 + x15*x17*x24 + x15*x8 + x1*x14*x8 + x1*x14*x17*x24 
 *             + x1*x17*x18*x24 + x1*x14*x17*x8 + x1*x17*x18*x8 + x1*x18*x8 
 *             + x12*x16*x8 + x15*x16*x8 + x14*x18*x8 + x15*x17*x8 
 *             + x12*x16*x17*x24 + x14*x17*x18*x24 + x15*x16*x17*x24 
 *             + x12*x16*x17*x8 + x14*x17*x18*x8 + x15*x16*x17*x8
 *
 * ------------------------------------------------------------------------- */

#define A12_cycle(x,feedin) \
   (x = (x >> ONE) | ((ONE & (XOR3(XOR3(feedin,\
                                        (x)>>30,\
                                        XOR3((x)>>23,(x)>>10,(x)>>9)),\
                                   XOR3(XOR3((x)>>7,(x)>>2,(x)),\
                                        AND2((x)>>15,(x)>>16),\
                                        MUX3((x)>>25,(x)>>15,(x)>>13)),\
                                   MUX3(MUX3((x)>>15,(x)>>12,(x)>>16),\
                                        MAJ3((x)>>14,(x)>>1,(x)>>18),\
                                        MUX3((x)>>8,(x)>>24,(x)>>17)))\
                     )) << 32))


/* ------------------------------------------------------------------------- *
 * Boolean combining function 
 *
 * F(x0, x1, ..., x12) = 
 *   x0 + x1 + x2 + x3 + x4 + x5 + x7 + x9 + x11 + x12+ x0*x5 + x2*x10 + x2*x11 
 *   + x4*x8 + x4*x12 + x5*x6 + x6*x8 + x6*x10 + x6*x11 + x6*x12 + x7*x8 
 *   + x7*x12 + x8*x9 + x8*x10 + x9*x10 + x9*x11 + x9*x12 + x10*x12 + x0*x5*x8 
 *   + x0*x5*x10 + x0*x5*x11 + x0*x5*x12 + x1*x2*x8 + x1*x2*x12 + x1*x4*x10 
 *   + x1*x4*x11 + x1*x8*x9 + x1*x9*x10 + x1*x9*x11 + x1*x9*x12 + x2*x3*x8 
 *   + x2*x3*x12 + x2*x4*x8 + x2*x4*x10 + x2*x4*x11 + x2*x4*x12 + x2*x7*x8 
 *   + x2*x7*x12 + x2*x8*x10 + x2*x8*x11 + x2*x9*x10 + x2*x9*x11 + x2*x10*x12
 *   + x2*x11*x12 + x3*x4*x8 + x3*x4*x12 + x3*x8*x9 + x3*x9*x12 + x4*x7*x8 
 *   + x4*x7*x12 + x4*x8*x9 + x4*x9*x12 + x5*x6*x8 + x5*x6*x10 + x5*x6*x11 
 *   + x5*x6*x12 + x6*x8*x10 + x6*x8*x11 + x6*x10*x12 + x6*x11*x12 + x7*x8*x9
 *   + x7*x9*x12 + x8*x9*x10 + x8*x9*x11 + x9*x10*x12 + x9*x11*x12 
 *   + x0*x5*x8*x10 + x0*x5*x8*x11 + x0*x5*x10*x12 + x0*x5*x11*x12 
 *   + x1*x2*x3*x8 + x1*x2*x3*x12 + x1*x2*x7*x8 + x1*x2*x7*x12 
 *   + x1*x3*x5*x8 + x1*x3*x5*x12 + x1*x3*x8*x9 + x1*x3*x9*x12 
 *   + x1*x4*x8*x10 + x1*x4*x8*x11 + x1*x4*x10*x12 + x1*x4*x11*x12 
 *   + x1*x5*x7*x8 + x1*x5*x7*x12 + x1*x7*x8*x9 + x1*x7*x9*x12 
 *   + x1*x8*x9*x10 + x1*x8*x9*x11 + x1*x9*x10*x12 + x1*x9*x11*x12 
 *   + x2*x3*x4*x8 + x2*x3*x4*x12 + x2*x3*x5*x8 + x2*x3*x5*x12 
 *   + x2*x4*x7*x8 + x2*x4*x7*x12 + x2*x4*x8*x10 + x2*x4*x8*x11 
 *   + x2*x4*x10*x12 + x2*x4*x11*x12 + x2*x5*x7*x8 + x2*x5*x7*x12 
 *   + x2*x8*x9*x10 + x2*x8*x9*x11 + x2*x9*x10*x12 + x2*x9*x11*x12 
 *   + x3*x4*x8*x9 + x3*x4*x9*x12  + x4*x7*x8*x9 + x4*x7*x9*x12 
 *   + x5*x6*x8*x10 + x5*x6*x8*x11 + x5*x6*x10*x12 + x5*x6*x11*x12
 *
 * ------------------------------------------------------------------------- */

u32 F (u32 x0, u32 x1, u32 x2, u32 x3, u32 x4, u32 x5, 
       u32 x6, u32 x7, u32 x8, u32 x9, u32 x10, u32 x11, u32 x12)
{
  u32 A = x1^x2,
      C = x2^x9,
      H = x3^x7,
      T = x4^x9,
      E = ((x0^x6)&x5)^x6,
      R = ((x1^x4)&C)^T,
      b = (R^(A&x5)^x2)&H,
      a = ((x10^x11)&(C^(A&T)^E))^E,
      h = (x8^x12)&(b^a^R^x7^x10),
      n = H^A^T^a^h^x0^x5^x6^x11^x12;
  return (n);
}

/* ------------------------------------------------------------------------- *
 * Macro to attach the Boolean combining function to the driving NLFSRs
 * ------------------------------------------------------------------------- */
#define KEYSTREAM_BITS F(ctx->A0 >> (A0_size -16),\
                         ctx->A1 >> (A1_size -16),\
                         ctx->A2 >> (A2_size -16),\
                         ctx->A3 >> (A3_size -16),\
                         ctx->A4 >> (A4_size -16),\
                         ctx->A5 >> (A5_size -16),\
                         ctx->A6 >> (A6_size -16),\
                         ctx->A7 >> (A7_size -16),\
                         ctx->A8 >> (A8_size -16),\
                         ctx->A9 >> (A9_size -16),\
                         ctx->A10>> (A10_size-16),\
                         ctx->A11>> (A11_size-16),\
                         (u32)(ctx->A12>> (A12_size-16)))

/* ------------------------------------------------------------------------- *
 * Macro to step forward all driving NLFSRs by one clock cycle
 * ------------------------------------------------------------------------- */
#define NLFSR_CYCLE(feedin) \
 { if (ctx->keysize == 128) A0_cycle (ctx->A0, feedin); \
   A1_cycle (ctx->A1, feedin); \
   A2_cycle (ctx->A2, feedin); \
   A3_cycle (ctx->A3, feedin); \
   A4_cycle (ctx->A4, feedin); \
   A5_cycle (ctx->A5, feedin); \
   A6_cycle (ctx->A6, feedin); \
   A7_cycle (ctx->A7, feedin); \
   A8_cycle (ctx->A8, feedin); \
   A9_cycle (ctx->A9, feedin); \
   A10_cycle(ctx->A10,feedin); \
   A11_cycle(ctx->A11,feedin); \
   if (ctx->keysize == 128) A12_cycle(ctx->A12,feedin); }

/* ------------------------------------------------------------------------- *
 *
 * Setup of Achterbahn stream cipher.
 *
 * Called by ECRYPT_keysetup() and ECRYPT_ivsetup().
 *
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_setup (
  ECRYPT_ctx *ctx
  )
{
  u32 i;

  /* ----------------------------------------------------------------------- *
   * step 1:
   * load all driving NLFSRs with the first key bits in parallel.
   * ----------------------------------------------------------------------- */
  if (ctx->keysize == 128) {
    ctx->A0  = (u32)(ctx->key33 & A0_mask);
    ctx->A12 = (u64)(ctx->key33 & A12_mask);
  } else { 
    ctx->A0  = ZERO;
    ctx->A12 = ZERO; 
  }
  ctx->A1  = (u32)(ctx->key33 & A1_mask);
  ctx->A2  = (u32)(ctx->key33 & A2_mask);
  ctx->A3  = (u32)(ctx->key33 & A3_mask);
  ctx->A4  = (u32)(ctx->key33 & A4_mask);
  ctx->A5  = (u32)(ctx->key33 & A5_mask);
  ctx->A6  = (u32)(ctx->key33 & A6_mask);
  ctx->A7  = (u32)(ctx->key33 & A7_mask);
  ctx->A8  = (u32)(ctx->key33 & A8_mask);
  ctx->A9  = (u32)(ctx->key33 & A9_mask);
  ctx->A10 = (u32)(ctx->key33 & A10_mask);
  ctx->A11 = (u32)(ctx->key33 & A11_mask);

  /* ----------------------------------------------------------------------- *
   * step 2:
   * for each driving NLFSR, feed-in the key bits, not already loaded into the
   * register in step 1, into the NLFSR. 
   * ----------------------------------------------------------------------- */
  if (ctx->keysize == 128) {
    for (i =  A0_size; i < ctx->keysize; ++i) A0_cycle (ctx->A0, ctx->ky[i]);
    for (i = A12_size; i < ctx->keysize; ++i) A12_cycle(ctx->A12,ctx->ky[i]);
  }
  for (i =  A1_size; i < ctx->keysize; ++i) A1_cycle (ctx->A1, ctx->ky[i]);
  for (i =  A2_size; i < ctx->keysize; ++i) A2_cycle (ctx->A2, ctx->ky[i]);
  for (i =  A3_size; i < ctx->keysize; ++i) A3_cycle (ctx->A3, ctx->ky[i]);
  for (i =  A4_size; i < ctx->keysize; ++i) A4_cycle (ctx->A4, ctx->ky[i]);
  for (i =  A5_size; i < ctx->keysize; ++i) A5_cycle (ctx->A5, ctx->ky[i]);
  for (i =  A6_size; i < ctx->keysize; ++i) A6_cycle (ctx->A6, ctx->ky[i]);
  for (i =  A7_size; i < ctx->keysize; ++i) A7_cycle (ctx->A7, ctx->ky[i]);
  for (i =  A8_size; i < ctx->keysize; ++i) A8_cycle (ctx->A8, ctx->ky[i]);
  for (i =  A9_size; i < ctx->keysize; ++i) A9_cycle (ctx->A9, ctx->ky[i]);
  for (i = A10_size; i < ctx->keysize; ++i) A10_cycle(ctx->A10,ctx->ky[i]);
  for (i = A11_size; i < ctx->keysize; ++i) A11_cycle(ctx->A11,ctx->ky[i]);

  /* ----------------------------------------------------------------------- *
   * step 3:
   * for each driving NLFSR, feed-in all IV bits, into the NLFSR.
   * ----------------------------------------------------------------------- */
  for (i = 0; i < ctx->ivsize; ++i)
    NLFSR_CYCLE(ctx->iv[i]);

  /* ----------------------------------------------------------------------- *
   * step 4:
   * Feed-in the keystream output into each NLFSR
   * ----------------------------------------------------------------------- */
  for (i = 0; i < 32; ++i) {
    u32 z = KEYSTREAM_BITS;
    NLFSR_CYCLE(z);
  }
  
  /* ----------------------------------------------------------------------- *
   * step 5:
   * set the least significant bit (LSB) of each NLFSR to 1.
   * ----------------------------------------------------------------------- */
  if (ctx->keysize == 128) {
    ctx->A0  |= ONE;  
    ctx->A12 |= ONE;
  }
  ctx->A1  |= ONE;
  ctx->A2  |= ONE;
  ctx->A3  |= ONE;
  ctx->A4  |= ONE;
  ctx->A5  |= ONE;
  ctx->A6  |= ONE;
  ctx->A7  |= ONE;
  ctx->A8  |= ONE;
  ctx->A9  |= ONE;
  ctx->A10 |= ONE;
  ctx->A11 |= ONE;

  /* ----------------------------------------------------------------------- *
   * step 6:
   * warming up.
   * ----------------------------------------------------------------------- */
  for (i = 0; i < 64; ++i) 
    NLFSR_CYCLE(ZERO);
}


/* ------------------------------------------------------------------------- *
 *
 * Key and message independent initialization.
 *
 * This function will be called once when the program starts.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_init()
{
  /* nothing to do */
}


/* ------------------------------------------------------------------------- *
 *
 * Key setup.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_keysetup(
  ECRYPT_ctx *ctx,
  const u8   *key,
  u32        keysize,              /* Key size in bits. */
  u32        ivsize                /* IV size in bits. */
  )
{
  u32 i;

  /* assert valid arguments: 
     This reference implementation supports key sizes 40, 48, 56, ..., 128.
     IV length  can be 0, 8, 16, ..., key length */
  assert( ctx && key &&
          (keysize >= 40) && (keysize <= 128) && ((keysize%8) == 0) &&
          ((ivsize%8) == 0) );

  /* save size of key in context */
  ctx->keysize = keysize;

  /* save key bits 0 to 33 in one context variable - for faster parallel loading */
  ctx->key33 = ((u64)key[4]<<32)
             | ((u64)key[3]<<24)
             | ((u64)key[2]<<16)
             | ((u64)key[1]<<8)
             | ((u64)key[0]);

  /* save key bits in context, 1 bit/word for faster lookup */
  for (i = 0; i < keysize; ++i)
    ctx->ky[i] = ONE & ((u32)key[i/8]>>(i%8));

  /* save size of IV in context */
  ctx->ivsize = ivsize;

  /* If the size of the IV is zero the key setup will be completed in this function,
     otherwise, a flag is set that the IV setup is not yet completed */
  if (ivsize)
    ctx->iv_setup_done = false;
  else {
    ACHTERBAHN_setup(ctx);
    ctx->iv_setup_done = true;
  }
}


/* ------------------------------------------------------------------------- *
 *
 * IV setup.
 *
 * After having called ECRYPT_keysetup(), the user is allowed to
 * call ECRYPT_ivsetup() different times in order to encrypt/decrypt
 * different messages with the same key but different IV's.
 *
 * If the size of the IV is zero the keystream can be generated right after
 * the key setup. i.e. it is allowed to call
 *
 *   ECRYPT_keysetup(ctx,key,128,0);
 *   ECRYPT_encrypt_bytes(ctx,plaintext,ciphertext,msglen);
 *
 * If there is an IV then ECRYPT_ivsetup() must be called before
 * keystream generation, e.g.
 *
 *   ECRYPT_keysetup(ctx,key,80,48);
 *   ECRYPT_ivsetup(ctx,iv);
 *   ECRYPT_encrypt_bytes(ctx,plaintext,ciphertext,msglen);
 *
 *
 * Achterbahn allows for the IV lengths any value of 0, 8, 16, ... upto 
 * the key length. (An IV length equal to 0, of course, means that no
 * synchronization is performed.)
 * It is necessary to define the mapping of the bits of the IV to the
 * bit positions in the byte array of argument iv of the function.
 * The following order for storing the bits of the initialization vector
 * IV[0...n], n = 0...keysize, in the argument array of bytes iv[0...k],
 * k = 0 (mod 8), is defined:
 *
 *     IV[keysize],.. , IV[16] ,IV[15], ... , IV[8], IV[7], ..., IV[0]
 *     |                       |                   |                 |
 *     | byte iv[k], ...       |    byte iv[1]     |    byte iv[0]   |
 *
 * The least significant bit of byte iv[0] is bit IV[0], the most significant
 * bit of byte iv[0] is bit IV[7], and so on.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_ivsetup(
  ECRYPT_ctx *ctx,
  const u8   *iv
  )
{
  u32 i;

  /* always assert valid arguments */
  assert( ctx && iv );

  /* save IV in context - 1 bit/word for faster lookup */
  for (i = 0; i < ctx->ivsize; ++i)
    ctx->iv[i] = ONE & ((u32)iv[i/8]>>(i%8));

  /* call setup function and set the initialization flag */
  ACHTERBAHN_setup(ctx);
  ctx->iv_setup_done = true;
}


/* ------------------------------------------------------------------------- *
 *
 * Encryption of arbitrary length messages.
 *
 * The ECRYPT_encrypt_bytes() function encrypts byte strings
 * of arbitrary length.
 *
 * For a description of the bit order of the generated keystream read
 * the comments to function ECRYPT_keystream_bytes() below.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_encrypt_bytes(
  ECRYPT_ctx *ctx,
  const u8   *plaintext,
  u8         *ciphertext,
  u32        msglen                 /* Message length in bytes. */
  )
{
  u32 i,k;

  /* always assert valid arguments
     and ensure that setup is done before encryption is started */
  assert( ctx && ctx->iv_setup_done && plaintext && ciphertext );

  /* return on zero message length */
  if ( !msglen ) return;

  /* loop over byte length of message */
  for (i = 0; i < msglen; ++i) {

    /* evaluate Boolean combining function in parallel for one byte */
    ciphertext[i] = plaintext[i] ^ (u8)KEYSTREAM_BITS;    

    /* cycle eight times to prepare a new byte */
    for (k = 0; k < 8; ++k) NLFSR_CYCLE(ZERO); 

  } /* for (i) */
}


/* ------------------------------------------------------------------------- *
 *
 * Decryption of arbitrary length messages.
 *
 * The ECRYPT_decrypt_bytes() function decrypts byte strings
 * of arbitrary length.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_decrypt_bytes(
  ECRYPT_ctx *ctx,
  const u8   *ciphertext,
  u8         *plaintext,
  u32        msglen                 /* Message length in bytes. */
)
{
  ECRYPT_encrypt_bytes(ctx,ciphertext,plaintext,msglen);
}


/* ------------------------------------------------------------------------- *
 *
 * Generates keystream without having to provide it with a zero plaintext.
 *
 * The following order for storing the bits of the keystream
 * z[0], z[1], ..., z[n] in the output byte array keystream[0...k],
 * is defined:
 *
 *     z[n], ...          , z[16] , z[15],  ...,  z[8], z[7], ...,   z[0]
 *     |                          |                   |                 |
 *     | keystream[k], ...        |   keystream[1]    |  keystream[0]   |
 *
 * The least significant bit of byte keystream[0] is bit z[0],
 * the most significant bit of byte keystream[0] is bit z[7], and so on.
 *
 * ------------------------------------------------------------------------- */

void ECRYPT_keystream_bytes(
  ECRYPT_ctx  *ctx,
  u8          *keystream,
  u32         length           /* Length of keystream in bytes. */
  )
{
  u32 i,k;

  /* assert valid arguments */
  assert( ctx && keystream );

  /* return on zero message length */
  if ( !length ) return;

  /* loop over byte length of key stream */
  for (i = 0; i < length; ++i) {

    /* evaluate Boolean combining function in parallel for one byte */
    keystream[i] = (u8)KEYSTREAM_BITS;

    /* cycle eight times to prepare a new byte */
    for (k = 0; k < 8; ++k) NLFSR_CYCLE(ZERO);

  } /* for (i) */
}

/* ------------------------------------------------------------------------- *
 * end of implementation
 * ------------------------------------------------------------------------- */



#ifdef COMPILE_TEST_CODE

/* ------------------------------------------------------------------------- *
 *
 *   A C H T E R B A H N  - 128 / 80 Test Suite
 *
 * ------------------------------------------------------------------------- */

typedef u8 key128_t[16];     /* type for max. 128 bit key */
typedef u8  iv128_t[16];     /* type for max. 128 bit IV */

/* ------------------------------------------------------------------------- *
 * Printing utility functions.
 * ------------------------------------------------------------------------- */

static void print_bitword (const char msg[], u32 x)
{
  int k;
  printf("%s",msg);
  for (k = 0; k < 32; ++k)
    printf("%d",(x>>(31-k))&1);
  printf("\n");
  fflush(0);
}

static void print_hexword (const char msg[], u32 x)
{
  int k;
  printf("%s",msg);
  for (k = 24; k >= 0; k -= 8)
    printf("%02X",(u8)(x>>k));
  printf("\n");
  fflush(0);
}

static void print_string (const char msg[], u32 len, const u8 *s)
{
  u32 k;
  printf("%s",msg);
  for (k = 0; k < len; ++k)
    printf("%c",s[k]);
  printf("\n");
  fflush(0);
}

static void print_hexstr (const char msg[], u32 len, const u8 *s)
{
  u32 k;
  printf("%s",msg);
  for (k = 0; k < len; ++k) {
    printf("%02X",(u32)s[k]);
    if (k % 32 == 31) printf("\n");
  }
  printf("\n");
  fflush(0);
}


/* ------------------------------------------------------------------------- *
 *
 * Print an introduction screen.
 *
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_splashscreen()
{
  printf("\n* ================================================================ *\n");
  printf("\n                    A C H T E R B A H N - 128 / 80\n");
  printf("\n     A Hardware Oriented Synchroneous Binary Stream Cipher\n");
  printf("\n                 Reference implementation V1.1 \n");
  printf("\n            %s\n",ECRYPT_AUTHORS);
  printf("\n                              Email:\n\n");
  printf("                        gammel@matpack.de\n");
  printf("                      rainer.goettfert@gmx.de\n");
  printf("                     oliver.kniffler@arcor.de\n");
  printf("\n                     Reference Implementation\n");
  printf("\n* ================================================================ *\n");
}

/* ------------------------------------------------------------------------- *
 * Period tests of all NLFSR components
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_NLFSRs()
{
  u32 seed; 
  u32 i, state;
  u64 l, lstate;

  printf("* ================================================================ *\n");
  printf("* Period tests of all driving NLFSRs (be patient)\n");
  printf("* ================================================================ *\n");
  fflush(0);

  seed = 0xc1a0be1a;

  printf("  A0(n=%u)...",A0_size); fflush(0);
  state = seed & A0_mask;
  for (i = 0; i < A0_mask; ++i) {
    A0_cycle(state,ZERO);
    if (state == (seed&A0_mask)) break;
  }
  assert(i == A0_mask-1);
  printf("passed **\n");

  printf("  A1(n=%u)...",A1_size); fflush(0);
  state = seed & A1_mask;
  for (i = 0; i < A1_mask; ++i) {
    A1_cycle(state,ZERO);
    if (state == (seed&A1_mask)) break;
  }
  assert(i == A1_mask-1);
  printf("passed **\n");

  printf("  A2(n=%u)...",A2_size); fflush(0);
  state = seed & A2_mask;
  for (i = 0; i < A2_mask; ++i) {
    A2_cycle(state,ZERO);
    if (state == (seed&A2_mask)) break;
  }
  assert(i == A2_mask-1);
  printf("passed **\n");

  printf("  A3(n=%u)...",A3_size); fflush(0);
  state = seed & A3_mask;
  for (i = 0; i < A3_mask; ++i) {
    A3_cycle(state,ZERO);
    if (state == (seed&A3_mask)) break;
  }
  assert(i == A3_mask-1);
  printf("passed **\n");

  printf("  A4(n=%u)...",A4_size); fflush(0);
  state = seed & A4_mask;
  for (i = 0; i < A4_mask; ++i) {
    A4_cycle(state,ZERO);
    if (state == (seed&A4_mask)) break;
  }
  assert(i == A4_mask-1);
  printf("passed **\n");

  printf("  A5(n=%u)...",A5_size); fflush(0);
  state = seed & A5_mask;
  for (i = 0; i < A5_mask; ++i) {
    A5_cycle(state,ZERO);
    if (state == (seed&A5_mask)) break;
  }
  assert(i == A5_mask-1);
  printf("passed **\n");

  printf("  A6(n=%u)...",A6_size); fflush(0);
  state = seed & A6_mask;
  for (i = 0; i < A6_mask; ++i) {
    A6_cycle(state,ZERO);
    if (state == (seed&A6_mask)) break;
  }
  assert(i == A6_mask-1);
  printf("passed **\n");

  printf("  A7(n=%u)...",A7_size); fflush(0);
  state = seed & A7_mask;
  for (i = 0; i < A7_mask; ++i) {
    A7_cycle(state,ZERO);
    if (state == (seed&A7_mask)) break;
  }
  assert(i == A7_mask-1);
  printf("passed **\n");

  printf("  A8(n=%u)...",A8_size); fflush(0);
  state = seed & A8_mask;
  for (i = 0; i < A8_mask; ++i) {
    A8_cycle(state,ZERO);
    if (state == (seed&A8_mask)) break;
  }
  assert(i == A8_mask-1);
  printf("passed **\n");

  printf("  A9(n=%u)...",A9_size); fflush(0);
  state = seed & A9_mask;
  for (i = 0; i < A9_mask; ++i) {
    A9_cycle(state,ZERO);
    if (state == (seed&A9_mask)) break;
  }
  assert(i == A9_mask-1);
  printf("passed **\n");

  printf("  A10(n=%u)...",A10_size); fflush(0);
  state = seed & A10_mask;
  for (i = 0; i < A10_mask; ++i) {
    A10_cycle(state,ZERO);
    if (state == (seed&A10_mask)) break;
  }
  assert(i == A10_mask-1);
  printf("passed **\n");

  printf("  A11(n=%u)...",A11_size); fflush(0);
  state = seed & A11_mask;
  for (i = 0; i < A11_mask; ++i) {
    A11_cycle(state,ZERO);
    if (state == (seed&A11_mask)) break;
  }
  assert(i == A11_mask-1);
  printf("passed **\n");

  printf("  A12(n=%u)...",A12_size); fflush(0);
  lstate = (u64)seed & A12_mask;
  for (l = 0; l < A12_mask; ++l) {
    A12_cycle(lstate,ZERO);
    if (lstate == ((u64)seed&A12_mask)) break;
  }
  assert(l == (u64)A12_mask-1);
  printf("passed **\n");
}
  
/* ------------------------------------------------------------------------- *
 * Verify the Boolean function implementation
 * ------------------------------------------------------------------------- */
static void ACHTERBAHN_verify_BooleanFunction()
{
  u32 N11 = (ONE << 11),
      N13 = (ONE << 13);
  u32 x, ref, t;

  printf("* ================================================================ *\n");
  printf("* Verify Boolean Functions\n");
  printf("* ================================================================ *\n");
  fflush(0);
 
  for (x = 0; x < N13; ++x) {

    /* ANF of 13-variable Boolean function */
    ref = (x>>0) ^ (x>>1) ^ (x>>2) ^ (x>>3) ^ (x>>4) ^ (x>>5) ^ (x>>7) 
        ^ (x>>9) ^ (x>>11) ^ (x>>12) ^ ((x>>0)&(x>>5)) ^ ((x>>2)&(x>>10))
        ^ ((x>>2)&(x>>11)) ^ ((x>>4)&(x>>8)) ^ ((x>>4)&(x>>12)) ^ ((x>>5)&(x>>6)) 
        ^ ((x>>6)&(x>>10)) ^ ((x>>6)&(x>>8)) ^ ((x>>6)&(x>>11)) ^ ((x>>6)&(x>>12)) 
        ^ ((x>>7)&(x>>8)) ^ ((x>>7)&(x>>12)) ^ ((x>>10)&(x>>9)) ^ ((x>>10)&(x>>8)) 
        ^ ((x>>10)&(x>>12)) ^ ((x>>9)&(x>>8)) ^ ((x>>9)&(x>>11)) ^ ((x>>9)&(x>>12)) 
        ^ ((x>>0)&(x>>5)&(x>>10)) ^ ((x>>0)&(x>>5)&(x>>8)) ^ ((x>>0)&(x>>5)&(x>>11)) 
        ^ ((x>>0)&(x>>5)&(x>>12)) ^ ((x>>1)&(x>>2)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>10)) ^ ((x>>1)&(x>>4)&(x>>11)) ^ ((x>>1)&(x>>10)&(x>>9)) 
        ^ ((x>>1)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>9)&(x>>11)) ^ ((x>>1)&(x>>9)&(x>>12)) 
        ^ ((x>>2)&(x>>3)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>12)) ^ ((x>>2)&(x>>4)&(x>>10)) 
        ^ ((x>>2)&(x>>4)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>11)) ^ ((x>>2)&(x>>4)&(x>>12)) 
        ^ ((x>>2)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>7)&(x>>12)) ^ ((x>>2)&(x>>10)&(x>>9)) 
        ^ ((x>>2)&(x>>10)&(x>>8)) ^ ((x>>2)&(x>>10)&(x>>12)) ^ ((x>>2)&(x>>9)&(x>>11)) 
        ^ ((x>>2)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>11)&(x>>12)) ^ ((x>>3)&(x>>4)&(x>>8)) 
        ^ ((x>>3)&(x>>4)&(x>>12)) ^ ((x>>3)&(x>>9)&(x>>8)) ^ ((x>>3)&(x>>9)&(x>>12)) 
        ^ ((x>>4)&(x>>7)&(x>>8)) ^ ((x>>4)&(x>>7)&(x>>12)) ^ ((x>>4)&(x>>9)&(x>>8)) 
        ^ ((x>>4)&(x>>9)&(x>>12)) ^ ((x>>5)&(x>>6)&(x>>10)) ^ ((x>>5)&(x>>6)&(x>>8) )
        ^ ((x>>5)&(x>>6)&(x>>11)) ^ ((x>>5)&(x>>6)&(x>>12)) ^ ((x>>6)&(x>>10)&(x>>8))
        ^ ((x>>6)&(x>>10)&(x>>12)) ^ ((x>>6)&(x>>8)&(x>>11)) ^ ((x>>6)&(x>>11)&(x>>12)) 
        ^ ((x>>7)&(x>>9)&(x>>8)) ^ ((x>>7)&(x>>9)&(x>>12)) ^ ((x>>10)&(x>>9)&(x>>8))
        ^ ((x>>10)&(x>>9)&(x>>12)) ^ ((x>>9)&(x>>8)&(x>>11)) ^ ((x>>9)&(x>>11)&(x>>12))
        ^ ((x>>0)&(x>>5)&(x>>10)&(x>>8)) ^ ((x>>0)&(x>>5)&(x>>10)&(x>>12)) 
        ^ ((x>>0)&(x>>5)&(x>>8)&(x>>11)) ^ ((x>>0)&(x>>5)&(x>>11)&(x>>12)) 
        ^ ((x>>1)&(x>>2)&(x>>3)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>3)&(x>>12)) 
        ^ ((x>>1)&(x>>2)&(x>>7)&(x>>8)) ^ ((x>>1)&(x>>2)&(x>>7)&(x>>12)) 
        ^ ((x>>1)&(x>>3)&(x>>5)&(x>>8)) ^ ((x>>1)&(x>>3)&(x>>5)&(x>>12)) 
        ^ ((x>>1)&(x>>3)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>3)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>10)&(x>>8)) ^ ((x>>1)&(x>>4)&(x>>10)&(x>>12)) 
        ^ ((x>>1)&(x>>4)&(x>>8)&(x>>11)) ^ ((x>>1)&(x>>4)&(x>>11)&(x>>12)) 
        ^ ((x>>1)&(x>>5)&(x>>7)&(x>>8)) ^ ((x>>1)&(x>>5)&(x>>7)&(x>>12)) 
        ^ ((x>>1)&(x>>7)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>7)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>10)&(x>>9)&(x>>8)) ^ ((x>>1)&(x>>10)&(x>>9)&(x>>12)) 
        ^ ((x>>1)&(x>>9)&(x>>8)&(x>>11)) ^ ((x>>1)&(x>>9)&(x>>11)&(x>>12)) 
        ^ ((x>>2)&(x>>3)&(x>>4)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>4)&(x>>12))
        ^ ((x>>2)&(x>>3)&(x>>5)&(x>>8)) ^ ((x>>2)&(x>>3)&(x>>5)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>7)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>10)&(x>>8)) ^ ((x>>2)&(x>>4)&(x>>10)&(x>>12)) 
        ^ ((x>>2)&(x>>4)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>4)&(x>>11)&(x>>12)) 
        ^ ((x>>2)&(x>>5)&(x>>7)&(x>>8)) ^ ((x>>2)&(x>>5)&(x>>7)&(x>>12)) 
        ^ ((x>>2)&(x>>10)&(x>>9)&(x>>8)) ^ ((x>>2)&(x>>10)&(x>>9)&(x>>12)) 
        ^ ((x>>2)&(x>>9)&(x>>8)&(x>>11)) ^ ((x>>2)&(x>>9)&(x>>11)&(x>>12)) 
        ^ ((x>>3)&(x>>4)&(x>>9)&(x>>8)) ^ ((x>>3)&(x>>4)&(x>>9)&(x>>12))  
        ^ ((x>>4)&(x>>7)&(x>>9)&(x>>8)) ^ ((x>>4)&(x>>7)&(x>>9)&(x>>12)) 
        ^ ((x>>5)&(x>>6)&(x>>10)&(x>>8)) ^ ((x>>5)&(x>>6)&(x>>10)&(x>>12)) 
        ^ ((x>>5)&(x>>6)&(x>>8)&(x>>11)) ^ ((x>>5)&(x>>6)&(x>>11)&(x>>12));
    
    t = F((x>>0), (x>>1), (x>>2), (x>>3), (x>>4),  (x>>5), 
          (x>>6), (x>>7), (x>>8), (x>>9), (x>>10), (x>>11), (x>>12));
    
    assert(ref == t);
  }

  for (x = 0; x < N11; ++x) {

    /* ANF of 11-variable Boolean function */
    ref =  (x>>0) ^ (x>>1) ^ (x>>2) ^ (x>>3) ^ (x>>4) ^ (x>>6) ^ (x>>8) ^ (x>>10) 
         ^ ((x>>1)&(x>>9)) ^ ((x>>1)&(x>>10)) ^ ((x>>3)&(x>>7)) ^ ((x>>4)&(x>>5)) 
         ^ ((x>>5)&(x>>9)) ^ ((x>>5)&(x>>7)) ^ ((x>>5)&(x>>10)) ^ ((x>>6)&(x>>7)) 
         ^ ((x>>9)&(x>>8)) ^ ((x>>9)&(x>>7)) ^ ((x>>8)&(x>>7)) ^ ((x>>8)&(x>>10)) 
         ^ ((x>>0)&(x>>1)&(x>>7)) ^ ((x>>0)&(x>>3)&(x>>9)) ^ ((x>>0)&(x>>3)&(x>>10)) 
         ^ ((x>>0)&(x>>9)&(x>>8)) ^ ((x>>0)&(x>>8)&(x>>7)) ^ ((x>>0)&(x>>8)&(x>>10)) 
         ^ ((x>>1)&(x>>2)&(x>>7)) ^ ((x>>1)&(x>>3)&(x>>9)) ^ ((x>>1)&(x>>3)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>10)) ^ ((x>>1)&(x>>6)&(x>>7)) ^ ((x>>1)&(x>>9)&(x>>8)) 
         ^ ((x>>1)&(x>>9)&(x>>7)) ^ ((x>>1)&(x>>8)&(x>>10)) ^ ((x>>1)&(x>>7)&(x>>10)) 
         ^ ((x>>2)&(x>>3)&(x>>7)) ^ ((x>>2)&(x>>8)&(x>>7)) ^ ((x>>3)&(x>>6)&(x>>7)) 
         ^ ((x>>3)&(x>>8)&(x>>7)) ^ ((x>>4)&(x>>5)&(x>>9)) ^ ((x>>4)&(x>>5)&(x>>7)) 
         ^ ((x>>4)&(x>>5)&(x>>10)) ^ ((x>>5)&(x>>9)&(x>>7)) ^ ((x>>5)&(x>>7)&(x>>10)) 
         ^ ((x>>6)&(x>>8)&(x>>7)) ^ ((x>>9)&(x>>8)&(x>>7)) ^ ((x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>0)&(x>>1)&(x>>2)&(x>>7)) ^ ((x>>0)&(x>>1)&(x>>6)&(x>>7)) 
         ^ ((x>>0)&(x>>2)&(x>>4)&(x>>7)) ^ ((x>>0)&(x>>2)&(x>>8)&(x>>7)) 
         ^ ((x>>0)&(x>>3)&(x>>9)&(x>>7)) ^ ((x>>0)&(x>>3)&(x>>7)&(x>>10)) 
         ^ ((x>>0)&(x>>4)&(x>>6)&(x>>7)) ^ ((x>>0)&(x>>6)&(x>>8)&(x>>7)) 
         ^ ((x>>0)&(x>>9)&(x>>8)&(x>>7)) ^ ((x>>0)&(x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>1)&(x>>2)&(x>>3)&(x>>7)) ^ ((x>>1)&(x>>2)&(x>>4)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>6)&(x>>7)) ^ ((x>>1)&(x>>3)&(x>>9)&(x>>7)) 
         ^ ((x>>1)&(x>>3)&(x>>7)&(x>>10)) ^ ((x>>1)&(x>>4)&(x>>6)&(x>>7)) 
         ^ ((x>>1)&(x>>9)&(x>>8)&(x>>7)) ^ ((x>>1)&(x>>8)&(x>>7)&(x>>10)) 
         ^ ((x>>2)&(x>>3)&(x>>8)&(x>>7)) ^ ((x>>3)&(x>>6)&(x>>8)&(x>>7)) 
         ^ ((x>>4)&(x>>5)&(x>>9)&(x>>7)) ^ ((x>>4)&(x>>5)&(x>>7)&(x>>10));
    
    t = F(0, (x>>0), (x>>1), (x>>2), (x>>3), (x>>4), 
	     (x>>5), (x>>6), (x>>7), (x>>8), (x>>9), (x>>10), 0);

    assert(ref == t);
  }
  
  printf("** passed **\n\n");
}


/* ------------------------------------------------------------------------- *
 * Verify consistency of ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()
 * ------------------------------------------------------------------------- */

static void  ACHTERBAHN_verify_encrypt()
{
 
  enum  { N = 512 };        /* buffer size */
  u8    plaintext[N],
        ciphertext[N],
        keystream[N];

  ECRYPT_ctx ctx;
  u32   length, i, j, k, ivlen, keylen;

  key128_t key[2] = { "DonaldDuck", "Bertrand Russell" };
  iv128_t  iv     = "Albert Einstein!";

  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes() and ECRYPT_encrypt_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  /* create a pseudo-random message */
  for (k = 0; k < N; ++k) plaintext[k] = (u8)(k*16807);

  /* loop over of the key sizes 80 and 128 and all allowed sizes IV */
  for (k = 0; ECRYPT_KEYSIZE(k) <= ECRYPT_MAXKEYSIZE; ++k) {
    keylen = ECRYPT_KEYSIZE(k);
    
    for (i = 0; ECRYPT_IVSIZE(i) <= keylen; ++i) {
      ivlen = ECRYPT_IVSIZE(i);
      
      /* setup key */
      ECRYPT_keysetup(&ctx,key[k],keylen,ivlen);
      printf("  keylen = %d, ivlen=%d\n",keylen,ivlen);
      fflush(0);
      
      /* loop over various message lengths */
      for (length = 0; length <= N; ++length) {
        
        /* generate key stream for given IV */
        ECRYPT_ivsetup(&ctx,iv);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        ECRYPT_keystream_bytes(&ctx,keystream,length);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        
        /* encrypt directly for given IV */
        ECRYPT_ivsetup(&ctx,iv);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        ECRYPT_encrypt_bytes(&ctx,plaintext,ciphertext,length);
        if (keylen <= 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
        
        /* verify */
        for (j = 0; j < length; ++j)
          assert( ciphertext[j] == (keystream[j] ^ plaintext[j]) );
      }
    }
  }
  
  printf("\n** passed ** \n\n");
}

/* ------------------------------------------------------------------------- *
 * Some encryption/decryption tests
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_encr_decr()
{
  enum  { N  = 1024 };      /* buffer size */
  u8    plaintext[N],
        ciphertext[N];

  ECRYPT_ctx ctx;
  u32   i, k, msglen, ivlen, keylen;

  key128_t key    = "Asterix & Obelix";
  iv128_t  iv     = "Albert Einstein!";

  /* 166 byte message */
  const u8 *original 
        = (u8*)"The wireless telegraph is not difficult to understand.\n"
               "The ordinary telegraph is like a very long cat.\n"
               "You pull the tail in New York, and it meows in Los Angeles.\n"
               "The wireless is the same, only without the cat. (A. Einstein)";

  printf("* ================================================================ *\n");
  printf("* Encryption/decryption test\n");
  printf("* ================================================================ *\n");
  fflush(0);

  msglen = (u32)strlen((char*)original);

  /* loop over all allowed sizes of the key and the IV */
  for (k = 0; ECRYPT_KEYSIZE(k) <= ECRYPT_MAXKEYSIZE; ++k) {
    keylen = ECRYPT_KEYSIZE(k);
    
    for (i = 0; ECRYPT_IVSIZE(i) <= keylen; ++i) {
      ivlen = ECRYPT_IVSIZE(i);
      
      ECRYPT_keysetup(&ctx,key,keylen,ivlen);
      ECRYPT_ivsetup(&ctx,iv);
      if (keylen == 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      ECRYPT_encrypt_bytes(&ctx,original,ciphertext,msglen);
      if (keylen == 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      
      ECRYPT_keysetup(&ctx,key,keylen,ivlen);
      ECRYPT_ivsetup(&ctx,iv);
      if (keylen == 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      ECRYPT_decrypt_bytes(&ctx,ciphertext,plaintext,msglen);
      if (keylen == 80) assert((ctx.A0 == 0) && (ctx.A12 == 0));
      
      if ((ivlen == keylen)) {
        print_string("\nkey: ",keylen/8,key);
        print_string("IV:  ",ivlen/8,iv);
        print_string("------------------------ original  -------------------------\n",msglen,original);
        print_hexstr("------------------------ encrypted -------------------------\n",msglen,ciphertext);
        print_string("------------------------ decrypted -------------------------\n",msglen,plaintext);
      }
      
      assert( !strncmp((char*)original,(char*)plaintext,msglen) );
    }
  }
  
  printf("\n** passed **\n\n");
}

/* ------------------------------------------------------------------------- *
 * Verify ECRYPT_keystream_bytes()
 * ------------------------------------------------------------------------- */

static void ACHTERBAHN_verify_keystream()
{
  u32 k;
  enum  { N  = 256 };       /* buffer size */
  u8 keystream[N];

  u8 reference80[N] = {
    0xE5,0x9C,0xD6,0x32,0xF3,0xF1,0xAF,0x7D,0xA4,0x7E,0x19,0xFF,0x46,0x65,0x1F,0x38,
    0x10,0x3A,0x29,0xF2,0xF6,0x55,0xED,0xC0,0x7F,0x5D,0x6D,0x2D,0xD6,0x2A,0x96,0xAA,
    0x30,0xF6,0x8D,0xAB,0x76,0x3F,0x28,0x5B,0xEC,0x05,0x26,0x5A,0xB5,0xCD,0xE6,0x43,
    0x78,0x9A,0x19,0x11,0x7A,0x92,0xE6,0x1F,0xC5,0x12,0xD9,0xA6,0xA7,0xA4,0x3F,0xC5,
    0x7D,0x16,0xCF,0xD8,0x4B,0x05,0xEC,0x11,0xD7,0x5A,0x03,0x9C,0x40,0xCA,0x49,0x6A,
    0x61,0xB2,0xA1,0x22,0x86,0xB2,0xC3,0xB1,0x5A,0x75,0x8C,0x80,0x63,0x8D,0x48,0xB7,
    0xA2,0x2F,0xAB,0x05,0x15,0xAB,0xBF,0xAB,0xDA,0x03,0x97,0x0F,0x4E,0x30,0x37,0x03,
    0x02,0x2C,0x81,0x79,0xDE,0xE8,0x18,0x52,0xAA,0x3C,0x7C,0xCF,0x8B,0x43,0xE6,0x92,
    0x20,0x5C,0xF2,0x07,0xF4,0xC7,0xEE,0xA2,0xAF,0x9D,0x48,0x4B,0x3B,0x8C,0x6E,0x05,
    0xEF,0x09,0xDF,0xDE,0x12,0x91,0x8C,0x0D,0x2F,0x71,0x9D,0x17,0xCE,0xD9,0x13,0x65,
    0x49,0xCD,0xBB,0x48,0xCD,0x30,0x5F,0x41,0xC4,0xA0,0xA2,0x59,0x9F,0xE2,0x63,0x9A,
    0x7E,0x4B,0x4A,0x50,0x3A,0x3F,0x72,0xA5,0xE0,0x86,0xDB,0x6C,0x6B,0x4D,0x75,0xF1,
    0x8A,0xF9,0x9A,0x57,0x1D,0x65,0x08,0x40,0x8F,0xEA,0xFE,0x96,0x11,0x09,0x2E,0x5B,
    0x87,0x48,0x09,0xBC,0xE8,0xE6,0x74,0x2D,0x29,0xCA,0x36,0x74,0x41,0x07,0xA7,0x72,
    0x76,0x75,0x2F,0x57,0xBE,0xA9,0x26,0xFA,0xD8,0x5E,0x3B,0x79,0x85,0x11,0xB0,0xDB,
    0x10,0x50,0x39,0x69,0x81,0x2A,0x92,0x7D,0xD4,0x61,0x1A,0xA2,0x07,0x55,0xBB,0x12
  };

  u8 reference128[N] = {
    0xDF,0x71,0xF0,0x42,0x73,0x8F,0x6D,0x9E,0xC2,0x1D,0x89,0x6D,0x0C,0xC1,0x2B,0xAF,
    0x54,0xC8,0xCE,0x55,0xA6,0x50,0x7A,0x12,0x43,0xB4,0x71,0xC2,0xCD,0xF0,0xEC,0x42,
    0x86,0xFC,0x01,0x45,0x43,0x80,0x90,0x13,0xDC,0xA4,0xCE,0xDB,0x0F,0x11,0xDA,0xF4,
    0x52,0xBD,0xCA,0x14,0x6F,0x6B,0x65,0x72,0x1D,0xBC,0x79,0x2C,0xD2,0xB6,0x36,0x14,
    0xF8,0xDB,0xE7,0xCB,0x7B,0x16,0x35,0x61,0xE8,0x10,0x4A,0x75,0xBD,0x4B,0x92,0x9E,
    0xA8,0x02,0x87,0x96,0x13,0x93,0x4A,0xB5,0xFD,0x91,0x16,0x1F,0x35,0x00,0xE5,0x3F,
    0x36,0x69,0x85,0x34,0xCC,0x9C,0xE6,0xE9,0xA5,0xC2,0x74,0xC8,0x2D,0x04,0x93,0x2E,
    0x75,0x06,0x40,0x7A,0xDE,0xAF,0x61,0x1F,0x00,0xF7,0xD3,0x8D,0x2F,0x1D,0x38,0x94,
    0xE2,0x12,0xCE,0x9F,0xF8,0xCD,0x9B,0x93,0x70,0x18,0xE2,0x56,0x9B,0xB5,0x54,0x45,
    0xEC,0x60,0xB8,0x52,0xB3,0xE6,0x6D,0x53,0x68,0x9B,0x3E,0x40,0x61,0x0A,0x22,0xA8,
    0xD3,0xA1,0x03,0x5C,0xC1,0x76,0x1D,0x50,0xB0,0x51,0xF4,0xDA,0xB4,0xE9,0x2E,0xA1,
    0x57,0xA3,0xD0,0x5F,0x11,0xDD,0x54,0xCF,0x6A,0x07,0xA4,0x4A,0x21,0x56,0x51,0xA6,
    0x91,0x5F,0xF4,0x77,0x58,0x81,0x90,0x29,0x50,0xE4,0x92,0xEA,0xFB,0x6C,0x66,0x28,
    0x85,0x06,0xF2,0xBD,0x60,0xF8,0x1A,0x73,0x68,0x4A,0xF7,0xCD,0xEC,0xCF,0xD0,0x1A,
    0xCD,0x09,0xA8,0x6B,0xC6,0x43,0x21,0x8A,0xEF,0xDF,0x27,0xC1,0x47,0x25,0xB9,0x06,
    0xA9,0x40,0x44,0x62,0x86,0x61,0x20,0x66,0x1E,0x70,0x76,0xED,0x93,0xEC,0x31,0xA9
  };

  ECRYPT_ctx ctx;
  iv128_t  iv     = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10 };
  key128_t key80  = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a };
  key128_t key128 = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10 };

  printf("* ================================================================ *\n");
  printf("* Verify ECRYPT_keystream_bytes()\n");
  printf("* ================================================================ *\n");
  fflush(0);

  print_hexstr("\nkey = ",10,key80);
  print_hexstr("iv  = ",10,iv);

  ECRYPT_keysetup(&ctx,key80,80,80);
  ECRYPT_ivsetup(&ctx,iv);
  ECRYPT_keystream_bytes(&ctx,keystream,N);

  print_hexstr ("keystream =\n", N, keystream);

  for (k = 0; k < N; ++k)
    assert(keystream[k] == reference80[k]);
  printf("** passed **\n");

  print_hexstr("\nkey = ",16,key128);
  print_hexstr("iv  = ",16,iv);

  ECRYPT_keysetup(&ctx,key128,128,128);
  ECRYPT_ivsetup(&ctx,iv);
  ECRYPT_keystream_bytes(&ctx,keystream,N);

  print_hexstr ("keystream =\n", N, keystream);

  for (k = 0; k < N; ++k)
    assert(keystream[k] == reference128[k]);

  printf("** passed **\n\n");
}


/* ------------------------------------------------------------------------- *
 *
 * Main program.
 * Perform functional verification tests.
 *
 * ------------------------------------------------------------------------- */

int main (void)
{
  /* ----------------------------------------------------------------------- *
   * Introduction screen
   * ----------------------------------------------------------------------- */

  ACHTERBAHN_splashscreen();

  /* ----------------------------------------------------------------------- *
   * Verify basic requirements.
   * ----------------------------------------------------------------------- */

  /* check required types */
  assert(sizeof(u32) >= 4);
  assert(sizeof(u64) >= 8);

  /* check constants in header file */
  assert(!strcmp(ECRYPT_NAME,"ACHTERBAHN-128/80"));
  assert(ECRYPT_MAXKEYSIZE == 128);
  assert(ECRYPT_KEYSIZE(0) ==  40);
  assert(ECRYPT_KEYSIZE(11)== 128);
  assert(ECRYPT_MAXIVSIZE  == 128);
  assert(ECRYPT_IVSIZE(0)  ==   0);
  assert(ECRYPT_IVSIZE(1)  ==   8);
  assert(ECRYPT_IVSIZE(10) ==  80);
  assert(ECRYPT_IVSIZE(16) == 128);
  
  /* ----------------------------------------------------------------------- *
   * Functional verification of the implementation.
   * ----------------------------------------------------------------------- */

  ACHTERBAHN_verify_keystream();
  ACHTERBAHN_verify_encr_decr();
  ACHTERBAHN_verify_encrypt();
  ACHTERBAHN_verify_BooleanFunction();
  ACHTERBAHN_verify_NLFSRs();

  printf("* ================================================================ *\n");
  printf("* all tests passed - hit return to finish\n");
  printf("* ================================================================ *\n");
  printf("*\n");

  getchar();
  return 0;
}

/* ------------------------------------------------------------------------- */

#endif
