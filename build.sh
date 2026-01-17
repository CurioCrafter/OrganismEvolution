#!/bin/bash
# Build script for OrganismEvolution

echo "Building OrganismEvolution..."
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make -j4

echo "Copying required DLLs..."
cp /c/msys64/mingw64/bin/glfw3.dll .
cp /c/msys64/mingw64/bin/glew32.dll .
cp /c/msys64/mingw64/bin/libgcc_s_seh-1.dll .
cp /c/msys64/mingw64/bin/libstdc++-6.dll .
cp /c/msys64/mingw64/bin/libwinpthread-1.dll .

echo "Build complete! Run OrganismEvolution.exe"
