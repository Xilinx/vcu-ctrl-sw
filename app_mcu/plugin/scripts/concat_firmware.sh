#!/bin/bash

set -euo pipefail

tmpDir=/tmp/plug-firmware-$$
trap "rm -rf $tmpDir" EXIT
mkdir -p $tmpDir

concat_firmware ()
{
  local -r firmware="$1"
  local -r plugin="$2"
  local -r pad_size="$3"

  local -r pad=$tmpDir/zero_pad;
  local -r zero="/dev/zero"

  >&2 echo "using zeroed pad of $pad_size"

  dd if="$zero" of="$pad" bs=$pad_size count=1 > /dev/null
  cat "$firmware" "$pad" "$plugin"
}

concat_firmware "$@"
