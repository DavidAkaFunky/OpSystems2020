#!/bin/bash

latest_image_id=$(docker images | grep 'ubuntu' | grep 'latest' | awk '{ print $3 }')
latest_container_id=$(docker ps -a | grep $latest_image_id | awk '{ print $1 }')
docker commit $latest_container_id ubuntu:latest
docker rm $latest_container_id