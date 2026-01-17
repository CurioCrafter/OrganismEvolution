# Ecosystem Dynamics Research & Design Document

## Overview

This document outlines the ecological research foundation and design decisions for implementing a multi-trophic, self-balancing ecosystem in the OrganismEvolution simulation.

---

## 1. Trophic Levels and Energy Transfer

### The 10% Rule

Energy transfer between trophic levels is inherently inefficient. On average, only **10% of energy** available at one trophic level is passed to the next. This is known as the **10% rule** (Lindeman's efficiency).

**Why energy transfer is inefficient:**
- Laws of thermodynamics: energy lost as heat (entropy)
- Cellular respiration costs for growth, maintenance, movement
- Non-predatory death (organisms die without being consumed)
- Egestion (waste products)
- Only biomass (not total energy consumed) transfers to next level

**Implications for our simulation:**
- Maximum of 4-5 trophic levels can be sustained
- Pyramid of biomass: producers >> primary consumers >> secondary consumers >> apex predators
- Population ratios should roughly follow 10:1 between adjacent trophic levels

### Trophic Structure Implementation

```
Level 5: Apex Predators (rare)
Level 4: Tertiary Consumers (small predators, omnivores)
Level 3: Secondary Consumers (small carnivores, parasites)
Level 2: Primary Consumers (herbivores - grazers, browsers, frugivores)
Level 1: Primary Producers (grass, bushes, trees)
Level 0: Decomposers (fungi, bacteria - recycle nutrients)
```

**Energy flow in simulation:**
- Producers: Convert sunlight → 100% base energy
- Primary consumers: Gain 10-15% of plant energy consumed
- Secondary consumers: Gain 8-12% of prey energy
- Tertiary/Apex: Gain 5-10% of prey energy

**Sources:**
- [National Geographic - Energy Flow](https://education.nationalgeographic.org/resource/energy-flow-and-10-percent-rule/)
- [Wikipedia - Ecological Efficiency](https://en.wikipedia.org/wiki/Ecological_efficiency)

---

## 2. Predator-Prey Dynamics

### Classical Lotka-Volterra Model

The Lotka-Volterra equations describe predator-prey population dynamics:

```
dN/dt = rN - aNP       (Prey population change)
dP/dt = baNP - mP      (Predator population change)

Where:
N = prey population
P = predator population
r = prey growth rate
a = predation rate coefficient
b = conversion efficiency (prey → predator biomass)
m = predator mortality rate
```

**Limitations of Classical Model:**
1. Assumes unlimited prey food supply
2. Predators have unlimited appetite (linear functional response)
3. No spatial or age structure
4. No environmental variability
5. Produces unstable oscillations dependent on initial conditions
6. No carrying capacity for prey

### Rosenzweig-MacArthur Model (Improved)

Incorporates density-dependent prey growth and saturating functional response:

```
dN/dt = rN(1 - N/K) - f(N)P
dP/dt = ef(N)P - mP

Where:
K = carrying capacity for prey
f(N) = functional response (Holling Type II or III)
e = conversion efficiency
```

### Holling's Functional Responses

**Type I (Linear):**
- f(N) = aN
- Unrealistic for most predators
- Used in basic Lotka-Volterra

**Type II (Saturating):**
- f(N) = aN / (1 + ahN)
- h = handling time per prey
- Predator consumption saturates at high prey density
- Most common in nature
- Can cause paradox of enrichment (system destabilization)

**Type III (Sigmoidal):**
- f(N) = aN² / (1 + ahN²)
- Low consumption at low prey density (prey switching/learning)
- Stabilizes prey populations at low density
- Best for generalist predators

**Implementation Decision:**
Use Type II functional response for specialist predators (carnivores hunting specific prey) and Type III for generalist predators (omnivores) to create natural stability.

**Sources:**
- [Wikipedia - Lotka-Volterra equations](https://en.wikipedia.org/wiki/Lotka–Volterra_equations)
- [Wikipedia - Functional response](https://en.wikipedia.org/wiki/Functional_response)
- [Mathematics LibreTexts - Lotka-Volterra Model](https://math.libretexts.org/Bookshelves/Applied_Mathematics/Mathematical_Biology_(Chasnov)/01:_Population_Dynamics/1.04:_The_Lotka-Volterra_Predator-Prey_Model)

---

## 3. Carrying Capacity and Density-Dependent Effects

### Carrying Capacity (K)

The maximum population size an environment can sustain indefinitely, determined by:
- Food/resource availability
- Space/territory
- Shelter availability
- Predation pressure
- Disease prevalence

### Logistic Growth Model

```
dN/dt = rN(1 - N/K)

At N << K: exponential growth
At N = K: zero growth (equilibrium)
At N > K: negative growth (population decline)
```

### Density-Dependent Factors

Effects that intensify as population density increases:

1. **Intraspecific Competition**
   - Contest competition: direct fighting for resources
   - Scramble competition: resources divided among all
   - Implementation: Reduce feeding efficiency when many creatures nearby

2. **Predation Pressure**
   - Higher density = easier to find prey
   - Predator aggregation in high-density areas
   - Implementation: Predator detection radius scales with prey density

3. **Disease/Parasite Transmission**
   - Higher density = faster disease spread
   - Implementation: Parasite transmission probability based on proximity

4. **Reproductive Suppression**
   - Crowding stress reduces reproduction
   - Implementation: Increase reproduction threshold energy at high density

5. **Resource Depletion**
   - Local resources consumed faster
   - Implementation: Plant regrowth affected by herbivore density

### Density-Independent Factors

Effects that occur regardless of population size:
- Seasonal changes (temperature, day length)
- Natural disasters
- Implementation: Seasonal cycles affecting food availability

**Sources:**
- [Britannica - Population Ecology](https://www.britannica.com/science/population-ecology/Logistic-population-growth)
- [Nature - Population Limiting Factors](https://www.nature.com/scitable/knowledge/library/population-limiting-factors-17059572/)

---

## 4. Symbiotic Relationships

### Mutualism (+/+)

Both species benefit from the interaction.

**Examples for Implementation:**
1. **Cleaner Relationships**
   - Small fish remove parasites from larger fish
   - Cleaner gains food, host loses parasites
   - Implementation: Cleaner creatures seek parasitized hosts, remove parasites, gain energy

2. **Pollinator Relationships**
   - Not applicable to current fauna focus

3. **Mutual Defense**
   - Species warning each other of predators
   - Implementation: Alarm calls spread fear to nearby prey species

### Commensalism (+/0)

One species benefits, the other is unaffected.

**Examples for Implementation:**
1. **Scavenger Following**
   - Scavengers follow apex predators to find kills
   - Implementation: Scavengers track predator kills

2. **Shelter Sharing**
   - Small creatures shelter near larger ones for protection
   - Implementation: Reduce predation risk near large non-predator creatures

### Parasitism (+/-)

Parasite benefits at host's expense (without immediate death).

**Implementation:**
```cpp
struct Parasite {
    float energyDrain;      // Energy stolen per second (0.1-0.5)
    float transmissionRate; // Probability of spreading (0.01-0.1)
    float virulence;        // Damage to host fitness (0.05-0.2)
    float duration;         // Time before dying/being cleared
};
```

**Parasite Behavior:**
- Spreads through close proximity
- Drains host energy slowly
- Reduces host reproduction success
- May transfer when host is eaten
- Natural immunity/resistance can evolve

**Sources:**
- [Wikipedia - Symbiosis](https://en.wikipedia.org/wiki/Symbiosis)
- [Biology LibreTexts - Symbiosis](https://bio.libretexts.org/Bookshelves/Introductory_and_General_Biology/General_Biology_(Boundless)/45:_Population_and_Community_Ecology/45.05:_Community_Ecology/45.5C:_Symbiosis)

---

## 5. Nutrient Cycling and Decomposition

### The Nutrient Cycle

```
Sunlight → Producers → Consumers → Death → Decomposition → Nutrients → Producers
```

Unlike energy (which flows unidirectionally), nutrients **cycle** through the ecosystem.

### Key Nutrient Cycles

**Simplified for Simulation:**
1. **Carbon Cycle**
   - Producers fix carbon (photosynthesis)
   - Consumers release carbon (respiration)
   - Decomposers return carbon to soil/atmosphere

2. **Nitrogen Cycle**
   - Essential for protein synthesis
   - Limiting nutrient in many ecosystems
   - Decomposition releases ammonia → nitrates → plant uptake

3. **Phosphorus Cycle**
   - Does not have atmospheric component
   - Cycles through soil/water/organisms
   - Often the limiting nutrient

### Implementation: Soil Nutrient System

```cpp
struct SoilTile {
    float nitrogen;      // 0-100, affects plant growth
    float phosphorus;    // 0-100, affects plant reproduction
    float organicMatter; // 0-100, from dead creatures
    float moisture;      // 0-100, affects decomposition rate
};
```

### Decomposer System

Decomposers break down dead organic matter, releasing nutrients:

```cpp
struct Corpse {
    glm::vec3 position;
    float biomass;           // Energy content remaining
    float decompositionRate; // Affected by moisture, temperature
    CreatureType sourceType; // What died
};

class DecomposerManager {
    void update(float deltaTime) {
        for (Corpse& corpse : corpses) {
            float rate = baseRate * moisture * temperature;
            float decomposed = corpse.biomass * rate * deltaTime;
            corpse.biomass -= decomposed;

            // Return nutrients to soil
            SoilTile& soil = getSoilAt(corpse.position);
            soil.nitrogen += decomposed * 0.3f;
            soil.phosphorus += decomposed * 0.1f;
            soil.organicMatter += decomposed * 0.5f;
        }
    }
};
```

**Sources:**
- [Wikipedia - Nutrient cycle](https://en.wikipedia.org/wiki/Nutrient_cycle)
- [Biology LibreTexts - Nutrient Cycles](https://bio.libretexts.org/Bookshelves/Botany/Botany_(Ha_Morrow_and_Algiers)/04:_Plant_Physiology_and_Regulation/4.03:_Nutrition_and_Soils/4.3.03:_Nutrient_Cycles)

---

## 6. Creature Type Specifications

### Trophic Level 1: Producers

#### Grass
- **Growth Rate:** Fast (regrows in 10-30 seconds)
- **Energy Value:** Low (5 energy per patch)
- **Distribution:** Dense in grassland biomes
- **Seasonal:** Grows fast in spring/summer, dormant in winter

#### Bushes
- **Growth Rate:** Medium (regrows in 30-60 seconds)
- **Energy Value:** Medium (15 energy, includes berries)
- **Distribution:** Forest edges, transition zones
- **Seasonal:** Berries in summer/fall only

#### Trees
- **Growth Rate:** Slow (fruit regrows in 60-120 seconds)
- **Energy Value:** High (25 energy from fruit/leaves)
- **Distribution:** Forest biomes
- **Seasonal:** Leaves in spring/summer/fall, fruit in summer/fall

### Trophic Level 2: Primary Consumers (Herbivores)

#### Grazer (e.g., cow, deer analog)
- **Diet:** Grass only
- **Size:** Medium-Large (1.0-1.5)
- **Speed:** Medium (8-12)
- **Social:** Herding behavior
- **Metabolism:** Low (efficient grass digestion)

#### Browser (e.g., giraffe analog)
- **Diet:** Tree leaves, bushes
- **Size:** Large (1.3-1.8)
- **Speed:** Slow-Medium (6-10)
- **Social:** Small groups
- **Special:** Reach stat for tree access

#### Frugivore (e.g., small mammal analog)
- **Diet:** Fruits/berries from trees and bushes
- **Size:** Small (0.4-0.8)
- **Speed:** Fast (10-15)
- **Social:** Solitary or pairs
- **Special:** Climbing ability for trees

### Trophic Level 3: Secondary Consumers

#### Small Predator (e.g., fox analog)
- **Diet:** Small herbivores (frugivores)
- **Size:** Small-Medium (0.6-1.0)
- **Speed:** Fast (12-16)
- **Hunting:** Solitary, ambush
- **Cannot hunt:** Large herbivores

#### Omnivore (e.g., bear analog)
- **Diet:** Plants AND small creatures
- **Size:** Large (1.4-2.0)
- **Speed:** Medium (8-12)
- **Behavior:** Diet switching based on availability
- **Special:** Type III functional response (prey switching)

### Trophic Level 4: Tertiary Consumers

#### Apex Predator (e.g., lion/wolf analog)
- **Diet:** All herbivores, small predators
- **Size:** Large (1.5-2.2)
- **Speed:** Fast (14-18)
- **Hunting:** Pack behavior or solitary ambush
- **Rare:** Low population due to energy constraints

#### Scavenger (e.g., vulture analog)
- **Diet:** Corpses (carrion)
- **Size:** Medium (0.8-1.2)
- **Speed:** Medium (10-14)
- **Behavior:** Tracks corpses, follows predators
- **Special:** No hunting cost, but competition for corpses

### Special: Parasites

#### Endoparasite (internal)
- **Transmission:** Proximity-based
- **Effect:** Continuous energy drain (0.2/sec)
- **Duration:** 30-60 seconds
- **Spread:** High at high density

#### Ectoparasite (external)
- **Transmission:** Contact with infected
- **Effect:** Energy drain + reduced speed
- **Duration:** Until cleaned by symbiont
- **Spread:** Medium

### Special: Symbionts

#### Cleaner (e.g., cleaner fish analog)
- **Size:** Very small (0.2-0.4)
- **Behavior:** Seeks parasitized hosts
- **Benefit:** Removes parasites (gains energy)
- **Protection:** Hosts don't attack cleaners

---

## 7. Ecosystem Stability Mechanisms

### Natural Stability Features

1. **Functional Response Saturation**
   - Predators can only consume so much
   - Prevents runaway predation

2. **Prey Refuges**
   - Dense vegetation provides hiding spots
   - Implementation: Reduced detection in forest biomes

3. **Predator Interference**
   - High predator density reduces hunting efficiency
   - Implementation: Beddington-DeAngelis functional response

4. **Diet Switching**
   - Generalists switch to abundant prey
   - Relieves pressure on rare species
   - Implementation: Type III response for omnivores

5. **Territoriality**
   - Limits local predator density
   - Implementation: Carnivore separation behavior

### Emergency Stabilizers

When ecosystem health metrics drop critically:

1. **Adaptive Spawning**
   - Increase spawn rate of depleted trophic level
   - Natural immigration simulation

2. **Carrying Capacity Adjustment**
   - Reduce reproduction at high density
   - Increase reproduction at low density

3. **Seasonal Rescue**
   - Spring "bloom" of producers
   - Reset mechanism for depleted plants

---

## 8. Seasonal Cycles

### Season Definitions

```cpp
enum class Season {
    SPRING,  // Days 0-90: Growth, reproduction
    SUMMER,  // Days 91-180: Peak abundance
    FALL,    // Days 181-270: Harvest, preparation
    WINTER   // Days 271-360: Scarcity, dormancy
};
```

### Seasonal Effects

| Factor | Spring | Summer | Fall | Winter |
|--------|--------|--------|------|--------|
| Plant Growth | 1.5x | 1.0x | 0.5x | 0.1x |
| Fruit/Berries | 0.0x | 1.0x | 1.5x | 0.0x |
| Herbivore Reproduction | 1.5x | 1.0x | 0.5x | 0.2x |
| Carnivore Activity | 1.0x | 1.0x | 1.2x | 0.8x |
| Metabolism | 1.0x | 1.0x | 1.2x | 1.5x |
| Decomposition | 1.0x | 1.5x | 1.0x | 0.3x |

### Implementation

```cpp
class SeasonManager {
    float dayLength = 60.0f;  // 60 real seconds = 1 game day
    float yearLength = 360.0f; // 360 days = 1 year

    Season getSeason(float gameTime) {
        float day = fmod(gameTime / dayLength, yearLength);
        if (day < 90) return Season::SPRING;
        if (day < 180) return Season::SUMMER;
        if (day < 270) return Season::FALL;
        return Season::WINTER;
    }

    float getGrowthMultiplier(Season s) {
        switch(s) {
            case SPRING: return 1.5f;
            case SUMMER: return 1.0f;
            case FALL: return 0.5f;
            case WINTER: return 0.1f;
        }
    }
};
```

---

## 9. Ecosystem Health Metrics

### Key Metrics to Track

```cpp
struct EcosystemMetrics {
    // Population metrics
    int producerBiomass;
    int herbivoreCount;
    int carnivoreCount;
    int decomposerActivity;

    // Diversity metrics
    float speciesDiversity;      // Shannon index
    float trophicBalance;        // Ratio between levels

    // Energy metrics
    float totalEnergyInSystem;
    float energyFlowRate;        // Energy/second moving through system
    float decompositionRate;

    // Stability metrics
    float populationVariance;    // Rolling variance over time
    float extinctionRisk;        // Species below critical threshold

    // Health score (0-100)
    float calculateHealthScore() {
        float score = 100.0f;

        // Penalize trophic imbalance
        float idealRatio = 10.0f;
        float actualRatio = (float)herbivoreCount / max(1, carnivoreCount);
        score -= abs(idealRatio - actualRatio) * 2;

        // Penalize low diversity
        score -= (1.0f - speciesDiversity) * 20;

        // Penalize extinction risk
        score -= extinctionRisk * 30;

        // Penalize high variance (unstable)
        score -= populationVariance * 0.5f;

        return clamp(score, 0.0f, 100.0f);
    }
};
```

### Warning Thresholds

| Metric | Warning | Critical |
|--------|---------|----------|
| Herbivore Population | < 20 | < 10 |
| Carnivore Population | < 5 | < 2 |
| Producer Coverage | < 40% | < 20% |
| Species Diversity | < 0.5 | < 0.3 |
| Trophic Ratio | < 5:1 or > 20:1 | < 3:1 or > 30:1 |

---

## 10. Implementation Roadmap

### Phase 1: Producer Layer
1. Implement grass patches with growth/consumption
2. Add bush system with berry production
3. Enhance tree system with fruit/leaf consumption
4. Add soil nutrient grid affecting plant growth

### Phase 2: Herbivore Diversity
1. Create Grazer type (grass specialist)
2. Create Browser type (tree/bush specialist)
3. Create Frugivore type (fruit specialist)
4. Implement dietary preferences and efficiency

### Phase 3: Carnivore Tiers
1. Add Small Predator (targets small herbivores)
2. Enhance existing Carnivore as Apex Predator
3. Add Scavenger type (corpse-based diet)
4. Implement size-based hunting restrictions

### Phase 4: Complex Relationships
1. Add Omnivore with diet switching
2. Implement parasite system
3. Add cleaner symbiont relationship
4. Implement alarm/warning behaviors

### Phase 5: Ecosystem Management
1. Create corpse/decomposition system
2. Implement nutrient cycling
3. Add seasonal cycles
4. Build ecosystem health dashboard

### Phase 6: Balancing & Polish
1. Tune energy transfer efficiencies
2. Balance population ratios
3. Test long-term stability
4. Add emergency stabilization mechanisms

---

## References

1. Lotka-Volterra equations - [Wikipedia](https://en.wikipedia.org/wiki/Lotka–Volterra_equations)
2. Functional Response - [Wikipedia](https://en.wikipedia.org/wiki/Functional_response)
3. Energy Flow and the 10% Rule - [National Geographic](https://education.nationalgeographic.org/resource/energy-flow-and-10-percent-rule/)
4. Population Ecology - [Britannica](https://www.britannica.com/science/population-ecology/Logistic-population-growth)
5. Symbiosis - [Biology LibreTexts](https://bio.libretexts.org/Bookshelves/Introductory_and_General_Biology/General_Biology_(Boundless)/45:_Population_and_Community_Ecology/45.05:_Community_Ecology/45.5C:_Symbiosis)
6. Nutrient Cycles - [Wikipedia](https://en.wikipedia.org/wiki/Nutrient_cycle)
7. Ecological Efficiency - [Wikipedia](https://en.wikipedia.org/wiki/Ecological_efficiency)
