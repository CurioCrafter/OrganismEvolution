# Quick Start Guide (Windows DX12)

## 1. Prerequisites

- Windows 10 or later
- Visual Studio 2022 with "Desktop development with C++" workload (or VS 2019+)
- CMake 3.15+
- DirectX 12 compatible GPU

## 2. Open a Developer Command Prompt

**IMPORTANT**: You MUST use a Visual Studio Developer Command Prompt, not a regular command prompt.

Options:
- "Developer PowerShell for VS 2022" (recommended)
- "x64 Native Tools Command Prompt for VS 2022"
- "Developer Command Prompt for VS 2022"

**Why?** The build requires MSVC compiler and Windows SDK paths that are only set up in these environments.

## 3. Build and Run

```batch
cd C:\Users\andre\Desktop\OrganismEvolution

cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Release
build_dx12\Release\OrganismEvolution.exe
```

**Expected output**: A window opens showing a 3D terrain with creatures moving around. Press F1 for debug panel.

## 4. First Steps After Launch

1. **Look around**: Hold left mouse button and drag to rotate camera
2. **Move camera**: W/A/S/D to move, Q/E to go down/up, Shift for faster movement
3. **Open debug panel**: Press F1 to see creature stats and spawn controls
4. **Spawn creatures**: Use the spawn buttons in the debug panel
5. **Pause/Resume**: Press P to pause the simulation
6. **Quick save**: Press F5 to save, F9 to load

## Daily Development

For debug builds (slower but with debug symbols):

```batch
cmake --build build_dx12 --config Debug
build_dx12\Debug\OrganismEvolution.exe
```

## Clean Rebuild

If you encounter build issues:

```batch
rmdir /s /q build_dx12
cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
cmake --build build_dx12 --config Release
```

## Troubleshooting

### "CMake not found"
Make sure CMake is installed and in your PATH. Visual Studio 2022 includes CMake - use Developer PowerShell.

### "'cl' is not recognized"
You're not using a Developer Command Prompt. Open "Developer PowerShell for VS 2022" from the Start menu.

### Build errors about missing DirectX headers
Your Windows SDK may be too old. Update Visual Studio or install Windows SDK 10.0.19041.0 or later.

### Application crashes on startup
- Check that you have a DirectX 12 compatible GPU
- Update your graphics drivers
- Try running from the build directory so shaders can be found

### Black screen / no creatures
- Press F1 to open the debug panel
- Use "Spawn Herbivores" button to add creatures
- Check that terrain rendering is enabled

## Controls Reference

| Key | Action |
|-----|--------|
| W/A/S/D | Move camera |
| Mouse drag | Look around |
| Q/E | Down/Up |
| Shift | Move faster |
| P | Pause |
| F1 | Debug panel |
| F2 | Performance profiler |
| F5 | Quick save |
| F9 | Quick load |
| ESC | Exit |

---

*Verified: January 16, 2026*
