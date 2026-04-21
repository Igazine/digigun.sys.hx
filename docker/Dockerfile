FROM ubuntu:24.04

# Avoid prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install Haxe and build dependencies
RUN apt-get update && apt-get install -y \
    software-properties-common \
    && add-apt-repository ppa:haxe/releases -y \
    && apt-get update && apt-get install -y \
    haxe \
    neko \
    build-essential \
    libglib2.0-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Setup haxelib
RUN mkdir -p /haxelib && haxelib setup /haxelib

# Install hxcpp
RUN haxelib install hxcpp

# Create a workspace
WORKDIR /project

# Command to run by default
CMD ["/bin/bash"]
