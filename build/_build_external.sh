#!/bin/bash
# sentry
mkdir sentry_scratch
pushd sentry_scratch
    git clone https://github.com/getsentry/sentry-native --recursive
    pushd ./sentry-native

        # static
        cmake -B build                                          \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo                   \
            -DSENTRY_BUILD_SHARED_LIBS=OFF                      \
            -DSENTRY_BUILD_FORCE32=yes                          \
            -DCMAKE_LIBRARY_PATH=/usr/lib/i386-linux-gnu        \
            -DCMAKE_INCLUDE_PATH=/usr/include/i386-linux-gnu
        cmake --build build --parallel

        # shared - do we need to do this
        cmake -B build                                          \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo                   \
            -DSENTRY_BUILD_SHARED_LIBS=ON                       \
            -DSENTRY_BUILD_FORCE32=yes                          \
            -DCMAKE_LIBRARY_PATH=/usr/lib/i386-linux-gnu        \
            -DCMAKE_INCLUDE_PATH=/usr/include/i386-linux-gnu
        cmake --build build --parallel

    popd ..
popd

find . -name libbreakpad_client.a   -exec cp {} ./ -v \;
find . -name libsentry.a            -exec cp {} ./ -v \;
