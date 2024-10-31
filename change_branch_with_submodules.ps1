$TARGET_BRANCH = Read-Host -Prompt 'what branch do ya want to switch to?'

git checkout $TARGET_BRANCH   --recurse-submodules
git submodule sync            --recursive
git submodule update          --recursive
pause