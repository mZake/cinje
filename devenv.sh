#!/bin/bash

if [[ $PS1 != *"(dev)"* ]]; then
    PS1="(dev) $PS1"
fi

export PATH="$PWD/build/toolchain/bin:$PWD/build/tools:$PATH"
