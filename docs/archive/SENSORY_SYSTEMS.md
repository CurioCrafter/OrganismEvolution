# Sensory Systems Research and Implementation

## Table of Contents
1. [Biological Sensory Modalities](#1-biological-sensory-modalities)
2. [Sensory Evolution and Trade-offs](#2-sensory-evolution-and-trade-offs)
3. [Prey Detection and Predator Evasion](#3-prey-detection-and-predator-evasion)
4. [Spatial Cognition and Mental Maps](#4-spatial-cognition-and-mental-maps)
5. [Swarm Intelligence and Collective Sensing](#5-swarm-intelligence-and-collective-sensing)
6. [Implementation Design](#6-implementation-design)
7. [References](#7-references)

---

## 1. Biological Sensory Modalities

### 1.1 Vision

Vision is the dominant sense for many animals, providing detailed spatial information about the environment. Key aspects include:

**Field of View (FOV):**
- **Predators** typically have forward-facing eyes with binocular vision (90-120° FOV) providing depth perception for accurate strikes
- **Prey** typically have lateral eyes with monocular vision (270-360° FOV) maximizing detection coverage at the cost of depth perception
- The trade-off: narrow FOV = better depth perception; wide FOV = better threat detection

**Color Perception:**
- Varies by number of cone types: humans have 3 (trichromatic), many mammals have 2 (dichromatic)
- Birds and some fish have 4 cone types (tetrachromatic), seeing into UV spectrum
- Color vision is metabolically expensive - nocturnal animals often lose it
- Color aids in: food identification, mate selection, camouflage detection

**Motion Detection:**
- Many species have specialized neurons for detecting motion
- Amphibians detect moving prey but may ignore stationary food
- Motion triggers startle responses faster than pattern recognition
- Peripheral vision is more sensitive to motion than central vision

**Acuity:**
- Visual acuity varies dramatically: eagles see 4-8x better than humans
- Higher acuity requires more photoreceptors and neural processing power
- Trade-off between acuity and light sensitivity (cones vs rods)

### 1.2 Hearing / Audition

Hearing provides information about distant events in all directions, including through obstacles.

**Sound Propagation:**
- Sound travels at ~343 m/s in air, ~1,500 m/s in water
- Attenuates with distance squared (inverse square law)
- Different frequencies propagate differently: low frequencies travel farther
- Obstacles create acoustic shadows, reflections, and diffraction

**Echolocation:**
- Used by bats, dolphins, some birds, and shrews
- Bats emit calls 10-200 kHz, can detect insects at 5m, wires as fine as human hair
- Dolphins can detect golf-ball-sized objects at 110m using 20-130 kHz clicks
- Two call types:
  - **FM (Frequency Modulated)**: Precise range discrimination, shorter range
  - **CF (Constant Frequency)**: Velocity detection via Doppler effect, longer range
- Energy cost: very high - bats turn off middle ear during emission

**Directional Hearing:**
- Uses interaural time difference (ITD) and interaural level difference (ILD)
- Owls can locate prey in complete darkness using hearing alone
- Asymmetric ear placement in owls provides vertical localization

### 1.3 Olfaction (Smell)

Olfaction provides chemical information about the environment, particularly useful for:
- Detecting food at distance
- Tracking prey or conspecifics via trails
- Identifying individuals, reproductive status
- Detecting predators through scent marks

**Pheromone Communication:**
- **Trail pheromones**: Ants mark paths to food, concentration indicates quality
- **Alarm pheromones**: Volatile chemicals spread rapidly, trigger flight or defense
- **Sex pheromones**: Moths can detect single molecules across kilometers
- **Territory markers**: Long-lasting chemicals define boundaries

**Scent Properties:**
- Affected by wind direction and speed
- Persist over time (unlike visual/auditory signals)
- Can indicate age of signal (predator scent freshness)
- Concentration gradients allow directional tracking

### 1.4 Mechanoreception (Touch/Vibration)

**Tactile Sensing:**
- Whiskers (vibrissae) in mammals detect air currents and nearby objects
- Useful in darkness or murky water
- Can detect prey movement through substrate vibrations

**Lateral Line System (Aquatic):**
- Found in fish and aquatic amphibians
- Neuromasts detect water pressure changes and flow
- Enables schooling behavior, predator detection, and prey localization
- Works in complete darkness
- Range: typically 1-2 body lengths

**Vibration Sensing:**
- Spiders detect prey in webs through vibration patterns
- Elephants detect seismic signals through feet
- Useful for detecting approaching large animals

### 1.5 Electroreception

Unique to aquatic and semi-aquatic animals:

**Passive Electroreception:**
- Sharks detect bioelectric fields (muscle contractions, heartbeats)
- Sensitivity: as low as 5 nV/cm - detect prey buried in sand
- Ampullae of Lorenzini: gel-filled canals connected to surface pores
- Range: typically < 1m, but sufficient for final strike guidance

**Active Electroreception:**
- Electric fish generate fields and sense distortions
- Used for navigation in murky water and communication
- Very high energy cost

---

## 2. Sensory Evolution and Trade-offs

### 2.1 Energy Costs of Sensory Systems

The nervous system consumes 20-25% of total energy in mammals, with sensory processing being a major component.

**Key Findings:**
- "The benefits of acquiring information need to be balanced against the costs of receiving and processing it"
- Neural tissue has very high metabolic rate - expensive to maintain
- Unused sensory capacity represents wasted energy
- Cave and island animals often show reduced visual systems

**Energy Cost Rankings (Approximate):**
1. **Vision** (highest): Complex optics, massive neural processing
2. **Echolocation**: High-intensity sound production + processing
3. **Electroreception**: Continuous sensing, specialized organs
4. **Hearing**: Moderate processing requirements
5. **Olfaction**: Low energy but requires memory for comparison
6. **Touch/Vibration** (lowest): Simple mechanoreceptors

### 2.2 Sensory Trade-offs

Research shows clear trade-offs between sensory modalities:

**Vision vs Olfaction:**
- Arboreal mammals: larger eyes, smaller noses
- Ground-dwelling mammals: smaller eyes, larger noses
- This represents allocation of neural resources

**Vision vs Echolocation in Bats:**
- Species with sophisticated echolocation have reduced visual capabilities
- Species in open habitats rely more on vision, less on sonar
- Strong negative correlation documented

**Tactile vs Other Senses:**
- Tactile-foraging ducks have increased mechanoreceptors at expense of temperature/pain sensors
- Moles have reduced eyes but enhanced whisker systems

### 2.3 Evolutionary Drivers

**Environmental Factors:**
- Light availability → vision investment
- Substrate type → vibration sensing value
- Medium (air/water) → sound propagation characteristics
- Chemical environment → olfaction utility

**Ecological Role:**
- Predators: invest in target-locating senses (vision, echolocation)
- Prey: invest in early-warning senses (wide FOV, hearing)
- Social species: invest in communication modalities

**Life History:**
- Nocturnal → reduced vision, enhanced hearing/olfaction
- Aquatic → lateral line, potential electroreception
- Fossorial (burrowing) → enhanced touch, reduced vision

---

## 3. Prey Detection and Predator Evasion

### 3.1 Predator Detection Strategies

**Multi-Modal Detection:**
- Animals use multiple cues for threat assessment
- Additive effects: more cues = stronger response
- Reduces false alarms while maintaining sensitivity

**Cue Types:**
- **Visual**: Movement, shape (looming detection), size
- **Chemical**: Predator scent, freshness indicates recency
- **Auditory**: Movement sounds, vocalizations
- **Tactile**: Substrate vibrations, air pressure changes

**Response Calibration:**
- Animals assess threat level, don't always flee immediately
- Factors: distance, predator type, escape route availability
- Energy cost of fleeing vs risk of staying

### 3.2 Evasion Strategies

**Unpredictable Movement (Protean Behavior):**
- Random direction changes prevent predator anticipation
- Particularly effective against single predators
- Less effective against coordinated pack hunters

**Evasion Tactics by Predator Type:**
- **Single predator**: Sharp turns, unpredictable zigzagging
- **Pack hunters**: Maintain speed, minimize turns, outrun
- **Ambush predators**: Freeze behavior, use camouflage

**Group Strategies:**
- **Confusion Effect**: Many similar targets impair predator targeting
- **Dilution Effect**: Individual risk decreases with group size
- **Many-Eyes Effect**: More individuals = faster detection

### 3.3 Camouflage and Crypsis

**Background Matching:**
- Color and pattern match typical background
- Works best when stationary
- Reduces detection probability at distance

**Disruptive Coloration:**
- High-contrast patterns break up body outline
- Prevents edge detection by predator vision

**Motion Camouflage:**
- Moving to appear stationary relative to target
- Intercept trajectory while maintaining constant bearing
- Used by dragonflies, some predators

**Counter-Evolution:**
- Predators evolve better camouflage detection
- "Cognition contra camouflage": larger brains correlate with better detection
- Arms race between crypsis and detection

**Camouflage Trade-offs:**
- Conspicuousness for mates vs concealment from predators
- Different optimal patterns for different backgrounds
- Energy cost of pigment production and behavior

---

## 4. Spatial Cognition and Mental Maps

### 4.1 Cognitive Maps

Animals maintain internal representations of their environment:

**Types of Spatial Knowledge:**
- **Route knowledge**: Sequence of landmarks/actions
- **Survey knowledge**: Map-like representation of relationships
- **Vector-based**: Distance and direction to goals

**Evidence for Cognitive Maps:**
- Novel shortcut use
- Flexible route planning
- Detour behavior around obstacles

### 4.2 Spatial Memory Systems

**Hippocampus and Spatial Memory:**
- Central role in vertebrate spatial memory
- Place cells encode specific locations
- Grid cells provide metric coordinate system

**Memory Types:**
- **Reference memory**: Stable environmental features
- **Working memory**: Current goals, recent events
- **Episodic-like memory**: What, where, when

### 4.3 Planning and Navigation

**Habitat Effects on Planning:**
- Complex terrestrial habitats (savanna-like) favor planning
- Simple or aquatic habitats favor reactive behavior
- Planning is computationally expensive but valuable in complex environments

**Path Integration:**
- Dead reckoning: tracking position via movement integration
- Used by desert ants, bees, mammals
- Accumulates error over distance

---

## 5. Swarm Intelligence and Collective Sensing

### 5.1 Principles of Swarm Intelligence

**Characteristics:**
- Decentralized control - no leader
- Simple individual rules
- Local interactions only
- Emergent collective behavior

**Examples in Nature:**
- Ant colonies: foraging, nest building
- Bird flocks: predator avoidance, navigation
- Fish schools: predator confusion, information sharing
- Bee swarms: nest site selection

### 5.2 Stigmergy: Communication Through Environment

**Pheromone Trails:**
- Individuals modify environment (leave chemical traces)
- Others respond to modifications
- Creates feedback loops (positive reinforcement of good paths)
- Evaporation provides temporal dynamics

**Benefits:**
- No direct communication required
- Scales well with group size
- Robust to individual loss
- Enables complex collective behavior from simple rules

### 5.3 Collective Sensing

**Information Pooling:**
- Many individuals sampling environment
- Reduces individual error through averaging
- Faster detection of changes

**Alarm Systems:**
- Alarm calls/pheromones spread information rapidly
- One detector can alert entire group
- Trade-off: alerting group may also alert predator

### 5.4 Communication Modalities

**Acoustic:**
- Alarm calls (specific to predator type in some species)
- Mating calls, territory advertisement
- Works through obstacles, over distance

**Visual:**
- Body postures, color changes
- Movement patterns (waggle dance in bees)
- Requires line of sight

**Chemical:**
- Pheromones for various purposes
- Long-lasting territorial markers
- Works in darkness, around obstacles

---

## 6. Implementation Design

### 6.1 Architecture Overview

```
SensorySuite
├── VisionSensor
│   ├── fieldOfView (radians)
│   ├── range
│   ├── colorSensitivity
│   ├── motionSensitivity
│   └── process() → detected entities, motion vectors
├── HearingSensor
│   ├── range
│   ├── frequencyRange
│   ├── directionalAccuracy
│   ├── echolocationEnabled
│   └── process() → sounds with direction/distance
├── SmellSensor
│   ├── range
│   ├── sensitivity
│   ├── pheromoneTypes[]
│   └── process() → scent gradients, pheromone trails
├── TouchSensor
│   ├── range (very short)
│   ├── vibrationSensitivity
│   └── process() → nearby movement, contact
└── Electroreception (optional)
    ├── range
    └── process() → bioelectric signatures
```

### 6.2 Evolvable Sensory Genome

New genome traits for sensory systems:

```cpp
// Vision traits
float visionFOV;           // 0.5π to 2π radians
float visionAcuity;        // 0.0 to 1.0 (detail at range)
float colorPerception;     // 0.0 to 1.0 (dichromat to tetrachromat)
float motionDetection;     // 0.0 to 1.0 (motion sensitivity)

// Hearing traits
float hearingRange;        // 5.0 to 100.0 units
float hearingAccuracy;     // 0.0 to 1.0 (directional precision)
float echolocation;        // 0.0 to 1.0 (0 = none, 1 = full)

// Smell traits
float smellRange;          // 10.0 to 200.0 units
float smellSensitivity;    // 0.0 to 1.0 (detection threshold)
float pheromoneProduction; // 0.0 to 1.0 (emission rate)

// Touch/Vibration
float touchRange;          // 0.5 to 5.0 units
float vibrationSensitivity;// 0.0 to 1.0

// Camouflage
float camouflageLevel;     // 0.0 to 1.0 (reduces visual detection)

// Communication
float alarmCallVolume;     // 0.0 to 1.0
float displayIntensity;    // 0.0 to 1.0 (mating displays)
```

### 6.3 Energy Cost Model

Each sensory trait has an associated energy cost:

```cpp
float calculateSensoryCost() {
    float cost = 0.0f;

    // Vision: highest cost, scales with quality
    cost += visionFOV * 0.1f;
    cost += visionAcuity * 0.3f;
    cost += colorPerception * 0.2f;
    cost += motionDetection * 0.15f;

    // Hearing: moderate cost
    cost += (hearingRange / 100.0f) * 0.1f;
    cost += hearingAccuracy * 0.1f;
    cost += echolocation * 0.4f;  // Echolocation is expensive

    // Smell: low cost
    cost += (smellRange / 200.0f) * 0.05f;
    cost += smellSensitivity * 0.05f;
    cost += pheromoneProduction * 0.1f;

    // Touch: very low cost
    cost += touchRange * 0.02f;
    cost += vibrationSensitivity * 0.02f;

    // Camouflage: moderate cost (pigment production)
    cost += camouflageLevel * 0.15f;

    return cost;  // Added to base energy consumption
}
```

### 6.4 Environmental Effects

```cpp
struct EnvironmentConditions {
    float visibility;    // 0.0 to 1.0 (fog, darkness reduces)
    float soundSpeed;    // Affected by medium (air/water)
    float scentCarrying; // Wind affects smell propagation
    float windDirection; // Radians
    float windSpeed;     // Affects scent range
};

// Vision modifier
float effectiveVisionRange = visionRange * visibility;

// Smell modifier (downwind = better, upwind = worse)
float smellModifier = 1.0f + windSpeed * cos(angleToTarget - windDirection);
float effectiveSmellRange = smellRange * max(0.1f, smellModifier);

// Hearing in water
float effectiveHearingRange = hearingRange * (isUnderwater ? 4.0f : 1.0f);
```

### 6.5 SpatialMemory System

```cpp
class SpatialMemory {
    struct MemoryEntry {
        glm::vec3 location;
        MemoryType type;  // FOOD, DANGER, SHELTER, TERRITORY
        float strength;   // Decays over time
        float timestamp;  // When observed
    };

    std::vector<MemoryEntry> memories;
    int maxMemories;  // Limited capacity based on genome
    float decayRate;  // How fast memories fade

    void remember(glm::vec3 pos, MemoryType type);
    void update(float deltaTime);  // Decay memories
    std::vector<MemoryEntry> recall(MemoryType type);  // Get relevant memories
    glm::vec3 getClosestRemembered(MemoryType type);
};
```

### 6.6 Communication Systems

```cpp
class CommunicationSystem {
    // Alarm calls
    void emitAlarmCall(glm::vec3 position, float volume, ThreatType threat);

    // Territory marking
    void markTerritory(glm::vec3 position, float strength);

    // Pheromone trails
    void emitPheromone(glm::vec3 position, PheromoneType type, float strength);

    // Mating displays
    void performDisplay(float intensity);  // Attracts mates, also predators
};
```

### 6.7 Detection Probability Model

```cpp
float calculateDetectionProbability(
    Creature* observer,
    Creature* target,
    EnvironmentConditions& env
) {
    float distance = glm::length(target->position - observer->position);
    float baseProb = 1.0f;

    // Distance attenuation
    float rangeRatio = distance / observer->getSensory().visionRange;
    baseProb *= exp(-rangeRatio * rangeRatio);

    // Environmental effects
    baseProb *= env.visibility;

    // Target camouflage
    baseProb *= (1.0f - target->getGenome().camouflageLevel * 0.8f);

    // Motion detection bonus
    if (glm::length(target->velocity) > 0.5f) {
        baseProb += observer->getSensory().motionDetection * 0.3f;
    }

    // FOV check
    float angleToTarget = calculateAngle(observer, target);
    float halfFOV = observer->getSensory().visionFOV / 2.0f;
    if (abs(angleToTarget) > halfFOV) {
        baseProb = 0.0f;  // Outside field of view
    }

    return clamp(baseProb, 0.0f, 1.0f);
}
```

### 6.8 Neural Network Integration

Expand neural network inputs to include sensory data:

```cpp
// Current: 4 inputs
// Proposed: 20+ inputs

struct SensoryInput {
    // Vision (6 inputs)
    float nearestFoodDistance;
    float nearestFoodAngle;
    float nearestThreatDistance;
    float nearestThreatAngle;
    float nearestMateDistance;
    float nearestMateAngle;

    // Hearing (4 inputs)
    float loudestSoundIntensity;
    float loudestSoundAngle;
    float alarmCallDetected;
    float echoReturnStrength;

    // Smell (4 inputs)
    float foodScentGradient;
    float foodScentDirection;
    float predatorScentStrength;
    float pheromoneTrailDirection;

    // Touch (2 inputs)
    float nearbyMovement;
    float contactDetected;

    // Internal state (4 inputs)
    float energyLevel;
    float fearLevel;
    float reproductiveUrge;
    float memoryPull;  // Direction to remembered resources
};
```

---

## 7. References

### Scientific Sources

1. [Energy limitation as a selective pressure on the evolution of sensory systems](https://pubmed.ncbi.nlm.nih.gov/18490395/) - Journal of Experimental Biology
2. [Exploring the mammalian sensory space: co-operations and trade-offs among senses](https://pubmed.ncbi.nlm.nih.gov/24043357/) - PubMed
3. [Evolution of sensory systems](https://www.sciencedirect.com/science/article/abs/pii/S0959438821000969) - ScienceDirect
4. [Evolution of Sensory Receptors](https://pmc.ncbi.nlm.nih.gov/articles/PMC11526382/) - PMC
5. [Physiology, Sensory Receptors](https://www.ncbi.nlm.nih.gov/books/NBK539861/) - NCBI Bookshelf

### Predator-Prey Dynamics

6. [Spatial planning with long visual range benefits escape from visual predators](https://www.nature.com/articles/s41467-020-16102-1) - Nature Communications
7. [Behavior Under Risk: How Animals Avoid Becoming Dinner](https://www.nature.com/scitable/knowledge/library/behavior-under-risk-how-animals-avoid-becoming-23646978/) - Nature Scitable
8. [Cognition and the evolution of camouflage](https://royalsocietypublishing.org/doi/10.1098/rspb.2015.2890) - Royal Society
9. [Unpredictable movement as an anti-predator strategy](https://royalsocietypublishing.org/doi/10.1098/rspb.2018.1112) - Royal Society
10. [Predator responses to prey camouflage strategies](https://pmc.ncbi.nlm.nih.gov/articles/PMC9470275/) - PMC

### Specialized Senses

11. [Animal echolocation](https://en.wikipedia.org/wiki/Animal_echolocation) - Wikipedia
12. [Echolocation is nature's built-in sonar](https://www.nationalgeographic.com/animals/article/echolocation-is-nature-built-in-sonar-here-is-how-it-works) - National Geographic
13. [Lateral line system](https://www.britannica.com/science/lateral-line-system) - Britannica
14. [Electroreception](https://www.britannica.com/science/electroreception) - Britannica
15. [Ampullae of Lorenzini](https://en.wikipedia.org/wiki/Ampullae_of_Lorenzini) - Wikipedia

### Swarm Intelligence

16. [Swarm intelligence](https://en.wikipedia.org/wiki/Swarm_intelligence) - Wikipedia
17. [Pheromone-based communication](https://fiveable.me/swarm-intelligence-and-robotics/unit-6/pheromone-based-communication/study-guide/AJDbluQ1WpNNVR5O) - Fiveable
18. [Testing the limits of pheromone stigmergy](https://royalsocietypublishing.org/doi/10.1098/rsos.190225) - Royal Society Open Science

### Vision and Animal Perception

19. [How animals see the World](https://www.veterinaryvision.co.uk/pet-owners/how-animals-see-the-world) - Veterinary Vision
20. [Prey Vision: How Animals See to Survive Predation](https://biologyinsights.com/prey-vision-how-animals-see-to-survive-predation/) - Biology Insights
21. [Stereopsis in animals: evolution, function and mechanisms](https://pmc.ncbi.nlm.nih.gov/articles/PMC5536890/) - PMC

---

## Summary

This document provides the biological foundation and technical design for implementing realistic sensory systems in the OrganismEvolution simulation. Key principles:

1. **Energy constrains sensory investment** - creatures must trade off between sensory capabilities
2. **Environment shapes optimal sensory strategies** - different niches favor different senses
3. **Multi-modal integration provides robustness** - combining senses reduces errors
4. **Communication enables collective intelligence** - social sensing through signals
5. **Memory extends sensing in time** - remembering past locations is valuable
6. **Camouflage and detection co-evolve** - arms race between prey hiding and predator finding

The implementation should allow creatures to evolve sensory strategies appropriate to their ecological niche, with predators developing different sensory profiles than prey, and environmental conditions affecting which strategies succeed.
