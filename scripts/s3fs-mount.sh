#!/bin/bash
ROOTDIR=$(git rev-parse --show-toplevel)
FILE="${ROOTDIR}/rgw-sts-profile"
if [ -f "$FILE" ]; then
  export $(grep -v '^#' $FILE | xargs -d '\n')
fi
mkdir -p $HOME/mnt/rgw
s3fs bucket $HOME/mnt/rgw \
  -o use_path_request_style \
  -o url=$ENDPOINT_URL \
  -o endpoint=$REGION_NAME \
  -o credlib=librgw-sts.so \
  -o credlib_opts=Info -f
