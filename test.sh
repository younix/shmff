#!/bin/sh

cpus=$(sysctl -n hw.ncpu)

./shmff "big.ff" "out.ff" <<EOF
./invert -j $cpus
./grey -j $cpus
./dummy
EOF
