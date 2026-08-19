#!/bin/bash

set -e

HERE="$(dirname "$(realpath "${0}")")"

CXX="${CXX:-c++}"

CXXFLAGS="-std=c++17 -I .. -Wall -Wextra"

INPUT_BINARY="${HERE}/data/input.bin"
ELF_OBJECT="${HERE}/data/elf.o"
OUTPUT_BINARY="${HERE}/data/output.bin"

build-test() {
    PROGRAM="${HERE}/${2}"

    ${CXX} ${CXXFLAGS} ${1} -o "${PROGRAM}"
}

expect-success() {
    PROGRAM="${HERE}/${1}"

    if "${PROGRAM}" "${INPUT_BINARY}" "${ELF_OBJECT}" "${OUTPUT_BINARY}"; then
        echo -e "\e[32;1mTest '${2}' succeeded"
    else
        echo -e "\e[31;1mTest '${2}' failed"
    fi
    echo -e "\e[0m"
}

expect-failure() {
    PROGRAM="${HERE}/${1}"

    if ! "${PROGRAM}" "${INPUT_BINARY}" "${ELF_OBJECT}" "${OUTPUT_BINARY}"; then
        echo -e "\e[32;1mTest '${2}' succeeded"
    else
        echo -e "\e[31;1mTest '${2}' failed"
    fi
    echo -e "\e[0m"
}

build-test good_patches.cpp good-patches
build-test bad_patches.cpp bad-patches

expect-success good-patches "Good Patches"
expect-failure bad-patches "Bad Patches"
