#!/bin/sh

ARCHES="i386 x86-64 uclibc-i386 uclibc-x86-64"
test -z "$NAME" && NAME="$(basename "$(pwd)")"
test -e "version.h" && VER="$(grep '#define VER ' version.h | tr -d \\\" | cut -d' ' -f3)"
test -z "$VER" && VER=0
export NAME
export VER
export CHROOT_BASE=/chroots
export WD="$(pwd)"
export PKG="pkg"

echo "chroot builder: building '$NAME' version '$VER'"

trap clean_exit INT QUIT ABRT HUP

clean_exit () {
	umount $CHROOT/proc $CHROOT/sys $CHROOT/tmp $CHROOT/dev $CHROOT/usr/src $CHROOT/home
}

do_build () {
	test -z "$WD" && echo "WD not set, aborting" && exit 1
	test -z "$PKG" && echo "PKG not set, aborting" && exit 1
	make clean
	if ! make -j$JOBS all
		then echo "Build failed"; exit 1
		else
		echo "WD/PKG: $WD/$PKG"
		test -d $WD/$PKG && rm -rf $WD/$PKG
		mkdir $WD/$PKG
		make DESTDIR=$WD/$PKG install && \
			tar -C pkg -c usr | xz -e > ${NAME}_$VER-$ARCH.pkg.tar.xz
	fi
}

if [ "$(id -u)" != "0" ]
	then echo "You must be root to auto-build chroot packages."
	exit 1
fi

if [ "$DO_CHROOT_BUILD" = "1" ]
	then
	test -z "$1" && echo "No arch specified" && exit 1
	test ! -d "$1" && echo "Not a directory: $1" && exit 1
	cd $1
	export WD="$1"
	do_build
	echo "finished: $1"
	exit

	else
	echo baz
	export DO_CHROOT_BUILD=1
	for X in $ARCHES
		do
		echo "Performing package build for $CHROOT"
		export CHROOT="$CHROOT_BASE/$X"
		export ARCH="$X"
		test ! -d $CHROOT && echo "$CHROOT not present, not building $CHROOT_ARCH package." && exit 1
		test ! -x $CHROOT/bin/sh && echo "$CHROOT does not seem to be a chroot; aborting." && exit 1
		mount --bind /dev $CHROOT/dev || clean_exit
		mount --bind /usr/src $CHROOT/usr/src || clean_exit
		mount --bind /home $CHROOT/home || clean_exit
		mount -t proc proc $CHROOT/proc || clean_exit
		mount -t sysfs sysfs $CHROOT/sys || clean_exit
		mount -t tmpfs tmpfs $CHROOT/tmp || clean_exit
		if echo "$CHROOT_ARCH" | grep -q "i386"
			then linux32 chroot $CHROOT $WD/$0 $WD
			else chroot $CHROOT $WD/$0 $WD
		fi
		umount $CHROOT/proc $CHROOT/sys $CHROOT/tmp $CHROOT/dev $CHROOT/usr/src $CHROOT/home
		test -d $WD/$PKG && rm -rf $WD/$PKG
	done
fi
