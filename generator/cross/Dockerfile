FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates cmake g++ gcc git make g++-mingw-w64-x86-64-posix \
    libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev \
    libxxf86vm-dev libxext-dev libxrender-dev libxfixes-dev libxau-dev libxdmcp-dev \
    libxcb1-dev python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
