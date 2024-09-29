# Dockerfile for Ubuntu 24.04 with C++ development tools.

# Set the base image.
FROM ubuntu:24.04

# Install gcc, clang and some supporting tools for downloading/installing later tools.
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    git \
    gpg \
    lcov \
    llvm \
    ninja-build \
    python-is-python3 \
    python3-pip \
    software-properties-common \
    ssh \
    unzip\
    wget \
&& rm -rf /var/lib/apt/lists/*

# Install newer CMake from kitware.
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
&& apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ noble main' \
&& apt-get update && apt-get install -y --no-install-recommends cmake \
&& rm -rf /var/lib/apt/lists/*

# Install bazel.
RUN wget https://github.com/bazelbuild/bazel/releases/download/7.3.1/bazel-7.3.1-installer-linux-x86_64.sh \
&& bash bazel-7.3.1-installer-linux-x86_64.sh && rm bazel-7.3.1-installer-linux-x86_64.sh \
&& rm -rf /var/lib/apt/lists/*

ENV PATH="/usr/local/bin:$PATH"
