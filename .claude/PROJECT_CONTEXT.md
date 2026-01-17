# Project Context for Claude Sessions

This repository builds the DirectX 12 implementation. Legacy OpenGL code is kept
under `deprecated/opengl` for reference only.

## Source of Truth (avoid confusion)

- The CMake target builds `src/main.cpp` using the Forge RHI (DX12).
- Runtime shaders are loaded from `Runtime/Shaders/` (copied next to the exe).
- `SimulationDemo/` contains additional docs and references, not the active build.

## Build and Run (short version)

- Follow `.claude/BUILD_INSTRUCTIONS.md` for the MSVC build commands.
- Run the executable from `build_dx12/Debug` or `build_dx12/Release`.

## Entry Point and Main Loop

- `src/main.cpp` handles window creation, input, simulation update, rendering,
  and the ImGui debug panel.

## Runtime Controls (main.cpp)

- Camera: W/A/S/D, mouse look (click to capture), Q/E or Space/C/Ctrl for
  vertical movement, Shift for faster movement.
- Simulation: P toggles pause; speed is adjusted in the ImGui panel.
- Debug: F3 toggles the ImGui debug panel.
- ESC releases the mouse and exits.

## System Map (where to edit)

- Active DX12 path: `src/main.cpp` plus Forge RHI (`ForgeEngine/*`).
- Additional systems exist in `src/*` but are not compiled by default; wire them
  into CMake if you want to use them.
- Legacy OpenGL systems are in `deprecated/opengl` and should not be modified.

## DX12 References

- `docs/DX12_RENDERING_BEST_PRACTICES.md`
- `SimulationDemo/docs/RENDERING.md`
- `SimulationDemo/docs/DX12_PIPELINE_GUIDE.md`
- `SimulationDemo/docs/DX12_RESOURCE_MANAGEMENT.md`
- `SimulationDemo/docs/HLSL_BEST_PRACTICES.md`

## External References

- DirectX-Graphics-Samples (HelloTriangle, HelloTexture)
- Microsoft Learn D3D12 documentation and debug layer guides
- DirectXTK12 DeviceResources pattern
- PIX for Windows
