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

BUILD_DIR = "build"
GFX_DIR = "graphics"
INC_DIR = "include"
PATCH_DIR = "patch"
TOOLS_DIR = "tools"

C_SRC_DIR = "src"
ASM_SRC_DIR = "asm"

C_BUILD_DIR = f"{BUILD_DIR}/{C_SRC_DIR}"
ASM_BUILD_DIR = f"{BUILD_DIR}/{ASM_SRC_DIR}"

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

    def build(
        self,
        rule: str,
        inputs: str | list[str],
        outputs: str | list[str],
        implicit_inputs: str | list[str] | None = None,
        implicit_outputs: str | list[str] | None = None,
        order_only: str | list[str] | None = None,
        **kwargs,
    ):
        all_inputs = as_list(inputs)
        if implicit_inputs is not None:
            all_inputs.append("|")
            all_inputs.extend(as_list(implicit_inputs))
        if order_only is not None:
            all_inputs.append("||")
            all_inputs.extend(as_list(order_only))

        all_outputs = as_list(outputs)
        if implicit_outputs is not None:
            all_outputs.append("|")
            all_outputs.extend(as_list(implicit_outputs))

        inputs_text = " ".join(as_list(all_inputs))
        outputs_text = " ".join(as_list(all_outputs))

        self._line(f"build {outputs_text}: {rule} {inputs_text}")
        for key, value in kwargs.items():
            self._line(f"{key} = {value}", indent=2)

    def build_group(
        self,
        rule: str,
        inputs: list[str],
        outputs: list[str],
        implicit_inputs: str | list[str] | None = None,
        implicit_outputs: str | list[str] | None = None,
        order_only: str | list[str] | None = None,
        **kwargs,
    ):
        if not (inputs and outputs):
            return
        for input, output in zip(inputs, outputs):
            self.build(rule, input, output, implicit_inputs, implicit_outputs, order_only, **kwargs)
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
    os.makedirs(C_BUILD_DIR, exist_ok=True)
    os.makedirs(ASM_BUILD_DIR, exist_ok=True)

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

def collect_files(directory: str, extension: str) -> list[str]:
    matches = glob.glob(f"{directory}/**/*{extension}", recursive=True)
    return [os.path.relpath(x, directory) for x in matches]

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

    png_inputs = collect_files(GFX_DIR, ".png")
    c_inputs = collect_files(C_SRC_DIR, ".c")
    asm_inputs = collect_files(ASM_SRC_DIR, ".s")

    # Inputs
    png_files = derive_files(png_inputs, f"{GFX_DIR}/%.png")
    c_sources = derive_files(c_inputs, f"{C_SRC_DIR}/%.c")
    asm_sources = derive_files(asm_inputs, f"{ASM_SRC_DIR}/%.s")

    # Outputs
    bpp1_files = derive_files(png_inputs, f"{GFX_DIR}/%.1bpp")
    bpp4_files = derive_files(png_inputs, f"{GFX_DIR}/%.4bpp")
    bpp8_files = derive_files(png_inputs, f"{GFX_DIR}/%.8bpp")

    bpp1_lz_files = derive_files(png_inputs, f"{GFX_DIR}/%.1bpp.lz")
    bpp4_lz_files = derive_files(png_inputs, f"{GFX_DIR}/%.4bpp.lz")
    bpp8_lz_files = derive_files(png_inputs, f"{GFX_DIR}/%.8bpp.lz")

    c_objects = derive_files(c_inputs, f"{C_BUILD_DIR}/%.o")
    asm_objects = derive_files(asm_inputs, f"{ASM_BUILD_DIR}/%.o")
    all_objects = c_objects + asm_objects

    # Patches are compiled for the host machine. They are getting placed
    # here to avoid ambiguity.    
    patch_sources = collect_files(PATCH_DIR, ".cpp")
    patch_headers = collect_files(PATCH_DIR, ".hpp")

    with open(f"{PATCH_DIR}/build.ninja", "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        writer.variable("cxx", "g++")
        writer.newline()
        writer.variable("cxxflags", f"-std=c++17 -O2 -Wall -Wextra -I{INC_DIR}")
        writer.newline()

        writer.rule("cxx", command="$cxx $cxxflags $in -o $out")

        writer.build("cxx",
                     inputs=patch_sources,
                     implicit_inputs=patch_headers,
                     outputs="patchbin")

    with open("build.ninja", "w", encoding="utf-8") as stream:
        writer = Writer(stream)

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

        writer.pool("ninja_pool", depth=1)

        writer.rule("ninja",
                    command="ninja -C $in",
                    pool="ninja_pool")

        writer.rule("cc", command=f"$cc -E -I{INC_DIR} -MMD -MF $out.d -MT $out $in | $preproc -i $in charmap.txt | $cc $cflags -xc -o $out -c -", depfile="$out.d")
        writer.rule("as", command=f"$cc $cflags -I{ASM_SRC_DIR} -o $out -c $in")
        writer.rule("ld", command="$ld $ldflags -o $out $in")
        writer.rule("gbagfx", command="$gbagfx $in $out")
        writer.rule("patchbin", command=f"$patchbin $in $out")

        writer.build("ninja", f"{TOOLS_DIR}/gbagfx",  "$gbagfx")
        writer.build("ninja", f"{TOOLS_DIR}/mid2agb", "$mid2agb")
        writer.build("ninja", PATCH_DIR,              "$patchbin")
        writer.build("ninja", f"{TOOLS_DIR}/preproc", "$preproc")
        writer.build("ninja", f"{TOOLS_DIR}/wav2agb", "$wav2agb")
        writer.newline()

        writer.build_group("gbagfx", png_files, bpp1_files, order_only="$gbagfx")
        writer.build_group("gbagfx", png_files, bpp4_files, order_only="$gbagfx")
        writer.build_group("gbagfx", png_files, bpp8_files, order_only="$gbagfx")

        writer.build_group("gbagfx", bpp1_files, bpp1_lz_files, order_only="$gbagfx")
        writer.build_group("gbagfx", bpp4_files, bpp4_lz_files, order_only="$gbagfx")
        writer.build_group("gbagfx", bpp8_files, bpp8_lz_files, order_only="$gbagfx")

        writer.build_group("cc", c_sources, c_objects, order_only="$preproc")
        writer.build_group("as", asm_sources, asm_objects)

        if all_objects:
            writer.build("ld", all_objects, BLOB_OBJECT)
            writer.build("patchbin", [BASE_ROM_FILE, BLOB_OBJECT], OUT_ROM_FILE, order_only="$patchbin")

if __name__ == "__main__": main()
