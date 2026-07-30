FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV UV_INSTALL_DIR=/usr/local/bin

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        ccache \
        cmake \
        curl \
        git \
        ninja-build \
        pkg-config \
        wget \
        file \
    && rm -rf /var/lib/apt/lists/*

ENV UV_LINK_MODE=symlink

RUN curl -LsSf https://astral.sh/uv/install.sh | sh
