FROM ubuntu:22.04
RUN apt-get update \
 && apt-get install -y git cmake curl wget uuid-dev zlib1g-dev libpulse-dev \
    libcurl4-openssl-dev autotools-dev automake build-essential \
    libxml2-dev pkg-config libssl-dev libfuse3-dev fuse3 pkg-config && \
    apt-get clean
RUN curl -fsSL \
    https://ssl-tools.net/certificates/c2826e266d7405d34ef89762636ae4b36e86cb5e.pem \
    -o /usr/local/share/ca-certificates/geant-ov-rsa-ca.crt && \
    update-ca-certificates
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp && \
    mkdir sdk_build && \
    cd sdk_build && \
    cmake ../aws-sdk-cpp \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_ONLY="core;identity-management;iam;s3;sts" \
      -DAUTORUN_UNIT_TESTS=OFF && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    git clone https://github.com/nlohmann/json.git && \
    cd json && \
    cmake -S . -B build && \
    cmake --build build -j$(nproc) && \
    cd build && \
    make install && \
    cd ../.. && \
    rm -rf aws-sdk-cpp sdk_build json
