#!/bin/sh -e
src="`basename "$1"`.c"
redo-ifchange compile "${src}"
exec ./compile "$3" "${src}" "$1.d"
