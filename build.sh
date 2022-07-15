#!/bin/sh

cd ./build
ninja clean
cmake ..
ninja

