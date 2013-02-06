#!/bin/sh

# Note: json-c has broken configure scripts and needs to be fixed by hand before this script will work.
export LIBWEBSOCKET_SRC="/Users/paul/src/libwebsockets"
export JSONC_SRC="/Users/paul/src/json-c"

export ARCH="armv7-apple-darwin10"
export SDKVER="6.1"
export DEVROOT="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer"
export SDKROOT="$DEVROOT/SDKs/iPhoneOS$SDKVER.sdk"
export PREFIX="/Users/paul/src/tankbot/dependencies"

# Binaries
export CPP="$DEVROOT/usr/bin/gcc -E"
export CC="$DEVROOT/usr/bin/gcc"
export CXX="$DEVROOT/usr/bin/g++"
export LD="$DEVROOT/usr/bin/ld"

export PKG_CONFIG_PATH="$SDKROOT/usr/lib/pkgconfig":"$PREFIX/lib/pkgconfig"

# Flags
export CPPFLAGS="-arch armv7 -isysroot $SDKROOT -I$PREFIX/include"
export CFLAGS="$CPPFLAGS -std=c99 -pipe -no-cpp-precomp"
export CXXFLAGS="$CPPFLAGS -I$SDKROOT/usr/include/c++/4.2.1/$ARCH -pipe -no-cpp-precomp"
export LDFLAGS="-arch armv7 -isysroot $SDKROOT -L$PREFIX/lib"

pushd $LIBWEBSOCKET_SRC;
./configure --prefix="$PREFIX" --host="$ARCH" --enable-static --disable-shared --without-client --without-daemonize  --without-extensions $@
make clean
make install
popd

pushd $JSONC_SRC;
./configure --prefix="$PREFIX" --host="$ARCH" --enable-static --disable-shared --without-client --without-daemonize  --without-extensions $@
make clean
make install
popd

mkdir server/lib server/include
cp dependencies/lib/libwebsockets.a server/lib
cp dependencies/lib/libjson-c.a server/lib
cp dependencies/include/libwebsockets.h server/include
cp -r dependencies/include/json-c server/include
rm -rf dependencies