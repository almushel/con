#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SRC_DIR=$SCRIPT_DIR/../src
BIN_DIR=$SCRIPT_DIR/../bin

if [ ! -d $BIN_DIR ]; then
	mkdir $BIN_DIR
fi

gcc $SRC_DIR/main.c -g -o $BIN_DIR/fredc
