# OrganismEvolution API Reference

This document provides a comprehensive API reference for all public classes and methods in the OrganismEvolution simulation framework.

---

## Table of Contents

1. [Entity Classes](#entity-classes)
   - [Creature](#creature)
   - [Genome](#genome)
   - [NeuralNetwork](#neuralnetwork)
   - [CreatureTraits](#creaturetraits)
2. [Environment Classes](#environment-classes)
   - [Terrain](#terrain)
   - [WeatherSystem](#weathersystem)
   - [ClimateSystem](#climatesystem)
3. [Core Systems](#core-systems)
   - [SpatialGrid](#spatialgrid)
   - [SaveManager](#savemanager)
   - [DayNightCycle](#daynightcycle)
4. [Animation Classes](#animation-classes)
   - [Skeleton](#skeleton)
   - [BoneTransform](#bonetransform)
   - [SkeletonFactory](#skeletonfactory)
5. [Enumerations](#enumerations)
   - [CreatureType](#creaturetype)
   - [DietType](#diettype)
   - [TrophicLevel](#trophiclevel)
   - [WeatherType](#weathertype)
   - [BiomeType](#biometype)
6. [Helper Functions](#helper-functions)

---

## Entity Classes

### Creature

The main entity class representing living organisms in the simulation. Creatures have genetics, neural networks for decision-making, and can interact with their environment and other creatures.

#### Constructors

```cpp
Creature(const glm::vec3& position, const Genome& genome, CreatureType type = CreatureType::HERBIVORE)
```

Creates a creature with a single-parent genome (asexual reproduction or initial spawn).

**Parameters:**
- `position` - Initial world position (x, y, z)
- `genome` - Genetic information defining physical and behavioral traits
- `type` - The creature type determining diet and behavior (default: HERBIVORE)

**Example:**
```cpp
Genome g;
g.randomize();
Creature herbivore(glm::vec3(10.0f, 0.0f, 10.0f), g, CreatureType::GRAZER);
```

---

```cpp
Creature(const glm::vec3& position, const Genome& parent1, const Genome& parent2, CreatureType type = CreatureType::HERBIVORE)
```

Creates a creature through sexual reproduction with genetic crossover.

**Parameters:**
- `position` - Initial world position
- `parent1` - First parent's genome
- `parent2` - Second parent's genome
- `type` - The creature type (default: HERBIVORE)

**Example:**
```cpp
Creature offspring(glm::vec3(15.0f, 0.0f, 15.0f), parent1.getGenome(), parent2.getGenome(), CreatureType::GRAZER);
```

---

```cpp
Creature(const glm::vec3& position, const genetics::DiploidGenome& diploidGenome, CreatureType type = CreatureType::HERBIVORE)
```

Creates a creature using the advanced diploid genome system.

**Parameters:**
- `position` - Initial world position
- `diploidGenome` - Diploid genetic information with dominant/recessive alleles
- `type` - The creature type

---

#### Methods

##### update

```cpp
void update(float deltaTime, const Terrain& terrain, const std::vector<glm::vec3>& foodPositions,
            const std::vector<Creature*>& otherCreatures, const SpatialGrid* spatialGrid = nullptr,
            const EnvironmentConditions* envConditions = nullptr,
            const std::vector<SoundEvent>* sounds = nullptr)
```

Updates the creature's state, including physics, behavior, and neural network processing.

**Parameters:**
- `deltaTime` - Time elapsed since last update (seconds)
- `terrain` - Reference to the terrain for height queries and bounds
- `foodPositions` - Vector of food source positions
- `otherCreatures` - Vector of pointers to other creatures for interaction
- `spatialGrid` - Optional spatial partitioning grid for optimized neighbor queries
- `envConditions` - Optional environmental conditions affecting behavior
- `sounds` - Optional sound events for hearing-based behaviors

**Example:**
```cpp
creature.update(0.016f, terrain, foodPositions, creatures, &spatialGrid);
```

---

##### render

```cpp
void render(uint32_t vaoHandle)
```

Renders the creature using the provided vertex array object.

**Parameters:**
- `vaoHandle` - OpenGL/DX12 vertex array handle for rendering

---

##### isAlive

```cpp
bool isAlive() const
```

Checks if the creature is still alive.

**Returns:** `true` if the creature has positive energy and hasn't been killed

**Example:**
```cpp
if (creature.isAlive()) {
    creature.update(dt, terrain, food, others);
}
```

---

##### canReproduce

```cpp
bool canReproduce() const
```

Checks if the creature meets reproduction requirements.

**Returns:** `true` if energy exceeds reproduction threshold and other conditions are met (e.g., kill count for carnivores)

---

##### consumeFood

```cpp
void consumeFood(float amount)
```

Adds energy from consuming food.

**Parameters:**
- `amount` - Energy value of consumed food

---

##### reproduce

```cpp
void reproduce(float& energyCost)
```

Initiates reproduction, deducting energy cost.

**Parameters:**
- `energyCost` - Output parameter receiving the energy cost of reproduction

---

##### attack

```cpp
void attack(Creature* target, float deltaTime)
```

Attacks another creature (for predators).

**Parameters:**
- `target` - Pointer to the creature being attacked
- `deltaTime` - Time step for damage calculation

---

##### takeDamage

```cpp
void takeDamage(float damage)
```

Reduces creature health/energy from an attack.

**Parameters:**
- `damage` - Amount of damage to inflict

---

##### emitAlarmCall

```cpp
void emitAlarmCall(std::vector<SoundEvent>& soundBuffer)
```

Emits an alarm call to warn nearby creatures of danger.

**Parameters:**
- `soundBuffer` - Buffer to add the sound event to

---

##### emitMatingCall

```cpp
void emitMatingCall(std::vector<SoundEvent>& soundBuffer)
```

Emits a mating call to attract potential mates.

**Parameters:**
- `soundBuffer` - Buffer to add the sound event to

---

##### Accessor Methods

```cpp
const glm::vec3& getPosition() const
const glm::vec3& getVelocity() const
const Genome& getGenome() const
const genetics::DiploidGenome& getDiploidGenome() const
float getEnergy() const
float getAge() const
float getFitness() const
int getGeneration() const
int getID() const
CreatureType getType() const
float getFear() const
int getKillCount() const
bool isBeingHunted() const
bool isSterile() const
float getCamouflageLevel() const
float getAnimationPhase() const
const std::vector<glm::mat4>& getSkinningMatrices() const
```

---

### Genome

Defines the genetic traits of a creature, including physical attributes, sensory capabilities, and neural network weights.

#### Constructor

```cpp
Genome()
```

Creates a genome with default trait values.

**Example:**
```cpp
Genome genome;
genome.randomize();
```

---

```cpp
Genome(const Genome& parent1, const Genome& parent2)
```

Creates a genome through crossover of two parent genomes.

**Parameters:**
- `parent1` - First parent genome
- `parent2` - Second parent genome

**Example:**
```cpp
Genome offspring(mother.getGenome(), father.getGenome());
offspring.mutate(0.1f, 0.2f);
```

---

#### Methods

##### mutate

```cpp
void mutate(float mutationRate, float mutationStrength)
```

Applies random mutations to genome traits.

**Parameters:**
- `mutationRate` - Probability of each trait mutating (0.0 to 1.0)
- `mutationStrength` - Maximum magnitude of mutations

**Example:**
```cpp
genome.mutate(0.05f, 0.1f);  // 5% chance, 10% max change
```

---

##### randomize

```cpp
void randomize()
```

Randomizes all genome traits within valid ranges.

**Example:**
```cpp
Genome g;
g.randomize();
```

---

##### randomizeFlying

```cpp
void randomizeFlying()
```

Initializes genome with traits suitable for generic flying creatures.

---

##### randomizeBird

```cpp
void randomizeBird()
```

Initializes genome with bird-specific traits (feathered wings, high glide ratio).

---

##### randomizeInsect

```cpp
void randomizeInsect()
```

Initializes genome with insect-specific traits (fast wing beats, agile flight).

---

##### randomizeAerialPredator

```cpp
void randomizeAerialPredator()
```

Initializes genome for aerial predators (hawks, eagles).

---

##### randomizeAquatic

```cpp
void randomizeAquatic()
```

Initializes genome for aquatic creatures (fish).

---

##### randomizeAquaticPredator

```cpp
void randomizeAquaticPredator()
```

Initializes genome for predatory fish (bass, pike).

---

##### randomizeShark

```cpp
void randomizeShark()
```

Initializes genome for apex aquatic predators.

---

##### calculateSensoryEnergyCost

```cpp
float calculateSensoryEnergyCost() const
```

Calculates the metabolic cost of the creature's sensory systems.

**Returns:** Energy cost per time unit for maintaining sensory capabilities

---

#### Public Member Variables

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| `size` | float | 0.5 - 2.0 | Physical size affecting speed and energy |
| `speed` | float | 5.0 - 20.0 | Movement speed |
| `visionRange` | float | 10.0 - 50.0 | Detection distance |
| `efficiency` | float | 0.5 - 1.5 | Energy consumption multiplier |
| `color` | glm::vec3 | RGB | Visual appearance |
| `visionFOV` | float | 1.0 - 6.0 | Field of view (radians) |
| `visionAcuity` | float | 0.0 - 1.0 | Detail perception at range |
| `colorPerception` | float | 0.0 - 1.0 | Color sensitivity |
| `motionDetection` | float | 0.0 - 1.0 | Motion sensitivity bonus |
| `hearingRange` | float | 10 - 100 | Maximum hearing distance |
| `echolocationAbility` | float | 0.0 - 1.0 | Echolocation capability |
| `smellRange` | float | 10 - 150 | Scent detection distance |
| `camouflageLevel` | float | 0.0 - 1.0 | Reduces visual detection |
| `wingSpan` | float | 0.5 - 2.0 | Wing size ratio (flying) |
| `flapFrequency` | float | 2.0 - 8.0 Hz | Wing flap speed |
| `glideRatio` | float | 0.3 - 0.8 | Gliding vs flapping preference |
| `finSize` | float | 0.3 - 1.0 | Fin size (aquatic) |
| `swimFrequency` | float | 1.0 - 4.0 Hz | Swimming frequency |
| `schoolingStrength` | float | 0.5 - 1.0 | Schooling behavior intensity |

---

### NeuralNetwork

Feedforward neural network for creature decision-making and behavior modulation.

#### Constructor

```cpp
NeuralNetwork(const std::vector<float>& weights)
```

Creates a neural network with specified connection weights.

**Parameters:**
- `weights` - Vector of neural connection weights

**Example:**
```cpp
std::vector<float> weights(genome.neuralWeights);
NeuralNetwork brain(weights);
```

---

#### Methods

##### process

```cpp
void process(const std::vector<float>& inputs, float& outAngle, float& outSpeed)
```

Legacy method for processing inputs and returning movement decisions.

**Parameters:**
- `inputs` - Input values (food distance, threat distance, energy, etc.)
- `outAngle` - Output turn angle (-PI to PI)
- `outSpeed` - Output speed multiplier (0 to 1)

---

##### forward

```cpp
NeuralOutputs forward(const std::vector<float>& inputs)
```

Full neural processing with behavior modulation outputs.

**Parameters:**
- `inputs` - Input values for neural processing

**Returns:** `NeuralOutputs` struct containing movement and behavior modulation values

**Example:**
```cpp
std::vector<float> inputs = {foodDist, foodAngle, threatDist, threatAngle, energy, speed, allies, fear};
NeuralOutputs outputs = brain.forward(inputs);
float turnAngle = outputs.turnAngle;
float aggression = outputs.aggressionMod;
```

---

##### getInputCount

```cpp
int getInputCount() const
```

**Returns:** Number of expected input neurons (8)

---

##### getOutputCount

```cpp
int getOutputCount() const
```

**Returns:** Number of output neurons (6)

---

### NeuralOutputs

Structure containing neural network output values.

#### Member Variables

| Variable | Type | Range | Description |
|----------|------|-------|-------------|
| `turnAngle` | float | -1 to 1 | Turn direction (scaled to -PI to PI) |
| `speedMultiplier` | float | 0 to 1 | Movement speed factor |
| `aggressionMod` | float | -1 to 1 | Aggression modifier |
| `fearMod` | float | -1 to 1 | Fear response modifier |
| `socialMod` | float | -1 to 1 | Social/herding behavior modifier |
| `explorationMod` | float | -1 to 1 | Exploration tendency modifier |

---

### CreatureTraits

Defines species-specific traits and capabilities based on creature type.

#### Constructor

```cpp
CreatureTraits()
```

Creates default traits (grazer configuration).

---

#### Static Methods

##### getTraitsFor

```cpp
static CreatureTraits getTraitsFor(CreatureType type)
```

Factory method returning appropriate traits for a creature type.

**Parameters:**
- `type` - The creature type

**Returns:** `CreatureTraits` configured for the specified type

**Example:**
```cpp
CreatureTraits traits = CreatureTraits::getTraitsFor(CreatureType::APEX_PREDATOR);
float attackRange = traits.attackRange;
bool isPack = traits.isPackHunter;
```

---

#### Member Variables

| Variable | Type | Description |
|----------|------|-------------|
| `type` | CreatureType | The creature classification |
| `diet` | DietType | What the creature can eat |
| `trophicLevel` | TrophicLevel | Position in food chain |
| `attackRange` | float | Melee attack range |
| `attackDamage` | float | Damage per attack |
| `fleeDistance` | float | Distance to start fleeing |
| `huntingEfficiency` | float | Prey-to-energy conversion rate |
| `minPreySize` | float | Minimum prey size |
| `maxPreySize` | float | Maximum prey size |
| `isPackHunter` | bool | Hunts in groups |
| `isHerdAnimal` | bool | Forms protective herds |
| `isTerritorial` | bool | Defends territory |
| `canClimb` | bool | Can reach elevated food |
| `canDigest[3]` | bool[3] | Digestibility: [grass, leaves, fruit] |
| `parasiteResistance` | float | Resistance to parasites (0-1) |

---

## Environment Classes

### Terrain

Generates and manages procedural terrain with height mapping and DirectX 12 rendering.

#### Constructor

```cpp
Terrain(int width, int depth, float scale = 1.0f)
```

Creates a terrain grid.

**Parameters:**
- `width` - Grid width in vertices
- `depth` - Grid depth in vertices
- `scale` - World scale factor (default: 1.0)

**Example:**
```cpp
Terrain terrain(256, 256, 2.0f);
terrain.generate(12345);
```

---

#### Methods

##### initializeDX12

```cpp
void initializeDX12(DX12Device* device, ID3D12PipelineState* pso, ID3D12RootSignature* rootSig)
```

Initializes DirectX 12 resources for rendering.

**Parameters:**
- `device` - DX12 device reference
- `pso` - Pipeline state object for terrain rendering
- `rootSig` - Root signature for shader parameters

---

##### generate

```cpp
void generate(unsigned int seed)
```

Generates terrain heightmap using procedural noise.

**Parameters:**
- `seed` - Random seed for terrain generation

**Example:**
```cpp
terrain.generate(std::random_device{}());
```

---

##### render

```cpp
void render(ID3D12GraphicsCommandList* commandList)
```

Renders terrain geometry using the provided command list.

**Parameters:**
- `commandList` - DX12 graphics command list

---

##### renderForShadow

```cpp
void renderForShadow(ID3D12GraphicsCommandList* commandList)
```

Renders terrain geometry for shadow mapping pass.

**Parameters:**
- `commandList` - DX12 graphics command list

---

##### getHeight

```cpp
float getHeight(float x, float z) const
```

Gets terrain height at world coordinates.

**Parameters:**
- `x` - World X coordinate
- `z` - World Z coordinate

**Returns:** Terrain height at the specified position (0.0 for out-of-bounds)

**Example:**
```cpp
float groundHeight = terrain.getHeight(creature.getPosition().x, creature.getPosition().z);
```

---

##### getHeightSafe

```cpp
bool getHeightSafe(float x, float z, float& outHeight) const
```

Gets terrain height with explicit bounds checking.

**Parameters:**
- `x` - World X coordinate
- `z` - World Z coordinate
- `outHeight` - Output parameter for height value

**Returns:** `true` if coordinates are valid, `false` otherwise

---

##### isInBounds

```cpp
bool isInBounds(float x, float z) const
```

Checks if world coordinates are within terrain bounds.

**Parameters:**
- `x` - World X coordinate
- `z` - World Z coordinate

**Returns:** `true` if within bounds

---

##### clampToBounds

```cpp
void clampToBounds(float& x, float& z) const
```

Clamps coordinates to valid terrain bounds.

**Parameters:**
- `x` - X coordinate to clamp (modified in place)
- `z` - Z coordinate to clamp (modified in place)

---

##### isWater

```cpp
bool isWater(float x, float z) const
```

Checks if position is underwater.

**Parameters:**
- `x` - World X coordinate
- `z` - World Z coordinate

**Returns:** `true` if below water level

---

##### Accessor Methods

```cpp
unsigned int getIndexCount() const
int getWidth() const
int getDepth() const
float getScale() const
D3D12_VERTEX_BUFFER_VIEW getVertexBufferView() const
D3D12_INDEX_BUFFER_VIEW getIndexBufferView() const
```

---

### WeatherSystem

Simulates dynamic weather conditions affecting gameplay and rendering.

#### Constructor

```cpp
WeatherSystem()
```

Creates a weather system with default clear weather.

---

#### Methods

##### initialize

```cpp
void initialize(SeasonManager* season, ClimateSystem* climate)
```

Initializes the weather system with references to related systems.

**Parameters:**
- `season` - Season manager for seasonal weather patterns
- `climate` - Climate system for biome-appropriate weather

---

##### update

```cpp
void update(float deltaTime)
```

Updates weather simulation.

**Parameters:**
- `deltaTime` - Time elapsed since last update

**Example:**
```cpp
weatherSystem.update(deltaTime);
if (weatherSystem.isRaining()) {
    // Adjust creature behavior
}
```

---

##### getCurrentWeather

```cpp
const WeatherState& getCurrentWeather() const
```

**Returns:** Current weather state for rendering/gameplay

---

##### getInterpolatedWeather

```cpp
WeatherState getInterpolatedWeather() const
```

**Returns:** Smoothly interpolated weather state during transitions

---

##### setWeather

```cpp
void setWeather(WeatherType type, float transitionDuration = 30.0f)
```

Forces a weather change (for debugging/testing).

**Parameters:**
- `type` - Target weather type
- `transitionDuration` - Seconds for weather transition (default: 30)

**Example:**
```cpp
weatherSystem.setWeather(WeatherType::THUNDERSTORM, 10.0f);
```

---

##### Weather Query Methods

```cpp
bool isRaining() const
bool isSnowing() const
bool isPrecipitating() const
bool isFoggy() const
bool isStormy() const
float getVisibility() const
float getSunIntensity() const
float getWindStrength() const
bool hasLightningFlash() const
glm::vec3 getLightningPosition() const
```

---

##### getWeatherName

```cpp
static const char* getWeatherName(WeatherType type)
```

Gets human-readable weather name.

**Parameters:**
- `type` - Weather type enumeration

**Returns:** String name of the weather type

---

##### Configuration Methods

```cpp
void setWeatherChangeInterval(float seconds)
void setAutoWeatherChange(bool enabled)
void setWeatherChangeCallback(WeatherChangeCallback callback)
```

---

### ClimateSystem

Manages biome distribution and climate conditions across the terrain.

#### Constructor

```cpp
ClimateSystem()
```

Creates a climate system with default parameters.

---

#### Methods

##### initialize

```cpp
void initialize(const Terrain* terrain, SeasonManager* seasonMgr)
```

Initializes with terrain and season references.

**Parameters:**
- `terrain` - Terrain for elevation data
- `seasonMgr` - Season manager for temperature variations

---

##### update

```cpp
void update(float deltaTime)
```

Updates climate simulation.

**Parameters:**
- `deltaTime` - Time elapsed since last update

---

##### getClimateAt

```cpp
ClimateData getClimateAt(const glm::vec3& worldPos) const
ClimateData getClimateAt(float x, float z) const
```

Gets climate data at a world position.

**Parameters:**
- `worldPos` - World position vector
- `x`, `z` - World coordinates

**Returns:** `ClimateData` structure with temperature, moisture, biome info

**Example:**
```cpp
ClimateData climate = climateSystem.getClimateAt(creature.getPosition());
BiomeType biome = climate.getBiome();
```

---

##### calculateBiomeBlend

```cpp
BiomeBlend calculateBiomeBlend(const ClimateData& climate) const
```

Calculates biome blend for smooth transitions.

**Parameters:**
- `climate` - Climate data at position

**Returns:** `BiomeBlend` with primary/secondary biomes and blend factor

---

##### simulateMoisture

```cpp
void simulateMoisture()
```

Simulates moisture distribution including rain shadows.

---

##### getVegetationDensity

```cpp
VegetationDensity getVegetationDensity(const glm::vec3& worldPos) const
```

Gets vegetation density parameters for a position.

**Parameters:**
- `worldPos` - World position

**Returns:** `VegetationDensity` with tree, grass, flower densities

---

##### Configuration Methods

```cpp
void setWorldLatitude(float lat)
float getWorldLatitude() const
void setPrevailingWindDirection(const glm::vec2& dir)
glm::vec2 getPrevailingWindDirection() const
float getSeasonalTemperature(float baseTemp) const
```

---

##### getBiomeName

```cpp
static const char* getBiomeName(BiomeType biome)
```

Gets human-readable biome name.

**Parameters:**
- `biome` - Biome type enumeration

**Returns:** String name of the biome

---

## Core Systems

### SpatialGrid

Optimized spatial partitioning grid for O(1) neighbor queries.

#### Constructor

```cpp
SpatialGrid(float worldWidth, float worldDepth, int gridSize = 20)
```

Creates a spatial partitioning grid.

**Parameters:**
- `worldWidth` - World width in units
- `worldDepth` - World depth in units
- `gridSize` - Number of cells per axis (default: 20)

**Example:**
```cpp
SpatialGrid grid(500.0f, 500.0f, 25);
```

---

#### Methods

##### clear

```cpp
void clear()
```

Clears all creatures from the grid. Call once per frame before inserting.

---

##### insert

```cpp
void insert(Creature* creature)
```

Inserts a creature into the appropriate grid cell.

**Parameters:**
- `creature` - Pointer to creature to insert

**Example:**
```cpp
grid.clear();
for (auto& creature : creatures) {
    grid.insert(&creature);
}
```

---

##### query

```cpp
const std::vector<Creature*>& query(const glm::vec3& position, float radius)
```

Queries all creatures within radius of a position.

**Parameters:**
- `position` - Center position for query
- `radius` - Search radius

**Returns:** Reference to internal buffer containing matching creatures (no allocation)

**Example:**
```cpp
const auto& nearby = grid.query(creature.getPosition(), 50.0f);
for (Creature* other : nearby) {
    // Process neighbor
}
```

---

##### queryByType

```cpp
const std::vector<Creature*>& queryByType(const glm::vec3& position, float radius, int creatureType)
```

Queries creatures of a specific type within radius.

**Parameters:**
- `position` - Center position
- `radius` - Search radius
- `creatureType` - Type filter (cast from CreatureType)

**Returns:** Reference to filtered creature buffer

**Example:**
```cpp
const auto& predators = grid.queryByType(pos, 100.0f, static_cast<int>(CreatureType::APEX_PREDATOR));
```

---

##### findNearest

```cpp
Creature* findNearest(const glm::vec3& position, float maxRadius, int typeFilter = -1) const
```

Finds the nearest creature to a position.

**Parameters:**
- `position` - Search origin
- `maxRadius` - Maximum search distance
- `typeFilter` - Optional type filter (-1 for any type)

**Returns:** Pointer to nearest creature, or nullptr if none found

---

##### countNearby

```cpp
int countNearby(const glm::vec3& position, float radius) const
```

Counts creatures in nearby cells without allocation.

**Parameters:**
- `position` - Center position
- `radius` - Search radius

**Returns:** Count of nearby creatures

---

##### Statistics Methods

```cpp
size_t getTotalCreatures() const
size_t getMaxCellOccupancy() const
size_t getQueryCount() const
void resetStats()
```

---

#### Constants

```cpp
static constexpr int MAX_PER_CELL = 64
```

Maximum creatures per cell (fixed allocation for performance).

---

### SaveManager

Handles game state persistence with auto-save and save slot management.

#### Constructor

```cpp
SaveManager()
```

Creates a save manager with default save directory.

---

#### Methods

##### setSaveDirectory / getSaveDirectory

```cpp
void setSaveDirectory(const std::string& dir)
const std::string& getSaveDirectory() const
```

Sets/gets the save file directory.

---

##### ensureSaveDirectory

```cpp
void ensureSaveDirectory()
```

Creates the save directory if it doesn't exist.

---

##### saveGame

```cpp
SaveResult saveGame(const std::string& filename, const SaveFileHeader& header,
                    const WorldSaveData& world,
                    const std::vector<CreatureSaveData>& creatures,
                    const std::vector<FoodSaveData>& food)
```

Saves the current game state to a file.

**Parameters:**
- `filename` - Save file name
- `header` - Save file metadata
- `world` - World state data
- `creatures` - Vector of creature data
- `food` - Vector of food source data

**Returns:** `SaveResult` indicating success or failure type

**Example:**
```cpp
SaveFileHeader header;
header.timestamp = time(nullptr);
header.creatureCount = creatures.size();
SaveResult result = saveManager.saveGame("my_save.evos", header, worldData, creatureData, foodData);
```

---

##### quickSave

```cpp
SaveResult quickSave(const SaveFileHeader& header,
                     const WorldSaveData& world,
                     const std::vector<CreatureSaveData>& creatures,
                     const std::vector<FoodSaveData>& food)
```

Saves to the quicksave slot.

**Returns:** `SaveResult`

---

##### loadGame

```cpp
LoadResult loadGame(const std::string& filename, SaveFileHeader& header,
                    WorldSaveData& world,
                    std::vector<CreatureSaveData>& creatures,
                    std::vector<FoodSaveData>& food)
```

Loads game state from a file.

**Parameters:**
- `filename` - Save file to load
- `header` - Output for save metadata
- `world` - Output for world state
- `creatures` - Output for creature data
- `food` - Output for food data

**Returns:** `LoadResult` indicating success or failure type

---

##### quickLoad

```cpp
LoadResult quickLoad(SaveFileHeader& header,
                     WorldSaveData& world,
                     std::vector<CreatureSaveData>& creatures,
                     std::vector<FoodSaveData>& food)
```

Loads from the quicksave slot.

**Returns:** `LoadResult`

---

##### Save Slot Management

```cpp
std::vector<SaveSlotInfo> listSaveSlots()
bool deleteSave(const std::string& filename)
bool renameSave(const std::string& oldName, const std::string& newName)
SaveSlotInfo getSaveInfo(const std::string& filename)
```

---

##### Auto-Save Methods

```cpp
void enableAutoSave(float intervalSeconds)
void disableAutoSave()
bool isAutoSaveEnabled() const
float getAutoSaveInterval() const
bool update(float dt)
void setAutoSaveCallback(AutoSaveCallback callback)
```

**Example:**
```cpp
saveManager.enableAutoSave(300.0f);  // 5 minute auto-save
saveManager.setAutoSaveCallback([&](const std::string& path) {
    // Gather current state and save
    saveManager.saveGame(path, header, world, creatures, food);
});
```

---

##### getLastError

```cpp
const std::string& getLastError() const
```

**Returns:** Description of the last error that occurred

---

##### getDefaultSaveDirectory

```cpp
static std::string getDefaultSaveDirectory()
```

**Returns:** Platform-appropriate default save directory path

---

### DayNightCycle

Time-of-day system for dynamic lighting and sky colors.

#### Public Member Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `dayTime` | float | 0.25 | Time of day (0=midnight, 0.5=noon) |
| `dayLengthSeconds` | float | 120.0 | Duration of full day cycle |
| `paused` | bool | false | Pause time progression |

---

#### Methods

##### Update

```cpp
void Update(float dt)
```

Advances the day/night cycle.

**Parameters:**
- `dt` - Delta time in seconds

**Example:**
```cpp
dayNight.Update(deltaTime);
if (dayNight.IsNight()) {
    // Reduce creature activity
}
```

---

##### Time Query Methods

```cpp
bool IsDay() const
bool IsNight() const
bool IsDawn() const
bool IsDusk() const
bool IsMidnight() const
bool IsNoon() const
```

**Returns:** `true` if currently in that time period

---

##### GetSunAngle

```cpp
float GetSunAngle() const
```

**Returns:** Sun angle in radians (0 to PI during day)

---

##### GetSunPosition

```cpp
Vec3 GetSunPosition() const
```

**Returns:** World position of the sun for lighting calculations

---

##### GetMoonPosition

```cpp
Vec3 GetMoonPosition() const
```

**Returns:** World position of the moon

---

##### GetLightDirection

```cpp
Vec3 GetLightDirection() const
```

**Returns:** Normalized direction from world origin to sun/moon

---

##### GetSkyColors

```cpp
SkyColors GetSkyColors() const
```

**Returns:** `SkyColors` structure with sky and lighting colors for current time

**Example:**
```cpp
SkyColors colors = dayNight.GetSkyColors();
shader.setUniform("skyTop", colors.skyTop);
shader.setUniform("sunColor", colors.sunColor);
shader.setUniform("sunIntensity", colors.sunIntensity);
```

---

##### GetStarVisibility

```cpp
float GetStarVisibility() const
```

**Returns:** Star visibility factor (0 = no stars, 1 = full stars)

---

##### GetCreatureActivityMultiplier

```cpp
float GetCreatureActivityMultiplier(CreatureType type) const
```

Gets activity modifier based on time of day and creature type.

**Parameters:**
- `type` - Creature type

**Returns:** Activity multiplier (carnivores more active at dawn/dusk)

---

##### GetTimeOfDayString

```cpp
const char* GetTimeOfDayString() const
```

**Returns:** Human-readable time of day ("Dawn", "Morning", "Noon", etc.)

---

## Animation Classes

### Skeleton

Bone hierarchy for skeletal animation.

#### Constructor

```cpp
Skeleton()
```

Creates an empty skeleton.

---

#### Methods

##### addBone

```cpp
int32_t addBone(const std::string& name, int32_t parentIndex, const BoneTransform& bindPose)
```

Adds a bone to the skeleton hierarchy.

**Parameters:**
- `name` - Unique bone identifier
- `parentIndex` - Index of parent bone (-1 for root)
- `bindPose` - Initial transform in local space

**Returns:** Index of the newly added bone

**Example:**
```cpp
Skeleton skeleton;
int root = skeleton.addBone("root", -1, BoneTransform::identity());
int spine = skeleton.addBone("spine", root, spineTransform);
```

---

##### findBoneIndex

```cpp
int32_t findBoneIndex(const std::string& name) const
```

Finds a bone by name.

**Parameters:**
- `name` - Bone name to search for

**Returns:** Bone index, or -1 if not found

---

##### getBone

```cpp
const Bone& getBone(uint32_t index) const
Bone& getBone(uint32_t index)
```

Gets bone by index.

**Parameters:**
- `index` - Bone index

**Returns:** Reference to the bone

---

##### getBones / getBoneCount

```cpp
const std::vector<Bone>& getBones() const
uint32_t getBoneCount() const
```

---

##### getRootBones

```cpp
std::vector<int32_t> getRootBones() const
```

**Returns:** Indices of all root bones (bones with no parent)

---

##### getChildBones

```cpp
std::vector<int32_t> getChildBones(int32_t parentIndex) const
```

Gets indices of all direct children of a bone.

**Parameters:**
- `parentIndex` - Parent bone index

**Returns:** Vector of child bone indices

---

##### isDescendant

```cpp
bool isDescendant(int32_t boneIndex, int32_t ancestorIndex) const
```

Checks if a bone is a descendant of another.

**Parameters:**
- `boneIndex` - Potential descendant
- `ancestorIndex` - Potential ancestor

**Returns:** `true` if boneIndex descends from ancestorIndex

---

##### calculateBoneWorldTransform

```cpp
glm::mat4 calculateBoneWorldTransform(uint32_t boneIndex) const
```

Calculates world-space transform for a bone.

**Parameters:**
- `boneIndex` - Bone to calculate transform for

**Returns:** 4x4 world transformation matrix

---

##### calculateBoneLengths

```cpp
void calculateBoneLengths()
```

Calculates bone lengths from the hierarchy structure.

---

##### isValid

```cpp
bool isValid() const
```

Validates skeleton integrity.

**Returns:** `true` if skeleton has valid structure

---

### BoneTransform

Transform data for a single bone.

#### Member Variables

| Variable | Type | Default | Description |
|----------|------|---------|-------------|
| `translation` | glm::vec3 | (0,0,0) | Position offset |
| `rotation` | glm::quat | identity | Orientation |
| `scale` | glm::vec3 | (1,1,1) | Scale factor |

---

#### Methods

##### toMatrix

```cpp
glm::mat4 toMatrix() const
```

Converts transform to 4x4 matrix (Translation * Rotation * Scale order).

**Returns:** Transformation matrix

---

##### lerp

```cpp
static BoneTransform lerp(const BoneTransform& a, const BoneTransform& b, float t)
```

Interpolates between two transforms.

**Parameters:**
- `a` - Start transform
- `b` - End transform
- `t` - Interpolation factor (0 to 1)

**Returns:** Interpolated transform

---

##### identity

```cpp
static BoneTransform identity()
```

**Returns:** Identity transform (no translation, rotation, or scale)

---

### SkeletonFactory

Factory functions for creating common skeleton types.

#### Functions

##### createBiped

```cpp
Skeleton createBiped(float height = 1.0f)
```

Creates a bipedal (humanoid) skeleton.

**Parameters:**
- `height` - Overall creature height (default: 1.0)

**Returns:** Configured biped skeleton

---

##### createQuadruped

```cpp
Skeleton createQuadruped(float length = 1.0f, float height = 0.5f)
```

Creates a four-legged skeleton.

**Parameters:**
- `length` - Body length (default: 1.0)
- `height` - Body height (default: 0.5)

**Returns:** Configured quadruped skeleton

---

##### createSerpentine

```cpp
Skeleton createSerpentine(float length = 2.0f, int segments = 8)
```

Creates a snake/worm skeleton.

**Parameters:**
- `length` - Total body length (default: 2.0)
- `segments` - Number of spine segments (default: 8)

**Returns:** Configured serpentine skeleton

---

##### createFlying

```cpp
Skeleton createFlying(float wingspan = 1.5f)
```

Creates a bird/bat skeleton with wings.

**Parameters:**
- `wingspan` - Total wing span (default: 1.5)

**Returns:** Configured flying skeleton

---

##### createAquatic

```cpp
Skeleton createAquatic(float length = 1.0f, int segments = 5)
```

Creates a fish skeleton.

**Parameters:**
- `length` - Body length (default: 1.0)
- `segments` - Spine segments (default: 5)

**Returns:** Configured aquatic skeleton

---

## Enumerations

### CreatureType

Classifies creatures by ecological role.

| Value | Description |
|-------|-------------|
| `GRAZER` | Grass-eating herbivore (cow/deer analog) |
| `BROWSER` | Leaf-eating herbivore (giraffe analog) |
| `FRUGIVORE` | Fruit-eating herbivore |
| `SMALL_PREDATOR` | Hunts small herbivores (fox analog) |
| `OMNIVORE` | Eats plants and small prey (bear analog) |
| `APEX_PREDATOR` | Top predator (wolf/lion analog) |
| `SCAVENGER` | Eats corpses (vulture analog) |
| `PARASITE` | Drains host energy |
| `CLEANER` | Removes parasites (symbiont) |
| `FLYING` | Generic flying creature |
| `FLYING_BIRD` | Bird with feathered wings |
| `FLYING_INSECT` | Insect with membrane wings |
| `AERIAL_PREDATOR` | Aerial hunter (hawk/eagle) |
| `AQUATIC` | Generic fish |
| `AQUATIC_HERBIVORE` | Small plant-eating fish |
| `AQUATIC_PREDATOR` | Predatory fish (bass/pike) |
| `AQUATIC_APEX` | Apex aquatic predator (shark) |
| `AMPHIBIAN` | Land and water creature |

---

### DietType

Defines what a creature can consume.

| Value | Description |
|-------|-------------|
| `GRASS_ONLY` | Grazers |
| `BROWSE_ONLY` | Browsers (leaves, twigs) |
| `FRUIT_ONLY` | Frugivores |
| `PLANT_GENERALIST` | Any plant matter |
| `SMALL_PREY` | Small creatures only |
| `LARGE_PREY` | Large creatures |
| `ALL_PREY` | Any creature |
| `CARRION` | Dead creatures only |
| `OMNIVORE_FLEX` | Plants + small prey |
| `AQUATIC_ALGAE` | Underwater plants |
| `AQUATIC_SMALL_PREY` | Small aquatic creatures |
| `AQUATIC_ALL_PREY` | All aquatic creatures |

---

### TrophicLevel

Position in the food chain.

| Value | Description |
|-------|-------------|
| `PRODUCER` | Plants (not a creature) |
| `PRIMARY_CONSUMER` | Herbivores |
| `SECONDARY_CONSUMER` | Small predators, omnivores |
| `TERTIARY_CONSUMER` | Apex predators |
| `DECOMPOSER` | Detritivores |

---

### WeatherType

Weather conditions.

| Value | Description |
|-------|-------------|
| `CLEAR` | Sunny, no clouds |
| `PARTLY_CLOUDY` | Some clouds |
| `OVERCAST` | Full cloud cover |
| `RAIN_LIGHT` | Drizzle |
| `RAIN_HEAVY` | Downpour |
| `THUNDERSTORM` | Heavy rain + lightning |
| `SNOW_LIGHT` | Light snowfall |
| `SNOW_HEAVY` | Blizzard |
| `FOG` | Ground fog |
| `MIST` | Light fog |
| `SANDSTORM` | Desert weather |
| `WINDY` | Strong winds |

---

### BiomeType

Ecological biome classifications.

| Value | Description |
|-------|-------------|
| `DEEP_OCEAN` | Deep water |
| `SHALLOW_WATER` | Near-shore water |
| `BEACH` | Coastal sand |
| `TROPICAL_RAINFOREST` | Hot, wet forest |
| `TROPICAL_SEASONAL` | Hot seasonal forest |
| `TEMPERATE_FOREST` | Moderate climate forest |
| `TEMPERATE_GRASSLAND` | Plains, prairie |
| `BOREAL_FOREST` | Cold conifer forest |
| `TUNDRA` | Arctic plains |
| `ICE` | Permanent ice |
| `DESERT_HOT` | Hot desert |
| `DESERT_COLD` | Cold desert |
| `SAVANNA` | Tropical grassland |
| `SWAMP` | Wetland |
| `MOUNTAIN_MEADOW` | Alpine meadow |
| `MOUNTAIN_ROCK` | Rocky highlands |
| `MOUNTAIN_SNOW` | Snow-capped peaks |

---

## Helper Functions

### Creature Type Helpers

```cpp
const char* getCreatureTypeName(CreatureType type)
```
Returns human-readable name for a creature type.

```cpp
bool isHerbivore(CreatureType type)
```
Returns `true` if type is a herbivore (GRAZER, BROWSER, FRUGIVORE).

```cpp
bool isPredator(CreatureType type)
```
Returns `true` if type is a predator.

```cpp
bool isFlying(CreatureType type)
```
Returns `true` if type is a flying creature.

```cpp
bool isBirdType(CreatureType type)
```
Returns `true` if type is a bird (FLYING_BIRD, AERIAL_PREDATOR).

```cpp
bool isInsectType(CreatureType type)
```
Returns `true` if type is FLYING_INSECT.

```cpp
bool isAquatic(CreatureType type)
```
Returns `true` if type is an aquatic creature.

```cpp
bool isAquaticPredator(CreatureType type)
```
Returns `true` if type is an aquatic predator.

```cpp
bool isAquaticPrey(CreatureType type)
```
Returns `true` if type is aquatic prey.

```cpp
bool canSurviveOnLand(CreatureType type)
```
Returns `true` if creature can survive on land.

```cpp
bool canSurviveInWater(CreatureType type)
```
Returns `true` if creature can survive in water.

```cpp
bool canBeHuntedBy(CreatureType prey, CreatureType predator, float preySize)
```
Determines if a prey type can be hunted by a predator type based on size.

```cpp
bool canBeHuntedByAquatic(CreatureType prey, CreatureType predator, float preySize)
```
Determines aquatic hunting relationships.

---

## Constants

### Skeleton Constants

```cpp
namespace animation {
    constexpr uint32_t MAX_BONES = 64;
    constexpr uint32_t MAX_BONES_PER_VERTEX = 4;
}
```

### Genome Constants

```cpp
class Genome {
    static const int NEURAL_WEIGHT_COUNT = 24;
    static const int SENSORY_NEURAL_WEIGHT_COUNT = 120;
};
```

### SpatialGrid Constants

```cpp
class SpatialGrid {
    static constexpr int MAX_PER_CELL = 64;
};
```

---

## Data Structures

### SaveSlotInfo

Information about a save file slot.

| Field | Type | Description |
|-------|------|-------------|
| `filename` | string | Full path to save file |
| `displayName` | string | User-friendly name |
| `timestamp` | uint64_t | Unix timestamp |
| `creatureCount` | uint32_t | Number of creatures |
| `generation` | uint32_t | Highest generation |
| `simulationTime` | float | Total simulation time |
| `valid` | bool | Whether file is valid |

### SkyColors

Sky rendering parameters.

| Field | Type | Description |
|-------|------|-------------|
| `skyTop` | Vec3 | Zenith sky color |
| `skyHorizon` | Vec3 | Horizon sky color |
| `sunColor` | Vec3 | Sun/moon light color |
| `ambientColor` | Vec3 | Ambient light color |
| `sunIntensity` | float | Light intensity multiplier |

### WeatherState

Current weather parameters.

| Field | Type | Description |
|-------|------|-------------|
| `type` | WeatherType | Current weather |
| `cloudCoverage` | float | Cloud coverage (0-1) |
| `precipitationIntensity` | float | Rain/snow intensity |
| `windDirection` | glm::vec2 | Wind direction vector |
| `windStrength` | float | Wind speed (0-1) |
| `fogDensity` | float | Fog density (0-1) |
| `lightningIntensity` | float | Lightning flash brightness |
| `temperatureModifier` | float | Temperature offset |
| `groundWetness` | float | Ground wetness (0-1) |

### ClimateData

Climate information at a position.

| Field | Type | Description |
|-------|------|-------------|
| `temperature` | float | Temperature in Celsius |
| `moisture` | float | Moisture level (0-1) |
| `elevation` | float | Normalized elevation |
| `slope` | float | Terrain slope (0-1) |
| `distanceToWater` | float | Distance to water |
| `latitude` | float | World latitude (-1 to 1) |

### VegetationDensity

Vegetation spawning parameters.

| Field | Type | Description |
|-------|------|-------------|
| `treeDensity` | float | Tree spawn density |
| `grassDensity` | float | Grass spawn density |
| `flowerDensity` | float | Flower spawn density |
| `shrubDensity` | float | Shrub spawn density |
| `fernDensity` | float | Fern spawn density |
| `cactusDensity` | float | Cactus spawn density |

---

*This API Reference was generated from the OrganismEvolution source code. For implementation details and examples, refer to the source files in the `src/` directory.*
