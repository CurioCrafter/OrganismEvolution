# OrganismEvolution User Manual

## Table of Contents

1. [Introduction](#1-introduction)
2. [Installation](#2-installation)
3. [Getting Started](#3-getting-started)
4. [Controls Reference](#4-controls-reference)
5. [User Interface Guide](#5-user-interface-guide)
6. [Understanding the Simulation](#6-understanding-the-simulation)
7. [Gameplay Tips](#7-gameplay-tips)
8. [Save and Load](#8-save-and-load)
9. [Troubleshooting](#9-troubleshooting)
10. [FAQ](#10-faq)

---

## 1. Introduction

### What is OrganismEvolution?

OrganismEvolution is a real-time 3D evolution simulator where creatures with randomly generated genetic traits compete for survival in a dynamic environment featuring islands, oceans, forests, and grasslands. The simulation uses genetic algorithms and neural networks to create emergent behaviors, allowing creatures to evolve across generations through natural selection.

Watch as herbivores learn to find food while avoiding predators, carnivores develop hunting strategies, fish school together in the ocean depths, and birds soar through the skies. Every creature has a unique neural network "brain" that controls its behavior, and successful traits are passed down to offspring through a sophisticated genetic inheritance system.

### Key Features

- **Procedural Terrain Generation**: Dynamic islands with varied biomes including grasslands, forests, beaches, and ocean depths
- **Genetic Algorithms**: Creatures inherit traits from their parents with realistic mutation and crossover mechanics
- **Neural Network Brains**: Each creature has a neural network that processes sensory inputs and determines behavior
- **Multiple Creature Types**: Herbivores, carnivores, flying creatures, and aquatic life all interact in a complex food web
- **Day/Night Cycle**: Dynamic lighting with dawn, noon, dusk, and night phases that affect creature behavior
- **Real-time Statistics**: Monitor population dynamics, genetic diversity, and ecosystem health
- **Save/Load System**: Save your simulations and continue watching evolution unfold later

### System Requirements

**Minimum Requirements:**
- Operating System: Windows 10 (64-bit) or Windows 11
- Processor: Intel Core i5 or AMD Ryzen 5 (4 cores)
- Memory: 8 GB RAM
- Graphics: DirectX 12 compatible GPU with 2 GB VRAM
  - NVIDIA GeForce GTX 1050 or better
  - AMD Radeon RX 560 or better
  - Intel UHD Graphics 620 or better
- Storage: 500 MB available space

**Recommended Requirements:**
- Operating System: Windows 10/11 (64-bit)
- Processor: Intel Core i7 or AMD Ryzen 7 (6+ cores)
- Memory: 16 GB RAM
- Graphics: DirectX 12 compatible GPU with 4+ GB VRAM
  - NVIDIA GeForce GTX 1660 or better
  - AMD Radeon RX 5600 or better
- Storage: 1 GB available space (for saves and logs)

---

## 2. Installation

### Prerequisites

Before installing OrganismEvolution, ensure you have the following:

1. **Windows 10 or Windows 11** (64-bit version required)
2. **DirectX 12 compatible GPU** with up-to-date drivers
3. **Visual C++ Redistributable** (2019 or later)

### Option A: Running Pre-built Executable

If you have a pre-built release:

1. Download the latest release from the project repository
2. Extract the ZIP archive to your desired location
3. Navigate to the extracted folder
4. Run `OrganismEvolution.exe`

**Important**: The `Runtime/Shaders` folder must be in the same directory as the executable.

### Option B: Building from Source

Building from source gives you the latest features and the ability to modify the simulation.

#### Step 1: Install Development Tools

1. **Install Visual Studio 2019 or later**
   - Download from [Visual Studio Downloads](https://visualstudio.microsoft.com/downloads/)
   - During installation, select "Desktop development with C++"
   - Ensure Windows 10/11 SDK is included

2. **Install CMake 3.15 or later**
   - Download from [CMake Downloads](https://cmake.org/download/)
   - Add CMake to your system PATH during installation

#### Step 2: Clone or Download the Source Code

```batch
git clone https://github.com/your-repo/OrganismEvolution.git
cd OrganismEvolution
```

Or download and extract the source ZIP archive.

#### Step 3: Build the Project

Open "Developer PowerShell for VS" or "x64 Native Tools Command Prompt for VS", then:

```batch
cd C:\path\to\OrganismEvolution

:: Configure the build
cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64

:: Build the project
cmake --build build_dx12 --config Debug
```

For a release build with optimizations:
```batch
cmake --build build_dx12 --config Release
```

#### Step 4: Run the Application

```batch
build_dx12\Debug\OrganismEvolution.exe
```

Or for release:
```batch
build_dx12\Release\OrganismEvolution.exe
```

### Troubleshooting Installation Issues

**"CMake not found" error:**
- Ensure CMake is installed and added to your system PATH
- Try running from a fresh Developer Command Prompt

**"Visual Studio generator not found" error:**
- Make sure Visual Studio is properly installed with C++ workload
- Update the generator name if using a different VS version:
  - VS 2019: `"Visual Studio 16 2019"`
  - VS 2022: `"Visual Studio 17 2022"`

**"Windows SDK not found" error:**
- Install Windows 10/11 SDK through Visual Studio Installer
- Or download separately from Microsoft

**Build fails with linker errors:**
- Perform a clean rebuild:
  ```batch
  rmdir /s /q build_dx12
  cmake -S . -B build_dx12 -G "Visual Studio 17 2022" -A x64
  cmake --build build_dx12 --config Debug
  ```

---

## 3. Getting Started

### Launching the Application

1. Navigate to the build directory or executable location
2. Double-click `OrganismEvolution.exe` or run from command line
3. A window will appear displaying the 3D simulation world
4. The console window shows startup messages and controls reference

### First-time Setup

On first launch, the simulation automatically:
- Generates procedural terrain with islands and ocean
- Spawns initial population of creatures (herbivores and carnivores)
- Distributes food sources across the terrain
- Begins the day/night cycle at dawn

No configuration is required to begin watching evolution unfold.

### Initial Camera Position

The camera starts positioned above the main island, looking down at the terrain. From here you can:
- See an overview of the entire simulation area
- Observe creature populations moving across the landscape
- Watch the interaction between different species

### Default Controls Quick Reference

| Action | Control |
|--------|---------|
| Move forward/back | W / S |
| Move left/right | A / D |
| Move up/down | E / Q (or Space / C) |
| Look around | Move mouse (after clicking) |
| Move faster | Hold Shift |
| Toggle debug panel | F3 |
| Pause/Resume | P |
| Quick save | F5 |
| Quick load | F9 |
| Release mouse | ESC |

### UI Overview

The main interface consists of:

1. **Main Viewport** - The 3D view of the simulation world
2. **Debug Panel (F3)** - Tabbed interface with simulation controls and statistics
3. **Status Bar** - Shows current time of day, generation count, and messages
4. **Creature Nametags** - Floating labels showing creature type and generation

---

## 4. Controls Reference

### Camera Controls

The camera uses an FPS-style control scheme:

**Movement:**
| Key | Action |
|-----|--------|
| W | Move forward |
| S | Move backward |
| A | Move left (strafe) |
| D | Move right (strafe) |
| E / Space | Move up |
| Q / C / Ctrl | Move down |
| Shift | Hold for 2x movement speed |

**Looking:**
| Control | Action |
|---------|--------|
| Left Click | Capture mouse for look control |
| Mouse Movement | Look around (when captured) |
| ESC | Release mouse cursor |

**Capturing the Mouse:**
- Click anywhere in the viewport to capture the mouse
- Once captured, mouse movement controls camera rotation
- Press ESC to release the mouse and interact with UI

### Simulation Controls

**Keyboard Shortcuts:**
| Key | Action |
|-----|--------|
| P | Toggle pause/resume simulation |
| F3 | Toggle debug panel visibility |
| F5 | Quick save simulation state |
| F9 | Quick load from quick save |
| ESC | Release mouse / Close dialogs |

**Speed Control (in Debug Panel):**
- Simulation speed slider: 0.1x to 5.0x
- Pause button: Completely stops simulation
- Step button: Advance one frame while paused

### Debug and Info Toggles

| Key | Action |
|-----|--------|
| F3 | Show/hide the debug panel |

The debug panel contains:
- Population statistics
- Performance metrics
- Simulation controls
- Creature inspector
- Neural network visualizer

### Mouse Controls

| Action | Control |
|--------|---------|
| Left Click (viewport) | Capture mouse for camera control |
| Left Click (UI) | Interact with UI elements |
| Mouse Wheel | Zoom in/out (adjust FOV) |
| Right Click | Context menu (in some panels) |

### Controller Support

Currently, the simulation is designed for keyboard and mouse input. Controller support may be added in future versions.

---

## 5. User Interface Guide

### Main Viewport

The 3D viewport displays:

- **Terrain**: Procedurally generated islands with grass, sand, and rock
- **Water**: Ocean surrounding the islands with underwater visibility
- **Vegetation**: Trees, bushes, and grass covering the land
- **Creatures**: Color-coded organisms moving across the world
- **Food**: Small markers indicating available food sources
- **Sky**: Dynamic sky that changes with the day/night cycle

**Visual Indicators:**
- Green creatures: Herbivores (grazers, browsers, frugivores)
- Red creatures: Carnivores (predators)
- Blue creatures: Aquatic life (fish, sharks)
- Purple/Mixed creatures: Flying creatures (birds, insects)
- Yellow dots: Food sources

### Debug Panel (F3)

The debug panel is organized into tabs:

#### Overview Tab
- **Population Counts**: Current number of each creature type
- **Generation Info**: Current maximum and average generation
- **Ecosystem Health**: Overall health score (0-100)
- **Time of Day**: Current phase of the day/night cycle

#### Genetics Tab
- **Trait Distributions**: Histograms of size, speed, vision range
- **Genetic Diversity Score**: How varied the population is
- **Species Breakdown**: Count of distinct species

#### Neural Tab
- **Brain Visualization**: Network topology of selected creature
- **Node Activity**: Real-time activity of neural network nodes
- **Connection Weights**: Visualization of weight magnitudes

#### World Tab
- **Day/Night Controls**: Manual time-of-day adjustment
- **Camera Info**: Current position and orientation
- **Performance Metrics**: FPS, frame time, memory usage

### Statistics Panel

The statistics panel shows real-time data:

**Population Section:**
```
Herbivores: 127  (+3 -5)
Carnivores: 23   (+1 -2)
Aquatic:    45   (+2 -1)
Flying:     18   (+0 -1)
Total:      213
```

The numbers in parentheses show births and deaths in the last minute.

**Generation Section:**
```
Max Generation: 47
Avg Generation: 23.4
Oldest Creature: Gen 45
```

**Ecosystem Health:**
- Green bar (80-100): Healthy, balanced ecosystem
- Yellow bar (50-79): Some imbalance, monitor closely
- Red bar (0-49): Critical issues, population at risk

### Time Controls

Located in the debug panel:

- **Play/Pause Button**: Toggle simulation running state
- **Speed Slider**: Adjust simulation speed (0.1x - 5.0x)
- **Time Display**: Shows simulation time elapsed
- **Day/Night**: Current phase (Dawn, Morning, Noon, etc.)

### Debug Overlays

Toggle various overlays for detailed information:

- **Show Nametags**: Display creature type and generation above each creature
- **Show Trees**: Toggle vegetation rendering
- **Performance Overlay**: Frame time graph and statistics

### Reading the Dashboard

**Population Graphs:**
- X-axis: Time (last 5 minutes of data)
- Y-axis: Population count
- Lines: Separate lines for herbivores, carnivores, and total

**Fitness Graphs:**
- Shows average fitness over time
- Rising line indicates evolutionary progress
- Plateaus indicate stable adaptations

**Predator-Prey Ratio:**
- Ideal ratio: 0.2 - 0.3 (20-30% carnivores)
- Too high: Carnivores will starve
- Too low: Herbivores will overpopulate

---

## 6. Understanding the Simulation

### Creature Types

The simulation features a complex food web with multiple creature types:

#### Land Herbivores

**Grazer (Green)**
- Eats grass only
- Forms herds for protection
- Fast reproduction rate
- Primary prey for carnivores

**Browser (Teal)**
- Eats tree leaves and bushes
- Can reach elevated food
- Medium size, slower than grazers

**Frugivore (Yellow-Green)**
- Eats fruits and berries
- Smallest land herbivore
- Most agile, can evade predators
- Prey for small predators and flying creatures

#### Land Carnivores

**Small Predator (Orange)**
- Hunts frugivores and small creatures
- Medium size, good speed
- Lower energy requirements

**Apex Predator (Red)**
- Hunts all herbivores
- Can hunt other predators
- Highest attack damage
- Requires successful kills to reproduce

**Omnivore (Brown)**
- Eats plants when food is scarce
- Hunts when prey is available
- Most adaptable diet

**Scavenger (Gray)**
- Eats corpses of dead creatures
- Does not hunt live prey
- Important for ecosystem cleanup

#### Flying Creatures

**Flying Bird (Sky Blue)**
- Feathered wings, soaring flight
- Can glide long distances
- Hunts small ground creatures from above

**Flying Insect (Purple)**
- Fast wing beats, agile flight
- Smaller body, lower energy needs
- Feeds on plants primarily

**Aerial Predator (Dark Red)**
- Hawks and eagles
- Dives from above to catch prey
- Can hunt other flying creatures

#### Aquatic Creatures

**Aquatic Herbivore (Light Blue)**
- Small fish eating algae and plants
- Schools together for protection
- Fast swimmers

**Aquatic Predator (Navy Blue)**
- Predatory fish like bass or pike
- Hunts smaller fish
- Solitary hunter

**Aquatic Apex - Shark (Dark Gray)**
- Top aquatic predator
- Can hunt all smaller aquatic life
- Rare but powerful

**Amphibian (Teal-Green)**
- Can survive on land and in water
- Versatile feeding habits
- Found near shorelines

### How Evolution Works

Evolution in the simulation follows these principles:

1. **Variation**: Each creature has unique genetic traits
2. **Selection**: Creatures that survive longer and find more food have higher fitness
3. **Reproduction**: High-fitness creatures are more likely to reproduce
4. **Inheritance**: Offspring inherit traits from their parents
5. **Mutation**: Random changes introduce new variations

**Fitness Calculation:**
```
Fitness = (Food Eaten * 2) + (Distance Traveled * 0.1) + (Age * 0.5) + (Offspring * 10)
```

Carnivores also gain fitness from successful kills.

### Neural Network Brains

Each creature has a neural network controlling its behavior:

**Input Neurons (Senses):**
- Target distance and angle (food or prey)
- Threat distance and angle (predators)
- Internal energy level
- Current speed
- Terrain height
- Time of day sensitivity

**Hidden Layer:**
- 12 neurons processing information
- Weighted connections from inputs
- Tanh activation function

**Output Neurons (Actions):**
- Turn amount (-1 to +1)
- Speed multiplier (0 to 1)
- Eat intention (0 to 1)
- Flee urgency (0 to 1)

**Evolution of Brains:**
- Neural network weights are part of the genome
- Successful behaviors are inherited
- Mutations can create new behavioral strategies

### Genetic Inheritance

When two creatures reproduce:

1. **Crossover**: Genes are mixed from both parents
   - Physical traits (size, speed, color)
   - Sensory abilities (vision, hearing, smell)
   - Neural network weights

2. **Mutation**: Random changes occur
   - Mutation rate: ~15%
   - Mutation strength: small random values
   - Can improve or harm fitness

3. **Trait Ranges:**
   - Size: 0.5 to 2.0 (affects speed and energy use)
   - Speed: 5.0 to 20.0 units/second
   - Vision: 10.0 to 50.0 unit range
   - Efficiency: 0.5 to 1.5 (energy consumption)

### Food and Energy System

**Energy Mechanics:**
- Creatures start with initial energy
- Movement costs energy based on speed and size
- Eating food restores energy
- Energy depleted = death

**Food Sources:**
- Grass: Fast regrowth, low energy value
- Bushes: Medium regrowth, medium energy
- Tree Fruit: Slow regrowth, high energy
- Carrion: From dead creatures (for scavengers)

**Reproduction Costs:**
- Herbivores: 80 energy to reproduce
- Carnivores: 100 energy + 2 kills required
- Flying: 70 energy + 1 kill required

### Predator-Prey Dynamics

The simulation models realistic predator-prey relationships:

**Predator Behavior:**
- Detect prey within vision range
- Chase when hungry
- Attack when within melee range
- Gain significant energy from kills

**Prey Behavior:**
- Detect predators within vision range
- Increase fear level when threatened
- Flee at increased speed
- Form herds for protection (safety in numbers)

**Population Cycles:**
1. Abundant prey leads to predator population growth
2. More predators reduce prey population
3. Reduced prey causes predator starvation
4. Fewer predators allows prey recovery
5. Cycle repeats

---

## 7. Gameplay Tips

### Observing Evolution in Action

**Best Viewing Positions:**
- Start high above the island for an overview
- Zoom in on groups of creatures to see individual behavior
- Follow specific creatures using the inspector

**What to Watch For:**
- Population balance between herbivores and carnivores
- Creature size changes over generations
- Speed adaptations (faster prey, faster predators)
- Color variations emerging in populations

**Time Lapse:**
- Increase simulation speed (up to 5x) to see evolution faster
- Pause occasionally to examine creature traits
- Use the generation counter to track evolutionary progress

### Finding Interesting Creatures

**Using the Inspector:**
1. Press F3 to open the debug panel
2. Go to the Overview tab
3. Click on a creature in the 3D view or list
4. Examine its traits, brain, and lineage

**Look For:**
- High generation number (successful lineage)
- Extreme trait values (very fast, very large)
- Unusual colors (mutations)
- High kill counts (successful predators)

**Following Creatures:**
- Select a creature in the inspector
- Watch its decision-making in real-time
- Observe how it interacts with others
- Note when it reproduces and dies

### Understanding Population Dynamics

**Healthy Ecosystem Signs:**
- Stable population with minor fluctuations
- Multiple generations coexisting
- Predator-prey ratio around 20-30%
- Food sources regenerating adequately

**Warning Signs:**
- Rapid population decline
- Single species dominance
- Extinction of creature types
- Age distribution skewing very young

**Recovery Strategies:**
If an ecosystem becomes unbalanced:
- Let it run; natural cycles often self-correct
- Load an earlier save if critical
- Observe which traits led to imbalance

### Recognizing Successful Traits

**Herbivore Success Traits:**
- Higher speed (escape predators)
- Better vision (detect threats early)
- Higher efficiency (survive longer between meals)
- Smaller size (harder to catch, but less energy storage)

**Carnivore Success Traits:**
- Speed matching or exceeding prey
- Good vision for tracking
- Larger size (more attack power)
- Patience (energy-efficient hunting)

**Flying Creature Traits:**
- Wide wingspan (better gliding)
- Higher altitude preference (safety)
- Good vision (spotting prey from above)

**Aquatic Creature Traits:**
- Schooling behavior (protection in numbers)
- Swim frequency matching environment
- Depth preference (avoiding surface predators)

### Long-term Evolution Experiments

**Tracking Progress:**
- Note the generation number at significant events
- Compare trait averages over time
- Watch for new behaviors emerging

**Interesting Scenarios:**
1. **Isolation**: Watch evolution on separate islands
2. **Predator Introduction**: Add carnivores to herbivore-only areas
3. **Environmental Change**: Observe adaptation to day/night
4. **Extinction Recovery**: See how populations recover after crashes

---

## 8. Save and Load

### Quick Save (F5)

Press F5 at any time to create a quick save:

- Saves immediately without dialog
- Overwrites previous quick save
- Includes all creatures, food, and world state
- Status bar shows "Game Saved (quicksave.evos)"

**When to Quick Save:**
- Before significant events
- When you see interesting evolution
- Before increasing simulation speed
- When leaving the simulation running

### Quick Load (F9)

Press F9 to restore from quick save:

- Loads immediately without dialog
- Restores exact simulation state
- Camera position is preserved
- Status bar shows "Game Loaded (quicksave.evos)"

**Important Notes:**
- Current state is lost when loading
- No confirmation dialog (be careful!)
- Make sure quick save exists first

### Auto-Save Feature

The simulation includes automatic saving:

- **Interval**: Every 5 minutes by default
- **Rotation**: 3 auto-save slots (cycles through them)
- **Location**: Same directory as manual saves

Auto-saves are named:
- `autosave_0.evos`
- `autosave_1.evos`
- `autosave_2.evos`

### Save File Locations

**Default Save Directory:**
```
Windows: %APPDATA%\OrganismEvolution\saves\
         (typically C:\Users\<username>\AppData\Roaming\OrganismEvolution\saves\)
```

**Save File Format:**
- Extension: `.evos` (Evolution Save)
- Format: Binary (not human-readable)
- Contents: Header, world state, creatures, food

**Save File Contents:**
- Simulation time and generation
- Day/night cycle state
- All creature positions and traits
- All creature neural networks
- Food positions and states
- Random number generator state

### Managing Saves

**Through the UI:**
1. Open debug panel (F3)
2. Navigate to File menu
3. Access save slot management

**Manual File Management:**
- Saves can be copied, renamed, or deleted
- Backup important saves externally
- Share saves with others (same version required)

---

## 9. Troubleshooting

### Common Issues and Solutions

#### "Shader not found" Error

**Symptoms:**
- Black screen or rendering errors
- Error message in console

**Solutions:**
1. Ensure `Runtime/Shaders` folder exists next to executable
2. Rebuild the project (CMake copies shaders during build)
3. Manually copy shaders:
   ```batch
   copy Runtime\Shaders build_dx12\Debug\Shaders
   ```

#### "DX12 device creation failed" Error

**Symptoms:**
- Application fails to start
- DirectX error message

**Solutions:**
1. Update GPU drivers to latest version
2. Ensure GPU supports DirectX 12
3. Check Windows Update for DirectX updates
4. Try running as Administrator

#### Low Frame Rate / Poor Performance

**Symptoms:**
- Stuttering or choppy movement
- FPS counter showing low values

**Solutions:**
1. Reduce creature population (in debug panel)
2. Lower simulation speed
3. Close other GPU-intensive applications
4. Update graphics drivers
5. Check if running in Debug vs Release mode

#### Creatures Not Moving

**Symptoms:**
- All creatures stationary
- Population changing but no visible movement

**Solutions:**
1. Check if simulation is paused (press P)
2. Verify simulation speed is not 0
3. Check if all creatures have died (generation may need restart)

#### Mouse Look Not Working

**Symptoms:**
- Camera doesn't rotate with mouse
- Cursor visible but look control inactive

**Solutions:**
1. Click in the viewport to capture mouse
2. Ensure window has focus
3. Check if cursor is over UI (UI takes priority)
4. Press ESC then click again to recapture

#### Save/Load Not Working

**Symptoms:**
- F5/F9 have no effect
- Error messages about save files

**Solutions:**
1. Check save directory exists and is writable
2. Ensure disk has sufficient space
3. Check file permissions
4. Look for error messages in console

### Performance Optimization

**For Better Performance:**

1. **Reduce Population:**
   - Open debug panel
   - Lower creature cap settings
   - Let natural selection reduce numbers

2. **Disable Visual Effects:**
   - Turn off nametags
   - Hide vegetation
   - Reduce shadow quality

3. **Hardware Solutions:**
   - Close background applications
   - Ensure GPU is not thermal throttling
   - Use Release build instead of Debug

4. **Monitor Performance:**
   - Watch FPS counter in debug panel
   - Check frame time graph
   - Identify bottlenecks

### Crash Recovery

**If the Application Crashes:**

1. Check for auto-save files in save directory
2. Auto-saves are made every 5 minutes
3. Load the most recent auto-save

**Preventing Data Loss:**
- Quick save regularly (F5)
- Check auto-save is enabled
- Backup important saves manually

### Known Limitations

- **Population Cap**: Performance degrades above ~1000 creatures
- **Save Compatibility**: Saves may not work across different versions
- **Memory Usage**: Large populations increase memory usage
- **GPU Requirements**: DirectX 12 required; no fallback to older APIs

---

## 10. FAQ

### General Questions

**Q: How long does it take to see evolution?**

A: Visible evolution can occur within 10-20 generations, which typically takes 5-15 minutes of real time depending on simulation speed. Major adaptations may take 50-100 generations.

**Q: Can creatures go extinct?**

A: Yes, both individual species and entire categories can go extinct. The simulation does not automatically replenish populations, allowing for realistic extinction events.

**Q: Why do all my carnivores die?**

A: Carnivores require prey to survive. If the herbivore population crashes or carnivores over-hunt, they will starve. This is part of the natural predator-prey cycle.

**Q: Can I add more creatures manually?**

A: Currently, creatures can be spawned through the debug panel's spawn controls. This allows you to introduce new creatures or rebalance populations.

### Technical Questions

**Q: What is the maximum number of creatures?**

A: The simulation can technically handle thousands of creatures, but performance is optimal with 200-500 creatures. Above 1000, frame rate may suffer.

**Q: Why does the simulation slow down over time?**

A: As generations progress, genetic data accumulates. Additionally, high creature counts require more processing. Regular saves and occasional restarts can help.

**Q: Can I modify creature parameters?**

A: If building from source, creature parameters are defined in the header files (Genome.h, CreatureType.h). Modifying these requires recompilation.

**Q: Does the simulation use the GPU for computation?**

A: Yes, DirectX 12 is used for rendering. Some steering calculations can use GPU compute shaders for improved performance.

### Simulation Questions

**Q: What determines which creatures survive?**

A: Survival depends on:
- Finding food efficiently
- Avoiding predators (for prey)
- Successfully hunting (for predators)
- Energy management
- Reproductive success

**Q: How do neural networks evolve?**

A: Neural network weights are part of each creature's genome. During reproduction, weights are inherited and mutated, leading to behavioral evolution.

**Q: What causes population cycles?**

A: Classic Lotka-Volterra dynamics: predators reduce prey, leading to predator starvation, which allows prey recovery, which enables predator recovery.

**Q: Why are some creatures larger/faster than others?**

A: Variation comes from:
- Initial random generation
- Genetic inheritance from parents
- Mutations during reproduction

### Save and Data Questions

**Q: Where are my save files?**

A: Default location is `%APPDATA%\OrganismEvolution\saves\` on Windows. The console shows the exact path at startup.

**Q: Can I share save files with others?**

A: Yes, save files can be shared if both users have the same version of the simulation. Copy the `.evos` file to the recipient's save directory.

**Q: How large are save files?**

A: Save file size depends on creature count. Typical saves are 1-10 MB. Larger populations create larger files.

**Q: What happens if I modify a save file?**

A: Save files are binary and should not be edited manually. Corruption will prevent loading and may cause crashes.

### Troubleshooting Questions

**Q: The simulation is running but nothing is visible?**

A: Check that:
- Camera is in a position to see creatures
- Debug overlays are enabled
- Creatures have not all died
- Simulation is not paused at 0 speed

**Q: Why do creatures walk into the water and die?**

A: Land creatures will avoid water when possible, but neural networks may not have evolved water avoidance yet. This is natural selection at work.

**Q: Can I run multiple simulations simultaneously?**

A: Each instance of the application runs independently. You can run multiple instances if your hardware supports it, but each uses separate memory and GPU resources.

---

## Appendix: Keyboard Shortcuts Summary

| Key | Action |
|-----|--------|
| W/A/S/D | Camera movement |
| Q/E/C/Space | Vertical movement |
| Shift | Fast movement modifier |
| Mouse | Look around (when captured) |
| Left Click | Capture mouse / UI interaction |
| ESC | Release mouse / Exit dialogs |
| F3 | Toggle debug panel |
| F5 | Quick save |
| F9 | Quick load |
| P | Pause/Resume simulation |

---

## Appendix: Creature Type Reference

| Type | Category | Diet | Color |
|------|----------|------|-------|
| Grazer | Herbivore | Grass only | Green |
| Browser | Herbivore | Leaves, bushes | Teal |
| Frugivore | Herbivore | Fruits, berries | Yellow-Green |
| Small Predator | Carnivore | Small prey | Orange |
| Apex Predator | Carnivore | All prey | Red |
| Omnivore | Omnivore | Plants + small prey | Brown |
| Scavenger | Carnivore | Carrion | Gray |
| Flying Bird | Flying | Small prey | Sky Blue |
| Flying Insect | Flying | Plants | Purple |
| Aerial Predator | Flying | Small creatures | Dark Red |
| Aquatic Herbivore | Aquatic | Algae, plants | Light Blue |
| Aquatic Predator | Aquatic | Small fish | Navy Blue |
| Shark | Aquatic | All fish | Dark Gray |
| Amphibian | Aquatic | Varied | Teal-Green |

---

## Appendix: Glossary

**Crossover**: The mixing of genetic material from two parents during reproduction.

**Fitness**: A measure of how successful a creature is, based on survival time, food eaten, and offspring produced.

**Generation**: The number of reproductive cycles from the original population. Higher generations indicate more evolved creatures.

**Genome**: The complete set of genetic information for a creature, including physical traits and neural network weights.

**Mutation**: Random changes to genetic information during reproduction that create variation.

**Natural Selection**: The process by which creatures with beneficial traits survive and reproduce more successfully.

**Neural Network**: A computational model inspired by biological brains, used to control creature behavior.

**Predator-Prey Dynamics**: The cyclical relationship between predator and prey populations.

**Speciation**: The formation of new species through evolutionary divergence.

**Trophic Level**: Position in the food chain (primary consumer, secondary consumer, etc.).

---

*OrganismEvolution - Watch life evolve before your eyes.*

*Version 1.0 - January 2026*
