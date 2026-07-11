#!/bin/sh

set -e

HERE="$(dirname "$(realpath "$0")")"

export PATH="$HERE/build/toolchain/bin:$HERE/build/tools:$PATH"

ninja -v
