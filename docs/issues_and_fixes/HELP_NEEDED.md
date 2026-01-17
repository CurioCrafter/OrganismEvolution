# Help Needed Summary (Repo Scan)

Date: 2026-01-15
Scope: Static repository scan only (docs + source). No build or runtime validation.

This is a prioritized list of the most important areas where help is needed,
based on evidence in code and existing documentation. It is intentionally
condensed and not exhaustive.

## 1) Build + Integration Clarity (highest priority)

The repo contains both a monolithic entry point (`src/main.cpp`) and a large
modular codebase under `src/`, plus multiple DX12 abstractions. Docs disagree
about what actually builds and runs, and there is a second CMake entry that
only compiles `main.cpp` + ForgeEngine + ImGui.

Evidence:
- `docs/phase_status/PHASE3_STATUS.md` (modular systems exist but not integrated)
- `docs/phase_status/CLEANUP_STATUS.md` (DX12 files called "not in current build")
- `CMakeLists_DX12.txt` (only `main.cpp` + ForgeEngine + ImGui)
- `CMakeLists.txt` (builds both monolithic and modular sources together)

Help needed:
- Decide on the authoritative build target(s): monolithic vs modular.
- Align CMake + docs to that choice (or remove obsolete configs).
- Clarify which DX12 device path is authoritative (ForgeEngine RHI vs
  `src/graphics/DX12Device.*`).

## 2) Save/Load + Replay Correctness

The save header still writes placeholder values (generation, terrain seed, RNG),
and replay snapshots are missing real neural state. ID counters are not updated
after load, risking ID reuse.

Evidence:
- `src/main.cpp:2315` (generation placeholder)
- `src/main.cpp:2317` (terrain seed placeholder)
- `src/main.cpp:2323` (RNG state placeholder)
- `src/main.cpp:2697` (water level placeholder)
- `docs/issues_and_fixes/PROJECT_ISSUES.md`
- `docs/phase_status/PHASE4_INTEGRATION_REPORT.md` (replay weights missing)
- `src/core/Serializer.h:177` + `src/core/Serializer.h:188` (no bounds checks)

Help needed:
- Persist real world state (generation, RNG, terrain, water) and update ID
  counters on load.
- Capture real genome/neural weights in replay frames.
- Add size/bounds checks to binary deserialization for robustness.

## 3) Rendering Pipeline Gaps (DX12)

Post-processing is disabled because PSOs are not created, and ForgeEngine RHI
still contains TODO stubs for compute pipelines and render passes. There is
also a TODO note about debugging/replacing the compute shader path.

Evidence:
- `ForgeEngine/RHI/DX12/DX12Device.cpp:1096` (compute pipeline TODO)
- `ForgeEngine/RHI/DX12/DX12Device.cpp:1408` (render pass TODO)
- `docs/phase_status/PHASE4_INTEGRATION_REPORT.md` (post-process PSOs missing)
- `src/main.cpp:3833` (compute shader debug TODO)

Help needed:
- Implement compute pipeline + render pass in ForgeEngine RHI.
- Implement post-process PSO creation and root signatures.
- Validate the steering compute shader path end-to-end.

## 4) Environment + Climate Integration

Environment systems exist but are not consistently wired into the simulation
loop, and `ClimateSystem::update()` is effectively empty.

Evidence:
- `docs/phase_status/PHASE4_INTEGRATION_REPORT.md`
- `docs/phase_status/FINAL_VERIFICATION_CHECKLIST.md`
- `docs/implementation/CLIMATE_DYNAMICS_PLAN.md`

Help needed:
- Initialize and update `VegetationManager`, `TerrainChunkManager`,
  `ClimateSystem`, and `WeatherSystem` in the main loop.
- Implement the climate update logic described in the plan.

## 5) AI Integration Fixes (NEAT + modular brain)

Multiple critical AI issues are documented as "needs fix" (eligibility decay,
NEAT crossover rules, mutation probability bug, class name collisions, and
sensory type mismatches).

Evidence:
- `docs/issues_and_fixes/AI_INTEGRATION_FIXES.md`

Help needed:
- Apply the listed fixes.
- Resolve the two `NeuralNetwork` class collision (naming or namespaces).
- Add a bridge from `SensorySystem` to `ai::SensoryInput`.
- Tighten AI tests to avoid false positives.

## 6) Testing + Verification Automation

Tests exist but are not wired into CMake/CTest, and verification checklists
are not marked complete.

Evidence:
- `docs/testing/TEST_RESULTS.md` (expected results, not execution logs)
- `docs/testing/TESTING_CHECKLIST.md` (unchecked)
- `CMakeLists.txt` (no `enable_testing()` or `add_subdirectory(tests)`)

Help needed:
- Add test targets to CMake and run them regularly.
- Populate actual test results and complete verification checklist.
- Optionally add CI to enforce test runs.

## 7) Documentation Drift + Repo Hygiene

Docs conflict with code and each other (e.g., shader debug mode vs actual
shader content), and repo hygiene issues make tooling harder.

Evidence:
- `docs/issues_and_fixes/KNOWN_ISSUES.md` (claims shader DEBUG mode on)
- `shaders/hlsl/Creature.hlsl` + `Runtime/Shaders/Creature.hlsl` (no debug mode)
- `README.md` (links to `BUILD_INSTRUCTIONS.md`, which is in `docs/archive/`)
- `docs/phase_status/FINAL_VERIFICATION.md` vs other status docs
- `build_dx12/` artifacts tracked, `imgui.ini` tracked, root `nul` file

Help needed:
- Pick a source-of-truth shader directory (`Runtime/Shaders` vs `shaders/hlsl`)
  and align docs/build accordingly.
- Clean or ignore build artifacts and user state files.
- Remove or rename the `nul` file to avoid tool errors.
- Reconcile conflicting docs (status, build instructions, known issues).

## Open Decisions (owner needed)

1) Which CMake entry is authoritative for the DX12 build?
2) Which shader directory is the source of truth?
3) Which DX12 abstraction should be kept long-term (ForgeEngine RHI vs
   `src/graphics/DX12Device.*`)?
