
#######################################
# The list of libraries we are building separated by spaces.
# The 'lib_' indicates that these build products will be installed
# in the $(libdir) directory. For example /usr/lib
lib_LTLIBRARIES = lib@PACKAGE_NAME@.la

#######################################
# Build information for each library

library_include_lzsdir=$(includedir)/@PACKAGE_NAME@
library_include_lzs_HEADERS = lzs.h
lib@PACKAGE_NAME@_la_SOURCES = lzs-compression.c lzs-compression-simple.c lzs-decompression.c
lib@PACKAGE_NAME@_la_SOURCES += lzs-common.h
lib@PACKAGE_NAME@_la_LDFLAGS = -version-info @LIB_SO_VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lib@PACKAGE_NAME@.pc
