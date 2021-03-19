#!/usr/bin/env sh

CURR_PATH=`dirname $0`
BIN=`basename $0 .sh`

LD_LIBRARY_PATH=$CURR_PATH/lib:$CURR_PATH/lib/provider:${LD_LIBRARY_PATH} $CURR_PATH/$BIN "$@"
