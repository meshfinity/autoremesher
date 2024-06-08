call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"

git clone https://github.com/meshfinity/autoremesher.git C:\autoremesher
cd C:\autoremesher

cd thirdparty/blosc/c-blosc-1.18.1
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A x64 -D "BUILD_SHARED=OFF" -D "BUILD_TESTS=OFF" -D "BUILD_BENCHMARKS=OFF" ../
cmake --build . --config Release
cd C:\autoremesher

cd thirdparty/zlib/zlib-1.2.11
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A x64 ../
cmake --build . --config Release
copy C:\autoremesher\thirdparty\zlib\zlib-1.2.11\build\zconf.h C:\autoremesher\thirdparty\zlib\zlib-1.2.11\zconf.h
cd C:\autoremesher

cd thirdparty/openexr/openexr-2.4.1
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A x64 -D "BUILD_SHARED_LIBS=OFF" -D "PYILMBASE_ENABLE=0" -D "OPENEXR_VIEWERS_ENABLE=0" -D "ZLIB_INCLUDE_DIR=C:\autoremesher/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=C:\autoremesher/thirdparty/zlib/zlib-1.2.11/build/Release/zlibstatic.lib" ../
cmake --build . --config Release
cd C:\autoremesher

cd thirdparty/tbb
mkdir build2
cd build2
cmake -G "Visual Studio 15 2017" -A x64 ../
cmake --build . --config Release
cd C:\autoremesher

cd thirdparty/openvdb/openvdb-7.0.0
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A x64 -D "OPENVDB_CORE_STATIC=OFF" -D "OPENVDB_BUILD_VDB_PRINT=OFF" -D "IlmBase_INCLUDE_DIR=C:\autoremesher/thirdparty/openexr/openexr-2.4.1" -D "IlmBase_Half_LIBRARY=C:\autoremesher/thirdparty/openexr/openexr-2.4.1/build/IlmBase/Half/Release/Half-2_4.lib" -D "TBB_INCLUDEDIR=C:\autoremesher/thirdparty/tbb/include" -D "TBB_LIBRARYDIR=C:\autoremesher/thirdparty/tbb/build2/Release" -D "ZLIB_INCLUDE_DIR=C:\autoremesher/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=C:\autoremesher/thirdparty/zlib/zlib-1.2.11/build/Release/zlibstatic.lib" -D "Blosc_INCLUDE_DIR=C:\autoremesher/thirdparty/blosc/c-blosc-1.18.1" -D "Blosc_LIBRARY=C:\autoremesher/thirdparty/blosc/c-blosc-1.18.1/build/blosc/Release/libblosc.lib" -D "BOOST_INCLUDEDIR=C:/Libraries/boost_1_66_0" -D "OPENVDB_DISABLE_BOOST_IMPLICIT_LINKING=ON" -D "CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=C:/Libraries/boost_1_66_0;C:\autoremesher/thirdparty/blosc/c-blosc-1.18.1/blosc;C:\autoremesher/thirdparty/zlib/zlib-1.2.11/build" ../
cmake --build . --config Release
cd C:\autoremesher

curl -O https://archives.boost.io/release/1.82.0/source/boost_1_82_0.zip
tar -xf boost_1_82_0.zip

curl -O -L https://github.com/CGAL/cgal/releases/download/releases/CGAL-5.1-beta1/CGAL-5.1-beta1-Setup.exe
CGAL-5.1-beta1-Setup.exe /S /D=C:\autoremesher\CGAL-5.1-beta1

curl -O -L https://github.com/miurahr/aqtinstall/releases/download/Continuous/aqt.exe
aqt install-qt windows desktop 5.13.2 win64_msvc2017_64

set QTDIR=C:\autoremesher\5.13.2\msvc2017_64
set PATH=%PATH%;%QTDIR%\bin
qmake "BOOST_INCLUDEDIR=C:\autoremesher\boost_1_82_0" "CGAL_DIR=C:\autoremesher\thirdparty\cgal\CGAL-5.1-beta1"
nmake -f Makefile.Release
