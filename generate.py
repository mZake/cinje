#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
import sys

# Change this if needed
offset_to_insert = 0x1400000
base_rom_file = "base.gba"
out_rom_file = "out.gba"

ASM_DIR = "asm"
BUILD_DIR = "build"
GFX_DIR = "graphics"
INC_DIR = "include"
SRC_DIR = "src"
TOOLS_DIR = "tools"

class Generator:
    def __init__(self, file_path: str):
        self.file_path = file_path
        self.stream = None

    def __enter__(self):
        self.stream = open(self.file_path, "w", encoding="utf-8")
        return self

    def __exit__(self, exc_type, exc, tb):
        if self.stream is not None:
            self.stream.close()
        return False

    def write_var(self, name: str, value: str):
        self.stream.write(f"{name} = {value}\n")

    def write_rule(self, name: str, **kwargs):
        self.stream.write(f"rule {name}\n")
        for key, value in kwargs.items():
            self.stream.write(f"  {key} = {value}\n")
        self.stream.write("\n")

    def write_build(self, rule: str, files_out: str | list, files_in: str | list, **kwargs):
        if isinstance(files_in, str): files_in = [files_in]
        if isinstance(files_out, str): files_out = [files_out]

        self.stream.write("build")
        for file_out in files_out:
            self.stream.write(f" {file_out}")

        self.stream.write(f": {rule}")
        for file_in in files_in:
            self.stream.write(f" {file_in}")

        self.stream.write("\n")
        for key, value in kwargs.items():
            self.stream.write(f"  {key} = {value}\n")

    def break_line(self):
        self.stream.write("\n")

def build_tools():
    # Required for MinGW, MSYS2 and Cygwin
    exe_suffix = ".exe" if os.getenv("OS") == "Windows_NT" else ""

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

        result = subprocess.run(["make", "-C", src_dir])
        if result.returncode != 0:
            sys.exit(1)

        exe_name = name + exe_suffix
        src_exe = os.path.join(src_dir, exe_name)
        dest_exe = os.path.join(BIN_DIR, exe_name)
        shutil.copy2(src_exe, dest_exe)

def change_file_ext(file: str, ext: str) -> str:
    return os.path.splitext(file)[0] + ext

def collect_gfx_files():
    build_jobs = []

    files_png = glob.glob(f"{GFX_DIR}/**/*.png", recursive=True)
    files_png.sort()
    for file_png in files_png:
        file_1bpp = change_file_ext(file_png, ".1bpp")
        file_4bpp = change_file_ext(file_png, ".4bpp")
        file_8bpp = change_file_ext(file_png, ".8bpp")
        file_gbapal = change_file_ext(file_png, ".gbapal")
        file_1bpp_lz = change_file_ext(file_png, ".1bpp.lz")
        file_4bpp_lz = change_file_ext(file_png, ".4bpp.lz")
        file_8bpp_lz = change_file_ext(file_png, ".8bpp.lz")

        build_jobs.append((file_png, file_1bpp))
        build_jobs.append((file_png, file_4bpp))
        build_jobs.append((file_png, file_8bpp))
        build_jobs.append((file_png, file_gbapal))
        build_jobs.append((file_1bpp, file_1bpp_lz))
        build_jobs.append((file_4bpp, file_4bpp_lz))
        build_jobs.append((file_8bpp, file_8bpp_lz))

    files_bin = glob.glob(f"{GFX_DIR}/**/*.bin", recursive=True)
    files_bin.sort()
    for file_bin in files_bin:
        file_bin_lz = change_file_ext(file_bin, ".bin.lz")
        build_jobs.append((file_bin, file_bin_lz))

    return build_jobs

def collect_c_files() -> tuple:
    files_out = []

    files_in = glob.glob(f"{SRC_DIR}/**/*.c", recursive=True)
    files_in.sort()
    for file_in in files_in:
        file_out = change_file_ext(file_in, ".o")
        file_out = os.path.join(BUILD_DIR, file_out)
        files_out.append(file_out)

    return (files_in, files_out)

def collect_asm_files() -> tuple:
    files_out = []

    files_in = glob.glob(f"{ASM_DIR}/**/*.s", recursive=True)
    files_in.sort()
    for file_in in files_in:
        file_out = change_file_ext(file_in, ".o")
        file_out = os.path.join(BUILD_DIR, file_out)
        files_out.append(file_out)

    return (files_in, files_out)

address_to_insert = 0x8000000 + offset_to_insert

build_tools()

with Generator("build.ninja") as gen:
    gen.write_var("preproc", os.path.join(BUILD_DIR, TOOLS_DIR, "preproc"))
    gen.write_var("gbagfx", os.path.join(BUILD_DIR, TOOLS_DIR, "gbagfx"))
    gen.write_var("gcc", "arm-none-eabi-gcc")
    gen.write_var("ld", "arm-none-eabi-ld")
    gen.write_var("objcopy", "arm-none-eabi-objcopy")
    gen.write_var("python", sys.executable)
    gen.break_line()

    gen.write_var("cflags", "-mthumb -mthumb-interwork -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu -mlong-calls -O2 -fno-toplevel-reorder")
    gen.write_var("ldflags", f"-T linker.ld BPRE.ld --defsym=BLOB_BEGIN=0x{address_to_insert:08X}")
    gen.break_line()

    gen.write_rule("gfx", command="$gbagfx $in $out")
    gen.write_rule("cc", command=f"$gcc -E -I{INC_DIR} -MMD -MF $out.d -MT $out $in | $preproc -i $in charmap.txt | $gcc $cflags -xc -o $out -c -", depfile="$out.d")
    gen.write_rule("asm", command=f"$gcc $cflags -I{ASM_DIR} -o $out -c $in")
    gen.write_rule("link", command="$ld $ldflags -o $out $in")
    gen.write_rule("insert", command="$objcopy -O binary $blob_obj $blob_obj.bin && $python insert.py")

    gfx_jobs = collect_gfx_files()
    for file_in, file_out in gfx_jobs:
        gen.write_build("gfx", file_out, file_in)

    if gfx_jobs: gen.break_line()
    c_srcs, c_objs = collect_c_files()
    for c_src, c_obj in zip(c_srcs, c_objs):
        gen.write_build("cc", c_obj, c_src)

    if c_srcs: gen.break_line()
    asm_srcs, asm_objs = collect_asm_files()
    for asm_src, asm_obj in zip(asm_srcs, asm_objs):
        gen.write_build("asm", asm_obj, asm_src)

    if asm_srcs: gen.break_line()
    all_objs = c_objs + asm_objs
    gen.write_build("link", os.path.join(BUILD_DIR, "blob.o"), all_objs)
