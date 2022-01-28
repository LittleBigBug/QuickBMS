#!/bin/sh

# Tornado build script for Unix, written by Joachim Henke

GCC_FLAGS='-pipe -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -fforce-addr -fno-exceptions -fno-rtti'
GCC_FLAGS_VER='-fno-threadsafe-statics -fno-ipa-cp-clone -fwhole-program -combine'
ICC_FLAGS='-O2 -fp-model fast=2 -fno-exceptions -fno-rtti'

cc_option () {
	$CC -S -xc++ $1 -o /dev/null /dev/null 2>/dev/null
}

export LC_ALL='C'
CC=${CC:-gcc}
if [ "${CC%icc*}" != "$CC" ]; then
	CFLAGS=${CFLAGS:-$ICC_FLAGS}
	PROF_GEN='-prof-gen'
	PROF_USE='-prof-use'
	PARM_PROF='-ipo'
else
	CFLAGS=${CFLAGS:-$GCC_FLAGS}
	for FLAG in $GCC_FLAGS_VER; do
		cc_option $FLAG && CFLAGS="$CFLAGS $FLAG"
	done
	PROF_GEN='-fprofile-generate'
	PROF_USE='-fprofile-use'
	PARM_PROF='-funroll-loops'
fi

cc_option -m32 && PARM_ADDR=' -m32' || PARM_ADDR=''
PARM_DEBUG='-s'
PARM_ENDIANNES='-DFREEARC_INTEL_BYTE_ORDER'
PARM_FULL=''
PARM_STAT=''
if [ "$MSYSTEM" = 'MINGW32' ]; then
	PARM_SYSTEM='-DFREEARC_WIN'
	PARM_TIME=''
else
	PARM_SYSTEM='-DFREEARC_UNIX'
	PARM_TIME='-lrt'
fi

for P; do
	if [ "$P" = "${P##-}" ]; then
		TEST_FILE="$P"
		PARM_PROF="$PROF_USE"
	else
		case $P in
		'-64')
			PARM_ADDR=' -m64'
			;;
		'-be')
			PARM_ENDIANNES='-DFREEARC_MOTOROLA_BYTE_ORDER'
			;;
		'-debug')
			CFLAGS=''
			PARM_DEBUG='-g -DDEBUG'
			PARM_PROF=''
			;;
		"-flags="*)
			CFLAGS="$CFLAGS ${P#*=}"
			;;
		'-full')
			PARM_FULL=' -DFULL_COMPILE'
			;;
		"-maxm="*)
			MAX_METHOD="${P#*=}"
			;;
		'-notiming')
			PARM_TIME='-DFREEARC_NO_TIMING'
			;;
		'-stats')
			PARM_STAT=' -DSTAT'
			;;
		'-h')
			echo "usage: $0 [OPTIONS] [FILE_FOR_PROFILING]"
			echo 'possible options:'
			echo '  -full      -  generate code for all possible coder/matchfinder combinations'
			echo '  -64        -  compile 64-bit version'
			echo '  -be        -  compile for big-endian CPUs (PowerPC, SPARC, 68k, ...)'
			echo '  -notiming  -  leave out progress/speed calculation code'
			echo '  -stats     -  print program'\''s internal compression statistics'
			echo '  -debug     -  add debugging info and printfs'
			echo '  -flags=... -  use additional compiler flags'
			echo '  -maxm=#    -  if a FILE_FOR_PROFILING is given as sample data, run up to method -# only'
			echo 'environment variables:'
			echo '  CC     (supports GNU & Intel compiler)'
			echo '  CFLAGS'
			exit 0
			;;
		*)
			echo "unknown option: $P"
			exit 1
		esac
	fi
done
CFLAGS="main.cpp -o tor $CFLAGS$PARM_FULL$PARM_STAT$PARM_ADDR $PARM_ENDIANNES $PARM_SYSTEM $PARM_TIME -D_FILE_OFFSET_BITS=64"

if [ "$PARM_PROF" = "$PROF_USE" ]; then
	echo 'compiling binary for profiling...'
	$CC $PROF_GEN $CFLAGS || exit 1
	M=1
	while [ $M -le ${MAX_METHOD:-11} ]; do
		echo "profiling method $M..."
		./tor -$M -q <"$TEST_FILE" >temp.tor && \
		./tor -d -q <temp.tor | cmp "$TEST_FILE" - || exit 1
		M=$(($M + 1))
	done
	rm -f temp.tor
fi
echo 'compiling final binary...'
$CC $PARM_PROF $PARM_DEBUG $CFLAGS
