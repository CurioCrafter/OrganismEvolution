# Setup Guide for OrganismEvolution (Windows DX12)

This project targets Windows with DirectX 12 and requires MSVC.

## Step 1: Install Visual Studio

Install Visual Studio 2019 or later with:
- "Desktop development with C++" workload
- Windows 10/11 SDK (included by default)

## Step 2: Install CMake

Download CMake from https://cmake.org/download/ and add it to your PATH, or use the version bundled with Visual Studio.

## Step 3: Verify Tools

Open a Visual Studio Developer Command Prompt and run:

```batch
cl
cmake --version
```

Both commands should produce output. If `cl` is not found, you are not in a Developer Command Prompt.

## Step 4: Build the Project

```batch
cd C:\Users\andre\Desktop\OrganismEvolution

cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Release
```

## Step 5: Run

```batch
build_dx12\Release\OrganismEvolution.exe
```

## Common Issues

### "DX12 device creation failed"

Update GPU drivers and verify the GPU supports DirectX 12.

### "Shader not found"

Make sure `Runtime/Shaders` exists next to the executable. Rebuilding the project should copy these automatically.

### "CMake generator not found"

Run CMake from a Visual Studio Developer Command Prompt, or ensure the correct Visual Studio version is installed.

### Linker errors

Ensure you have the Windows SDK installed as part of the Visual Studio C++ workload.
