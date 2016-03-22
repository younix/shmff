#!/bin/sh

cpus=$(sysctl -n hw.ncpu)

./shmff "maria.ff" "out.ff" <<EOF
./invert -j $cpus
./grey -j $cpus
EOF
#./dummy
#./invert -j $cpus
#./grey -j $cpus
#./crop 0 0 100 100
