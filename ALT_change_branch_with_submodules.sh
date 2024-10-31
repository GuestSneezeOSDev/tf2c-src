#!/bin/bash
echo "what branch do ya want to switch to?"
echo "this will whack all ya changes. be careful!"
read TARGET_BRANCH
#git checkout ${TARGET_BRANCH} --recurse-submodules  && \
#git submodule sync            --recursive           && \
#git submodule update          --recursive



# export TARGET_BRANCH="my-branch-name"
export CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ -f ".gitmodules" ]; then
  git submodule deinit --all -f
  mkdir -p .git/git-tools/modules
  cp .git/modules .git/git-tools/modules/$CURRENT_BRANCH -Rf
  rm .git/modules -Rf
fi

git checkout $TARGET_BRANCH

if [ -f ".gitmodules" ]; then
  if [ -f ".git/git-tools/modules/$TARGET_BRANCH" ]; then
    git mv .git/git-tools/modules/$TARGET_BRANCH .git/modules -f
  fi

  git submodule sync && git submodule update --init
fi