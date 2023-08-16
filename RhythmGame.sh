#!/bin/sh
appname=`basename $0 | sed s,\.sh$,,`

dirname=bin/`dirname $0`
tmp="${dirname#?}"

if [ "${dirname%$tmp}" != "/" ]; then
dirname=$PWD/$dirname
fi
LD_LIBRARY_PATH=$dirname/../lib/
echo $LD_LIBRARY_PATH
export LD_LIBRARY_PATH
$dirname/$appname "$@"
