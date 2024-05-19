#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SRC_DIR=$SCRIPT_DIR/../src
BIN_DIR=$SCRIPT_DIR/../bin

if [ ! -d $BIN_DIR ]; then
	mkdir $BIN_DIR
fi

if gcc $SRC_DIR/test.c -g -o $BIN_DIR/fredc_test; then
	$BIN_DIR/fredc_test
fi
