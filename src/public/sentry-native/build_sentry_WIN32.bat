:: by https://sappho.io
mkdir build
pushd build
    git clone https://github.com/getsentry/sentry-native --recursive

    pushd sentry-native
        :: v143 is vs2022 and Win32 is obviously 32 bit
        :: sys_vers = 10 is windows 10
        cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -T v143 -A Win32 -D CMAKE_SYSTEM_VERSION=10
        cmake --build build --parallel
    popd

    copy "sentry-native\build\crashpad_build\handler\Debug\crashpad_handler.exe"    "crashpad_handler.exe"
    copy "sentry-native\build\crashpad_build\handler\Debug\crashpad_handler.pdb"    "crashpad_handler.pdb"
    copy "sentry-native\build\Debug\sentry.dll"                                     "sentry.dll"
    copy "sentry-native\build\Debug\sentry.lib"                                     "sentry.lib"
    copy "sentry-native\build\Debug\sentry.pdb"                                     "sentry.pdb"

    del /f /s /q "sentry-native 1>nul
    rmdir /s /q  "sentry-native"
popd

pause