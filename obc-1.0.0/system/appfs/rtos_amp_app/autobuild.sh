#!/bin/sh

if [ ! -d build/rtos_lib ] ; then
    mkdir -p build/rtos_lib
fi


make -f Makefile_libcsl csl_clean install
make -f Makefile_libosal clean install
make -f Makefile_freertos freertos_clean install

make -f Makefile clean all














