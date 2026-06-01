#!/bin/sh

root_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$root_dir/.build"

mkdir -p "$build_dir"
mkdir -p "$build_dir/tools"

echo "Root directory: $root_dir"
echo "Build directory: $build_dir"

git clone --quiet --depth=1 https://github.com/pret/pokefirered.git "$build_dir/tools-src"

for tool_dir in "$build_dir"/tools-src/tools/*/; do
    bin_name="$(basename "$tool_dir")"

    echo "Building $bin_name..."

    if make -C "$tool_dir" > /dev/null; then
        cp "$tool_dir/$bin_name" "$build_dir/tools"
    fi
done

rm -rf "$build_dir/tools-src"

