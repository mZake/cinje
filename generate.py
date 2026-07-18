#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
import sys

from io import BufferedWriter

# Change this if needed
OFFSET_TO_INSERT = 0x1400000
BASE_ROM_FILE = "base.gba"
OUT_ROM_FILE = "out.gba"

ASM_DIR = "asm"
BUILD_DIR = "build"
GFX_DIR = "graphics"
INC_DIR = "include"
PATCH_DIR = "patch"
SRC_DIR = "src"
TOOLS_DIR = "tools"

BLOB_OBJECT = f"{BUILD_DIR}/blob.o"

GBAGFX   = f"{TOOLS_DIR}/gbagfx/gbagfx"
MID2AGB  = f"{TOOLS_DIR}/mid2agb/mid2agb"
PATCHBIN = f"{PATCH_DIR}/patchbin"
PREPROC  = f"{TOOLS_DIR}/preproc/preproc"
WAV2AGB  = f"{TOOLS_DIR}/wav2agb/wav2agb"

ADDRESS_TO_INSERT = OFFSET_TO_INSERT + 0x8000000

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

    def build(self, rule: str, inputs: str | list[str], outputs: str | list[str], **kwargs):
        inputs_text = " ".join(as_list(inputs))
        outputs_text = " ".join(as_list(outputs))

        self._line(f"build {outputs_text}: {rule} {inputs_text}")
        for key, value in kwargs.items():
            self._line(f"{key} = {value}", indent=2)

    def build_list(self, rule: str, inputs: list[str], outputs: list[str], **kwargs):
        if not (inputs and outputs):
            return
        for input, output in zip(inputs, outputs):
            self.build(rule, input, output, **kwargs)
        self.newline()

    def _line(self, text: str, indent: int = 0):
        stripped = text.lstrip()
        length = len(stripped) + indent
        self.stream.write(f"{stripped:>{length}}\n")

def as_list(input: str | list[str] | None) -> list[str]:
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

def configure_directories():
    os.makedirs(f"{BUILD_DIR}/{TOOLS_DIR}", exist_ok=True)
    os.makedirs(f"{BUILD_DIR}/{SRC_DIR}", exist_ok=True)
    os.makedirs(f"{BUILD_DIR}/{ASM_DIR}", exist_ok=True)

def configure_toolchain():
    toolchain_path = os.getenv("DEVKITARM")
    if not toolchain_path:
        fatal("DEVKITARM environment is not set")
    if not os.path.isdir(toolchain_path):
        fatal(f"devkitARM directory not found: {toolchain_path}")

    symlink_path = f"{BUILD_DIR}/toolchain"
    if os.path.exists(symlink_path):
        if not os.path.islink(symlink_path):
            fatal(f"{symlink_path} is not a symlink")
        os.unlink(symlink_path)

    os.symlink(toolchain_path, symlink_path, target_is_directory=True)

def collect_files(directory: str, extension: str):
    return glob.glob(f"{directory}/**/*{extension}", recursive=True)

def derive_files(inputs: str | list[str], pattern: str) -> list[str]:
    outputs = list()
    for input in as_list(inputs):
        stem = os.path.splitext(input)[0]
        output = pattern.replace("%", stem)
        outputs.append(output)

    return outputs

def main():
    configure_directories()
    configure_toolchain()
    # configure_tools()

    # Inputs
    png_files = collect_files(GFX_DIR, ".png")
    c_sources = collect_files(SRC_DIR, ".c")
    asm_sources = collect_files(ASM_DIR, ".s")

    # Outputs
    bpp1_files = derive_files(png_files, "%.1bpp")
    bpp4_files = derive_files(png_files, "%.4bpp")
    bpp8_files = derive_files(png_files, "%.8bpp")

    bpp1_lz_files = derive_files(png_files, "%.1bpp.lz")
    bpp4_lz_files = derive_files(png_files, "%.4bpp.lz")
    bpp8_lz_files = derive_files(png_files, "%.8bpp.lz")

    c_objects = derive_files(c_sources, f"{BUILD_DIR}/%.o")
    asm_objects = derive_files(asm_sources, f"{BUILD_DIR}/%.o")
    all_objects = c_objects + asm_objects

    # Patches are compiled for the host machine. They are getting placed
    # here to avoid ambiguity.
    patch_sources = collect_files(PATCH_DIR, ".cpp")
    patch_objects = derive_files(patch_sources, f"{BUILD_DIR}/%.o")

    with open("build.ninja", "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        writer.pool("ninja_pool", depth=1)

        writer.rule("ninja",
                    command="ninja -C $in",
                    pool="ninja_pool")

        writer.variable("host_cxx", "g++")
        writer.newline()

        writer.variable("cxxflags", f"-std=c++17 -O2 -Wall -Wextra -I{INC_DIR}")
        writer.newline()

        writer.rule("host_cxx", command="$host_cxx $cxxflags -MMD -MF $out.d -MT $out -o $out -c $in", depfile="$out.d")
        writer.rule("host_ld", command="$host_cxx -o $out $in")

        writer.build_list("host_cxx", patch_sources, patch_objects)

        if patch_objects:
            writer.build("host_ld", patch_objects, PATCHBIN)
            writer.newline()

        writer.variable("cc", "arm-none-eabi-gcc")
        writer.variable("ld", "arm-none-eabi-ld")
        writer.newline()
        writer.variable("gbagfx",   GBAGFX)
        writer.variable("mid2agb",  MID2AGB)
        writer.variable("patchbin", PATCHBIN)
        writer.variable("preproc",  PREPROC)
        writer.variable("wav2agb",  WAV2AGB)
        writer.newline()
        writer.variable("cflags", "-mthumb -mthumb-interwork -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu -mlong-calls -O2 -fno-toplevel-reorder")
        writer.variable("ldflags", f"-T linker.ld BPRE.ld -r --defsym=BLOB_BEGIN=0x{ADDRESS_TO_INSERT:08X}")
        writer.newline()

        writer.rule("cc", command=f"$cc -E -I{INC_DIR} -MMD -MF $out.d -MT $out $in | $preproc -i $in charmap.txt | $cc $cflags -xc -o $out -c -", depfile="$out.d")
        writer.rule("as", command=f"$cc $cflags -I{ASM_DIR} -o $out -c $in")
        writer.rule("ld", command="$ld $ldflags -o $out $in")
        writer.rule("gbagfx", command="$gbagfx $in $out")
        writer.rule("patchbin", command=f"$patchbin $in $out")

        writer.build("ninja", f"{TOOLS_DIR}/gbagfx",  "$gbagfx")
        writer.build("ninja", f"{TOOLS_DIR}/mid2agb", "$mid2agb")
        writer.build("ninja", f"{TOOLS_DIR}/preproc", "$preproc")
        writer.build("ninja", f"{TOOLS_DIR}/wav2agb", "$wav2agb")
        writer.newline()

        writer.build_list("gbagfx", png_files, bpp1_files)
        writer.build_list("gbagfx", png_files, bpp4_files)
        writer.build_list("gbagfx", png_files, bpp8_files)

        writer.build_list("gbagfx", bpp1_files, bpp1_lz_files)
        writer.build_list("gbagfx", bpp4_files, bpp4_lz_files)
        writer.build_list("gbagfx", bpp8_files, bpp8_lz_files)

        writer.build_list("cc", c_sources, c_objects)
        writer.build_list("as", asm_sources, asm_objects)

        if all_objects:
            writer.build("ld", all_objects, BLOB_OBJECT)
            writer.build("patchbin", [BASE_ROM_FILE, BLOB_OBJECT, "|", PATCHBIN], OUT_ROM_FILE)

if __name__ == "__main__": main()
