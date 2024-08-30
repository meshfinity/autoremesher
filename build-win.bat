call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

set AUTOREMESHER_BUILD_DIR=%~dp0

cd thirdparty/blosc/c-blosc-1.18.1
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -D "BUILD_SHARED=OFF" -D "BUILD_TESTS=OFF" -D "BUILD_BENCHMARKS=OFF" ../
cmake --build . --config Release
cd %AUTOREMESHER_BUILD_DIR%

cd thirdparty/zlib/zlib-1.2.11
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ../
cmake --build . --config Release
copy %AUTOREMESHER_BUILD_DIR%\thirdparty\zlib\zlib-1.2.11\build\zconf.h %AUTOREMESHER_BUILD_DIR%\thirdparty\zlib\zlib-1.2.11\zconf.h
cd %AUTOREMESHER_BUILD_DIR%

cd thirdparty/openexr/openexr-2.4.1
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -D "BUILD_SHARED_LIBS=OFF" -D "PYILMBASE_ENABLE=0" -D "OPENEXR_VIEWERS_ENABLE=0" -D "ZLIB_INCLUDE_DIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=%AUTOREMESHER_BUILD_DIR%/thirdparty/zlib/zlib-1.2.11/build/Release/zlibstatic.lib" ../
cmake --build . --config Release
cd %AUTOREMESHER_BUILD_DIR%

cd thirdparty/tbb
mkdir build2
cd build2
cmake -G "Visual Studio 17 2022" -A x64 ../
cmake --build . --config Release
cd %AUTOREMESHER_BUILD_DIR%

curl -O https://archives.boost.io/release/1.66.0/source/boost_1_66_0.zip
tar -xf boost_1_66_0.zip
mkdir %AUTOREMESHER_BUILD_DIR%\Libraries
echo D | xcopy boost_1_66_0 %AUTOREMESHER_BUILD_DIR%\Libraries\boost_1_66_0 /s /e /q /y

cd thirdparty/openvdb/openvdb-7.0.0
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -D "OPENVDB_CORE_STATIC=OFF" -D "OPENVDB_BUILD_CORE=ON" -D "OPENVDB_BUILD_BINARIES=ON" -D "IlmBase_INCLUDE_DIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/openexr/openexr-2.4.1" -D "IlmBase_Half_LIBRARY=%AUTOREMESHER_BUILD_DIR%/thirdparty/openexr/openexr-2.4.1/build/IlmBase/Half/Release/Half-2_4.lib" -D "TBB_INCLUDEDIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/tbb/include" -D "TBB_LIBRARYDIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/tbb/build2/Release" -D "ZLIB_INCLUDE_DIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/zlib/zlib-1.2.11" -D "ZLIB_LIBRARY=%AUTOREMESHER_BUILD_DIR%/thirdparty/zlib/zlib-1.2.11/build/Release/zlibstatic.lib" -D "Blosc_INCLUDE_DIR=%AUTOREMESHER_BUILD_DIR%/thirdparty/blosc/c-blosc-1.18.1" -D "Blosc_LIBRARY=%AUTOREMESHER_BUILD_DIR%/thirdparty/blosc/c-blosc-1.18.1/build/blosc/Release/libblosc.lib" -D "BOOST_INCLUDEDIR=%AUTOREMESHER_BUILD_DIR%/Libraries/boost_1_66_0" -D "OPENVDB_DISABLE_BOOST_IMPLICIT_LINKING=ON" -D "CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=%AUTOREMESHER_BUILD_DIR%/Libraries/boost_1_66_0;%AUTOREMESHER_BUILD_DIR%/thirdparty/blosc/c-blosc-1.18.1/blosc;%AUTOREMESHER_BUILD_DIR%/thirdparty/zlib/zlib-1.2.11/build" ../
cmake --build . --config Release
cmake --build . --target install
cd %AUTOREMESHER_BUILD_DIR%
if not exist "release" mkdir release
echo D | xcopy thirdparty\openvdb\openvdb-7.0.0 release\openvdb-7.0.0 /s /e /q /y
