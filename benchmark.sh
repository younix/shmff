#!/bin/sh

cpus=$(sysctl -n hw.ncpu)
target=${1:-./invert -j $cpus}

./shmff -b "in.ff" "out.ff" "$target" &
sleep 10
kill -s HUP %1
wait
