## Process this file with automake to create Makefile.in
#
# Copyright (C) 1996-2004 Markus F.X.J. Oberhumer
#

VPATH = @srcdir@@UCL_ASM_VPATH@
SUFFIXES = .S

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)
CPPFLAGS = $(UCL_CPPFLAGS) $(configure_CPPFLAGS) $(UCL_EXTRA_CPPFLAGS)
CFLAGS = $(UCL_CFLAGS) $(configure_CFLAGS) $(UCL_EXTRA_CFLAGS)

lib_LTLIBRARIES = libucl.la

libucl_la_LDFLAGS = -version-info 1:0:0

libucl_la_SOURCES = \
	alloc.c \
	n2b_99.c n2b_d.c n2b_ds.c n2b_to.c \
	n2d_99.c n2d_d.c n2d_ds.c n2d_to.c \
	n2e_99.c n2e_d.c n2e_ds.c n2e_to.c \
	ucl_crc.c \
	ucl_init.c \
	ucl_ptr.c \
	ucl_str.c \
	ucl_util.c

EXTRA_DIST = \
	getbit.h \
	n2_99.ch \
	ucl_conf.h \
	ucl_dll.ch \
	ucl_mchw.ch \
	ucl_ptr.h \
	ucl_swd.ch

libucl_la_LIBADD = $(UCL_ASM_OBJECTS)
libucl_la_DEPENDENCIES = $(libucl_la_LIBADD)


# /***********************************************************************
# // assembler support
# ************************************************************************/

if UCL_USE_ASM

UCL_ASM_OBJECTS = \
	cpuid.lo crc_asm.lo crcs_asm.lo rdtsc.lo rdtsca.lo \
	n2b_d_f1.lo n2b_d_f2.lo n2b_d_f3.lo n2b_d_f4.lo n2b_d_f5.lo n2b_d_f6.lo \
	n2b_d_n1.lo n2b_d_n2.lo n2b_d_n3.lo n2b_d_n4.lo n2b_d_n5.lo n2b_d_n6.lo \
	n2b_d_s1.lo n2b_d_s2.lo n2b_d_s3.lo n2b_d_s4.lo n2b_d_s5.lo n2b_d_s6.lo \
	n2d_d_f1.lo n2d_d_f2.lo n2d_d_f3.lo n2d_d_f4.lo n2d_d_f5.lo n2d_d_f6.lo \
	n2d_d_n1.lo n2d_d_n2.lo n2d_d_n3.lo n2d_d_n4.lo n2d_d_n5.lo n2d_d_n6.lo \
	n2d_d_s1.lo n2d_d_s2.lo n2d_d_s3.lo n2d_d_s4.lo n2d_d_s5.lo n2d_d_s6.lo \
	n2e_d_f1.lo n2e_d_f2.lo n2e_d_f3.lo n2e_d_f4.lo n2e_d_f5.lo n2e_d_f6.lo \
	n2e_d_n1.lo n2e_d_n2.lo n2e_d_n3.lo n2e_d_n4.lo n2e_d_n5.lo n2e_d_n6.lo \
	n2e_d_s1.lo n2e_d_s2.lo n2e_d_s3.lo n2e_d_s4.lo n2e_d_s5.lo n2e_d_s6.lo

.S.o:
	$(CC) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(CC) -c $<

endif

