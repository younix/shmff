#!/bin/sh

cpus=$(sysctl -n hw.ncpu)

./shmff "maria.ff" "out.ff" <<EOF
./grey -j $cpus
./invert -j $cpus
./dummy
EOF
#./crop 0 0 100 100
#./invert -j $cpus
#./grey -j $cpus
#./dummy
