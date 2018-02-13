#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: bash $0 [ARM | x86]"
  exit 1
fi

if [ -d "build" ]; then
 # rm -rf ./build
 echo "Using cached dir build"
else
  mkdir ./build
fi
cd ./build

if [ "$1" == "ARM" ] || [ "$1" == "arm" ]; then
  BUILD_SWITCH=ON
else
  BUILD_SWITCH=OFF
fi

cmake -D BUILD_ARM=$BUILD_SWITCH ..
make

if [ $BUILD_SWITCH == "ON" ]; then
  echo "Now run: qemu-arm -L /usr/arm-linux-gnueabi/ ./VM --help"
  echo "Or: qemu-arm -L /usr/arm-linux-gnueabi/ ./VM example_programs/test_schedulers/ blocks/ FIFO true"
else
  echo "Now run: ./VM --help"
  echo "Or: ./VM example_programs/test_schedulers/ blocks/ FIFO true"
fi
