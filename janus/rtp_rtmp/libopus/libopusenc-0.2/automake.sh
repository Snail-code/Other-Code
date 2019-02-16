#!/bin/sh

SRC_PATH=$(pwd)
echo get src path $SRC_PATH
cd ../
INSTALL_PAHT=$(pwd)
echo get install path $INSTALL_PAHT

cd $SRC_PATH

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$INSTALL_PAHT/lib/pkgconfig

./configure CFLAGS='-O0'  --prefix=$INSTALL_PAHT --disable-shared  --with-pic --with-pic LDFLAGS="-L$INSTALL_PAHT/lib/"

make && make install
