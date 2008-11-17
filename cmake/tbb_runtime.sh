#! /bin/sh

# Script to generate the runtime version string as done by the TBB 2.1 makefile

KERNEL_VER=`uname -r | sed -e 's/-.*$//'`
GLIBC_VER=`getconf GNU_LIBC_VERSION | grep glibc | sed -e 's/^glibc //' | sed -e '2,$d' -e 's/-.*$//'`
GCC_VER=`gcc --version | grep 'gcc'| egrep -o ' [0-9]+\.[0-9]+\.[0-9]+.*' | sed -e 's/^\ //' | egrep -o '^[0-9]+\.[0-9]+\.[0-9]+\s*' | head -n 1 | sed -e 's/ *//g'`

RUNTIME=cc$GCC_VER\_libc$GLIBC_VER\_kernel$KERNEL_VER

echo $RUNTIME
