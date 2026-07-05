#!/bin/bash
set -e

[ ! -d ../build ] && mkdir -p ../build

pushd ../build

c++ -DHANDMADE_SDL=1 ../code/sdl_handmade.cpp -o handmadehero -g $(sdl2-config --cflags --libs)
popd
