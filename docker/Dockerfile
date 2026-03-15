# Dockerfile for Ubuntu 24.04 with C++ development tools.
FROM ubuntu:24.04

# Install gcc, clang and some supporting tools for downloading/installing later tools.
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    curl \
    g++ \
    gdb \
    git \
    gpg \
    lcov \
    llvm \
    ninja-build \
    python-is-python3 \
    python3-pip \
    python3-venv \
    software-properties-common \
    ssh \
    unzip \
    wget \
&& rm -rf /var/lib/apt/lists/*

# Install bazelisk.
RUN wget https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-linux-amd64 -O /usr/local/bin/bazelisk \
    && chmod +x /usr/local/bin/bazelisk \
    && ln -s /usr/local/bin/bazelisk /usr/local/bin/bazel

# Create non-root user
RUN useradd -m -s /bin/bash vscode \
    && mkdir -p /workspace \
    && chown vscode:vscode /workspace

# Install Node.js (required for Gemini CLI)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && rm -rf /var/lib/apt/lists/*

# Install Gemini CLI globally
RUN npm install -g @google/gemini-cli

# Switch to non-root user
USER vscode
WORKDIR /workspace

# Set up uv and Python environment
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Set up environment variables
ENV PATH="/home/vscode/.local/bin:/usr/local/bin:${PATH}"
ENV UV_PROJECT_ENVIRONMENT="/home/vscode/.venv"

# Pre-configure Gemini CLI
RUN mkdir -p /home/vscode/.gemini \
    && echo '{"/workspace": "TRUST_FOLDER"}' > /home/vscode/.gemini/trustedFolders.json \
    && echo '{"security": {"auth": {"selectedType": "gemini-api-key"}}}' > /home/vscode/.gemini/settings.json
