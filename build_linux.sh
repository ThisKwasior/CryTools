#!/bin/bash

PROGRAMS=("test_mpeg" "test_adx" "sfdmux")
INCLUDES=-I./include
LIBS="./lib/mpeg.c ./lib/utils.c ./lib/adx.c ./lib/sfd.c"
ARGS="-Qn -O2 -s"

mkdir bin

for prog in "${PROGRAMS[@]}"
do
	echo Building $prog
	gcc "./src/$prog.c" -o "./bin/$prog" $ARGS $INCLUDES $LIBS
done
