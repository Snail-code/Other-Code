#!/bin/sh

SRC_PATH=$(pwd)

OPUS_INC_PATH=$SRC_PATH/rtp_rtmp/libopus/include/opus
OPUS_LIB_PATH=$SRC_PATH/rtp_rtmp/libopus/lib

SOFIA_INC_PATH=/usr/local/include/sofia-sip-1.12/
SOFIA_LIBS_PATH=/usr/local/lib

#sh -x ./autogen.sh

./configure CFLAGS="-fstack-protector-all -ggdb -O0" --enable-libsrtp2 --disable-docs SOFIA_CFLAGS="-I$SOFIA_INC_PATH" SOFIA_LIBS="-L$SOFIA_LIBS_PATH" OPUS_CFLAGS="-I$OPUS_INC_PATH" OPUS_LIBS="-L$OPUS_LIB_PATH -lopus"


