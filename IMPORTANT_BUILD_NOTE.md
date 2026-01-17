# IMPORTANT: How to Build This Project

This project is Windows-only and uses DirectX 12. You must use Visual Studio with MSVC.

## Required

- Windows 10 or later
- Visual Studio 2019+ with "Desktop development with C++" workload
- Windows SDK (10/11)
- CMake 3.15+

## Terminal

Use one of:
- "x64 Native Tools Command Prompt for VS"
- "Developer PowerShell for VS"

These provide the correct MSVC toolchain and SDK paths.

## Build

```batch
cd C:\Users\andre\Desktop\OrganismEvolution

cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Release
```

## Run

```batch
build_dx12\Release\OrganismEvolution.exe
```

## More Information

- [QUICK_START.md](QUICK_START.md) - Fast setup
- [BUILD_AND_RUN.md](BUILD_AND_RUN.md) - Detailed build instructions
- [SETUP_GUIDE.md](SETUP_GUIDE.md) - Environment setup
