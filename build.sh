#!/bin/sh

set -e

HERE="$(dirname "$(realpath "$0")")"

export PATH="$HERE/build/devkitarm/bin:$HERE/build/tools:$PATH"

ninja -v
