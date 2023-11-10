# Dockerfile for Ubuntu 22.04 with C++ development tools.

# Set the base image.
FROM ubuntu:22.04

# Install gcc, clang and some supporting tools for downloading/installing later tools.
RUN apt-get update && apt-get install -y --no-install-recommends wget g++ lcov llvm git gpg ninja-build software-properties-common unzip && rm -rf /var/lib/apt/lists/*

# Install newer CMake from kitware.
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
&& apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ jammy main' && apt-get update && apt-get install -y --no-install-recommends cmake && rm -rf /var/lib/apt/lists/*

# Install bazel.
RUN wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-installer-linux-x86_64.sh \
&& bash bazel-6.4.0-installer-linux-x86_64.sh && rm bazel-6.4.0-installer-linux-x86_64.sh

RUN rm -rf /var/lib/apt/list/*

ENV PATH="/usr/local/bin:$PATH"
