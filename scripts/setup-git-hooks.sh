#!/bin/sh

set -eux -o pipefail

# Create a Python virtual env
python3 -m venv .venv

# Activate the virtual env for bash by source.
source ./.venv/bin/activate

# Install latest requirements including pre-commit
pip install --upgrade pip
pip install -r requirements.txt

# Install pre-commit hooks
pre-commit install
pre-commit install-hooks
