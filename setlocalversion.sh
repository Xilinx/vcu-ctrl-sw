#!/bin/bash

set -euo pipefail
scriptDir=$(dirname -- "$(readlink -f -- "$0")")

res=""

has ()
{
  which $1 > /dev/null
}

if has svnversion ; then
  svnversion="$(svnversion -n $scriptDir)"
  if !(echo $svnversion | grep "Unversioned" > /dev/null 2>&1) ; then
    if !(echo $svnversion | grep "exported" > /dev/null 2>&1) ; then
      if !(echo $svnversion | grep "Uncommited" > /dev/null 2>&1) ; then
        res="+$svnversion"
      fi
    fi
  fi
fi

echo -n "$res"

