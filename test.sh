#!/bin/sh

cpus=$(sysctl -n hw.ncpu)

./shmff "maria.ff" "out.ff" <<EOF
./invert
./dummy
./grey -j $cpus
./gauss
EOF
#./dummy
#./invert -j $cpus
#./grey -j $cpus
#./crop 0 0 100 100
#./gauss
