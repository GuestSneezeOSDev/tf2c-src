#!/bin/bash
echo "what branch do ya want to switch to?"
read TARGET_BRANCH
git checkout ${TARGET_BRANCH} --recurse-submodules  && \
git submodule sync            --recursive           && \
git submodule update          --recursive