#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=PLib
TEST_TYPE=-f
FILE=src/plib.h

DIE=0

have_libtool=false
if libtoolize --version < /dev/null > /dev/null 2>&1 ; then
	libtool_version=`libtoolize --version |
			 head -1 |
			 sed -e 's/^\(.*\)([^)]*)\(.*\)$/\1\2/g' \
			     -e 's/^[^0-9]*\([0-9.][0-9.]*\).*/\1/'`
	case $libtool_version in
	    1.4*|1.5*|2.2*|2.4*)
		have_libtool=true
		;;
	esac
fi
if $have_libtool ; then : ; else
	echo
	echo "You must have libtool 1.4 installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/libtool/"
	DIE=1
fi

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
	DIE=1
}

if automake-1.13 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.13
    ACLOCAL=aclocal-1.13
else if automake-1.12 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.12
    ACLOCAL=aclocal-1.12
else if automake-1.11 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.11
    ACLOCAL=aclocal-1.11
else if automake-1.10 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.10
    ACLOCAL=aclocal-1.10
else if automake-1.9 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.9
    ACLOCAL=aclocal-1.9
else
	echo
	echo "You must have automake 1.9.x, 1.10.x, 1.11.x, 1.12.x or 1.13.x"
	echo "installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	DIE=1
fi
fi
fi
fi
fi

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        if test -z "$*"; then
                echo "I am going to run ./configure with no arguments - if you wish "
                echo "to pass any to it, please specify them on the $0 command line."
        fi
fi

rm -rf autom4te.cache

# README and INSTALL are required by automake, but may be deleted by clean
# up rules. To get automake to work, simply touch these here, they will be
# regenerated from their corresponding *.in files by ./configure anyway.
touch README INSTALL

$ACLOCAL $ACLOCAL_FLAGS || exit $?

libtoolize --force || exit $?

autoheader || exit $?

$AUTOMAKE --add-missing || exit $?
autoconf || exit $?
cd $ORIGDIR || exit $?

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        $srcdir/configure --enable-maintainer-mode $AUTOGEN_CONFIGURE_ARGS "$@" || exit $?

        echo 
        echo "Now type 'make' to compile $PROJECT."
fi
