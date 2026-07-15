FROM ffornari/s3fs-rgw-dependencies:latest
ENV LD_LIBRARY_PATH=/usr/local/lib
RUN git clone https://github.com/ffornari90/s3fs-rgw-sts-lib.git && \
    cd s3fs-rgw-sts-lib && \
    cmake -S . -B build && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    cd .. && \
    rm -rf s3fs-rgw-sts-lib
RUN adduser --disabled-password --gecos '' docker
USER docker
RUN mkdir -p $HOME/.aws && \
    printf '%s\n%s\n%s\n%s\n' \
    "[default]" \
    "aws_access_key_id = access" \
    "aws_secret_access_key = secret" \
    "aws_session_token = token" \
    > $HOME/.aws/credentials && \
    chmod 600 $HOME/.aws/credentials && \
    mkdir -p $HOME/mnt/rgw && \
    echo "access:secret" > $HOME/.passwd-s3fs && \
    chmod 600 $HOME/.passwd-s3fs
ENTRYPOINT ["sh", "-c"]
CMD ["s3fs $BUCKET_NAME $HOME/mnt/rgw \
      -o use_path_request_style \
      -o url=$ENDPOINT_URL \
      -o endpoint=$REGION_NAME \
      -o credlib=librgw-sts.so \
      -o credlib_opts=Info -f"]
