#!/usr/bin/env python3

import glob
import os
import shutil
import subprocess
import sys

# Change this if needed
offset_to_insert = 0x1400000

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

    def write_build(self, rule: str, file_out: str, file_in: str, **kwargs):
        self.stream.write(f"build {file_out}: {rule} {file_in}\n")
        for key, value in kwargs.items():
            self.stream.write(f"  {key} = {value}\n")

    def write_build_list(self, rule: str, files_out: list, files_in: list, **kwargs):
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

    tools_src_dir = "tools"
    tools_build_dir = "build/tools"

    if not os.path.exists(tools_src_dir):
        print("Error: tools directory not found")
        sys.exit(1)
    if not os.path.isdir(tools_src_dir):
        print("Error: tools is not a directory")
        sys.exit(1)

    os.makedirs(tools_build_dir, exist_ok=True)

    for name in os.listdir(tools_src_dir):
        src_dir = os.path.join(tools_src_dir, name)
        if not os.path.isdir(src_dir):
            continue

        result = subprocess.run(["make", "-C", src_dir])
        if result.returncode != 0:
            sys.exit(1)

        exe_name = name + exe_suffix
        src_exe = os.path.join(src_dir, exe_name)
        dest_exe = os.path.join(tools_build_dir, exe_name)
        shutil.copy2(src_exe, dest_exe)

address_to_insert = 0x8000000 + offset_to_insert

build_tools()

with Generator("build.ninja") as gen:
    gen.write_var("preproc", "build/tools/preproc")
    gen.write_var("gbagfx", "build/tools/gbagfx")
    gen.write_var("gcc", "arm-none-eabi-gcc")
    gen.write_var("ld", "arm-none-eabi-ld")
    gen.write_var("objcopy", "arm-none-eabi-objcopy")
    gen.write_var("python", sys.executable)
    gen.break_line()

    gen.write_var("cflags", "-mthumb -mthumb-interwork -march=armv4t -mtune=arm7tdmi -mabi=apcs-gnu -mlong-calls -O2 -fno-toplevel-reorder")
    gen.write_var("ldflags", f"-T linker.ld BPRE.ld --defsym=BLOB_BEGIN=0x{address_to_insert:08X}")
    gen.break_line()

    gen.write_rule("gfx", command="$gbagfx $in $out")
    gen.break_line()

    gen.write_rule("cc", command="$gcc -Iinclude -E $in | $preproc -i $in charmap.txt | $gcc $cflags -MD -MF $out.d -xc -c -o $out -", depfile="$out.d")
    gen.break_line()

    gen.write_rule("asm", command="$gcc $cflags -Iasm -c -o $out $in")
    gen.break_line()

    gen.write_rule("link", command="$ld $ldflags -o $out $in && $objcopy -O binary $out $out.bin && $python insert.py")
    gen.break_line()

    png_files = glob.glob("graphics/**/*.png", recursive=True)
    for png_file in png_files:
        out_1bpp_file = os.path.splitext(png_file)[0] + ".1bpp"
        out_1bpp_lz_file = out_1bpp_file + ".lz"
        out_4bpp_file = os.path.splitext(png_file)[0] + ".4bpp"
        out_4bpp_lz_file = out_4bpp_file + ".lz"
        out_8bpp_file = os.path.splitext(png_file)[0] + ".8bpp"
        out_8bpp_lz_file = out_8bpp_file + ".lz"
        out_gbapal_file = os.path.splitext(png_file)[0] + ".gbapal"

        gen.write_build("gfx", out_1bpp_file, png_file)
        gen.write_build("gfx", out_4bpp_file, png_file)
        gen.write_build("gfx", out_8bpp_file, png_file)
        gen.write_build("gfx", out_gbapal_file, png_file)
        gen.write_build("gfx", out_1bpp_lz_file, out_1bpp_file)
        gen.write_build("gfx", out_4bpp_lz_file, out_4bpp_file)
        gen.write_build("gfx", out_8bpp_lz_file, out_8bpp_file)
    if len(png_files) > 0: gen.break_line()

    bin_files = glob.glob("graphics/**/*.bin", recursive=True)
    for bin_file in bin_files:
        out_lz_file = bin_file + ".lz"
        gen.write_build("gfx", out_lz_file, bin_file)
    if len(bin_files) > 0: gen.break_line()

    c_obj_files = []
    c_files = glob.glob("src/**/*.c", recursive=True)
    for c_file in c_files:
        obj_file = os.path.join("build", c_file) + ".o"
        gen.write_build("cc", obj_file, c_file)
        c_obj_files.append(obj_file)
    if len(c_files): gen.break_line()

    asm_obj_files = []
    asm_files = glob.glob("asm/**/*.s", recursive=True)
    for asm_file in asm_files:
        out_file = os.path.join("build", asm_file) + ".o"
        gen.write_build("asm", out_file, asm_file)
        asm_obj_files.append(out_file)
    if len(asm_files) > 0: gen.break_line()

    obj_files = c_obj_files + asm_obj_files
    gen.write_build_list("link", ["build/linked.o"], obj_files)
