#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Usage: bash $0 [ARM | x86] [NORMAL | DEBUG]"
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
  BUILD_ARM=ON
else
  BUILD_ARM=OFF
fi

if [ "$2" == "DEBUG" ] || [ "$2" == "debug" ]; then
  BUILD_DEBUG=1
  if [ "$BUILD_ARM" == "ON" ]; then
    echo "Ncurses (DEBUG option) works only with x86"
    BUILD_DEBUG=0
  fi
else
  BUILD_DEBUG=0
fi

cmake -D BUILD_ARM=$BUILD_ARM -D DEBUG=$BUILD_DEBUG ..
make

if [ "$BUILD_ARM" == "ON" ]; then
  echo "Now run: qemu-arm -L /usr/arm-linux-gnueabi/ ./VM --help"
  echo "Or: qemu-arm -L /usr/arm-linux-gnueabi/ ./VM example_programs/test_schedulers/ blocks/ FIFO true"
else
  echo "Now run: ./VM --help"
  echo "Or: ./VM example_programs/test_schedulers/ blocks/ FIFO true"
fi
