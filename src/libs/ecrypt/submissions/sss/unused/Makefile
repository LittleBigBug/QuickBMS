#
# NLS/SSS master makefile.
# This makefile drives various sub-makefiles, if they are present.
#
# Generic targets: all dist test time clean
#
# $Id: Makefile 388 2005-04-28 21:04:09Z mwp $
#

SHELL =	/bin/sh
TARGETS = all dist test time clean\
	nlstest nlstime ssstest ssstime\
	ecrypt-nls-test ecrypt-sss-test

$(TARGETS):
	@+$(MAKE) `for m in Makefile.*; do echo -f $$m; done` $@
