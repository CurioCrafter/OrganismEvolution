# Feature Expansion Plan
## OrganismEvolution - Future Feature Wishlist & Technical Requirements

### Document Version
- **Created**: January 2026
- **Last Updated**: January 2026
- **Scope**: Feature roadmap and technical feasibility analysis

---

## Feature Categories

1. [Creature Variety](#1-creature-variety)
2. [Environment Systems](#2-environment-systems)
3. [Ecosystem Complexity](#3-ecosystem-complexity)
4. [Player Interaction](#4-player-interaction)
5. [Visualization & UI](#5-visualization--ui)
6. [Save/Load & Persistence](#6-saveload--persistence)
7. [Multiplayer & Sharing](#7-multiplayer--sharing)

---

## 1. Creature Variety

### 1.1 Flying Creatures (Partially Implemented)
**Status**: Basic implementation exists
**Complexity**: Medium

#### Current State
- Flying creatures exist with `wingSpan`, `flapFrequency`, `glideRatio`
- Basic altitude maintenance via `updateBehaviorFlying()`
- Can hunt and eat food

#### Expansion Requirements
```
Technical Requirements:
├── Physics
│   ├── Lift/drag simulation
│   ├── Thermal detection (rising air currents)
│   ├── Wind effects on flight path
│   └── Energy cost based on flight mode (flapping vs gliding)
│
├── AI Behaviors
│   ├── Diving attacks on ground prey
│   ├── Aerial hunting (bird vs bird)
│   ├── Roosting/perching behavior
│   ├── Migration patterns
│   └── Flock formation flying
│
├── Rendering
│   ├── Wing animation (flapping, folding, gliding poses)
│   ├── Feather/membrane effects
│   ├── Motion blur for fast movement
│   └── Shadow casting from above
│
└── Genetics
    ├── Wing shape genes (aspect ratio affects flight style)
    ├── Feather coloration
    └── Beak/claw morphology
```

#### Dependencies
- Improved physics system for aerodynamics
- 3D spatial queries (current grid is 2D)
- Animation system for wing states

### 1.2 Aquatic Creatures (Partially Implemented)
**Status**: Basic implementation exists
**Complexity**: Medium

#### Current State
- Aquatic creatures exist with fin/tail traits
- Basic schooling behavior
- Confined to water regions

#### Expansion Requirements
```
Technical Requirements:
├── Physics
│   ├── Buoyancy simulation
│   ├── Current/flow effects
│   ├── Pressure at depth
│   └── Oxygen levels affecting stamina
│
├── AI Behaviors
│   ├── Depth preferences (benthic, pelagic, surface)
│   ├── Schooling with predator avoidance
│   ├── Hunting from above/below
│   ├── Coral/reef interaction
│   └── Migratory routes
│
├── Rendering
│   ├── Underwater lighting/caustics
│   ├── Swim animation blending
│   ├── Scale/fin detail at LOD 0
│   └── Bubble trails
│
├── Environment
│   ├── Water depth map
│   ├── Underwater terrain
│   ├── Aquatic vegetation
│   └── Coral reef generation
│
└── Genetics
    ├── Body shape (streamlined vs flat)
    ├── Coloration (camouflage, warning, display)
    └── Bioluminescence trait
```

#### Dependencies
- Volumetric water system (not just surface)
- 3D pathfinding underwater
- Separate underwater food web

### 1.3 Amphibious Creatures
**Status**: Not implemented
**Complexity**: High

#### Requirements
```
New Systems Needed:
├── Transition physics (land ↔ water)
├── Dual movement system (walking + swimming)
├── Breath/oxygen management
├── Amphibious reproduction (eggs in water)
└── Metamorphosis system (tadpole → frog style)

Genetic Traits:
├── Water tolerance (time out of water)
├── Lung/gill balance
├── Skin moisture requirements
└── Limb adaptation (webbed feet)
```

### 1.4 Burrowing/Underground Creatures
**Status**: Not implemented
**Complexity**: Very High

#### Requirements
```
New Systems Needed:
├── Underground terrain mesh
├── Tunnel network generation
├── 3D pathfinding through solid terrain
├── Darkness adaptation mechanics
├── Soil types affecting digging speed
└── Tunnel collapse physics

Creature Types:
├── True burrowers (moles) - create tunnels
├── Den users (rabbits) - use existing holes
└── Ambush predators (trapdoor spiders)

Technical Challenges:
├── Dynamic mesh modification for tunnels
├── Visibility culling for underground
├── Memory for tunnel network storage
└── Integration with surface ecosystem
```

### 1.5 Plant-Animal Hybrids (Photosynthetic Creatures)
**Status**: Not implemented
**Complexity**: Medium

#### Requirements
```
New Mechanics:
├── Photosynthesis energy gain (based on sun exposure)
├── Root systems (optional anchoring)
├── Seasonal dormancy
└── Hybrid feeding (sun + food)

Genetic Traits:
├── Chlorophyll coverage (0-1)
├── Mobility vs photosynthesis tradeoff
└── Seed dispersal method
```

---

## 2. Environment Systems

### 2.1 Weather System
**Status**: Basic day/night cycle exists
**Complexity**: Medium

#### Current State
- `dayNightCycle` and `weatherCycle` floats in Simulation
- Affects visibility in sensory system

#### Expansion Requirements
```
Weather Types:
├── Rain
│   ├── Visual: Particle system, wet surfaces
│   ├── Gameplay: Reduced visibility, louder sounds
│   └── Ecosystem: Increased plant growth
│
├── Wind
│   ├── Visual: Grass/tree sway, particle direction
│   ├── Gameplay: Affects flying creatures, scent trails
│   └── Ecosystem: Seed dispersal
│
├── Temperature
│   ├── Visual: Heat shimmer, frost effects
│   ├── Gameplay: Metabolism rates, activity levels
│   └── Ecosystem: Seasonal availability
│
├── Storms
│   ├── Visual: Lightning, heavy rain, fog
│   ├── Gameplay: Danger zones, shelter seeking
│   └── Ecosystem: Population events
│
└── Implementation
    ├── Weather state machine
    ├── Transition blending
    ├── Regional weather differences
    └── Weather prediction (creatures react to approaching storms)
```

### 2.2 Advanced Day/Night Cycle
**Status**: Basic float value
**Complexity**: Low-Medium

#### Expansion Requirements
```
Visual:
├── Sun position based on time
├── Moon phases
├── Star rendering at night
├── Twilight/dawn gradients
└── Artificial light sources (bioluminescence)

Gameplay:
├── Nocturnal creatures
├── Diurnal creatures (sleep at night)
├── Crepuscular creatures (dawn/dusk active)
├── Vision range changes with light
└── Hunting strategy changes

Ecosystem:
├── Predator/prey role reversals at night
├── Nighttime flowering plants
└── Sleep behavior + vulnerability
```

### 2.3 Biome System
**Status**: Single terrain type
**Complexity**: High

#### Requirements
```
Biome Types:
├── Grassland (current default)
├── Forest (dense vegetation, limited visibility)
├── Desert (sparse food, extreme temperature)
├── Tundra (seasonal, harsh winter)
├── Jungle (high biodiversity, layered canopy)
├── Wetland (mixed land/water)
└── Mountains (altitude effects, terrain difficulty)

Per-Biome Parameters:
├── Vegetation density + types
├── Food availability patterns
├── Temperature range
├── Water availability
├── Terrain traversal cost
└── Natural shelter locations

Transitions:
├── Gradient biome blending
├── Ecotone behaviors (edge specialists)
└── Migration between biomes
```

### 2.4 Geological Events
**Status**: Not implemented
**Complexity**: High

#### Requirements
```
Event Types:
├── Earthquakes (terrain deformation, creature panic)
├── Volcanic activity (new terrain, destruction)
├── Floods (water level changes)
├── Landslides (terrain blocking)
└── Erosion (gradual terrain change)

Implementation:
├── Terrain modification system
├── Event scheduling (random or triggered)
├── Creature response behaviors
└── Long-term ecosystem recovery
```

---

## 3. Ecosystem Complexity

### 3.1 Food Web Expansion
**Status**: Herbivore → Plant, Carnivore → Herbivore
**Complexity**: Medium

#### Expansion Requirements
```
Trophic Levels:
├── L1: Producers (plants, algae, phytoplankton)
├── L2: Primary consumers (herbivores, filter feeders)
├── L3: Secondary consumers (small predators)
├── L4: Tertiary consumers (apex predators)
├── L5: Decomposers (already partially implemented)
└── Omnivores (cross-level feeders)

New Feeding Strategies:
├── Scavenging (eat corpses)
├── Parasitism (slow drain, host dependency)
├── Filter feeding (passive food collection)
├── Symbiosis (mutual benefit relationships)
└── Kleptoparasitism (steal food from others)
```

### 3.2 Reproduction Expansion
**Status**: Asexual splitting with crossover
**Complexity**: Medium-High

#### Requirements
```
Reproduction Modes:
├── Sexual reproduction (current, with mate selection)
├── Asexual reproduction (cloning with mutation)
├── Budding (attached offspring)
├── Egg laying (vulnerable offspring phase)
├── Live birth (internal development)
└── Metamorphosis (larval → adult stages)

Mating Systems:
├── Monogamy
├── Polygyny (one male, many females)
├── Polyandry (one female, many males)
├── Lekking (display arenas)
└── Tournament (combat for mates)

Parental Care:
├── None (lay and leave)
├── Guarding (protect eggs/young)
├── Feeding (bring food to young)
├── Teaching (transfer learned behaviors)
└── Social groups (pack/herd raising)
```

### 3.3 Social Behavior
**Status**: Basic flocking/schooling
**Complexity**: High

#### Requirements
```
Social Structures:
├── Solitary (current default)
├── Pair bonds
├── Family groups
├── Herds/flocks (already basic implementation)
├── Packs (cooperative hunting)
├── Colonies (eusocial, specialized roles)
└── Hierarchies (dominance systems)

Communication:
├── Visual signals (displays, postures)
├── Auditory (alarm calls - implemented, songs)
├── Chemical (pheromones - basic implementation)
├── Tactile (grooming, fighting)
└── Electrical (aquatic)

Cooperation:
├── Mobbing predators
├── Cooperative hunting
├── Food sharing
├── Alloparenting
└── Sentinel behavior
```

### 3.4 Speciation System
**Status**: Basic genetics/diploid system exists
**Complexity**: High

#### Requirements
```
Current State:
├── DiploidGenome with alleles
├── Species.h exists
├── HybridZone.h exists

Expansion:
├── Reproductive isolation mechanics
│   ├── Geographic isolation (physical barriers)
│   ├── Behavioral isolation (mating preferences)
│   ├── Temporal isolation (breeding season differences)
│   └── Genetic incompatibility (hybrid sterility)
│
├── Speciation visualization
│   ├── Phylogenetic tree rendering
│   ├── Species color coding
│   ├── Hybrid zone mapping
│   └── Genetic distance metrics
│
└── Extinction tracking
    ├── Species death records
    ├── Last common ancestor
    └── Biodiversity index over time
```

---

## 4. Player Interaction

### 4.1 Creature Selection & Tracking
**Status**: Basic ray-cast selection (I key)
**Complexity**: Low

#### Expansion Requirements
```
Selection Features:
├── Click-to-select (not just I key)
├── Multi-select (box select, Ctrl+click)
├── Follow camera mode
├── Family tree view for selected creature
├── Lineage tracking ("watch descendants")
└── Bookmark creatures

Information Display:
├── Real-time stat overlay
├── Behavior state indicator
├── Relationship lines (mate, offspring, prey)
├── Historical graphs (energy, fitness over time)
└── Genome visualization
```

### 4.2 Environment Manipulation
**Status**: Not implemented
**Complexity**: Medium

#### Requirements
```
Terrain Tools:
├── Raise/lower terrain (sculpting)
├── Paint biomes
├── Add/remove water
├── Place obstacles
└── Create barriers

Entity Manipulation:
├── Spawn creatures (with genome editor)
├── Remove creatures
├── Place food
├── Trigger reproduction
└── Set creature properties (energy, position)

Time Controls:
├── Pause/resume (implemented)
├── Speed control (implemented)
├── Rewind (requires state history)
└── Branch timeline ("what if" scenarios)
```

### 4.3 Experiment Mode
**Status**: Not implemented
**Complexity**: High

#### Requirements
```
Scenario Setup:
├── Define starting conditions
├── Set hypothesis to test
├── Configure measurement metrics
├── Run multiple trials
└── Statistical analysis

Example Experiments:
├── "Does larger size lead to higher fitness?"
├── "Optimal predator/prey ratio?"
├── "Effect of mutation rate on adaptation speed?"
├── "Island biogeography test"
└── "Competitive exclusion principle"

Output:
├── Data export (CSV, JSON)
├── Graph generation
├── Report templates
└── Comparison between runs
```

---

## 5. Visualization & UI

### 5.1 Data Visualization Dashboard
**Status**: Basic ImGui dashboard (DashboardUI)
**Complexity**: Medium

#### Expansion Requirements
```
Graphs:
├── Population over time (per species)
├── Trait distribution histograms
├── Phylogenetic tree
├── Food web diagram
├── Spatial heatmaps (creature density)
├── Energy flow diagram
└── Fitness landscape visualization

Controls:
├── Filter by species/trait
├── Time range selection
├── Export to image/data
└── Overlay on 3D view
```

### 5.2 Genetic Visualization
**Status**: Not implemented
**Complexity**: Medium

#### Requirements
```
Views:
├── Chromosome map (gene locations)
├── Allele dominance diagram
├── Trait correlation matrix
├── Mutation history tree
├── Gene flow between populations
└── Hardy-Weinberg equilibrium check

Genome Editor:
├── Visual gene editing
├── Import/export genomes
├── Preset genomes
└── Mutation playground
```

### 5.3 Rendering Enhancements
**Status**: Basic forward rendering
**Complexity**: High

#### Requirements
```
Visual Quality:
├── Ambient occlusion
├── Soft shadows (PCF or VSM)
├── Screen-space reflections
├── Volumetric lighting
├── Bloom + tone mapping
└── Depth of field

Creature Rendering:
├── Fur/feather rendering
├── Subsurface scattering (skin)
├── Eye rendering (reflections)
├── Animation blending
└── Procedural detail textures

Environment:
├── Volumetric fog
├── Cloud rendering
├── Water caustics
├── Vegetation wind animation
└── Particle effects (dust, pollen)
```

---

## 6. Save/Load & Persistence

### 6.1 Save System
**Status**: Not implemented
**Complexity**: Medium

#### Requirements
```
Save Data:
├── All creature states (position, genome, brain weights)
├── Terrain state
├── Ecosystem metrics history
├── Random seed for reproducibility
├── Player camera position
└── Simulation settings

Format:
├── Binary for speed (protobuf, flatbuffers)
├── JSON for debugging/editing
├── Compression (zstd)
└── Versioning for backwards compatibility

Features:
├── Auto-save (configurable interval)
├── Quick save/load
├── Multiple save slots
├── Save thumbnails
└── Cloud save (future)
```

### 6.2 Replay System
**Status**: Not implemented
**Complexity**: High

#### Requirements
```
Recording:
├── Delta compression (only record changes)
├── Keyframes every N frames
├── Input recording (deterministic replay)
└── Memory-efficient storage

Playback:
├── Variable speed
├── Scrubbing timeline
├── Branch from any point
├── Overlay annotations
└── Export to video
```

---

## 7. Multiplayer & Sharing

### 7.1 Creature Sharing
**Status**: Not implemented
**Complexity**: Low-Medium

#### Requirements
```
Export:
├── Genome export (JSON/binary)
├── Creature "snapshot" (genome + brain weights)
├── Population sample
└── QR code generation

Import:
├── Validate genome compatibility
├── Inject into existing population
├── Create isolated population
└── Competition mode (imported vs local)
```

### 7.2 Networked Simulation
**Status**: Not implemented
**Complexity**: Very High

#### Requirements
```
Architecture Options:
├── Peer-to-peer (each client owns region)
├── Client-server (authoritative server)
├── Lockstep simulation (deterministic sync)
└── Eventual consistency (async sync)

Challenges:
├── Deterministic simulation (floating point issues)
├── Bandwidth for large creature counts
├── Latency hiding for interactions
├── Cheating prevention
└── Player drop-in/drop-out

Scope Options:
├── Shared world (MMO style)
├── Creature trading only
├── Competition mode (isolated sims, compare results)
└── Cooperative experiments
```

---

## Priority Matrix

| Feature | User Value | Technical Effort | Dependencies | Priority |
|---------|-----------|------------------|--------------|----------|
| Weather System | High | Medium | None | **P1** |
| Advanced Day/Night | Medium | Low | None | **P1** |
| Save/Load | High | Medium | None | **P1** |
| Data Viz Dashboard | High | Medium | ImGui (exists) | **P1** |
| Creature Sharing | Medium | Low | Save System | **P2** |
| Flying Expansion | Medium | Medium | 3D Spatial | **P2** |
| Aquatic Expansion | Medium | High | Water System | **P2** |
| Biome System | High | High | Terrain Refactor | **P2** |
| Social Behavior | High | High | AI Refactor | **P3** |
| Experiment Mode | Medium | High | Save System, Stats | **P3** |
| Speciation Viz | Medium | Medium | Genetics System | **P3** |
| Amphibious | Low | High | Multiple Systems | **P4** |
| Burrowing | Low | Very High | Terrain Mod | **P4** |
| Networked Sim | Low | Very High | Everything | **P5** |

---

## External Resources

### Similar Games/Simulations
- [Spore](https://www.spore.com/) - Creature evolution, procedural generation
- [Niche](https://store.steampowered.com/app/440650/Niche__a_genetics_survival_game/) - Genetics-focused survival
- [Ecosystem](https://store.steampowered.com/app/1176820/Ecosystem/) - Aquatic evolution simulation
- [Thrive](https://revolutionarygamesstudio.com/) - Open-source evolution game
- [The Bibites](https://leocaussan.itch.io/the-bibites) - Neural network creatures

### Academic References
- [Artificial Life Simulation Techniques](https://mitpress.mit.edu/9780262514064/artificial-life/)
- [Genetic Algorithms in Games](https://www.gamedev.net/tutorials/_/technical/artificial-intelligence/using-genetic-algorithms-r3043/)
- [Procedural Content Generation](http://pcgbook.com/)

### GDC Talks
- [AI-Driven Ecosystems in Games](https://www.gdcvault.com/)
- [Procedural Animation Techniques](https://www.gdcvault.com/)
- [Managing Simulation Complexity](https://www.gdcvault.com/)
