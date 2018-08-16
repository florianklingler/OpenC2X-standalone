FROM amd64/ubuntu:14.04

RUN echo "deb http://dl.bintray.com/fynnh/debian trusty main" | tee -a /etc/apt/sources.list && \
  apt-get update && \
  apt-get install -y --allow-unauthenticated \
  build-essential \
  cmake \
  libboost-all-dev \
  protobuf-compiler \
  libprotobuf-dev \
  libnl-3-dev \
  libnl-genl-3-dev \
  libnl-route-3-dev \
  asn1c \
  libzmq3-dev \
  libgps-dev \
  uci
