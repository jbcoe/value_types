#!/bin/bash

# This script is written to be run by a docker image and is not intended to be run directly.

INPUT_FILE=$INPUT_FILE
if [ -z "$INPUT_FILE" ]; then
    echo "No file specified. Please set INPUT_FILE environment variable."
    exit 1
fi

if [ ! -f "/app/workdir/$INPUT_FILE" ]; then
    echo "File $INPUT_FILE does not exist."
    exit 1
fi

OUTPUT_FILE="${INPUT_FILE%.md}.pdf"

cp /app/workdir/$INPUT_FILE /app/wg21/
pushd /app/wg21
make "$OUTPUT_FILE"
cp /app/wg21/generated/"$OUTPUT_FILE" /app/workdir/
popd
