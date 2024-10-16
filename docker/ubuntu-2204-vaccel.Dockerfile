FROM ubuntu:22.04

ARG DEBUG=false
ARG TARGETPLATFORM
# Shell setup
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
  sudo \
  wget \
  curl \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN wget --no-check-certificate https://s3.nbfc.io/nbfc-assets/github/vaccel/plugins/rpc/rev/main/x86_64/release/vaccel-rpc_latest_amd64.deb  \
    && wget --no-check-certificate https://s3.nbfc.io/nbfc-assets/github/vaccel/rev/main/x86_64/release/vaccel_latest_amd64.deb  \
    && dpkg -i vaccel_latest_amd64.deb \
    && dpkg -i vaccel-rpc_latest_amd64.deb \
    && rm *.deb

ENV VACCEL_BACKENDS=/usr/lib/x86_64-linux-gnu/libvaccel-rpc.so
ENV VACCEL_DEBUG_LEVEL=3
ENV VACCEL_PROF_ENABLED=enabled
ENV VACCEL_RPC_ADDRESS=vsock://2:2048

