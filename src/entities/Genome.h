#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

// Forward declaration
struct PlanetChemistry;

// ============================================================================
// EVOLUTION START PRESETS
// ============================================================================
// Determines the initial complexity level of creatures at world creation.
// Allows worlds to start from different evolutionary stages.

enum class EvolutionStartPreset : uint8_t {
    PROTO = 0,       // Primordial soup - very simple organisms, minimal traits
    EARLY_LIMB = 1,  // Early multi-cellular - basic body plans, simple locomotion
    COMPLEX = 2,     // Complex organisms - developed sensory systems, varied morphology
    ADVANCED = 3,    // Advanced life - sophisticated behaviors, specialized niches

    COUNT = 4
};

// ============================================================================
// EVOLUTION GUIDANCE BIAS
// ============================================================================
// Provides soft pressure toward certain evolutionary directions without
// hard-constraining evolution. Affects initial trait distributions and
// mutation biases.

enum class EvolutionGuidanceBias : uint8_t {
    NONE = 0,        // No guidance - pure natural selection
    LAND = 1,        // Bias toward land-based locomotion
    AQUATIC = 2,     // Bias toward aquatic traits (swimming, gills, etc.)
    FLIGHT = 3,      // Bias toward aerial traits (wings, lightweight bodies)
    UNDERGROUND = 4, // Bias toward burrowing/subterranean traits

    COUNT = 5
};

// ============================================================================
// REGION EVOLUTION CONFIG (for multi-region world generation)
// ============================================================================
// Allows different regions/islands to have different evolutionary starting points

struct RegionEvolutionConfig {
    EvolutionStartPreset preset = EvolutionStartPreset::EARLY_LIMB;
    EvolutionGuidanceBias bias = EvolutionGuidanceBias::NONE;
    float mutationRateModifier = 1.0f;    // Multiplier on mutation rate (e.g., 1.5 = 50% faster evolution)
    float selectionPressure = 1.0f;       // Strength of natural selection (higher = more deaths for unfit)
    bool allowExoticBiochemistry = true;  // Whether to allow non-standard biochemistry adaptations
};

class Genome {
public:
    // Physical traits
    float size;           // 0.5 to 2.0 (affects speed and energy)
    float speed;          // 5.0 to 20.0 (movement speed)
    float visionRange;    // 10.0 to 50.0 (detection distance) - kept for backward compatibility
    float efficiency;     // 0.5 to 1.5 (energy consumption multiplier)

    // Visual traits
    glm::vec3 color;      // RGB color

    // Neural network weights
    std::vector<float> neuralWeights;

    // ==========================================
    // SENSORY SYSTEM TRAITS (Evolvable)
    // ==========================================

    // Vision traits
    float visionFOV;              // Field of view in radians (1.0 to 6.0, ~57° to ~344°)
    float visionAcuity;           // Detail perception at range (0-1)
    float colorPerception;        // Color sensitivity (0-1, 0=monochrome)
    float motionDetection;        // Motion sensitivity bonus (0-1)

    // Hearing traits
    float hearingRange;           // Maximum hearing distance (10-100)
    float hearingDirectionality;  // Directional accuracy (0-1)
    float echolocationAbility;    // 0=none, 1=full echolocation capability

    // Smell traits
    float smellRange;             // Detection distance (10-150)
    float smellSensitivity;       // Detection threshold (0-1)
    float pheromoneProduction;    // Emission rate (0-1)

    // Touch/Vibration traits
    float touchRange;             // Very short range detection (0.5-8)
    float vibrationSensitivity;   // Ground/water vibration detection (0-1)

    // Camouflage (reduces visual detection by others)
    float camouflageLevel;        // 0-1

    // Communication traits
    float alarmCallVolume;        // 0-1
    float displayIntensity;       // Mating display strength (0-1)

    // Memory traits
    float memoryCapacity;         // Affects spatial memory size (0-1)
    float memoryRetention;        // How long memories last (0-1)

    // ==========================================
    // FLYING CREATURE TRAITS
    // ==========================================

    // Basic flight traits (legacy - kept for compatibility)
    float wingSpan;               // 0.5 - 2.0 (ratio to body size)
    float flapFrequency;          // 2.0 - 8.0 Hz (for birds) or 20-200 Hz (insects)
    float glideRatio;             // 0.3 - 0.8 (higher = more gliding vs flapping)
    float preferredAltitude;      // 15.0 - 40.0 (height above terrain)

    // Wing morphology (aerodynamic characteristics)
    float wingChord;              // 0.1 - 0.5 (wing width at root, ratio to wingspan)
    float wingAspectRatio;        // 2.0 - 20.0 (wingspan² / wing area, higher = better glide)
    float wingLoading;            // 10.0 - 100.0 (mass per wing area, N/m²)
    float wingCamber;             // 0.0 - 0.15 (curvature of wing for lift)
    float wingTaper;              // 0.2 - 1.0 (tip chord / root chord)
    float wingTwist;              // -5.0 - 5.0 degrees (washout/washin)
    float dihedralAngle;          // 0.0 - 15.0 degrees (V-shape for stability)
    float sweepAngle;             // -10.0 - 45.0 degrees (backward sweep)

    // Wing type (encoded as float for evolution, maps to WingType enum)
    uint8_t wingType;             // 0=feathered, 1=membrane, 2=insect_single, etc.
    uint8_t featherType;          // 0=contour, 1=primary, 2=secondary, 3=covert

    // Tail configuration
    float tailLength;             // 0.2 - 1.5 (ratio to body length)
    float tailSpan;               // 0.2 - 0.8 (horizontal tail spread)
    uint8_t tailType;             // 0=forked, 1=rounded, 2=pointed, 3=fan, 4=notched

    // Flight musculature
    float breastMuscleRatio;      // 0.1 - 0.4 (pectoralis size, affects power)
    float supracoracoideus;       // 0.02 - 0.1 (upstroke muscle, affects recovery speed)
    float muscleOxygenCapacity;   // 0.5 - 1.5 (endurance factor)
    float anaerobicCapacity;      // 0.3 - 1.0 (burst power capability)

    // Body aerodynamics
    float bodyDragCoeff;          // 0.01 - 0.1 (streamlining)
    float fuselageLength;         // 0.8 - 2.0 (body length ratio)
    float bodyDensity;            // 0.8 - 1.2 (affects weight, hollow bones = lower)

    // Specialized flight capabilities
    float hoveringAbility;        // 0.0 - 1.0 (hummingbird-style hovering)
    float divingSpeed;            // 0.0 - 1.0 (stoop/dive performance)
    float maneuverability;        // 0.0 - 1.0 (turning/agility)
    float thermalSensingAbility;  // 0.0 - 1.0 (finding and using thermals)
    float windResistance;         // 0.0 - 1.0 (stability in turbulence)

    // Flight behavior genetics
    float flockingStrength;       // 0.0 - 1.0 (tendency to flock)
    float territorialRadius;      // 5.0 - 100.0 (defended airspace)
    float migrationInstinct;      // 0.0 - 1.0 (seasonal movement drive)
    float nocturnalFlight;        // 0.0 - 1.0 (night flight adaptation)

    // Energy management
    float flightMetabolism;       // 0.5 - 2.0 (energy consumption rate in flight)
    float fatStorageCapacity;     // 0.1 - 0.5 (fuel reserves for long flights)
    float restingRecoveryRate;    // 0.1 - 0.5 (energy recovery when perched)

    // ==========================================
    // AQUATIC TRAITS (for fish/water creatures)
    // ==========================================

    // Swimming morphology
    float finSize;           // 0.3 - 1.0 (dorsal/pectoral fin size)
    float tailSize;          // 0.5 - 1.2 (caudal fin size for propulsion)
    float swimFrequency;     // 1.0 - 4.0 Hz (body wave frequency)
    float swimAmplitude;     // 0.1 - 0.3 (S-wave amplitude)
    float bodyStreamlining;  // 0.5 - 1.0 (drag coefficient reduction)

    // Depth behavior
    float preferredDepth;    // 0.1 - 0.5 (normalized depth below water surface)
    float minDepthTolerance; // 0.0 - 0.3 (minimum safe depth)
    float maxDepthTolerance; // 0.5 - 1.0 (maximum safe depth, pressure resistance)
    float pressureResistance;// 0.5 - 2.0 (resistance to pressure damage)

    // Social behavior
    float schoolingStrength; // 0.5 - 1.0 (how strongly creature schools with others)
    float schoolingRadius;   // 2.0 - 10.0 (preferred distance to neighbors)
    float schoolingAlignment;// 0.3 - 1.0 (alignment with school direction)

    // Respiration
    float gillEfficiency;    // 0.5 - 1.5 (oxygen extraction efficiency)
    float oxygenStorage;     // 0.0 - 1.0 (for air-breathers: breath hold capacity)
    bool canBreathAir;       // Whether creature can surface for air

    // Buoyancy
    float swimbladderSize;   // 0.0 - 1.0 (0 for sharks, 1 for bony fish)
    float neutralBuoyancyDepth;// 0.1 - 0.8 (depth at which naturally neutral)

    // Special abilities - Bioluminescence
    bool hasBioluminescence;      // Whether creature can produce light
    float biolumIntensity;        // 0.0 - 1.0 (brightness)
    float biolumRed;              // 0.0 - 1.0 (color components)
    float biolumGreen;            // 0.0 - 1.0
    float biolumBlue;             // 0.0 - 1.0
    uint8_t biolumPattern;        // 0=glow, 1=pulse, 2=flash, 3=lure, 4=counter-illum
    float biolumPulseSpeed;       // 0.5 - 3.0 Hz (pulse frequency for bioluminescence)

    // Convenience accessors for bioluminescence (used by BioluminescenceSystem)
    glm::vec3 getBioluminescentColor() const { return glm::vec3(biolumRed, biolumGreen, biolumBlue); }
    float getGlowIntensity() const { return biolumIntensity; }
    float getPulseSpeed() const { return biolumPulseSpeed; }

    // Alias for backwards compatibility
    glm::vec3 bioluminescentColor{0.0f, 1.0f, 0.5f};  // Default teal glow
    float glowIntensity = 0.0f;
    float pulseSpeed = 1.0f;

    // Special abilities - Echolocation (aquatic)
    float aquaticEcholocation;    // 0.0 - 1.0 (ability level, 0=none)
    float echolocationRange;      // 10.0 - 200.0 (detection distance)
    float echolocationPrecision;  // 0.3 - 1.0 (accuracy)

    // Special abilities - Electroreception (sharks)
    float electroreception;       // 0.0 - 1.0 (ability level)
    float electroRange;           // 0.0 - 30.0 (detection range)

    // Special abilities - Lateral line
    float lateralLineSensitivity; // 0.3 - 1.0 (water pressure/vibration sensing)

    // Special abilities - Venom/Toxicity
    float venomPotency;           // 0.0 - 1.0 (damage dealt)
    float toxicity;               // 0.0 - 1.0 (defense - makes creature unpalatable)

    // Special abilities - Camouflage (aquatic-specific)
    float aquaticCamouflage;      // 0.0 - 1.0 (ability to blend with surroundings)
    float colorChangeSpeed;       // 0.0 - 1.0 (for octopus-like color changing)

    // Special abilities - Ink defense
    float inkCapacity;            // 0.0 - 1.0 (for cephalopods)
    float inkRechargeRate;        // 0.1 - 0.5 (recovery per second)

    // Special abilities - Electric discharge
    float electricDischarge;      // 0.0 - 1.0 (electric eel ability)
    float electricRechargeRate;   // 0.05 - 0.2 (recovery per second)

    // Air-breathing behavior
    float breathHoldDuration;     // 0.0 - 90.0 minutes (for whales, dolphins)
    float surfaceBreathRate;      // 0.5 - 2.0 (breaths per surfacing)

    // Fin configurations (for procedural mesh)
    float dorsalFinHeight;        // 0.1 - 0.5
    float pectoralFinWidth;       // 0.2 - 0.6
    float caudalFinType;          // 0.0 - 1.0 (0=rounded, 0.5=forked, 1.0=lunate)
    float analFinSize;            // 0.0 - 0.3
    float pelvicFinSize;          // 0.0 - 0.3
    uint8_t finCount;             // 3-8 total fins

    // Scale/skin patterns
    float scaleSize;              // 0.01 - 0.1 (for texture generation)
    float scaleShininess;         // 0.2 - 0.9 (reflectivity)
    float patternFrequency;       // 1.0 - 10.0 (stripe/spot frequency)
    uint8_t patternType;          // 0=solid, 1=stripes, 2=spots, 3=gradient, 4=counter-shading

    // Enhanced pattern parameters for visual diversity
    float patternIntensity;       // 0.0 - 1.0 (how visible the pattern is)
    float patternSecondaryHue;    // 0.0 - 1.0 (secondary color hue offset from primary)
    float spotSize;               // 0.02 - 0.3 (for spotted patterns)
    uint8_t stripeCount;          // 3 - 15 (number of stripes)
    float gradientDirection;      // 0.0 - 1.0 (0=vertical, 0.5=diagonal, 1.0=horizontal)
    float markingContrast;        // 0.1 - 1.0 (contrast between pattern colors)

    // ==========================================
    // MORPHOLOGY DIVERSITY GENES (Phase 7)
    // ==========================================

    // Body structure genes
    uint8_t segmentCount;         // 1 - 8 (body segments, affects silhouette)
    float bodyAspect;             // 0.3 - 3.0 (length/width ratio, compact vs elongated)
    float bodyTaper;              // 0.5 - 1.5 (how body tapers front-to-back)

    // Appendage diversity
    uint8_t dorsalFinCount;       // 0 - 3 (number of dorsal fins)
    uint8_t pectoralFinCount;     // 0 - 4 (number of pectoral fin pairs)
    uint8_t ventralFinCount;      // 0 - 2 (ventral fins)
    float finAspect;              // 0.3 - 3.0 (fin shape: rounded vs swept)
    float finRayCount;            // 3 - 12 (for ray-finned creatures)

    // Crests and dorsal features
    float crestHeight;            // 0.0 - 0.8 (dorsal crest/ridge height)
    float crestExtent;            // 0.0 - 1.0 (how much of body crest covers)
    uint8_t crestType;            // 0=none, 1=ridge, 2=sail, 3=frill, 4=spiny

    // Horns and antennae
    uint8_t hornCount;            // 0 - 6 (total horns)
    float hornLength;             // 0.1 - 1.5 (relative to head size)
    float hornCurvature;          // -1.0 - 1.0 (negative=forward, positive=backward)
    uint8_t hornType;             // 0=straight, 1=curved, 2=spiral, 3=branched
    uint8_t antennaeCount;        // 0 - 4 (sensory antennae)
    float antennaeLength;         // 0.2 - 2.0 (relative to body length)

    // Tail variants
    uint8_t tailVariant;          // 0=standard, 1=clubbed, 2=fan, 3=whip, 4=forked, 5=prehensile, 6=spiked
    float tailFinHeight;          // 0.0 - 0.5 (for aquatic tail fins)
    float tailBulbSize;           // 0.0 - 0.4 (for club tails)

    // Mouth and feeding apparatus
    uint8_t jawType;              // 0=standard, 1=underslung, 2=protruding, 3=beak, 4=filter
    float jawProtrusion;          // -0.3 - 0.5 (negative=recessed, positive=extended)
    float barbels;                // 0.0 - 1.0 (whisker-like sensory organs)

    // Limb variation
    uint8_t limbSegments;         // 2 - 5 (segments per limb)
    float limbTaper;              // 0.3 - 1.0 (thickness reduction along limb)
    float footSpread;             // 0.3 - 2.0 (webbing/toe spread)
    bool hasClaws;                // Presence of claws on limbs
    float clawLength;             // 0.0 - 0.5 (relative to foot size)

    // Spikes and protrusions
    uint8_t spikeRows;            // 0 - 4 (rows of spikes along body)
    float spikeLength;            // 0.0 - 0.6 (relative to body width)
    float spikeDensity;           // 0.1 - 1.0 (spikes per unit area)

    // Shell and armor
    float shellCoverage;          // 0.0 - 1.0 (body coverage by shell/armor)
    float shellSegmentation;      // 0.0 - 1.0 (articulation of shell)
    uint8_t shellTexture;         // 0=smooth, 1=ridged, 2=bumpy, 3=plated

    // Frills and displays
    bool hasNeckFrill;            // Frill around neck/head
    float frillSize;              // 0.1 - 1.5 (relative to head size)
    bool hasBodyFrills;           // Frills along body sides
    float displayFeatherSize;     // 0.0 - 1.0 (for display feathers/fins)

    // Eye diversity
    uint8_t eyeArrangement;       // 0=paired, 1=forward, 2=stalked, 3=compound, 4=wide-set
    float eyeProtrusion;          // 0.0 - 0.5 (how much eyes bulge)
    bool hasEyeSpots;             // False eye patterns
    uint8_t eyeSpotCount;         // 0 - 8 (for intimidation patterns)

    // ==========================================
    // BIOCHEMISTRY TRAITS (Planet Chemistry Adaptation)
    // ==========================================
    // These traits determine how well a creature is adapted to the planet's
    // unique chemistry. Creatures with poor compatibility suffer fitness penalties.

    // Biopigment family (0-5, discrete)
    // 0 = chlorophyll-based (green, oxygen worlds)
    // 1 = carotenoid-based (orange/red, high radiation)
    // 2 = phycocyanin-based (blue, low light)
    // 3 = bacteriorhodopsin-based (purple, extreme conditions)
    // 4 = melanin-based (black/brown, UV protection)
    // 5 = flavin-based (yellow, sulfur worlds)
    uint8_t biopigmentFamily;

    // Membrane fluidity (0.0 - 1.0)
    // Low values = rigid membranes (cold adaptation, high pressure)
    // High values = fluid membranes (warm adaptation, low pressure)
    float membraneFluidity;

    // Oxygen tolerance (0.0 - 1.0)
    // Low values = anaerobic preference (low/no oxygen environments)
    // High values = aerobic efficiency (oxygen-rich environments)
    float oxygenTolerance;

    // Mineralization bias (0.0 - 1.0)
    // Low values = soft-bodied, organic structures
    // High values = heavy mineralization (shells, bones, silicate structures)
    float mineralizationBias;

    // Solvent affinity (0.0 - 1.0)
    // Determines how well cellular chemistry works with the planet's solvent
    // 0.5 = water-adapted (Earth-like)
    // 0.0 = ammonia/methane affinity (cold worlds)
    // 1.0 = acid/alcohol affinity (extreme worlds)
    float solventAffinity;

    // Temperature adaptation range (in degrees from optimal)
    // Narrower range = more specialized, wider = more generalist
    float temperatureTolerance;      // 5.0 - 50.0 degrees

    // Radiation resistance (0.0 - 1.0)
    // Affects survival in high UV/cosmic ray environments
    float radiationResistance;

    // pH tolerance (0.0 - 1.0)
    // Low = acid-loving (acidophile)
    // 0.5 = neutral preference
    // High = base-loving (alkaliphile)
    float pHPreference;

    // Metabolic pathway type (0-3, discrete)
    // 0 = aerobic respiration (oxygen-based)
    // 1 = anaerobic fermentation (no oxygen)
    // 2 = chemosynthesis (chemical energy)
    // 3 = photosynthesis variant
    uint8_t metabolicPathway;

    // ==========================================
    // MUTATION CONTROL PARAMETERS
    // ==========================================

    // Heavy-tailed mutation support (for rare extreme traits)
    static constexpr float MACRO_MUTATION_CHANCE = 0.02f;   // 2% chance of macro-mutation
    static constexpr float EXTREME_TRAIT_CHANCE = 0.015f;   // 1.5% chance of extreme trait value

    Genome();
    Genome(const Genome& parent1, const Genome& parent2); // Crossover constructor

    void mutate(float mutationRate, float mutationStrength);

    // Chemistry-aware mutation: applies constraints based on planet chemistry
    // Creatures are more likely to mutate toward chemistry-compatible values
    void mutateWithChemistry(float mutationRate, float mutationStrength, const struct PlanetChemistry& chemistry);
    void randomize();
    void randomizeFlying();         // Initialize genome for generic flying creatures
    void randomizeBird();           // Initialize genome for bird-type flyers
    void randomizeInsect();         // Initialize genome for insect-type flyers
    void randomizeAerialPredator(); // Initialize genome for aerial predators (raptors)
    void randomizeHummingbird();    // Initialize genome for hovering specialists
    void randomizeOwl();            // Initialize genome for silent nocturnal hunters
    void randomizeSeabird();        // Initialize genome for ocean-going birds
    void randomizeBat();            // Initialize genome for membrane-winged flyers
    void randomizeDragonfly();      // Initialize genome for insect aerial predators
    void randomizeButterfly();      // Initialize genome for lepidopteran flyers
    void randomizeBee();            // Initialize genome for hymenopteran flyers
    void randomizeDragon();         // Initialize genome for fantasy flying creatures
    void randomizeAquatic();         // Initialize genome for aquatic creatures (fish)
    void randomizeAquaticPredator(); // Initialize genome for predatory fish
    void randomizeShark();           // Initialize genome for apex aquatic predators (sharks)
    void randomizeJellyfish();       // Initialize genome for jellyfish/cnidarians
    void randomizeCrustacean();      // Initialize genome for crabs, lobsters
    void randomizeEel();             // Initialize genome for eels/serpentine fish
    void randomizeDolphin();         // Initialize genome for dolphins/marine mammals
    void randomizeWhale();           // Initialize genome for whales
    void randomizeOctopus();         // Initialize genome for cephalopods
    void randomizeDeepSeaFish();     // Initialize genome for deep-sea creatures
    void randomizePlankton();        // Initialize genome for plankton/krill

    // ==========================================
    // EVOLUTION PRESET INITIALIZATION
    // ==========================================
    // Initialize genome using preset and guidance bias for varied starting populations.
    // Chemistry parameter adapts biochemistry traits to the planet.

    // Initialize genome for a specific evolution preset and guidance bias
    void initializeForPreset(EvolutionStartPreset preset,
                             EvolutionGuidanceBias bias,
                             const PlanetChemistry& chemistry);

    // Initialize genome with regional configuration
    void initializeForRegion(const RegionEvolutionConfig& config,
                             const PlanetChemistry& chemistry);

    // Adapt biochemistry traits to match planet chemistry (call after other randomization)
    void adaptToChemistry(const PlanetChemistry& chemistry);

    // Calculate total energy cost of sensory systems
    float calculateSensoryEnergyCost() const;

    // Neural network weight count for behavior evolution
    // Network architecture: 8 inputs -> 8 hidden -> 6 outputs
    // Weights needed: (8 * 8) + (8 * 6) = 64 + 48 = 112 weights
    static const int NEURAL_WEIGHT_COUNT = 112;
    static const int SENSORY_NEURAL_WEIGHT_COUNT = 120; // Legacy - kept for compatibility
};
