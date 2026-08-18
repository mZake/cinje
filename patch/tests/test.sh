#!/bin/bash

set -e

HERE="$(dirname "$(realpath "${0}")")"

CXX="${CXX:-c++}"

CXXFLAGS="-std=c++17 -I .. -Wall -Wextra"

build-test() {
    ${CXX} ${CXXFLAGS} ${1} -o ${2}
}

expect-success() {
    if "${1}" "${HERE}/data/input.bin" "${HERE}/data/elf.o" "${HERE}/data/output.bin"; then
        echo -e "\e[32;1mTest '${2}' succeeded"
    else
        echo -e "\e[31;1mTest '${2}' failed"
    fi
    echo -e "\e[0m"
}

expect-failure() {
    if ! "${1}" "${HERE}/data/input.bin" "${HERE}/data/elf.o" "${HERE}/data/output.bin"; then
        echo -e "\e[32;1mTest '${2}' succeeded"
    else
        echo -e "\e[31;1mTest '${2}' failed"
    fi
    echo -e "\e[0m"
}

build-test good_patches.cpp "${HERE}/good-patches"
build-test bad_patches.cpp "${HERE}/bad-patches"

expect-success "${HERE}/good-patches" "Good Patches"
expect-failure "${HERE}/bad-patches" "Bad Patches"
