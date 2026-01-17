# OrganismEvolution Documentation Index

This directory contains all project documentation, organized into logical categories.

> **IMPORTANT:** This project has significant integration gaps. Many systems are implemented but not connected. See [INTEGRATION_AUDIT.md](research/INTEGRATION_AUDIT.md) for the honest assessment of 46 identified issues.

> **Note:** This index was updated on January 16, 2026 to reflect actual project state.

## Quick Navigation

| Category | Files | Description | Key Files |
|----------|-------|-------------|-----------|
| [User Guide](user_guide/) | 3 | End-user documentation | USER_MANUAL, CONTROLS, DEVELOPER_GUIDE |
| [Features](features/) | 4 | Feature specs (some partial) | Flying/Aquatic Creatures, Ecosystem, Genetics |
| [Implementation](implementation/) | 20 | Design & architecture docs | System designs, NEAT, animation |
| [Research](research/) | 13 | Technical research | **INTEGRATION_AUDIT.md (critical)**, analysis |
| [Performance](performance/) | 3 | Optimization guides | Benchmarks, optimization strategies |
| [Issues & Fixes](issues_and_fixes/) | 9 | Bug tracking & fixes | **KNOWN_ISSUES.md**, PROJECT_ISSUES.md |
| [Testing](testing/) | 2 | Test documentation | Test results, checklists |
| [Platform Specific](platform_specific/) | 5 | Platform docs | Win32 input system |
| [Phase Status](phase_status/) | 9 | Project phases | Phase completion reports |
| [Reference](reference/) | 5 | Planning & roadmap | Roadmap, comprehensive plans |
| [Agent Prompts](agent_prompts/) | 6 | AI agent prompts | Phase-specific agent instructions |
| [Archive](archive/) | 30 | Legacy/historical/aspirational | Deprecated, unimplemented, historical |

---

## Honest Feature Matrix (Updated Jan 16, 2026)

| Feature | Code Status | Integration Status | Actually Works? |
|---------|-------------|-------------------|-----------------|
| **DirectX 12 Rendering** | Complete | Integrated | YES |
| **Procedural Terrain** | Complete | Integrated | YES |
| **Grass/Tree Rendering** | Complete | Integrated | YES |
| **Water Rendering** | Complete | Integrated | YES |
| **Creature Simulation** | Complete | Integrated | YES (basic) |
| **ImGui Debug Panel** | Complete | Integrated | YES |
| **Day/Night Cycle** | Complete | Integrated | YES (visual only) |
| **Camera System** | Complete | Integrated | YES |
| **Save/Load System** | Complete | Integrated | **YES - F5/F9 work** |
| **Replay System** | Complete | Integrated | **YES - F10 works** |
| **GPU Compute Steering** | Complete | Integrated (disabled) | CPU fallback active |
| **Neural Network Brains** | Complete | **NOT INTEGRATED** | NO - never called |
| **NEAT Evolution** | Partial | **NOT INTEGRATED** | NO |
| **Weather Effects** | Complete | **DISCONNECTED** | Partial - ignores biomes |
| **Climate System** | Complete | **DISCONNECTED** | Partial - static snapshot |
| **Vegetation Biomes** | Complete | **DISCONNECTED** | Partial - ignores climate |
| **Skeletal Animation** | Complete | Partial | Partial - basic rotation only |
| **Advanced Behaviors** | Scaffolded | **NOT INTEGRATED** | NO |
| **Audio System** | Scaffolded | **NOT INTEGRATED** | NO |
| **Multi-Island** | Headers only | **NOT IMPLEMENTED** | NO |

---

## Directory Structure

```
docs/
├── user_guide/           # End-user documentation (3 files)
│   ├── USER_MANUAL.md         # Complete user guide
│   ├── CONTROLS.md            # Controls reference
│   └── DEVELOPER_GUIDE.md     # Developer setup & practices
│
├── features/             # IMPLEMENTED feature documentation (4 files)
│   ├── FLYING_CREATURES.md    # Flying creature behavior & animation
│   ├── AQUATIC_CREATURES.md   # Aquatic creature behavior & swimming
│   ├── ECOSYSTEM_DYNAMICS.md  # Ecosystem balance & trophic levels
│   └── GENETICS_ANALYSIS.md   # Unified genetics system
│
├── implementation/       # Design & implementation guides (20 files)
│   ├── ANIMATION_ARCHITECTURE.md
│   ├── NEAT_ARCHITECTURE.md
│   ├── ECOSYSTEM_IMPLEMENTATION.md
│   ├── GENETICS_IMPLEMENTATION.md
│   ├── MORPHOLOGY_IMPLEMENTATION.md
│   ├── *_DESIGN.md (system designs)
│   └── *_PLAN.md (integration plans)
│
├── research/             # Technical research documents (13 files)
│   ├── COMPREHENSIVE_RESEARCH_DOCUMENT.md
│   ├── ANIMATION_RESEARCH.md
│   ├── NEAT_RESEARCH.md
│   ├── INTEGRATION_AUDIT.md   # Critical: Current system audit
│   ├── PERFORMANCE_ANALYSIS.md # Critical: Optimization roadmap
│   └── (various system research)
│
├── performance/          # Performance & optimization (3 files)
│   ├── PERFORMANCE.md
│   ├── PERFORMANCE_OPTIMIZATION.md
│   └── OPTIMIZATION_PLAN.md
│
├── issues_and_fixes/     # Bug tracking & fixes (7 files)
│   ├── PROJECT_ISSUES.md      # Critical: Structural audit findings
│   ├── KNOWN_ISSUES.md        # Current known issues
│   ├── BUG_FIXES.md           # Applied bug fixes
│   ├── AI_INTEGRATION_FIXES.md
│   ├── CODE_REVIEW_FIXES.md
│   ├── DX12_RENDERER_FIXES.md
│   └── VEGETATION_FIX_SUMMARY.md
│
├── testing/              # Test documentation (2 files)
│   ├── TEST_RESULTS.md
│   └── TESTING_CHECKLIST.md
│
├── platform_specific/    # Platform-specific docs (5 files)
│   ├── WIN32_INPUT_README.md
│   ├── WIN32_INPUT_CHECKLIST.md
│   ├── WIN32_INPUT_CODE_EXAMPLES.md
│   ├── WIN32_INPUT_DIAGNOSTICS.md
│   └── WIN32_INPUT_VISUAL_REFERENCE.md
│
├── phase_status/         # Project phase tracking (9 files)
│   ├── PHASE2_COMPLETE.md
│   ├── PHASE3_STATUS.md
│   ├── PHASE4_INTEGRATION_REPORT.md
│   ├── DEVELOPMENT_COMPLETE.md
│   └── FINAL_VERIFICATION.md
│
├── reference/            # Reference & planning (5 files)
│   ├── ROADMAP.md
│   ├── COMPREHENSIVE_PLAN.md
│   ├── DOCUMENTATION.md
│   ├── IMPROVEMENTS_SUMMARY.md
│   └── COMPREHENSIVE_TODO_DOCUMENTATION.md
│
├── agent_prompts/        # AI agent instructions (6 files)
│   └── AGENT_PROMPTS_PHASE*.md
│
└── archive/              # Legacy & historical (30 files)
    ├── BUILD_INSTRUCTIONS.md      # Legacy MSYS2 build
    ├── BUILD_LINUX.md             # Legacy cross-platform
    ├── PROJECT_PLAN.md            # Original OpenGL plan
    ├── API_REFERENCE.md           # Outdated API docs
    ├── DX12_RENDERING_BEST_PRACTICES.md  # Misleading status
    ├── WEBASSEMBLY.md             # Not implemented
    ├── DEPRECATED_OPENGL.md       # Migration complete
    ├── OPENGL_FILES_TO_REMOVE.md  # Migration complete
    ├── GLFWTIME_FIX.md            # GLFW no longer used
    ├── CREATURE_EXPANSION.md      # Aspirational/not implemented
    ├── GENETICS_SPECIATION.md     # Aspirational/not implemented
    ├── NEURAL_NETWORKS.md         # Aspirational/not implemented
    ├── SENSORY_SYSTEMS.md         # Aspirational/not implemented
    ├── MORPHOLOGY_EVOLUTION.md    # Aspirational/not implemented
    ├── WATER_SYSTEM_COMPREHENSIVE.md  # Outdated shader refs
    └── *_AGENT*.md                # Agent investigation logs
```

---

## Getting Started

1. **New Users**: Start with [user_guide/USER_MANUAL.md](user_guide/USER_MANUAL.md)
2. **Developers**: See [user_guide/DEVELOPER_GUIDE.md](user_guide/DEVELOPER_GUIDE.md)
3. **Controls Reference**: [user_guide/CONTROLS.md](user_guide/CONTROLS.md)
4. **Known Issues**: [issues_and_fixes/KNOWN_ISSUES.md](issues_and_fixes/KNOWN_ISSUES.md)
5. **Project Roadmap**: [reference/ROADMAP.md](reference/ROADMAP.md)

---

## Critical Documents - READ THESE FIRST

These documents contain the honest assessment of the project state:

| Document | Location | Why It's Critical |
|----------|----------|-------------------|
| **INTEGRATION_AUDIT.md** | research/ | **46 identified issues** (7 critical) - systems built but not connected |
| **PROJECT_ISSUES.md** | issues_and_fixes/ | Build config issues, data integrity problems |
| **KNOWN_ISSUES.md** | issues_and_fixes/ | Honest summary of critical and high-priority issues |
| KNOWN_ISSUES.md | **Root level** | More current version with workarounds |
| PERFORMANCE_ANALYSIS.md | research/ | Bottleneck identification & optimization roadmap |

### Key Findings (Updated Jan 16, 2026)

**Working (contrary to older audit docs):**
- Save/Load: F5/F9 work, auto-save every 5 minutes
- Replay system: F10 toggles replay, recording at 1 fps
- GPU compute: Integrated but disabled (CPU fallback active)

**Still Not Integrated:**
- **Neural networks**: Code exists but never called - behavior is hardcoded
- **Weather/Climate**: Systems disconnected - weather ignores biome data
- **Vegetation**: Uses height, ignores biome data

---

## Archive Contents

The `archive/` folder contains 30 documents that are:
- **Historical**: Old build systems (MSYS2, OpenGL, GLFW)
- **Aspirational/Not Implemented**: Features researched but **never actually built**:
  - `NEURAL_NETWORKS.md` - Describes features that don't work
  - `GENETICS_SPECIATION.md` - Species system not integrated
  - `SENSORY_SYSTEMS.md` - Sensory system doesn't feed neural network
  - `MORPHOLOGY_EVOLUTION.md` - Not implemented
- **Superseded**: Documentation replaced by newer versions
- **Completed**: Migration guides for finished transitions

**WARNING**: Do not use archive documents as feature references - they describe aspirational features, not reality.

---

## Root-Level Documentation

The following essential docs remain in the project root for visibility:

| File | Purpose |
|------|---------|
| `README.md` | Project overview & quick start |
| `QUICK_START.md` | Fast setup guide |
| `BUILD_AND_RUN.md` | Primary build instructions |
| `IMPORTANT_BUILD_NOTE.md` | Critical build requirements |
| `SETUP_GUIDE.md` | Environment setup guide |
