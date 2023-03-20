#!/usr/bin/env sh

set -xe

./build.sh

./easm2nasm ./examples/123i.easm > 123i.asm
nasm -felf64 123i.asm
ld -o 123i 123i.o

./easm2nasm ./examples/fib.easm > fib.asm
nasm -felf64 fib.asm
ld -o fib fib.o

# TODO: not all of the examples are translatable with basm2nasm
