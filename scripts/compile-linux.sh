#!/bin/bash
echo "--- Starting Linux Compilation (Docker) ---"
haxe -cp src -cp test -main Main -cpp bin/linux -debug
