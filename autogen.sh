#!/bin/sh
# Bootstrap the CLucene installation.
# Run this to generate all the initial makefiles, etc.
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=clucene-core
TEST_TYPE=-f
FILE=src/CLucene/StdHeader.h

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to call autogen.sh."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

AUTOMAKE=automake-1.9
ACLOCAL=aclocal-1.9

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
     AUTOMAKE=automake
     ACLOCAL=aclocal
}

($AUTOMAKE --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to call autogen.sh."
	echo "Get ftp://sourceware.cygnus.com/pub/automake/automake-1.4.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to call autogen.sh."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.2d.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test "$1" = "--clean" || test "$1" = "--clear"; then
  make maintainer-clean
  rm -fdr build libltdl config autom4te.cache _configs.sed
  exit 0
fi

#create the temp gcc config dir
mkdir config 2>/dev/null

set -x
libtoolize --force --copy --ltdl --automake
$ACLOCAL -I m4
autoconf
autoheader
$AUTOMAKE --add-missing --copy

cd "$ORIGDIR"
