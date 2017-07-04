#!/bin/bash

set -euo pipefail
scriptDir=$(dirname -- "$(readlink -f -- "$0")")

res=""

has ()
{
  if which $1 > /dev/null ; then
    return 0
  else
    return 1
  fi
}

if (has svnversion) ; then
  svnversion="$(svnversion -n $scriptDir)"
  if !(echo $svnversion | grep "Unversioned" > /dev/null 2>&1) ; then
    res="+$svnversion"
  fi
fi

echo -n "$res"

