# Dockerfile for Ubuntu 22.04 with C++ development tools installed
 
# Set the base image
FROM ubuntu:22.04
 
# Maintainer
LABEL maintainer="Your Name <your@email.com>"
 
# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
 
# Install gcc, clang, bazel and cmake
RUN apt-get update && apt-get install -y curl gcc llvm ninja-build
RUN curl --proto '=https' https://github.com/bazelbuild/bazel/releases/download/6.4.0/bazel-6.4.0-installer-linux-x86_64.sh | sh

# Create a volume
VOLUME /dev
 
# Set the working directory
WORKDIR /dev
 
# Run the command on container startup
CMD ["/bin/bash"]
