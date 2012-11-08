#!/bin/bash
DIR="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$DIR/../lib/:$LD_LIBRARY_PATH
export PATH=$PATH:$DIR/../
export DYLD_LIBRARY_PATH=$DIR/../lib/:$DYLD_LIBRARY_PATH
exec "$DIR/idp" -d "$DIR/../" "$@"
