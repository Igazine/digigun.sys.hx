#!/bin/bash
set -e

# Register the local library in the current environment
haxelib dev digigun.sys.hx .

echo "--- Starting Linux Compilation (Docker) ---"
haxe -cp src -cp test -main Main -L digigun.sys.hx -cpp bin/linux --debug

echo "--- Running Linux Tests ---"
./bin/linux/Main-debug
