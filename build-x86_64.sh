#!/usr/bin/env sh

set -xe

./build.sh

./easm2nasm ./examples/123i.easm > ./build/examples/123i.asm
nasm -F dwarf -g -felf64 ./build/examples/123i.asm -o ./build/examples/123i.o
ld -o ./build/examples/123i.exe ./build/examples/123i.o

./easm2nasm ./examples/fib.easm > ./build/examples/fib.asm
nasm -felf64 -F dwarf -g ./build/examples/fib.asm -o ./build/examples/fib.o
ld -o ./build/examples/fib.exe ./build/examples/fib.o

# TODO: not all of the examples are translatable with basm2nasm
