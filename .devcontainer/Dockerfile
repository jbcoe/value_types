# Dockerfile for Ubuntu 22.04 with extra C++ development tools.
 
# Set the base image.
FROM ubuntu:22.04
 
# Maintainer
LABEL maintainer="Your Name <your@email.com>"
 
# Install gcc, clang and some supporting tools for downloading/installing later tools.
RUN apt update && apt install -y wget g++ lcov llvm gcov git ninja-build software-properties-common unzip

# Install newer CMake from kitware.
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null \
&& apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ jammy main' \
&& apt update \
&& apt install -y cmake

# Install bazel.
RUN wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-installer-linux-x86_64.sh \
&& bash bazel-6.4.0-installer-linux-x86_64.sh && rm bazel-6.4.0-installer-linux-x86_64.sh

RUN PATH="/usr/local/bin:$PATH"