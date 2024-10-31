#!/bin/bash

# dockerimage="registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest"
# dockerimage="scell555/steam-runtime-gcc-9"
dockerimage="registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest"
# dockerimage="registry.gitlab.steamos.cloud/steamrt/medic/sdk:latest"
# we do this so that we can be agnostic about where we're invoked from
# meaning you can exec this script anywhere and it should work the same
thisiswhereiam=${BASH_SOURCE[0]}
# this should be /whatever/directory/structure/Open-Fortress-Source
script_folder=$( cd -- "$( dirname -- "${thisiswhereiam}" )" &> /dev/null && pwd )
# this should be /whatever/directory/structure/[sdkmod-source]/build
build_dir="build"

pushd "${script_folder}" &> /dev/null || exit 99

# this is relative to our source dir/build
internalscript="_build.sh"

# this should always be our open fortress srcdir
pushd ../ &> /dev/null
dev_srcdir=$(pwd)
dev_rootdir="tf2cd"

# add -it flags automatically if in null tty
itflag=""
if [ -t 0 ] ; then
    itflag="-it"
else
    itflag=""
fi


totalCPUS=$(getconf _NPROCESSORS_ONLN)
highestCPU=$(( ${totalCPUS} - 1 ))

cpulimit_flags="--cpuset-cpus=2-${highestCPU}"

#cpulimit_flags=""
#if [[ -v CI_SERVER ]]; then
#    cpulimit_flags="--cpuset-cpus=2-${highestCPU}"
#else
#    cpulimit_flags="--cpus=$(( ${totalCPUS} - 2 ))"
#fi

compiletype="release"
if [[ -v dbg ]]; then
    compiletype="debug"
fi

mkdir ${HOME}/.ccache || echo "ccache exists, not remaking it"

echo ${compiletype}
podman run -e CFG=${compiletype} ${itflag}              \
${cpulimit_flags}                                       \
-v ${HOME}/.ccache:/root/.ccache   			\
-v "${dev_srcdir}":/${dev_rootdir}                      \
-w /${dev_rootdir}                                      \
${dockerimage}                                          \
bash ./${build_dir}/${internalscript} "$@"

ecodereal=$?
echo "real exit code ${ecodereal}"

popd &> /dev/null || exit
popd &> /dev/null || exit

exit ${ecodereal}
# --user "$(id -u):$(id -g)"				\
# -v "${dev_srcdir}"/${build_dir}/.ccache:/root/.ccache   \
