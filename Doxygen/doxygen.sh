#!/usr/bin/env bash

dir_script=$(dirname -- "$(readlink -f -- "$0")")

pushd "$dir_script" > /dev/null
Version=`grep AL_ENCODER_VERSION_STR "../exe_encoder/resource.h" | sed -e 's/^.*\"\(.*\)\".*$/\1/'`

trap "rm -f $dir_script/doxygen_tmp.cfg" EXIT

DOXYGEN_CFG=doxygen.cfg

if [[ -e ../doxygen.append ]]
then
  cat doxygen.cfg ../doxygen.append > doxygen_tmp.cfg
  DOXYGEN_CFG=doxygen_tmp.cfg
fi

AL_VERSION=$Version doxygen "$DOXYGEN_CFG"

./SvgCleanAndLink.py Encoder.svg doc/html/Encoder.svg doc/html/globals_func.html
./SvgCleanAndLink.py Decoder.svg doc/html/Decoder.svg doc/html/globals_func.html
popd > /dev/null
