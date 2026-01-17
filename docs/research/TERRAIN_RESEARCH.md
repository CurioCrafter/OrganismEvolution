# Procedural Terrain Generation Research

## Overview

This document compiles research on procedural terrain generation techniques applicable to the OrganismEvolution project. The goal is to create realistic, varied terrain that supports an ecosystem simulation with multiple biomes and natural-looking landscapes.

---

## 1. Heightmap Generation Techniques

### 1.1 Perlin Noise

**Background:**
Ken Perlin developed Perlin noise in 1982 for the movie Tron. It remains the foundation of most procedural terrain generation.

**Key Characteristics:**
- Produces "coherent" noise where each point's value relates to its neighbors
- Creates smooth, continuous gradients ideal for natural topography
- Computational complexity: O(2^n) for n dimensions

**Implementation Details:**
The current project implementation (`src/utils/PerlinNoise.cpp`) uses the classic Perlin noise algorithm:
- 512-element permutation table for hash-based gradient selection
- Quintic interpolation (`fade` function): `t * t * t * (t * (t * 6 - 15) + 10)`
- Output range normalized to [0, 1]

**Strengths:**
- Well-documented with extensive resources
- No patent concerns
- Established tiling/periodicity techniques
- Faster for 2D terrain generation

**Weaknesses:**
- Directional artifacts (visible axis-aligned patterns)
- Limited dynamic range (tends toward mid-gray values)
- O(2^n) scaling makes higher dimensions expensive

### 1.2 Simplex Noise

**Background:**
Ken Perlin designed simplex noise in 2001 to address limitations of classic Perlin noise, especially in higher dimensions.

**Key Improvements:**
- Uses simplices (triangles in 2D, tetrahedrons in 3D) instead of hypercubes
- Lower computational complexity in higher dimensions
- Fewer directional artifacts
- Greater dynamic range (more contrast between highs and lows)

**Performance Comparison:**
| Dimension | Perlin Noise | Simplex Noise |
|-----------|-------------|---------------|
| 2D        | Faster      | Comparable    |
| 3D        | Comparable  | Faster        |
| 4D+       | Much slower | Significantly faster |

**Recommendation:**
For the OrganismEvolution project, simplex noise is recommended for:
- 3D terrain features (caves, overhangs)
- Time-varying terrain (erosion simulation)
- Any computation requiring 4+ dimensions

**Note:** Use OpenSimplex to avoid patent issues with Perlin's simplex algorithm.

### 1.3 Voronoi Diagrams

**Definition:**
A Voronoi diagram partitions space into regions (cells) around a set of seed points. Each cell contains all points closer to its seed than any other seed.

**Applications in Terrain Generation:**

1. **Biome Distribution:** Each Voronoi cell can represent a distinct biome
2. **Tectonic Plates:** Model continental drift and mountain formation
3. **River Networks:** Edges between cells naturally form drainage patterns
4. **Irregular Grids:** More natural-looking than square grids

**Implementation Techniques:**

**Lloyd Relaxation:**
Iteratively moves seeds to cell centroids, producing more uniform polygons:
```
1. Generate initial Voronoi diagram
2. Calculate centroid of each cell
3. Move seed to centroid
4. Repeat until desired uniformity
```

**Poisson Disc Sampling:**
Generates well-distributed points from the start without iteration. Produces natural-looking distributions ideal for:
- Resource placement
- Tree distribution
- Structure locations

**Hybrid Approaches:**
Combine Voronoi with noise-based elevation:
```
1. Create Voronoi diagram for region boundaries
2. Assign base elevation to each cell
3. Add noise-based detail within cells
4. Smooth transitions along cell edges
```

**Reference Implementation:**
See Amit Patel's [Polygonal Map Generation](http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/) for a comprehensive approach.

---

## 2. Fractal Techniques

### 2.1 Fractal Brownian Motion (fBm)

**Concept:**
Sum multiple layers (octaves) of noise at different frequencies and amplitudes to create detailed terrain.

**Parameters:**
- **Octaves:** Number of noise layers (typically 4-8)
- **Lacunarity:** Frequency multiplier between octaves (typically 2.0)
- **Persistence/Gain:** Amplitude multiplier between octaves (typically 0.5)

**Formula:**
```
total = 0
frequency = 1
amplitude = 1
for each octave:
    total += noise(x * frequency, y * frequency) * amplitude
    frequency *= lacunarity
    amplitude *= persistence
return total / maxAmplitude
```

**Current Implementation:**
The project's `PerlinNoise::octaveNoise()` function already implements basic fBm with configurable octaves and persistence.

**Limitations:**
Simple fBm produces "roundish" terrain without sharp peaks or deep valleys.

### 2.2 Ridged Noise

**Concept:**
Transform noise to create sharp ridges by inverting and squaring absolute values.

**Algorithm:**
```
n = noise(x, y)
n = abs(n)           // Create creases
n = offset - n       // Invert (creases become peaks)
n = n * n            // Sharpen ridges
```

**Applications:**
- Mountain ridges
- Cliff faces
- Eroded canyons

**Ridged Multifractal:**
Apply the ridge transformation at each octave of fBm:
```
for each octave:
    n = noise(x * freq, y * freq)
    n = offset - abs(n)
    n = n * n
    total += n * amplitude * weight
    weight = clamp(n * gain, 0, 1)  // Signal-dependent amplitude
```

### 2.3 Hybrid Multifractal

**Background:**
Developed by F. Kenton Musgrave, this technique combines characteristics of fBm and ridged noise to produce more realistic mountainous terrain with proper peaks and valleys.

**Key Innovation:**
Uses signal-dependent weighting where the contribution of each octave depends on the previous octave's output. This creates:
- Sharp peaks in high areas
- Smooth valleys in low areas

**Formula:**
```
result = (noise(x, y) + offset) * exponent
weight = result
for each octave (starting from 1):
    weight = clamp(weight * gain, 0, 1)
    signal = (noise(x * freq, y * freq) + offset) * exponent
    result += weight * signal
    weight *= signal
```

### 2.4 Domain Warping

**Concept:**
Distort the input coordinates (domain) before evaluating noise to create organic, flowing patterns.

**Basic Formula:**
```
Instead of: f(p)
Use: f(p + h(p))

Where h(p) is a displacement function (often another noise function)
```

**Single-Level Warping:**
```
warp = vec2(noise(p), noise(p + offset))
result = noise(p + warp * strength)
```

**Multi-Level Warping (Inigo Quilez):**
```
q = vec2(fbm(p), fbm(p + offset1))
r = vec2(fbm(p + scale1*q + offset2), fbm(p + scale2*q + offset3))
result = fbm(p + scale3*r)
```

**Octave-Level Warping:**
Apply warping at each octave for a "liquified" effect:
```
for each octave:
    warpedCoord = coord + noise(coord * warpFreq) * warpStrength
    total += noise(warpedCoord * freq) * amplitude
```

**Applications:**
- Organic terrain shapes
- River meanders
- Coastline irregularity
- Cave system shapes

**Reference:**
[Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)

---

## 3. Erosion Algorithms

### 3.1 Hydraulic Erosion

**Concept:**
Simulate water flowing across terrain, eroding material and depositing sediment downstream.

**Data Tracked (Per Grid Cell):**
| Variable | Description |
|----------|-------------|
| `b` | Terrain height (bedrock) |
| `d` | Water height |
| `s` | Suspended sediment |
| `v` | Velocity vector |
| `f` | Outflow flux (fL, fR, fT, fB) |

**Two Main Approaches:**

**Grid-Based (Shallow Water Model):**
1. For each cell, calculate water pressure from depth
2. Compute outflow flux to neighbors based on height differences
3. Update water levels based on net flux
4. Erode/deposit sediment based on velocity
5. Transport suspended sediment with water flow

**Particle-Based (Raindrop Simulation):**
1. Spawn particles at random locations
2. Each particle flows downhill following terrain normals
3. Particle picks up sediment proportional to speed
4. Particle deposits sediment when slowing down
5. Particle dies after set iterations or reaching water

**Key Parameters:**
- **Erosion Rate:** How quickly material is removed
- **Deposition Rate:** How quickly sediment settles
- **Evaporation Rate:** Water loss over time
- **Sediment Capacity:** Maximum sediment per water volume
- **Inertia:** Resistance to direction change

**GPU Implementation:**
Sebastian Lague's implementation demonstrates efficient GPU-based particle erosion:
- [GitHub Repository](https://github.com/SebLague/Hydraulic-Erosion)
- [Demo](https://sebastian.itch.io/hydraulic-erosion)

**Grid-based GPU approach (Stava 2008, Jako 2011):**
- Uses compute shaders for parallel processing
- Requires careful handling of race conditions
- Atomic operations for sediment transfer between cells

### 3.2 Thermal Erosion

**Concept:**
Material falls from steep slopes until the "angle of repose" (talus angle) is reached.

**Algorithm:**
```
for each terrain vertex:
    maxSlope = 0
    steepestNeighbor = null
    for each neighbor:
        slope = (currentHeight - neighborHeight) / distance
        if slope > maxSlope:
            maxSlope = slope
            steepestNeighbor = neighbor

    if maxSlope > talusAngle:
        transfer = (currentHeight - steepestNeighbor.height - talusAngle) * 0.5
        currentHeight -= transfer
        steepestNeighbor.height += transfer
```

**Parameters:**
- **Talus Angle:** Critical slope angle (typically 30-45 degrees)
- **Transfer Rate:** Fraction of excess material moved per iteration
- **Iterations:** Number of simulation steps

**Multi-Neighbor Distribution:**
Distribute material to all downhill neighbors proportionally:
```
totalSlope = sum of slopes to all lower neighbors
for each lower neighbor:
    transfer = excessMaterial * (slope / totalSlope)
```

**Results:**
- Creates talus slopes at cliff bases
- Smooths sharp ridges
- Produces debris fields

### 3.3 Combined Erosion Strategy

**Recommended Pipeline:**
1. Generate base heightmap (fBm + ridged noise)
2. Apply hydraulic erosion (20-50 iterations)
3. Apply thermal erosion (5-10 iterations)
4. Repeat 2-3 for aged terrain

**GPU Pipeline:**
The project's `TerrainHeightmapGenerator` already has infrastructure for compute shaders. Erosion could be added as additional compute passes:
1. Heightmap generation pass
2. Hydraulic erosion pass (multiple dispatches)
3. Thermal erosion pass
4. Final readback for collision

---

## 4. Biome Distribution

### 4.1 Whittaker Diagram

**Background:**
Robert Whittaker's 1975 classification system maps biomes based on two axes:
- **Temperature:** Annual average (horizontal axis)
- **Precipitation:** Annual rainfall (vertical axis)

**Standard Biome Regions:**

| Temperature | Low Precipitation | Medium Precipitation | High Precipitation |
|-------------|------------------|---------------------|-------------------|
| Cold | Tundra | Taiga/Boreal Forest | Taiga |
| Cool | Temperate Grassland | Temperate Forest | Temperate Rainforest |
| Warm | Desert | Savanna | Tropical Seasonal Forest |
| Hot | Desert | Savanna/Shrubland | Tropical Rainforest |

**Implementation:**
```cpp
Biome getBiome(float temperature, float precipitation) {
    // Normalize to 0-1 range
    // Use lookup table or conditional logic
    if (temperature < 0.2f) {
        return (precipitation < 0.3f) ? TUNDRA : TAIGA;
    }
    // ... additional conditions
}
```

### 4.2 Elevation-Based Distribution

**Concept:**
Derive temperature from elevation (lapse rate ~6.5C per 1000m) and combine with moisture for biome selection.

**Implementation:**
```cpp
float getTemperature(float elevation, float baseTemp, float latitude) {
    float lapseRate = 6.5f / 1000.0f;  // degrees per meter
    float latitudeEffect = cos(latitude * PI / 180.0f);
    return baseTemp * latitudeEffect - elevation * lapseRate;
}

float getMoisture(float distanceToWater, float elevation, float windward) {
    // Orographic precipitation on windward slopes
    // Rain shadow on leeward slopes
    float orographic = windward * elevation * 0.001f;
    float rainShadow = (1.0f - windward) * elevation * 0.0005f;
    return baseMoisture + orographic - rainShadow;
}
```

### 4.3 Climate Simulation

**Advanced Approach:**
Simulate atmospheric circulation for realistic climate patterns.

**Components:**
1. **Solar Heating:** Latitude-dependent temperature
2. **Hadley Cells:** Tropical air circulation
3. **Coriolis Effect:** Wind deflection from rotation
4. **Orographic Effects:** Mountain rain shadows
5. **Ocean Currents:** Temperature moderation

**Simplified Implementation:**
```cpp
// Wind direction based on latitude (Hadley cells)
float getWindDirection(float latitude) {
    if (abs(latitude) < 30.0f) return EAST;  // Trade winds
    if (abs(latitude) < 60.0f) return WEST;  // Westerlies
    return EAST;  // Polar easterlies
}

// Precipitation based on wind and terrain
float getPrecipitation(float x, float y, float windDir, const Heightmap& terrain) {
    // Trace upwind to find moisture source
    // Reduce moisture as air rises over terrain
    // Increase precipitation on windward slopes
}
```

**Reference:**
[Joe Duffy - Climate Simulation for Procedural World Generation](https://www.joeduffy.games/climate-simulation-for-procedural-world-generation)

### 4.4 Multi-Parameter Biome Selection

**Extended Whittaker Approach:**
Use additional parameters beyond temperature and precipitation:

| Parameter | Effect |
|-----------|--------|
| Temperature | Primary biome selector |
| Precipitation | Primary biome selector |
| Elevation | Mountain/alpine variants |
| Drainage | Wetland/swamp detection |
| Latitude | Climate zone |
| Distance to Coast | Maritime vs continental |

**Current Implementation Note:**
The project's `Terrain::getTerrainColor()` uses simple height-based biome coloring. This could be extended with a proper Whittaker system using generated temperature and moisture maps.

---

## 5. Game Implementation Case Studies

### 5.1 Minecraft

**World Generation Pipeline:**
1. **Seed:** 64-bit number determines all generation
2. **Biome Parameters:** Continentalness, erosion, peaks/valleys, temperature, humidity
3. **Noise Generation:** Fractal Brownian motion with multiple octaves
4. **Chunk-Based:** 16x16x256 block chunks generated on demand
5. **Carvers:** Subtract caves and ravines from solid terrain
6. **Features:** Place structures, vegetation, ores

**Key Techniques:**
- **Gradient Noise:** All parameters smoothly transition across regions
- **3D Noise:** Enables overhangs, caves, floating islands
- **Density Function:** Determines if block is solid based on multiple noise layers
- **Aquifer System:** Underground water bodies

**Biome Selection (1.18+):**
```
biome = lookup(continentalness, erosion, peaksValleys, temperature, humidity)
```
Each parameter ranges from -1 to +1, enabling smooth biome transitions.

### 5.2 No Man's Sky

**Techniques:**
1. **Deterministic Generation:** Same seed always produces identical results
2. **Voxel-Based:** Base terrain from voxel density field
3. **Polygonization:** Marching cubes for smooth terrain mesh
4. **Perlin Noise:** Foundation for all procedural content
5. **Fractal Mathematics:** Self-similar patterns at multiple scales
6. **L-Systems:** Plant and creature limb generation
7. **Cellular Automata:** Cave systems and structures

**Scale Handling:**
- Galaxy fits in a few gigabytes (mostly audio files)
- Nothing stored permanently - regenerated on demand
- 18.4 quintillion possible planets

**Worlds Updates (Recent):**
- Vastly expanded world size
- Greater topographical diversity
- More varied biome distribution

### 5.3 Valheim

**Hybrid Approach:**
1. **Pre-baked Heightmap:** Foundation stored as image data
2. **Seed Selection:** Chooses which section of heightmap to use
3. **Stamp System:** Pre-made terrain features placed procedurally
4. **Biome Rules:** Location-dependent biome probability
5. **Adjacency Rules:** Stamps connect based on compatibility

**Key Innovation:**
Combines procedural placement with handcrafted detail through "stamps" - pre-designed terrain pieces that procedurally snap together.

**Biome Distribution:**
- Biomes more likely in certain world regions
- Distance from center influences biome type
- Creates progression from starter area outward

---

## 6. Implementation Recommendations

### 6.1 Immediate Improvements

**Enhance Current Perlin Noise:**
1. Add simplex noise implementation for comparison
2. Implement ridged noise variant
3. Add domain warping option

**Code Structure:**
```cpp
class NoiseGenerator {
public:
    enum class NoiseType { Perlin, Simplex, OpenSimplex };
    enum class FractalType { FBM, Ridged, Hybrid };

    void setNoiseType(NoiseType type);
    void setFractalType(FractalType type);
    void setOctaves(int octaves);
    void setLacunarity(float lacunarity);
    void setPersistence(float persistence);
    void setDomainWarp(float strength);

    float sample(float x, float y);
    float sample(float x, float y, float z);
};
```

### 6.2 Advanced Features

**Erosion Pipeline:**
```cpp
class TerrainErosion {
public:
    void setHydraulicIterations(int iterations);
    void setThermalIterations(int iterations);
    void setErosionStrength(float strength);

    // GPU-accelerated erosion
    void applyErosion(ID3D12GraphicsCommandList* cmdList,
                      ID3D12Resource* heightmap);
};
```

**Biome System:**
```cpp
class BiomeManager {
public:
    void generateTemperatureMap(const Heightmap& terrain);
    void generateMoistureMap(const Heightmap& terrain);

    Biome getBiome(float x, float z);
    glm::vec3 getBiomeColor(Biome biome);
    float getVegetationDensity(Biome biome);
};
```

### 6.3 Performance Considerations

**GPU Compute:**
The project already has `TerrainHeightmapGenerator` using compute shaders. Extend this for:
- Noise generation on GPU
- Erosion simulation
- Biome calculation

**Level of Detail:**
- Generate multiple mipmap levels of heightmap
- Use lower detail for distant terrain
- Transition smoothly between LOD levels

**Chunking:**
The project has chunk infrastructure (`TerrainChunk`, `TerrainChunkManager`). Ensure:
- Seamless noise across chunk boundaries
- Consistent erosion at edges
- Biome continuity

---

## 7. References and Resources

### Academic Papers
- F. Kenton Musgrave - [Procedural Fractal Terrains](https://www.classes.cs.uchicago.edu/archive/2015/fall/23700-1/final-project/MusgraveTerrain00.pdf)
- Stava et al. (2008) - [Interactive Terrain Modeling Using Hydraulic Erosion](https://cgg.mff.cuni.cz/~jaroslav/papers/2008-sca-erosim/2008-sca-erosiom-fin.pdf)
- Jako (2011) - [Fast Hydraulic and Thermal Erosion on the GPU](https://old.cescg.org/CESCG-2011/papers/TUBudapest-Jako-Balazs.pdf)

### Online Tutorials
- [The Book of Shaders - Fractal Brownian Motion](https://thebookofshaders.com/13/)
- [Inigo Quilez - Domain Warping](https://iquilezles.org/articles/warp/)
- [Red Blob Games - Polygonal Map Generation](http://www-cs-students.stanford.edu/~amitp/game-programming/polygon-map-generation/)
- [Job Talle - Simulating Hydraulic Erosion](https://jobtalle.com/simulating_hydraulic_erosion.html)
- [Alan Zucconi - Minecraft World Generation](https://www.alanzucconi.com/2022/06/05/minecraft-world-generation/)
- [Axel Paris - Terrain Erosion on GPU](https://aparis69.github.io/public_html/posts/terrain_erosion.html)

### Open Source Implementations
- [Sebastian Lague - Hydraulic Erosion](https://github.com/SebLague/Hydraulic-Erosion) (C#, MIT License)
- [terrain-erosion-3-ways](https://github.com/dandrino/terrain-erosion-3-ways) (Python)
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) (Multi-language noise library)

### Game Developer Resources
- [Minecraft Wiki - World Generation](https://minecraft.wiki/w/World_generation)
- [No Man's Sky Wiki - Procedural Generation](https://nomanssky.fandom.com/wiki/Procedural_generation)
- [GDC Vault - No Man's Sky Continuous World Generation](https://www.gdcvault.com/play/1024265/Continuous-World-Generation-in-No)

---

## 8. Appendix: Current Project Analysis

### Existing Implementation
The OrganismEvolution project currently uses:

**Noise Generation (`src/utils/PerlinNoise.cpp`):**
- Classic Perlin noise with permutation table
- Octave noise (basic fBm) with configurable persistence
- Output normalized to [0, 1]

**Terrain Generation (`src/environment/Terrain.cpp`):**
- 6-octave Perlin noise for base heightmap
- Island falloff using distance-based smoothstep
- Simple height-based biome coloring
- Water level threshold at 0.35

**GPU Heightmap Generator (`src/environment/TerrainHeightmapGenerator.h`):**
- 2048x2048 heightmap texture
- Compute shader-based generation
- Parameters include: base frequency, octaves, persistence, lacunarity
- Erosion strength and ridge sharpness parameters (not fully implemented)
- Voronoi scale and weight parameters (not fully implemented)

### Recommended Enhancements
1. **Noise Library:** Replace custom Perlin with FastNoiseLite for more options
2. **Ridged Noise:** Implement ridged multifractal for mountain peaks
3. **Domain Warping:** Add coordinate distortion for organic shapes
4. **Erosion:** Implement GPU hydraulic and thermal erosion passes
5. **Biome System:** Create temperature/moisture maps for Whittaker classification
6. **Voronoi:** Use cellular noise for biome boundaries

---

*Document generated: January 2026*
*Project: OrganismEvolution*
*Path: C:\Users\andre\Desktop\OrganismEvolution\docs\TERRAIN_RESEARCH.md*
