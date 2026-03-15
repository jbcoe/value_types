FROM python:3.12-slim

# Install gcc, clang and some supporting tools for downloading/installing later tools.
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    curl \
    g++ \
    git \
    gpg \
    lcov \
    llvm \
    ninja-build \
    python-is-python3 \
    python3-pip \
    ssh \
    unzip \
    wget \
&& rm -rf /var/lib/apt/lists/*

# Install bazel.
RUN wget https://github.com/bazelbuild/bazel/releases/download/9.0.1/bazel-9.0.1-installer-linux-x86_64.sh \
    && bash bazel-9.0.1-installer-linux-x86_64.sh && rm bazel-9.0.1-installer-linux-x86_64.sh;


# Create non-root user
RUN useradd -m -s /bin/bash vscode \
    && mkdir -p /workspace \
    && chown vscode:vscode /workspace \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Switch to non-root user
USER vscode

# Set up uv and Python environment
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Set up environment variables
ENV PATH="/home/vscode/.local/bin:${PATH}"
ENV UV_PROJECT_ENVIRONMENT="/home/vscode/.venv"
ENV TERM="xterm-256color"
ENV COLORTERM="truecolor"

# Switch back to root to install Node.js and Gemini CLI
USER root

# Install Node.js (required for Gemini CLI)
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y --no-install-recommends nodejs

# Install Gemini CLI globally
RUN npm install -g @google/gemini-cli

# Switch back to non-root user
USER vscode

# Pre-configure Gemini CLI
RUN mkdir -p /home/vscode/.gemini \
    && echo '{"/workspace": "TRUST_FOLDER"}' > /home/vscode/.gemini/trustedFolders.json \
    && echo '{"security": {"auth": {"selectedType": "gemini-api-key"}}}' > /home/vscode/.gemini/settings.json
