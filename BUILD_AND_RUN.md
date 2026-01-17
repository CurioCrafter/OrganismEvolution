# Build and Run Guide (Windows DX12)

This project uses DirectX 12 and requires MSVC on Windows.

## Prerequisites

- Windows 10 or later
- Visual Studio 2019 or later with "Desktop development with C++" workload
- Windows 10/11 SDK
- CMake 3.15+

## Quick Build (Recommended)

Open a Visual Studio Developer Command Prompt and run:

```batch
cd C:\Users\andre\Desktop\OrganismEvolution

:: Clean build from scratch
rmdir /s /q build
mkdir build
cd build

:: Configure with Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTS=OFF

:: Build Release
cmake --build . --config Release --parallel

:: Run
Release\OrganismEvolution.exe
```

## Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| BUILD_TESTS | ON | Build test executables (disable for faster builds) |

For debug builds:

```batch
cmake --build . --config Debug
Debug\OrganismEvolution.exe
```

## Build Output

The build produces:
- `build/Release/OrganismEvolution.exe` - Main executable
- `build/Release/Runtime/Shaders/` - HLSL shaders (auto-copied from source)

CMake automatically copies `Runtime/Shaders` next to the executable after build.

## Architecture Notes

### Build System
- **Single CMakeLists.txt** - One unified build configuration
- **MSVC required** - Uses MSVC-specific features like `_uuidof`

### Shader System
- **Runtime/Shaders/** - The authoritative shader location
- Shaders are copied to build directory automatically
- All code references `Runtime/Shaders/` paths

### DX12 Device Architecture
- **src/graphics/DX12Device.cpp** - Application-level DX12 wrapper
- **ForgeEngine/RHI/DX12/DX12Device.cpp** - ForgeEngine RHI abstraction
- Both exist intentionally - they serve different purposes

### USE_FORGE_ENGINE Define
- Always enabled (USE_FORGE_ENGINE=1)
- Controls GPU compute integration with ForgeEngine RHI
- AI/environment systems use DX12DeviceAdapter when enabled

## What You Should See

A window with procedural terrain, water, vegetation, and creatures moving around. Console output shows initialization messages.

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move camera |
| Mouse drag | Look around (click to capture) |
| Q/E | Move camera down/up |
| Shift | Move faster |
| F3 | Toggle debug panel |
| P | Pause/Resume simulation |
| ESC | Release mouse / Exit |

## Troubleshooting

### "Shader not found"

Ensure `Runtime/Shaders` exists next to the executable. Rebuild the project:
```batch
cmake --build . --config Release
```

### "DX12 device creation failed"

Update GPU drivers and verify the GPU supports DirectX 12.

### Build errors with MinGW

This project requires MSVC. If CMake picked MinGW:
```batch
rmdir /s /q build
cmake .. -G "Visual Studio 17 2022" -A x64
```

### Link errors (unresolved externals)

Ensure you're using the main `CMakeLists.txt` (not any legacy build files).

### CMake errors

Make sure you are using a Visual Studio Developer Command Prompt and that Visual Studio and the Windows SDK are installed correctly.
