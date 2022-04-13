#!/usr/bin/env bash

set -euo pipefail

readonly scriptDir=$(dirname -- "$(readlink -f -- "$0")")

main ()
{
  local -r firmware_text="$1"
  local -r plugin_text="$2"
  # plugin code start at 0x80080000
  local -r size="$(wc -c < "$firmware_text")"
  local -r pad_size=$(echo "524288 - $size" | bc)
  local -r output="$3"

  if [ $pad_size -lt 0 ]; then
    >&2 echo "Error, the firmware text size is too big. Make room for the plugin."
    exit 1
  fi

  "$scriptDir/concat_firmware.sh" "$firmware_text" "$plugin_text" "$pad_size" > "$output"
}

main "$@"
