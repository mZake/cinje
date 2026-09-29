#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
import sys

from io import BufferedWriter
from typing import Dict, List, Optional, Union

# Change this if needed
OFFSET_TO_INSERT = 0x1400000
BASE_ROM_FILE = "BPRE.gba"
OUT_ROM_FILE = "BPRE_out.gba"

ADDRESS_TO_INSERT = OFFSET_TO_INSERT + 0x8000000

ASM_DIR = "asm"
GFX_DIR = "graphics"
INC_DIR = "include"
SRC_DIR = "src"
BUILD_DIR = "build"
PATCH_DIR = "patch"
TOOLS_DIR = "tools"

BLOB_OBJECT = f"{BUILD_DIR}/blob.o"

GBAGFX   = f"{TOOLS_DIR}/gbagfx/gbagfx"
MID2AGB  = f"{TOOLS_DIR}/mid2agb/mid2agb"
PATCHBIN = f"{PATCH_DIR}/patchbin"
PREPROC  = f"{TOOLS_DIR}/preproc/preproc"
SCANINC  = f"{TOOLS_DIR}/scaninc/scaninc"
WAV2AGB  = f"{TOOLS_DIR}/wav2agb/wav2agb"

# Tools for cross-compiling ARMv4T binaries
TARGET_CC = "arm-none-eabi-gcc"
TARGET_AS = "arm-none-eabi-as"
TARGET_LD = "arm-none-eabi-ld"

# Tools for compiling native binaries
HOST_CC  = "gcc"
HOST_CXX = "g++"

CFLAGS   = f"-mthumb -mthumb-interwork -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu -mlong-calls -O2 -fno-toplevel-reorder"
ASFLAGS  = f"-mthumb -mthumb-interwork -march=armv4t -mcpu=arm7tdmi -meabi=gnu -I {ASM_DIR}"
LDFLAGS  = f"-T linker.ld BPRE.ld --defsym=BLOB_BEGIN=0x{ADDRESS_TO_INSERT:08X}"
CPPFLAGS = f"-I {INC_DIR}"
SCANINC_INCLUDES = f"-I {INC_DIR} -I {ASM_DIR}"

class Writer:
    def __init__(self, stream: BufferedWriter):
        self.stream = stream

    def newline(self):
        self.stream.write("\n")

    def comment(self, text: str):
        self._line(f"# {text}")

    def variable(self, key: str, value: str):
        self._line(f"{key} = {value}")

    def rule(self, name: str, **kwargs):
        self._line(f"rule {name}")
        for key, value in kwargs.items():
            self._line(f"{key} = {value}", indent=2)
        self.newline()

    def pool(self, name: str, depth: int):
        self._line(f"pool {name}")
        self._line(f"depth = {depth}", indent=2)
        self.newline()

    def include(self, path: str):
        self._line(f"include {path}")

    def subninja(self, path: str):
        self._line(f"subninja {path}")

    def build(
        self,
        rule: str,
        inputs: Union[List[str], str],
        outputs: Union[List[str], str],
        implicit_inputs: Optional[Union[List[str], str]] = None,
        implicit_outputs: Optional[Union[List[str], str]] = None,
        order_only_deps: Optional[Union[List[str], str]] = None,
        variables: Optional[Dict[str, Optional[str]]] = None,
    ):
        all_inputs = as_list(inputs)
        if implicit_inputs is not None:
            all_inputs.append("|")
            all_inputs.extend(as_list(implicit_inputs))
        if order_only_deps is not None:
            all_inputs.append("||")
            all_inputs.extend(as_list(order_only_deps))

        all_outputs = as_list(outputs)
        if implicit_outputs is not None:
            all_outputs.append("|")
            all_outputs.extend(as_list(implicit_outputs))

        inputs_text = " ".join(as_list(all_inputs))
        outputs_text = " ".join(as_list(all_outputs))

        self._line(f"build {outputs_text}: {rule} {inputs_text}")
        for key, value in variables.items():
            if value is not None:
                self._line(f"{key} = {value}", indent=2)

        self.newline()

    def _line(self, text: str, indent: int = 0):
        stripped = text.lstrip()
        length = len(stripped) + indent
        self.stream.write(f"{stripped:>{length}}\n")

def as_list(input: Optional[Union[List[str], str]]) -> List[str]:
    if input is None:
        return []
    if isinstance(input, list):
        return input
    return [input]

def warning(message: str):
    sys.stderr.write(f"generate.py: warning: {message}")
    sys.stderr.write("\n")

def fatal(message: str):
    sys.stderr.write(f"generate.py: error: {message}")
    sys.stderr.write("\n")
    sys.exit(1)

def collect_files(directory: str, extensions: Union[str, List[str]]) -> List[str]:
    matches = []
    for extension in as_list(extensions):
        matches.extend(glob.glob(f"{directory}/**/*{extension}", recursive=True))
    return matches

def derive_files(inputs: Union[List[str], str], pattern: str) -> List[str]:
    outputs = list()
    for input in as_list(inputs):
        stem = os.path.splitext(input)[0]
        output = pattern.replace("%", stem)
        outputs.append(output)

    return outputs

def build_gfx(
    writer: Writer,
    inputs: Union[List[str], str],
    outputs: Union[List[str], str]
):
    for input, output in zip(inputs, outputs):
        writer.build(
            "gbagfx",
            inputs=input,
            outputs=output,
            implicit_inputs=GBAGFX,
            variables={"GBAGFX": GBAGFX},
        )

def build_c(
    writer: Writer,
    inputs: Union[List[str], str],
    outputs: Union[List[str], str],
    depfiles: Union[List[str], str]
):
    for input, output, depfile in zip(inputs, outputs, depfiles):
        writer.build(
            "target_cc",
            inputs=input,
            outputs=output,
            implicit_inputs=PREPROC,
            variables={
                "CC": TARGET_CC,
                "PREPROC": PREPROC,
                "CFLAGS": CFLAGS,
                "CPPFLAGS": CPPFLAGS,
                "DEPFILE": depfile,
            },
        )

def build_asm(
    writer: Writer,
    inputs: Union[List[str], str],
    outputs: Union[List[str], str],
    depfiles: Union[List[str], str]
):
    for input, output, depfile in zip(inputs, outputs, depfiles):
        writer.build(
            "target_as",
            inputs=input,
            outputs=output,
            implicit_inputs=PREPROC,
            variables={
                "AS": TARGET_AS,
                "CC": TARGET_CC,
                "PREPROC": PREPROC,
                "ASFLAGS": ASFLAGS,
                "CPPFLAGS": CPPFLAGS,
                "DEPFILE": depfile,
            },
        )

def build_depfile(
    writer: Writer,
    inputs: Union[List[str], str],
    outputs: Union[List[str], str]
):
    for input, output in zip(inputs, outputs):
        writer.build(
            "scaninc",
            inputs=input,
            outputs=output,
            implicit_inputs=SCANINC,
            variables={
                "SCANINC": SCANINC,
                "INCLUDES": SCANINC_INCLUDES,
            },
        )

def build_c_project(
    directory: str,
    output: str,
    cflags: Optional[str] = None,
    ldflags: Optional[str] = None,
):
    source_files = collect_files(directory, ".c")
    object_files = derive_files(source_files, f"{BUILD_DIR}/%.o")
    depfiles = derive_files(source_files, f"{BUILD_DIR}/%.d")

    build_file_path = os.path.join(directory, "build.ninja")
    with open(build_file_path, "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        for src_file, obj_file, depfile in zip(source_files, object_files, depfiles):
            writer.build(
                "host_cc",
                inputs=src_file,
                outputs=obj_file,
                variables={
                    "CC": HOST_CC,
                    "CFLAGS": cflags,
                    "DEPFILE": depfile,
                },
            )

        writer.build(
            "ld",
            inputs=object_files,
            outputs=output,
            variables={
                "LD": HOST_CC,
                "LDFLAGS": ldflags,
            },
        )

def build_cxx_project(
    directory: str,
    output: str,
    cxxflags: Optional[str] = None,
    ldflags: Optional[str] = None,
):
    source_files = collect_files(directory, ".cpp")
    object_files = derive_files(source_files, f"{BUILD_DIR}/%.o")
    depfiles = derive_files(source_files, f"{BUILD_DIR}/%.d")

    build_file_path = os.path.join(directory, "build.ninja")
    with open(build_file_path, "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        for src_file, obj_file, depfile in zip(source_files, object_files, depfiles):
            writer.build(
                "host_cxx",
                inputs=src_file,
                outputs=obj_file,
                variables={
                    "CXX": HOST_CXX,
                    "CXXFLAGS": cxxflags,
                    "DEPFILE": depfile,
                },
            )

        writer.build(
            "ld",
            inputs=object_files,
            outputs=output,
            variables={
                "LD": HOST_CXX,
                "LDFLAGS": ldflags,
            },
        )

def main():
    # Inputs
    png_files   = collect_files(GFX_DIR, ".png")
    c_sources   = collect_files(SRC_DIR, ".c")
    asm_sources = collect_files(ASM_DIR, ".s")

    # Outputs
    bpp1_files = derive_files(png_files, "%.1bpp")
    bpp4_files = derive_files(png_files, "%.4bpp")
    bpp8_files = derive_files(png_files, "%.8bpp")

    bpp1_lz_files = derive_files(png_files, "%.1bpp.lz")
    bpp4_lz_files = derive_files(png_files, "%.4bpp.lz")
    bpp8_lz_files = derive_files(png_files, "%.8bpp.lz")

    c_objects   = derive_files(c_sources,   f"{BUILD_DIR}/%.o")
    asm_objects = derive_files(asm_sources, f"{BUILD_DIR}/%.o")
    all_objects = c_objects + asm_objects

    c_depfiles   = derive_files(c_sources,   f"{BUILD_DIR}/%.d")
    asm_depfiles = derive_files(asm_sources, f"{BUILD_DIR}/%.d")

    build_c_project(
        "tools/gbagfx",
        output=GBAGFX,
        cflags="-Wall -Wextra -Werror -Wno-sign-compare -std=c11 -O3 -flto -DPNG_SKIP_SETJMP_CHECK `pkg-config --cflags libpng`",
        ldflags="`pkg-config --libs libpng`",
    )

    build_cxx_project(
        "tools/mid2agb",
        output=MID2AGB,
        cxxflags="-std=c++11 -O2 -Wall -Wno-switch -Werror",
    )

    build_cxx_project(
        "tools/preproc",
        output=PREPROC,
        cxxflags="-std=c++11 -O2 -Wall -Wno-switch -Werror",
    )

    build_cxx_project(
        "tools/scaninc",
        output=SCANINC,
        cxxflags="-Wall -Werror -std=c++11 -O2",
    )

    build_cxx_project(
        "tools/wav2agb",
        output=WAV2AGB,
        cxxflags="-Wall -Werror -std=c++17 -O2",
    )

    build_cxx_project(
        "patch",
        output=PATCHBIN,
        cxxflags=f"-Wall -Wextra -std=c++17 -O2 -I {INC_DIR}",
    )

    with open("build.ninja", "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        writer.rule(
            "target_cc",
            command="$CC -E $CPPFLAGS $in | $PREPROC -i $in charmap.txt | $CC $CFLAGS -xc -c - -o $out",
            depfile="$DEPFILE",
            description="Building C object $out",
        )

        writer.rule(
            "target_as",
            command="$PREPROC $in charmap.txt | $CC -E $CPPFLAGS - | $PREPROC -ie $in charmap.txt | $AS $ASFLAGS -o $out",
            depfile="$DEPFILE",
            description="Building ASM object $out",
        )

        writer.rule(
            "host_cc",
            command="$CC -MMD -MF $DEPFILE -MT $out $CFLAGS -c $in -o $out",
            depfile="$DEPFILE",
            description="Building C object $out",
        )

        writer.rule(
            "host_cxx",
            command=f"$CXX -MMD -MF $DEPFILE -MT $out $CXXFLAGS -c $in -o $out",
            depfile="$DEPFILE",
            description="Building C++ object $out",
        )

        writer.rule(
            "ld",
            command="$LD $LDFLAGS $in -o $out",
            description="Linking ELF executable $out",
        )

        writer.rule(
            "gbagfx",
            command="$GBAGFX $in $out",
            description="Building graphics $out",
        )

        writer.rule(
            "patchbin",
            command="$PATCHBIN $in $out",
            description="Patching $out",
        )

        writer.rule(
            "scaninc",
            command=f"$SCANINC -M $out $INCLUDES $in",
            description="Building depfile $out",
        )

        writer.subninja(f"{TOOLS_DIR}/gbagfx/build.ninja")
        writer.subninja(f"{TOOLS_DIR}/mid2agb/build.ninja")
        writer.subninja(f"{PATCH_DIR}/build.ninja")
        writer.subninja(f"{TOOLS_DIR}/preproc/build.ninja")
        writer.subninja(f"{TOOLS_DIR}/scaninc/build.ninja")
        writer.subninja(f"{TOOLS_DIR}/wav2agb/build.ninja")

        writer.newline()

        build_gfx(writer, png_files, bpp1_files)
        build_gfx(writer, png_files, bpp4_files)
        build_gfx(writer, png_files, bpp8_files)

        build_gfx(writer, bpp1_files, bpp1_lz_files)
        build_gfx(writer, bpp4_files, bpp4_lz_files)
        build_gfx(writer, bpp8_files, bpp8_lz_files)

        build_c(writer, c_sources, c_objects, c_depfiles)
        build_asm(writer, asm_sources, asm_objects, asm_depfiles)

        build_depfile(writer, c_sources, c_depfiles)
        build_depfile(writer, asm_sources, asm_depfiles)

        if all_objects:
            writer.build(
                "ld",
                inputs=all_objects,
                outputs=BLOB_OBJECT,
                variables={
                    "LD": TARGET_LD,
                    "LDFLAGS": LDFLAGS,
                },
            )

            writer.build(
                "patchbin",
                inputs=[BASE_ROM_FILE, BLOB_OBJECT],
                outputs=OUT_ROM_FILE,
                implicit_inputs=PATCHBIN,
                variables={"PATCHBIN": PATCHBIN},
            )

if __name__ == "__main__": main()
