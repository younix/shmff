#!/bin/sh

cpus=$(sysctl -n hw.ncpu)

./shmff "maria.ff" "out.ff" <<EOF
./invert -j $cpus
./grey -j $cpus
./crop 550 50 300 300
EOF
#./dummy
#./invert -j $cpus
#./grey -j $cpus
#./crop 0 0 100 100
