############################
# BUILDER
############################
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
      git \
      cmake \
      curl \
      wget \
      uuid-dev \
      zlib1g-dev \
      libpulse-dev \
      libcurl4-openssl-dev \
      autotools-dev \
      automake \
      build-essential \
      libxml2-dev \
      pkg-config \
      libssl-dev \
      libfuse3-dev \
      fuse3 && \
    rm -rf /var/lib/apt/lists/*

RUN curl -fsSL \
    https://ssl-tools.net/certificates/c2826e266d7405d34ef89762636ae4b36e86cb5e.pem \
    -o /usr/local/share/ca-certificates/geant-ov-rsa-ca.crt && \
    update-ca-certificates

#
# AWS SDK
#
RUN git clone --depth 1 --recurse-submodules \
      https://github.com/aws/aws-sdk-cpp.git && \
    mkdir sdk_build && \
    cd sdk_build && \
    cmake ../aws-sdk-cpp \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_ONLY="core;identity-management;iam;s3;sts" \
      -DAUTORUN_UNIT_TESTS=OFF && \
    make -j"$(nproc)" && \
    make install

#
# nlohmann/json
#
RUN git clone --depth 1 \
      https://github.com/nlohmann/json.git && \
    cd json && \
    cmake -S . -B build && \
    cmake --build build -j"$(nproc)" && \
    cmake --install build

#
# s3fs-rgw-sts-lib
#
RUN git clone \
      https://github.com/ffornari90/s3fs-rgw-sts-lib.git && \
    cd s3fs-rgw-sts-lib && \
    cmake -S . -B build && \
    cmake --build build -j"$(nproc)" && \
    cmake --install build

############################
# RUNTIME
############################
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib

RUN apt-get update && \
    apt-get install -y \
      fuse3 \
      libfuse3-3 \
      libcurl4 \
      libssl3 \
      libxml2 \
      zlib1g \
      ca-certificates && \
    rm -rf /var/lib/apt/lists/*

#
# Copy runtime artifacts
#
COPY --from=builder /usr/local/bin/s3fs /usr/local/bin/s3fs
COPY --from=builder /usr/local/lib/ /usr/local/lib/

#
# Copy CA bundle generated in builder
#
COPY --from=builder /etc/ssl/certs/ /etc/ssl/certs/
COPY --from=builder /usr/local/share/ca-certificates/ /usr/local/share/ca-certificates/

RUN ldconfig

RUN adduser --disabled-password --gecos "" docker

USER docker

RUN mkdir -p /home/docker/.aws && \
    printf '%s\n%s\n%s\n%s\n' \
      "[default]" \
      "aws_access_key_id = access" \
      "aws_secret_access_key = secret" \
      "aws_session_token = token" \
      > /home/docker/.aws/credentials && \
    chmod 600 /home/docker/.aws/credentials && \
    mkdir -p /home/docker/mnt/rgw && \
    echo "access:secret" > /home/docker/.passwd-s3fs && \
    chmod 600 /home/docker/.passwd-s3fs

ENTRYPOINT ["sh","-c"]

CMD ["s3fs ${BUCKET_NAME} /home/docker/mnt/rgw \
        -o use_path_request_style \
        -o url=${ENDPOINT_URL} \
        -o endpoint=${REGION_NAME} \
        -o credlib=librgw-sts.so \
        -o credlib_opts=Info -f"]