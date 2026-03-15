#!/bin/bash

# Exit on error
set -euo pipefail

# Check if GEMINI_API_KEY is set
if [ -z "$GEMINI_API_KEY" ]; then
    echo "Error: GEMINI_API_KEY environment variable is not set."
    echo "Please set it before running this script:"
    echo "  export GEMINI_API_KEY='your_api_key_here'"
    exit 1
fi

IMAGE_NAME="value-types-sandbox"
DOCKERFILE="docker/Dockerfile"

# Build the image
echo "--- Building Docker Sandbox: $IMAGE_NAME ---"
docker build -t "$IMAGE_NAME" -f "$DOCKERFILE" .

# Run the container
echo "--- Starting Sandboxed Gemini Session ---"
echo "Note: Your current directory $(pwd) is mounted to /workspace"

docker run -it --rm \
    -v "$(pwd):/workspace" \
    -e GEMINI_API_KEY="$GEMINI_API_KEY" \
    "$IMAGE_NAME" \
    gemini
    # -e TERM=xterm-256color \
    # -e COLORTERM=truecolor \
