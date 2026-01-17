# Release Checklist

Use this checklist before sharing the project or creating a release.

## Pre-Release Verification

### Build Verification
- [ ] Delete build_dx12 directory and rebuild from scratch
- [ ] Build completes with no errors (warnings OK)
- [ ] Executable size is reasonable (~1MB)
- [ ] All 8 shaders are copied to Runtime/Shaders in build directory

### Functionality Testing
- [ ] Application launches without crash
- [ ] 3D terrain is visible
- [ ] Creatures spawn and move
- [ ] Camera controls work (WASD, mouse drag)
- [ ] F1 opens debug panel
- [ ] Spawn buttons create creatures
- [ ] Pause (P) works
- [ ] Quick save (F5) and load (F9) work
- [ ] Day/night cycle visible over time

### Performance Testing
- [ ] Runs at 60+ FPS with ~1000 creatures
- [ ] Runs at 30+ FPS with ~5000 creatures
- [ ] GPU steering kicks in above 200 creatures
- [ ] No obvious memory leaks during extended run

### Documentation Check
- [ ] README.md has accurate feature status
- [ ] QUICK_START.md build instructions work
- [ ] KNOWN_ISSUES.md lists current limitations
- [ ] No documentation claims features that don't work

## What to Tell Users

### This Works Well
- DirectX 12 rendering with terrain, water, grass, trees
- Creature simulation with multiple types
- GPU-accelerated steering for large populations
- Save/load and replay system
- Performance profiler and debug tools
- Camera controls and follow modes

### Set Expectations About
- Evolution is basic trait inheritance, not full genetic algorithms
- Neural networks exist in code but don't drive behavior
- Many subsystems (audio, disasters, climate effects) are scaffolded but not active
- Advanced behaviors (territorial, social, pack hunting) are not integrated
- Performance degrades above ~5000 creatures

### Don't Demo
- UI Dashboard (disabled)
- Audio (no sound)
- Multi-island mode (not integrated)
- NEAT evolution (infrastructure only)
- Natural disasters (never triggered)

## Safe Demo Script

1. Launch application
2. Wait 2 seconds for loading
3. Press F1 to show debug panel
4. Show terrain, water, grass, trees by moving camera
5. Spawn 100 herbivores, 20 carnivores
6. Show day/night cycle progression
7. Demonstrate pause/resume
8. Show quick save/load
9. Press F2 for performance profiler
10. Demonstrate camera follow mode on a creature

## Files to Include in Release

```
OrganismEvolution/
├── build_dx12/Release/
│   ├── OrganismEvolution.exe     # Main executable
│   └── Runtime/Shaders/          # All 8 HLSL files
├── README.md                      # Main documentation
├── QUICK_START.md                 # Build instructions
├── KNOWN_ISSUES.md                # Limitations
├── LICENSE                        # MIT license
└── (source code if distributing)
```

## Version Information

- **Build Date**: January 16, 2026
- **Compiler**: MSVC 19.44.35222.0
- **C++ Standard**: C++20
- **DirectX**: 12
- **Platform**: Windows x64

## Post-Release Notes

After sharing:
- Monitor for crash reports
- Document any new issues found by users
- Note performance on different hardware
- Track feature requests vs. what's actually possible

---

*Checklist created: January 16, 2026*
