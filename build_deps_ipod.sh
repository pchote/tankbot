#!/bin/sh

#!/bin/sh

# Note: json-c has broken configure scripts and needs to be fixed by hand before this script will work.
export LIBWEBSOCKET_SRC="/var/root/media/src/libwebsockets"
export JSONC_SRC="/var/root/media/src/json-c"

export ARCH="armv6-apple-darwin10"
export SDKVER="3.1"
export DEVROOT="/"
export SDKROOT="$DEVROOT"
export PREFIX="/var/root/media/src/tankbot/dependencies"

# Binaries
export CPP="$DEVROOT/usr/bin/gcc -E"
export CC="$DEVROOT/usr/bin/gcc"
export CXX="$DEVROOT/usr/bin/g++"
export LD="$DEVROOT/usr/bin/ld"

export PKG_CONFIG_PATH="$SDKROOT/usr/lib/pkgconfig":"$PREFIX/lib/pkgconfig"

# Flags
export CPPFLAGS="-I$PREFIX/include -I/var/sdk/usr/include"
export CFLAGS="$CPPFLAGS -std=c99 -pipe -no-cpp-precomp"
export CXXFLAGS="$CPPFLAGS -I$SDKROOT/usr/include/c++/4.2.1/$ARCH -pipe -no-cpp-precomp"
export LDFLAGS="-L$PREFIX/lib -L/usr/lib -L/var/sdk/usr/lib"

pushd $LIBWEBSOCKET_SRC;
# hacked together after the fact. A couple of source files will need removing first
cd lib
gcc -c *.c -I. -DLWS_LIBRARY_VERSION="\"1.1\"" -DLWS_BUILD_HASH="\"xxxxx\"" -I/var/sdk/usr/include  -std=c99 -pipe -no-cpp-precomp -DLWS_NO_CLIENT -DLWS_NO_DAEMONIZE -D_DEBU
ar rcs libwebsockets.a *.o
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