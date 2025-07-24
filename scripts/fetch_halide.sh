#!/bin/bash

curl -L -o halide.tar.gz https://github.com/halide/Halide/releases/download/v19.0.0/Halide-19.0.0-x86-64-linux-5f17d6f8a35e7d374ef2e7e6b2d90061c0530333.tar.gz
mkdir -p thirdparty/halide
tar -xzf halide.tar.gz -C thirdparty/halide --strip-components=1
rm halide.tar.gz