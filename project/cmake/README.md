# Kodi CMake based buildsystem

## Dependencies

Before building Kodi with CMake, please ensure that you have the platform
specific dependencies installed.

For Linux the required dependencies can be found in
[docs/README.xxx](https://github.com/xbmc/xbmc/tree/master/docs).

For Windows the dependencies can be found in the
[Wiki](http://kodi.wiki/view/HOW-TO:Compile_Kodi_for_Windows) (Step 1-4).

For OSX the required dependencies can be found in
[docs/README.osx](https://github.com/xbmc/xbmc/tree/master/docs/README.osx).

## Building Kodi

In order to configure Kodi with CMake execute the following.
For an out-of-source build and Kodi cloned into a `kodi` directory.

```
mkdir kodi-build
cd kodi-build
cmake ../kodi/project/cmake
```

Then, to start the build on Linux:

```
make -j$(nproc)
```

On Windows:

```
nmake
```

For OSX the Toolchain file that is generated in the dependency build has to be
passed to CMake:

```
mkdir kodi-build
cd kodi-build
cmake -DCMAKE_TOOLCHAIN_FILE=../kodi/tools/depends/target/Toolchain.cmake \
      ../kodi/project/cmake
make -j$(sysctl -n hw.ncpu)
```

## Debugging the build

This section covers some tips that can be useful for debugging a CMake
based build.

### Verbosity (show compiler and linker parameters)

In order to see the exact compiler commands `make` and `nmake` can be
executed with a `VERBOSE=1` parameter.

On Windows, this is unfortunately not enough because `nmake` uses
temporary files to workaround `nmake`'s command string length limitations.
In order to see verbose output the file
[Modules/Platform/Windows.cmake](https://github.com/Kitware/CMake/blob/master/Modules/Platform/Windows.cmake#L40)
in the local CMake installation has to be adapted by uncommenting these
lines:

```
# uncomment these out to debug nmake and borland makefiles
#set(CMAKE_START_TEMP_FILE "")
#set(CMAKE_END_TEMP_FILE "")
#set(CMAKE_VERBOSE_MAKEFILE 1)
```
