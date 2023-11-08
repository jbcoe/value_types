# Dockerfile for Ubuntu 22.04 with C++ development tools installed
 
# Set the base image
FROM ubuntu:22.04
 
# Maintainer
LABEL maintainer="Your Name <your@email.com>"
 
# Install gcc, clang, bazel and cmake
RUN apt-get update && apt-get install -y wget g++ llvm ninja-build unzip
RUN wget https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-installer-linux-x86_64.sh \
&& bash bazel-6.4.0-installer-linux-x86_64.sh && rm bazel-6.4.0-installer-linux-x86_64.sh

RUN PATH="/usr/local/bin:$PATH"