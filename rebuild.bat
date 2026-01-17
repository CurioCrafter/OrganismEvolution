@echo off
echo ============================================
echo OrganismEvolution Build Script (Windows)
echo ============================================
echo.
echo This script must be run from MSYS2 MinGW 64-bit terminal
echo.
echo Please open: MSYS2 MinGW 64-bit
echo Then navigate to this folder and run:
echo   cd /c/Users/andre/Desktop/OrganismEvolution
echo   ./build.sh
echo.
echo Or run these commands manually:
echo   cd /c/Users/andre/Desktop/OrganismEvolution/build
echo   cmake .. -G "MinGW Makefiles"
echo   mingw32-make -j4
echo   cp /c/msys64/mingw64/bin/*.dll .
echo.
pause
