#!/usr/bin/env sh

ulimit -c unlimited
LD_LIBRARY_PATH=lib:${LD_LIBRARY_PATH} ./Serverd
