#!/usr/bin/env sh

set -xe

./build.sh

rm -r test/
mkdir -p test/examples

for example in `find examples/ -name \*.easm | sed "s/\.easm//"`; do
    	./evmr -p "build/$example.evm" -ao "test/$example.expected.out"
done
