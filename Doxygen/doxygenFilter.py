#!/usr/bin/env python3
from __future__ import annotations

import re
import sys

if (len(sys.argv) < 2):
    print("No input file")
else:
    f = open(sys.argv[1])
    line = f.readline()
    while line:
        introspectPattern = re.compile(r"AL_INTROSPECT[ ]*\(.*\)[ ]*struct")
        alignedPattern = re.compile(r"__AL_ALIGNED__[ ]*\(\d\)")
        line = re.sub(introspectPattern, "struct", line)
        line = re.sub(alignedPattern, "", line)
        print(line, end="")
        line = f.readline()
    f.close()
