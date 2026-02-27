#! /bin/sh

rm -rf ./build/ && mkdir build && cd build && cmake -S .. -B . -DCMAKE_GENERATOR:INTERNAL=Ninja
