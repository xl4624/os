#!/bin/bash
# Generate compile_commands.json for clangd

set -e

echo "Generating compile_commands.json..."

make clean
bear -- make
bear --append -- make test
bear --append -- make ktest

echo "Created compile_commands.json with $(grep -c '"file"' compile_commands.json) entries"
