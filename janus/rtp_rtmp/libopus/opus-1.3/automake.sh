#!/bin/sh

SRC_PATH=$(pwd)
echo get src path $SRC_PATH
cd ../
INSTALL_PAHT=$(pwd)
echo get install path $INSTALL_PAHT

cd $SRC_PATH

./configure CFLAGS='-O0'  --prefix=$INSTALL_PAHT --disable-shared  --with-pic

make && make install
