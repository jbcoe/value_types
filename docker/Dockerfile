FROM ubuntu:24.04
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Install base tools and C++ development tools.
RUN apt-get update && apt-get install -y --no-install-recommends \
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

# Install newer CMake from kitware.
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
&& apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ noble main' \
&& apt-get update && apt-get install -y --no-install-recommends cmake \
&& rm -rf /var/lib/apt/lists/*

# Install bazelisk.
RUN ARCH=$(dpkg --print-architecture) && \
    wget https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-linux-${ARCH} -O /usr/local/bin/bazelisk \
    && chmod +x /usr/local/bin/bazelisk \
    && ln -s /usr/local/bin/bazelisk /usr/local/bin/bazel

# Create non-root user.
RUN useradd -m -s /bin/bash vscode \
    && mkdir -p /workspace \
    && chown vscode:vscode /workspace

# Install Node.js (required for Gemini CLI and Claude Code).
RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && rm -rf /var/lib/apt/lists/*

# Switch to non-root user.
USER vscode
WORKDIR /workspace

# Configure npm for user-local global installs.
ENV NPM_CONFIG_PREFIX=/home/vscode/.npm-global
ENV PATH="/home/vscode/.npm-global/bin:/home/vscode/.local/bin:/usr/local/bin:${PATH}"
RUN mkdir -p /home/vscode/.npm-global

# Install AI agent CLIs for compatibility with existing tooling that
# may invoke `gemini` or `claude`.
RUN npm install -g @google/gemini-cli @anthropic-ai/claude-code

# Install Antigravity CLI (installs binary as `agy` into ~/.local/bin).
RUN curl -fsSL https://antigravity.google/cli/install.sh | bash

# Set up uv and Python environment.
RUN curl -LsSf https://astral.sh/uv/install.sh | sh
ENV UV_PROJECT_ENVIRONMENT="/home/vscode/.venv"
