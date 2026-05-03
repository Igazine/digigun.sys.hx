#!/bin/bash
set -e

# Register the local library in the current environment
haxelib dev digigun.sys.hx .

echo "--- Starting Linux Compilation (Docker) ---"
haxe --cwd test build-linux.hxml

echo "--- Running Linux Tests ---"
./bin/linux/Main-debug
