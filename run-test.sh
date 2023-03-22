#!/usr/bin/env sh

set -xe

./build.sh

for example in `find examples/ -name \*.easm | sed "s/\.easm//"`; do
    ./evmr -p "build/$example.evm" -eo "test/$example.expected.out"
done
