#!/usr/bin/env bash

set -euo pipefail
scriptDir=$(dirname -- "$(readlink -f -- "$0")")

res=""

has ()
{
  which $1 > /dev/null
}

if has git; then
  git_hash="$(git rev-parse --verify HEAD 2> /dev/null)"
  if [ $? -eq 0 ]; then
    res="+$git_hash"
  fi
fi

echo -n "$res"
