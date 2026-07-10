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
SRC_DIR = "src"
TOOLS_DIR = "tools"

# Required for MinGW, MSYS2 and Cygwin
EXE_SUFFIX = ".exe" if os.getenv("OS") == "Windows_NT" else ""

DEVKITARM = os.getenv("DEVKITARM")
if not DEVKITARM:
    print("Error: DEVKITARM enviroment variable is not set")
    sys.exit(1)

ROM_BEGIN = 0x8000000
ROM_END = 0x9FFFFFF

ADDRESS_TO_INSERT = OFFSET_TO_INSERT + 0x8000000
if ADDRESS_TO_INSERT < ROM_BEGIN or ADDRESS_TO_INSERT > ROM_END:
    print("Error: offset to insert must be between 0x0 - 0x1FFFFFF")
    sys.exit(1)

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

def build_tools():
    if not os.path.exists(TOOLS_DIR):
        print("Error: tools directory not found")
        sys.exit(1)
    if not os.path.isdir(TOOLS_DIR):
        print("Error: tools is not a directory")
        sys.exit(1)

    BIN_DIR = os.path.join(BUILD_DIR, TOOLS_DIR)
    os.makedirs(BIN_DIR, exist_ok=True)

    for name in os.listdir(TOOLS_DIR):
        src_dir = os.path.join(TOOLS_DIR, name)
        if not os.path.isdir(src_dir):
            continue

        result = subprocess.run(["ninja", "-C", src_dir])
        if result.returncode != 0:
            sys.exit(1)

        exe_name = name + EXE_SUFFIX
        src_exe = os.path.join(src_dir, exe_name)
        dest_exe = os.path.join(BIN_DIR, exe_name)
        shutil.copy2(src_exe, dest_exe)

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
    BLOB_OBJ = os.path.join(BUILD_DIR, "blob.o")

    os.makedirs(BUILD_DIR, exist_ok=True)

    devkitarm_symlink = os.path.join(BUILD_DIR, "devkitarm")
    if not os.path.exists(devkitarm_symlink):
        os.symlink(DEVKITARM, os.path.join(BUILD_DIR, "devkitarm"), target_is_directory=True)

    build_tools()

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

    with open("build.ninja", "w", encoding="utf-8") as stream:
        writer = Writer(stream)

        writer.variable("cflags", "-mthumb -mthumb-interwork -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu -mlong-calls -O2 -fno-toplevel-reorder")
        writer.variable("ldflags", f"-T linker.ld BPRE.ld --defsym=BLOB_BEGIN=0x{ADDRESS_TO_INSERT:08X}")
        writer.newline()

        writer.rule("gfx", command="gbagfx $in $out")
        writer.rule("cc", command=f"arm-none-eabi-gcc -E -I{INC_DIR} -MMD -MF $out.d -MT $out $in | preproc -i $in charmap.txt | arm-none-eabi-gcc $cflags -xc -o $out -c -", depfile="$out.d")
        writer.rule("asm", command=f"arm-none-eabi-gcc $cflags -I{ASM_DIR} -o $out -c $in")
        writer.rule("link", command="arm-none-eabi-ld $ldflags -o $out $in")
        writer.rule("insert", command="arm-none-eabi-objcopy -O binary $obj_file $obj_file.bin && patchbin $base_rom $out $obj_file.bin $offset")

        writer.build_list("gfx", png_files, bpp1_files)
        writer.build_list("gfx", png_files, bpp4_files)
        writer.build_list("gfx", png_files, bpp8_files)

        writer.build_list("gfx", bpp1_files, bpp1_lz_files)
        writer.build_list("gfx", bpp4_files, bpp4_lz_files)
        writer.build_list("gfx", bpp8_files, bpp8_lz_files)

        writer.build_list("cc", c_sources, c_objects)
        writer.build_list("asm", asm_sources, asm_objects)

        if all_objects:
            writer.build("link", all_objects, BLOB_OBJ)
            writer.build("insert", [BASE_ROM_FILE, BLOB_OBJ], OUT_ROM_FILE,
                     base_rom=BASE_ROM_FILE,
                     obj_file=BLOB_OBJ,
                     offset=f"{OFFSET_TO_INSERT:06X}")

if __name__ == "__main__": main()
