#!/usr/bin/env python3

# This script is automatically called by Ninja

import os
import shutil
import sys

from generate import offset_to_insert

base_rom_file = "base.gba"
out_rom_file = "out.gba"

if not os.path.exists(base_rom_file):
    print(f"Error: {base_rom_file} not found")
    sys.exit(1)
if not os.path.isfile(base_rom_file):
    print(f"Error: {base_rom_file} is not a file")
    sys.exit(1)

shutil.copy2(base_rom_file, out_rom_file)

with open(out_rom_file, "r+b") as rom_stream:
    with open("build/linked.o.bin", "rb") as blob_stream:
        blob = blob_stream.read()
        rom_stream.seek(offset_to_insert)
        rom_stream.write(blob)
