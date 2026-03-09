#!/bin/bash
grep define ../Gem/glew.h |
grep -v _MESA |
grep -v GLEXT_PROTOTYPES |
grep "GL_" |
awk '{print $2}' |
egrep "^GL_" |
sort -u |
awk '{print "#ifdef "$1"\n  {\""$1"\", "$1"},\n#endif"}'
