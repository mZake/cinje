#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys

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
