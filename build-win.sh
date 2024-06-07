#!/bin/bash

AUTOREMESHER_BUILD_DIR=$(pwd)
INSTALL_DIR=$AUTOREMESHER_BUILD_DIR/install

export CMAKE_APPLE_SILICON_PROCESSOR=x86_64
export CMAKE_SYSTEM_PROCESSOR=x86_64
export CMAKE_OSX_ARCHITECTURES=x86_64

pacman -S --noconfirm cmake gcc make mingw-w64-x86_64-boost mingw-w64-x86_64-qt6 mingw-w64-x86_64-qt-creator

cd thirdparty/blosc/c-blosc-1.18.1
mkdir build
cd build
cmake -D "CMAKE_INSTALL_PREFIX=$INSTALL_DIR" -D "BUILD_SHARED=OFF" -D "BUILD_TESTS=OFF" -D "BUILD_BENCHMARKS=OFF" ..
cmake --build . --config Release
cd $AUTOREMESHER_BUILD_DIR

cd thirdparty/zlib/zlib-1.2.11
mkdir build
cd build
cmake -D "CMAKE_INSTALL_PREFIX=$INSTALL_DIR" ..
cmake --build . --config Release
cp "$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11/build/zconf.h" "$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11/zconf.h"
cd $AUTOREMESHER_BUILD_DIR

cd thirdparty/openexr/openexr-2.4.1
mkdir build
cd build
cmake -D "CMAKE_INSTALL_PREFIX=$INSTALL_DIR" -D "BUILD_SHARED_LIBS=OFF" -D "PYILMBASE_ENABLE=0" -D "OPENEXR_VIEWERS_ENABLE=0" -D "ZLIB_INCLUDE_DIR=$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11/build/libz.a" ..
cmake --build . --config Release
cd $AUTOREMESHER_BUILD_DIR

cd thirdparty/tbb
mkdir build2
cd build2
cmake -D "CMAKE_INSTALL_PREFIX=$INSTALL_DIR" ..
cmake --build . --config Release
cd $AUTOREMESHER_BUILD_DIR

cd thirdparty/openvdb/openvdb-7.0.0
mkdir build
cd build
cmake -D "CMAKE_INSTALL_PREFIX=$INSTALL_DIR" -D "OPENVDB_CORE_SHARED=OFF" -D "OPENVDB_BUILD_VDB_PRINT=OFF" -D "IlmBase_INCLUDE_DIR=$AUTOREMESHER_BUILD_DIR/thirdparty/openexr/openexr-2.4.1" -D "IlmBase_Half_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/openexr/openexr-2.4.1/build/IlmBase/Half/libHalf-2_4.a" -D "Tbb_INCLUDE_DIR=$AUTOREMESHER_BUILD_DIR/thirdparty/tbb/include" -D "Tbb_tbb_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/tbb/build2/libtbb_static.a" -D "Tbb_tbbmalloc_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/tbb/build2/libtbbmalloc_static.a" -D "Tbb_tbbmalloc_proxy_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/tbb/build2/libtbbmalloc_proxy_static.a" -D "ZLIB_INCLUDE_DIR=$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11/build/libz.a" -D "Blosc_INCLUDE_DIR=$AUTOREMESHER_BUILD_DIR/thirdparty/blosc/c-blosc-1.18.1" -D "Blosc_LIBRARY=$AUTOREMESHER_BUILD_DIR/thirdparty/blosc/c-blosc-1.18.1/build/blosc/libblosc.a" -D "BOOST_INCLUDEDIR=/usr/local/opt/boost/include" -D "OPENVDB_DISABLE_BOOST_IMPLICIT_LINKING=ON" -D "CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=/usr/local/opt/boost/include;$AUTOREMESHER_BUILD_DIR/thirdparty/blosc/c-blosc-1.18.1/blosc;$AUTOREMESHER_BUILD_DIR/thirdparty/zlib/zlib-1.2.11/build" ..
cmake --build . --config Release
cd $AUTOREMESHER_BUILD_DIR

/c/msys64/mingw64/bin/qmake6 -o Makefile .
make
