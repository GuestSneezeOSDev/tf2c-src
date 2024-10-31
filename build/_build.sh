#!/bin/bash

if [ ! -f /.dockerenv ]; then
    echo "this script needs to be run in docker. try linux_build.sh!"; exit 1;
fi
apt update
apt install git-lfs

# export PATH="/usr/lib/binutils-2.32/bin:$PATH"

#export CC=clang
#export CXX=clang++
#export C++=clang++
#export CC=gcc-9
#export CXX=g++-9
export CC=gcc-10
export C++=g++-10
export CXX=g++-10
export STEAM_RUNTIME_PATH=/usr
# export MAKE_VERBOSE=1
# parallelization
export MAKE_JOBS

export TERM=screen
tabs 32

pushd src/

# Generate project files
chmod +x ./ -Rf
# ./createallprojects
./createtf2classic

make -f Tf2Classic.mak server_tf -lstdc++fs

ecode=$?
if [[ ${ecode} != 0 ]]; then
    echo "ECODE = ${ecode}"
    exit ${ecode}
fi

popd
