#!/usr/bin/env bash

dir_script=$(dirname -- "$(readlink -f -- "$0")")

pushd "$dir_script" > /dev/null
Version=`grep AL_ENCODER_VERSION_STR "../exe_encoder/resource.h" | sed -e 's/^.*\"\(.*\)\".*$/\1/'`

AL_VERSION=$Version doxygen doxygen.cfg

./SvgCleanAndLink.py Encoder.svg doc/html/Encoder.svg doc/html/globals_func.html
./SvgCleanAndLink.py Decoder.svg doc/html/Decoder.svg doc/html/globals_func.html
popd > /dev/null
