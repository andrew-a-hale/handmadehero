#!/bin/bash
set -e

[ ! -d ../build ] && mkdir -p ../build

pushd ../build

## RAYLIB
# g++ -O3 -o main -std=c++23 \
#   -I /usr/local/include \
#   -I ../code/vendor/raylib/include \
#   -lGL -lm -lpthread -ldl -lX11 \
#   -lXrandr -lXinerama -lXcursor -lXxf86vm -lXi \
#   ../code/rl_handmade.cpp ../code/vendor/raylib/lib/libraylib.a

c++ ../code/sdl_handmade.cpp -o handmadehero -g $(sdl2-config --cflags --libs)
popd
