#!/bin/bash

set -euo pipefail

readonly scriptDir=$(dirname -- "$(readlink -f -- "$0")")

main ()
{
  local -r firmware_data="$1"
  local -r plugin_data="$2"
  # plugin data start at 0x6F00
  # and up to 0x6FD0
  local -r size="$(wc -c < "$firmware_data")"
  local -r pad_size=$(echo "28416 - $size" | bc)
  local -r output="$3"

  if [ $pad_size -lt 0 ]; then
    >&2 echo "Error, the firmware data size is too big. Make room for the plugin."
    exit 1
  fi

  "$scriptDir/concat_firmware.sh" "$firmware_data" "$plugin_data" "$pad_size" > "$output"
}

main "$@"
