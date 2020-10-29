#!/bin/bash

latest_image_id=$(docker images | grep 'ubuntu' | grep 'latest' | awk '{ print $3 }')

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
new_dir=$(basename $dir)

docker run -it -v $dir:/$new_dir $latest_image_id





