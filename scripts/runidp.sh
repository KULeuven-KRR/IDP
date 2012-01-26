#!/bin/bash
DIR="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$DIR/../lib/:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$DIR/../lib/:$DYLD_LIBRARY_PATH
$DIR/idp -d $DIR/../ "$@"