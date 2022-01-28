#!/bin/sh

# ==========================(LICENSE BEGIN)============================
#
# Copyright (c) 2007-2010  Projet RNRT SAPHIR
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# ===========================(LICENSE END)=============================
#
# @author   Thomas Pornin <thomas.pornin@cryptolog.com>
#
#
# This script tries to autodetect the make utility, C compiler and
# library archiver. These parameters can be overridden with command-line
# arguments.
#

#
# For Solaris, we need to switch to a more POSIX-compliant /bin/sh binary.
#
if [ -z "$SPH_SCRIPT_LOOP" ] ; then
	SPH_SCRIPT_LOOP=yes
	export SPH_SCRIPT_LOOP
	if [ -x /usr/xpg6/bin/sh ] ; then
		exec /usr/xpg6/bin/sh "$0" "$@"
	fi
	if [ -x /usr/xpg4/bin/sh ] ; then
		exec /usr/xpg4/bin/sh "$0" "$@"
	fi
fi

#
# Exit on first error.
#
set -e

#
# Go to the directory hosting the file.
#
cd "$(dirname "$0")"

#
# Print out usage (on stderr) and exit with an error status.
#
usage() {
	cat >&2 <<EOHELP
usage: build.sh [ options ]
options:
  -h | --help            print this help
  -q | --quiet           do not print out configuration information
  -i | --install         also install
  -c | --clean           do not build; instead, clean directory
  --disable-tests        do not build unit tests
  --disable-runtests     build but do not run the tests (for cross-compilation)
  --prefix=dir           use 'dir' as prefix [/usr/local]
  --installdir-bin=dir   install executable tools in 'dir' [prefix/bin]
  --installdir-lib=dir   install compiled static library in 'dir' [prefix/lib]
  --installdir-inc=dir   install library headers in 'dir' [prefix/include]
  --with-make=cmd        use 'cmd' as 'make' command
  --with-cc=cmd          use 'cmd' as C compiler
  --with-cflags=flags    use 'flags' as C compiler flags
  --with-ar=cmd          use 'cmd' as library archiver
EOHELP
	exit 1
}

#
# Print out the argument (on stderr) and exit with an error status.
#
die() {
	echo "error: $1" >&2
	exit 1
}

#
# Find out an executable by trying the provided possibilities.
#
findexe() {
	varname="$1"
	shift
	while [ "$#" -gt 0 ]; do
		exename="${1%% *}"
		if type "$exename" > /dev/null 2> /dev/null ; then
			eval "$varname='$1'"
			return
		fi
		shift
	done
}

#
# Find an executable to fill a variable, unless already specified. Exit
# if no executable is found.
#
checkexe() {
	varname="$1"
	shift
	if [ '!' -z "$(eval echo \$$varname)" ]; then
		return
	fi
	cmddisplay="$1"
	shift
	findexe "$varname" "$@"
	if [ -z "$(eval echo \$$varname)" ]; then
		die "no suitable \"$cmddisplay\" command found"
	fi
}

verbose=yes
install=no
clean=no
buildtests=yes
runtests=yes
prefix=/usr/local
cflags="SPH_UNSET"

#
# Parse arguments
#
while [ "$#" -gt 0 ]; do
	case "$1" in
		-h | --help )
			usage ;;
		-q | --quiet )
			verbose=no ;;
		-c | --clean )
			clean=yes ;;
		-i | --install )
			install=yes ;;
		--disable-tests )
			buildtests=no
			runtests=no ;;
		--disable-runtests )
			runtests=no ;;
		--prefix=* )
			prefix=${1#*=} ;;
		--installdir-bin=* )
			installdirbin=${1#*=} ;;
		--installdir-lib=* )
			installdirlib=${1#*=} ;;
		--installdir-inc=* )
			installdirinc=${1#*=} ;;
		--with-make=* )
			makecmd=${1#*=} ;;
		--with-cc=* )
			cccmd=${1#*=} ;;
		--with-cflags=* )
			cflags=${1#*=} ;;
		--with-ar=* )
			arcmd=${1#*=} ;;
		* )
			usage ;;
	esac
	shift
done

if [ -z "$installdirbin" ]; then
	installdirbin="$prefix/bin"
fi
if [ -z "$installdirlib" ]; then
	installdirlib="$prefix/lib"
fi
if [ -z "$installdirinc" ]; then
	installdirinc="$prefix/include"
fi

checkexe makecmd make gmake make /usr/ccs/bin/make
checkexe cccmd cc gcc c99 cc /usr/ccs/bin/cc
checkexe arcmd ar "ar rcs"

if $cccmd -v 2>&1 | grep "gcc version" > /dev/null ; then
	cclass=gcc
fi

#
# If the machine is Ultrasparc, we somewhat assume that
#
case "$(uname -m)" in
	sun4u )
esac

case "$cflags" in
	SPH_UNSET )
		case "$cclass" in
			gcc )
				cflags="-W -Wall -O1 -fomit-frame-pointer" ;;
			* )
				cflags="-O" ;;
		esac ;;
esac

if [ "$clean" = "yes" ]; then
	mtarget="clean"
else
	mtarget="compile"
	case "$buildtests" in
		yes )
			mtarget="$mtarget build-tests" ;;
	esac
	case "$runtests" in
		yes )
			mtarget="$mtarget run-tests" ;;
	esac
fi
if [ "$install" = "yes" ]; then
	mtarget="$mtarget install"
fi

case "$verbose" in
	yes )
		echo "=============================================="
		uname -a
		echo "make:    $makecmd"
		echo "cc:      $cccmd $cflags"
		echo "ar:      $arcmd"
		echo "targets: $mtarget"
		echo "=============================================="
		;;
esac

$makecmd CC="$cccmd" CFLAGS="$cflags" AR="$arcmd" INSTALLBIN="$installdirbin" INSTALLLIB="$installdirlib" INSTALLINC="$installdirinc" -f Makefile.unix $mtarget
