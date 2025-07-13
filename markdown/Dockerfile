FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package list and install required packages
RUN apt-get update && apt-get install -y --install-recommends \
    ca-certificates \
    curl \
    git \
    make \
    python3 \
    python3-venv \
    texlive-xetex \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Clone the wg21 repository with submodules
RUN git clone --recursive https://github.com/mpark/wg21.git

# Create a working directory
RUN mkdir -p /app/workdir

# Set the working directory to the cloned repo
WORKDIR /app/wg21

COPY _generate_pdf.docker.sh /app/wg21/generate_pdf.sh

RUN chmod +x /app/wg21/generate_pdf.sh

ENTRYPOINT ["/app/wg21/generate_pdf.sh"]
