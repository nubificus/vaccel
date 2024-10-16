FROM ubuntu:22.04

ARG DEBUG=false
ARG TARGETPLATFORM
# Shell setup
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
  apt-transport-https \
  apt-utils \
  ca-certificates \
  curl \
  gcc \
  git \
  iproute2 \
  iptables \
  jq \
  libyaml-dev \
  locales \
  lsb-release \
  openssl \
  pigz \
  pkg-config \
  software-properties-common \
  time \
  tzdata \
  uidmap \
  unzip \
  wget \
  xz-utils \
  zip \
  gnupg-agent \
  openssh-client \
  make \
  rsync \
  jq \
  sudo \
  gcc-10 g++-10 lcov \
  python3-pip python3-dev \
  build-essential cmake gcc-12 g++-12 ninja-build dh-make \
  git-buildpackage \
  libxml2-dev libxslt1-dev \
  libclang-dev valgrind cppcheck pkg-config protobuf-c-compiler protobuf-compiler \
  && apt-get clean \
  && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 --slave /usr/bin/g++ g++ /usr/bin/g++-10 \
  && rm -rf /var/lib/apt/lists/*

#RUN pip install --break-system-packages meson gcovr pycobertura codespell
RUN pip install meson gcovr pycobertura codespell

ARG BRANCH

RUN [ -z "${BRANCH}" ] && BRANCH="main"; \ 
    git clone https://github.com/nubificus/vaccel -b $BRANCH \
    && cd vaccel \
    && CC=gcc-12 CXX=g++-12 meson setup -Dauto_features=enabled build \
    && meson compile -C build \
    && meson install -C build \
    && cd .. && rm -rf vaccel && ldconfig

