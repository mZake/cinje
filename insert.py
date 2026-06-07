#!/usr/bin/env python3

# This script is automatically called by Ninja

import os
import shutil
import sys

from generate import OFFSET_TO_INSERT, BASE_ROM_FILE, OUT_ROM_FILE

if not os.path.exists(BASE_ROM_FILE):
    print(f"Error: {BASE_ROM_FILE} not found")
    sys.exit(1)
if not os.path.isfile(BASE_ROM_FILE):
    print(f"Error: {BASE_ROM_FILE} is not a file")
    sys.exit(1)

shutil.copy2(BASE_ROM_FILE, OUT_ROM_FILE)

with open(OUT_ROM_FILE, "r+b") as rom_stream:
    with open("build/blob.o.bin", "rb") as blob_stream:
        blob = blob_stream.read()
        rom_stream.seek(OFFSET_TO_INSERT)
        rom_stream.write(blob)
