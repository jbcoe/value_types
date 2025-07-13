#!/bin/bash

set -eu

# Build the Docker image from the repository root with:
#
#```
# docker build -t parkdown markdown/ -f markdown/Dockerfile
#```

docker run --mount type=bind,source="$(pwd)",target=/app/workdir \
    -e INPUT_FILE=$1 -it parkdown \
