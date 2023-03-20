#!/usr/bin/env sh

set -xe

CC=gcc
CFLAGS="-pedantic -Wall -Wextra -Werror -Wfatal-errors -Wswitch-enum -Wmissing-prototypes -Wconversion -Ofast -flto -march=native -pipe"
LIBS=

$CC $CFLAGS -o easm src/easm.c $LIBS
$CC $CFLAGS -o evmi src/evmi.c $LIBS
$CC $CFLAGS -o deasm src/deasm.c $LIBS
$CC $CFLAGS -o edbug src/edbug.c $LIBS
$CC $CFLAGS -o easm2nasm src/easm2nasm.c $LIBS

