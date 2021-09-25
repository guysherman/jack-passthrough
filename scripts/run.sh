#! /bin/bash

set -euo pipefail
export LD_LIBRARY_PATH=$(pwd)/build-debug${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
build-debug/jack-passthru
exit $?
