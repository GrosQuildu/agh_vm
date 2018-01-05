#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: bash $0 [ARM | x86]"
  exit 1
fi

if [ -d "build" ]; then
  rm -rf ./build
fi
mkdir ./build && cd ./build

if [ "$1" == "ARM" ]; then
  cmake -D BUILD_ARM=ON ..
else
  cmake -D BUILD_ARM=OFF ..
fi

make
