#!/bin/bash
# Generate compile_commands.json for clangd

set -ex

echo "Generating compile_commands.json..."

make clean
bear -- make
bear --append -- make test

echo "Created compile_commands.json with $(grep -c '"file"' compile_commands.json) entries"
