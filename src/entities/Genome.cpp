#include "Genome.h"
#include "../utils/Random.h"
#include "../environment/BiomeSystem.h"
#include "../environment/PlanetChemistry.h"
#include <algorithm>
#include <cmath>

Genome::Genome() {
    randomize();
}

Genome::Genome(const Genome& parent1, const Genome& parent2) {
    // Crossover - blend traits from both parents
    if (Random::chance(0.5f)) {
        size = parent1.size;
        speed = parent2.speed;
    } else {
        size = parent2.size;
        speed = parent1.speed;
    }

    if (Random::chance(0.5f)) {
        visionRange = parent1.visionRange;
        efficiency = parent2.efficiency;
    } else {
        visionRange = parent2.visionRange;
        efficiency = parent1.efficiency;
    }

    // Blend colors
    color = (parent1.color + parent2.color) * 0.5f;

    // Uniform crossover for neural weights
    // Network needs 112 weights: (8 inputs * 8 hidden) + (8 hidden * 6 outputs)
    neuralWeights.resize(NEURAL_WEIGHT_COUNT);
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        // Handle case where parents might have fewer weights (legacy genomes)
        bool useParent1 = Random::chance(0.5f);
        if (useParent1 && i < static_cast<int>(parent1.neuralWeights.size())) {
            neuralWeights[i] = parent1.neuralWeights[i];
        } else if (!useParent1 && i < static_cast<int>(parent2.neuralWeights.size())) {
            neuralWeights[i] = parent2.neuralWeights[i];
        } else if (i < static_cast<int>(parent1.neuralWeights.size())) {
            neuralWeights[i] = parent1.neuralWeights[i];
        } else if (i < static_cast<int>(parent2.neuralWeights.size())) {
            neuralWeights[i] = parent2.neuralWeights[i];
        } else {
            // Initialize new weights randomly if parents don't have them
            neuralWeights[i] = Random::range(-1.0f, 1.0f);
        }
    }

    // ==========================================
    // SENSORY TRAIT CROSSOVER
    // ==========================================

    // Vision traits - tend to inherit as a group (linked genes)
    if (Random::chance(0.5f)) {
        visionFOV = parent1.visionFOV;
        visionAcuity = parent1.visionAcuity;
        colorPerception = parent1.colorPerception;
        motionDetection = parent1.motionDetection;
    } else {
        visionFOV = parent2.visionFOV;
        visionAcuity = parent2.visionAcuity;
        colorPerception = parent2.colorPerception;
        motionDetection = parent2.motionDetection;
    }

    // Hearing traits
    if (Random::chance(0.5f)) {
        hearingRange = parent1.hearingRange;
        hearingDirectionality = parent1.hearingDirectionality;
        echolocationAbility = parent1.echolocationAbility;
    } else {
        hearingRange = parent2.hearingRange;
        hearingDirectionality = parent2.hearingDirectionality;
        echolocationAbility = parent2.echolocationAbility;
    }

    // Smell traits
    if (Random::chance(0.5f)) {
        smellRange = parent1.smellRange;
        smellSensitivity = parent1.smellSensitivity;
        pheromoneProduction = parent1.pheromoneProduction;
    } else {
        smellRange = parent2.smellRange;
        smellSensitivity = parent2.smellSensitivity;
        pheromoneProduction = parent2.pheromoneProduction;
    }

    // Touch traits
    if (Random::chance(0.5f)) {
        touchRange = parent1.touchRange;
        vibrationSensitivity = parent1.vibrationSensitivity;
    } else {
        touchRange = parent2.touchRange;
        vibrationSensitivity = parent2.vibrationSensitivity;
    }

    // Camouflage - individual trait
    camouflageLevel = Random::chance(0.5f) ? parent1.camouflageLevel : parent2.camouflageLevel;

    // Communication traits
    if (Random::chance(0.5f)) {
        alarmCallVolume = parent1.alarmCallVolume;
        displayIntensity = parent1.displayIntensity;
    } else {
        alarmCallVolume = parent2.alarmCallVolume;
        displayIntensity = parent2.displayIntensity;
    }

    // Memory traits
    if (Random::chance(0.5f)) {
        memoryCapacity = parent1.memoryCapacity;
        memoryRetention = parent1.memoryRetention;
    } else {
        memoryCapacity = parent2.memoryCapacity;
        memoryRetention = parent2.memoryRetention;
    }

    // Flying traits - inherit as linked groups

    // Basic flight traits group
    if (Random::chance(0.5f)) {
        wingSpan = parent1.wingSpan;
        flapFrequency = parent1.flapFrequency;
        glideRatio = parent1.glideRatio;
        preferredAltitude = parent1.preferredAltitude;
    } else {
        wingSpan = parent2.wingSpan;
        flapFrequency = parent2.flapFrequency;
        glideRatio = parent2.glideRatio;
        preferredAltitude = parent2.preferredAltitude;
    }

    // Wing morphology group (aerodynamic characteristics tightly linked)
    if (Random::chance(0.5f)) {
        wingChord = parent1.wingChord;
        wingAspectRatio = parent1.wingAspectRatio;
        wingLoading = parent1.wingLoading;
        wingCamber = parent1.wingCamber;
        wingTaper = parent1.wingTaper;
        wingTwist = parent1.wingTwist;
        dihedralAngle = parent1.dihedralAngle;
        sweepAngle = parent1.sweepAngle;
    } else {
        wingChord = parent2.wingChord;
        wingAspectRatio = parent2.wingAspectRatio;
        wingLoading = parent2.wingLoading;
        wingCamber = parent2.wingCamber;
        wingTaper = parent2.wingTaper;
        wingTwist = parent2.wingTwist;
        dihedralAngle = parent2.dihedralAngle;
        sweepAngle = parent2.sweepAngle;
    }

    // Wing type traits (discrete)
    wingType = Random::chance(0.5f) ? parent1.wingType : parent2.wingType;
    featherType = Random::chance(0.5f) ? parent1.featherType : parent2.featherType;

    // Tail configuration group
    if (Random::chance(0.5f)) {
        tailLength = parent1.tailLength;
        tailSpan = parent1.tailSpan;
        tailType = parent1.tailType;
    } else {
        tailLength = parent2.tailLength;
        tailSpan = parent2.tailSpan;
        tailType = parent2.tailType;
    }

    // Flight musculature group
    if (Random::chance(0.5f)) {
        breastMuscleRatio = parent1.breastMuscleRatio;
        supracoracoideus = parent1.supracoracoideus;
        muscleOxygenCapacity = parent1.muscleOxygenCapacity;
        anaerobicCapacity = parent1.anaerobicCapacity;
    } else {
        breastMuscleRatio = parent2.breastMuscleRatio;
        supracoracoideus = parent2.supracoracoideus;
        muscleOxygenCapacity = parent2.muscleOxygenCapacity;
        anaerobicCapacity = parent2.anaerobicCapacity;
    }

    // Body aerodynamics group
    if (Random::chance(0.5f)) {
        bodyDragCoeff = parent1.bodyDragCoeff;
        fuselageLength = parent1.fuselageLength;
        bodyDensity = parent1.bodyDensity;
    } else {
        bodyDragCoeff = parent2.bodyDragCoeff;
        fuselageLength = parent2.fuselageLength;
        bodyDensity = parent2.bodyDensity;
    }

    // Specialized flight capabilities - blend with variation
    hoveringAbility = (parent1.hoveringAbility + parent2.hoveringAbility) * 0.5f + Random::range(-0.05f, 0.05f);
    hoveringAbility = std::clamp(hoveringAbility, 0.0f, 1.0f);
    divingSpeed = (parent1.divingSpeed + parent2.divingSpeed) * 0.5f + Random::range(-0.05f, 0.05f);
    divingSpeed = std::clamp(divingSpeed, 0.0f, 1.0f);
    maneuverability = (parent1.maneuverability + parent2.maneuverability) * 0.5f + Random::range(-0.05f, 0.05f);
    maneuverability = std::clamp(maneuverability, 0.0f, 1.0f);
    thermalSensingAbility = (parent1.thermalSensingAbility + parent2.thermalSensingAbility) * 0.5f + Random::range(-0.05f, 0.05f);
    thermalSensingAbility = std::clamp(thermalSensingAbility, 0.0f, 1.0f);
    windResistance = (parent1.windResistance + parent2.windResistance) * 0.5f + Random::range(-0.05f, 0.05f);
    windResistance = std::clamp(windResistance, 0.0f, 1.0f);

    // Flight behavior genetics
    if (Random::chance(0.5f)) {
        flockingStrength = parent1.flockingStrength;
        territorialRadius = parent1.territorialRadius;
        migrationInstinct = parent1.migrationInstinct;
        nocturnalFlight = parent1.nocturnalFlight;
    } else {
        flockingStrength = parent2.flockingStrength;
        territorialRadius = parent2.territorialRadius;
        migrationInstinct = parent2.migrationInstinct;
        nocturnalFlight = parent2.nocturnalFlight;
    }

    // Energy management - blend
    flightMetabolism = (parent1.flightMetabolism + parent2.flightMetabolism) * 0.5f + Random::range(-0.1f, 0.1f);
    flightMetabolism = std::clamp(flightMetabolism, 0.5f, 2.0f);
    fatStorageCapacity = (parent1.fatStorageCapacity + parent2.fatStorageCapacity) * 0.5f + Random::range(-0.02f, 0.02f);
    fatStorageCapacity = std::clamp(fatStorageCapacity, 0.1f, 0.5f);
    restingRecoveryRate = (parent1.restingRecoveryRate + parent2.restingRecoveryRate) * 0.5f + Random::range(-0.02f, 0.02f);
    restingRecoveryRate = std::clamp(restingRecoveryRate, 0.1f, 0.5f);

    // Aquatic traits - average with variation
    finSize = (parent1.finSize + parent2.finSize) * 0.5f + Random::range(-0.05f, 0.05f);
    finSize = std::clamp(finSize, 0.3f, 1.0f);
    tailSize = (parent1.tailSize + parent2.tailSize) * 0.5f + Random::range(-0.05f, 0.05f);
    tailSize = std::clamp(tailSize, 0.5f, 1.2f);
    swimFrequency = (parent1.swimFrequency + parent2.swimFrequency) * 0.5f + Random::range(-0.2f, 0.2f);
    swimFrequency = std::clamp(swimFrequency, 1.0f, 4.0f);
    swimAmplitude = (parent1.swimAmplitude + parent2.swimAmplitude) * 0.5f + Random::range(-0.02f, 0.02f);
    swimAmplitude = std::clamp(swimAmplitude, 0.1f, 0.3f);
    preferredDepth = (parent1.preferredDepth + parent2.preferredDepth) * 0.5f + Random::range(-0.03f, 0.03f);
    preferredDepth = std::clamp(preferredDepth, 0.1f, 0.5f);
    schoolingStrength = (parent1.schoolingStrength + parent2.schoolingStrength) * 0.5f + Random::range(-0.05f, 0.05f);
    schoolingStrength = std::clamp(schoolingStrength, 0.5f, 1.0f);

    // Pattern trait crossover
    if (Random::chance(0.5f)) {
        scaleSize = parent1.scaleSize;
        scaleShininess = parent1.scaleShininess;
        patternFrequency = parent1.patternFrequency;
        patternType = parent1.patternType;
    } else {
        scaleSize = parent2.scaleSize;
        scaleShininess = parent2.scaleShininess;
        patternFrequency = parent2.patternFrequency;
        patternType = parent2.patternType;
    }

    // Enhanced pattern parameters crossover
    if (Random::chance(0.5f)) {
        patternIntensity = parent1.patternIntensity;
        patternSecondaryHue = parent1.patternSecondaryHue;
        spotSize = parent1.spotSize;
        stripeCount = parent1.stripeCount;
    } else {
        patternIntensity = parent2.patternIntensity;
        patternSecondaryHue = parent2.patternSecondaryHue;
        spotSize = parent2.spotSize;
        stripeCount = parent2.stripeCount;
    }

    gradientDirection = Random::chance(0.5f) ? parent1.gradientDirection : parent2.gradientDirection;
    markingContrast = (parent1.markingContrast + parent2.markingContrast) * 0.5f;

    // ==========================================
    // MORPHOLOGY DIVERSITY GENE CROSSOVER (Phase 7)
    // ==========================================

    // Body structure (linked genes)
    if (Random::chance(0.5f)) {
        segmentCount = parent1.segmentCount;
        bodyAspect = parent1.bodyAspect;
        bodyTaper = parent1.bodyTaper;
    } else {
        segmentCount = parent2.segmentCount;
        bodyAspect = parent2.bodyAspect;
        bodyTaper = parent2.bodyTaper;
    }

    // Fin configuration (linked for aquatic)
    if (Random::chance(0.5f)) {
        dorsalFinCount = parent1.dorsalFinCount;
        pectoralFinCount = parent1.pectoralFinCount;
        ventralFinCount = parent1.ventralFinCount;
        finAspect = parent1.finAspect;
        finRayCount = parent1.finRayCount;
    } else {
        dorsalFinCount = parent2.dorsalFinCount;
        pectoralFinCount = parent2.pectoralFinCount;
        ventralFinCount = parent2.ventralFinCount;
        finAspect = parent2.finAspect;
        finRayCount = parent2.finRayCount;
    }

    // Crest traits (linked)
    if (Random::chance(0.5f)) {
        crestHeight = parent1.crestHeight;
        crestExtent = parent1.crestExtent;
        crestType = parent1.crestType;
    } else {
        crestHeight = parent2.crestHeight;
        crestExtent = parent2.crestExtent;
        crestType = parent2.crestType;
    }

    // Horn traits (linked)
    if (Random::chance(0.5f)) {
        hornCount = parent1.hornCount;
        hornLength = parent1.hornLength;
        hornCurvature = parent1.hornCurvature;
        hornType = parent1.hornType;
    } else {
        hornCount = parent2.hornCount;
        hornLength = parent2.hornLength;
        hornCurvature = parent2.hornCurvature;
        hornType = parent2.hornType;
    }

    // Antennae (linked)
    if (Random::chance(0.5f)) {
        antennaeCount = parent1.antennaeCount;
        antennaeLength = parent1.antennaeLength;
    } else {
        antennaeCount = parent2.antennaeCount;
        antennaeLength = parent2.antennaeLength;
    }

    // Tail variant (linked)
    if (Random::chance(0.5f)) {
        tailVariant = parent1.tailVariant;
        tailFinHeight = parent1.tailFinHeight;
        tailBulbSize = parent1.tailBulbSize;
    } else {
        tailVariant = parent2.tailVariant;
        tailFinHeight = parent2.tailFinHeight;
        tailBulbSize = parent2.tailBulbSize;
    }

    // Jaw traits (linked)
    if (Random::chance(0.5f)) {
        jawType = parent1.jawType;
        jawProtrusion = parent1.jawProtrusion;
        barbels = parent1.barbels;
    } else {
        jawType = parent2.jawType;
        jawProtrusion = parent2.jawProtrusion;
        barbels = parent2.barbels;
    }

    // Limb structure (linked)
    if (Random::chance(0.5f)) {
        limbSegments = parent1.limbSegments;
        limbTaper = parent1.limbTaper;
        footSpread = parent1.footSpread;
        hasClaws = parent1.hasClaws;
        clawLength = parent1.clawLength;
    } else {
        limbSegments = parent2.limbSegments;
        limbTaper = parent2.limbTaper;
        footSpread = parent2.footSpread;
        hasClaws = parent2.hasClaws;
        clawLength = parent2.clawLength;
    }

    // Spike traits (linked)
    if (Random::chance(0.5f)) {
        spikeRows = parent1.spikeRows;
        spikeLength = parent1.spikeLength;
        spikeDensity = parent1.spikeDensity;
    } else {
        spikeRows = parent2.spikeRows;
        spikeLength = parent2.spikeLength;
        spikeDensity = parent2.spikeDensity;
    }

    // Shell traits (linked)
    if (Random::chance(0.5f)) {
        shellCoverage = parent1.shellCoverage;
        shellSegmentation = parent1.shellSegmentation;
        shellTexture = parent1.shellTexture;
    } else {
        shellCoverage = parent2.shellCoverage;
        shellSegmentation = parent2.shellSegmentation;
        shellTexture = parent2.shellTexture;
    }

    // Frill traits (linked)
    if (Random::chance(0.5f)) {
        hasNeckFrill = parent1.hasNeckFrill;
        frillSize = parent1.frillSize;
        hasBodyFrills = parent1.hasBodyFrills;
        displayFeatherSize = parent1.displayFeatherSize;
    } else {
        hasNeckFrill = parent2.hasNeckFrill;
        frillSize = parent2.frillSize;
        hasBodyFrills = parent2.hasBodyFrills;
        displayFeatherSize = parent2.displayFeatherSize;
    }

    // Eye diversity (linked)
    if (Random::chance(0.5f)) {
        eyeArrangement = parent1.eyeArrangement;
        eyeProtrusion = parent1.eyeProtrusion;
        hasEyeSpots = parent1.hasEyeSpots;
        eyeSpotCount = parent1.eyeSpotCount;
    } else {
        eyeArrangement = parent2.eyeArrangement;
        eyeProtrusion = parent2.eyeProtrusion;
        hasEyeSpots = parent2.hasEyeSpots;
        eyeSpotCount = parent2.eyeSpotCount;
    }

    // ==========================================
    // BIOCHEMISTRY TRAIT CROSSOVER (Planet Chemistry)
    // ==========================================
    // Biochemistry traits are inherited as linked groups because they represent
    // tightly coupled cellular machinery that must work together.

    // Core biochemistry group (cellular foundation)
    if (Random::chance(0.5f)) {
        biopigmentFamily = parent1.biopigmentFamily;
        membraneFluidity = parent1.membraneFluidity;
        solventAffinity = parent1.solventAffinity;
        metabolicPathway = parent1.metabolicPathway;
    } else {
        biopigmentFamily = parent2.biopigmentFamily;
        membraneFluidity = parent2.membraneFluidity;
        solventAffinity = parent2.solventAffinity;
        metabolicPathway = parent2.metabolicPathway;
    }

    // Environmental adaptation group (can recombine independently)
    if (Random::chance(0.5f)) {
        oxygenTolerance = parent1.oxygenTolerance;
        radiationResistance = parent1.radiationResistance;
    } else {
        oxygenTolerance = parent2.oxygenTolerance;
        radiationResistance = parent2.radiationResistance;
    }

    // Structural biochemistry group
    if (Random::chance(0.5f)) {
        mineralizationBias = parent1.mineralizationBias;
        pHPreference = parent1.pHPreference;
    } else {
        mineralizationBias = parent2.mineralizationBias;
        pHPreference = parent2.pHPreference;
    }

    // Temperature tolerance - blend with slight variation
    temperatureTolerance = (parent1.temperatureTolerance + parent2.temperatureTolerance) * 0.5f + Random::range(-2.0f, 2.0f);
    temperatureTolerance = std::clamp(temperatureTolerance, 5.0f, 50.0f);
}

void Genome::mutate(float mutationRate, float mutationStrength) {
    // Mutate physical traits
    if (Random::chance(mutationRate)) {
        size = std::clamp(size + Random::range(-mutationStrength, mutationStrength), 0.5f, 2.0f);
    }

    if (Random::chance(mutationRate)) {
        speed = std::clamp(speed + Random::range(-mutationStrength * 3.0f, mutationStrength * 3.0f), 5.0f, 20.0f);
    }

    if (Random::chance(mutationRate)) {
        visionRange = std::clamp(visionRange + Random::range(-mutationStrength * 10.0f, mutationStrength * 10.0f), 10.0f, 50.0f);
    }

    if (Random::chance(mutationRate)) {
        efficiency = std::clamp(efficiency + Random::range(-mutationStrength * 0.2f, mutationStrength * 0.2f), 0.5f, 1.5f);
    }

    // Mutate color slightly
    if (Random::chance(mutationRate)) {
        color.r = std::clamp(color.r + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
        color.g = std::clamp(color.g + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
        color.b = std::clamp(color.b + Random::range(-0.1f, 0.1f), 0.0f, 1.0f);
    }

    // Mutate neural weights
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        if (Random::chance(mutationRate)) {
            neuralWeights[i] = std::clamp(
                neuralWeights[i] + Random::range(-mutationStrength, mutationStrength),
                -1.0f, 1.0f
            );
        }
    }

    // ==========================================
    // SENSORY TRAIT MUTATION
    // ==========================================

    // Vision mutations
    if (Random::chance(mutationRate)) {
        visionFOV = std::clamp(visionFOV + Random::range(-mutationStrength * 0.5f, mutationStrength * 0.5f), 1.0f, 6.0f);
    }
    if (Random::chance(mutationRate)) {
        visionAcuity = std::clamp(visionAcuity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        colorPerception = std::clamp(colorPerception + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        motionDetection = std::clamp(motionDetection + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Hearing mutations
    if (Random::chance(mutationRate)) {
        hearingRange = std::clamp(hearingRange + Random::range(-mutationStrength * 10.0f, mutationStrength * 10.0f), 10.0f, 100.0f);
    }
    if (Random::chance(mutationRate)) {
        hearingDirectionality = std::clamp(hearingDirectionality + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate * 0.5f)) {  // Echolocation is rare
        echolocationAbility = std::clamp(echolocationAbility + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }

    // Smell mutations
    if (Random::chance(mutationRate)) {
        smellRange = std::clamp(smellRange + Random::range(-mutationStrength * 15.0f, mutationStrength * 15.0f), 10.0f, 150.0f);
    }
    if (Random::chance(mutationRate)) {
        smellSensitivity = std::clamp(smellSensitivity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        pheromoneProduction = std::clamp(pheromoneProduction + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Touch mutations
    if (Random::chance(mutationRate)) {
        touchRange = std::clamp(touchRange + Random::range(-mutationStrength * 1.0f, mutationStrength * 1.0f), 0.5f, 8.0f);
    }
    if (Random::chance(mutationRate)) {
        vibrationSensitivity = std::clamp(vibrationSensitivity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Camouflage mutation
    if (Random::chance(mutationRate)) {
        camouflageLevel = std::clamp(camouflageLevel + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Communication mutations
    if (Random::chance(mutationRate)) {
        alarmCallVolume = std::clamp(alarmCallVolume + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        displayIntensity = std::clamp(displayIntensity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Memory mutations
    if (Random::chance(mutationRate)) {
        memoryCapacity = std::clamp(memoryCapacity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        memoryRetention = std::clamp(memoryRetention + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Flying trait mutations - basic flight
    if (Random::chance(mutationRate)) {
        wingSpan = std::clamp(wingSpan + Random::range(-mutationStrength * 0.2f, mutationStrength * 0.2f), 0.5f, 2.0f);
    }
    if (Random::chance(mutationRate)) {
        flapFrequency = std::clamp(flapFrequency + Random::range(-mutationStrength * 1.0f, mutationStrength * 1.0f), 2.0f, 200.0f);
    }
    if (Random::chance(mutationRate)) {
        glideRatio = std::clamp(glideRatio + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 0.95f);
    }
    if (Random::chance(mutationRate)) {
        preferredAltitude = std::clamp(preferredAltitude + Random::range(-mutationStrength * 5.0f, mutationStrength * 5.0f), 5.0f, 100.0f);
    }

    // Wing morphology mutations
    if (Random::chance(mutationRate)) {
        wingChord = std::clamp(wingChord + Random::range(-mutationStrength * 0.05f, mutationStrength * 0.05f), 0.1f, 0.5f);
    }
    if (Random::chance(mutationRate)) {
        wingAspectRatio = std::clamp(wingAspectRatio + Random::range(-mutationStrength * 1.0f, mutationStrength * 1.0f), 2.0f, 20.0f);
    }
    if (Random::chance(mutationRate)) {
        wingLoading = std::clamp(wingLoading + Random::range(-mutationStrength * 5.0f, mutationStrength * 5.0f), 10.0f, 100.0f);
    }
    if (Random::chance(mutationRate)) {
        wingCamber = std::clamp(wingCamber + Random::range(-mutationStrength * 0.02f, mutationStrength * 0.02f), 0.0f, 0.15f);
    }
    if (Random::chance(mutationRate)) {
        wingTaper = std::clamp(wingTaper + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.2f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        wingTwist = std::clamp(wingTwist + Random::range(-mutationStrength * 1.0f, mutationStrength * 1.0f), -5.0f, 5.0f);
    }
    if (Random::chance(mutationRate)) {
        dihedralAngle = std::clamp(dihedralAngle + Random::range(-mutationStrength * 2.0f, mutationStrength * 2.0f), 0.0f, 15.0f);
    }
    if (Random::chance(mutationRate)) {
        sweepAngle = std::clamp(sweepAngle + Random::range(-mutationStrength * 3.0f, mutationStrength * 3.0f), -10.0f, 45.0f);
    }

    // Wing type mutation (rare - major structural change)
    if (Random::chance(mutationRate * 0.1f)) {
        int newWingType = static_cast<int>(wingType) + Random::rangeInt(-1, 1);
        wingType = static_cast<uint8_t>(std::clamp(newWingType, 0, 7));
    }

    // Tail mutations
    if (Random::chance(mutationRate)) {
        tailLength = std::clamp(tailLength + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.2f, 1.5f);
    }
    if (Random::chance(mutationRate)) {
        tailSpan = std::clamp(tailSpan + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.2f, 0.8f);
    }
    if (Random::chance(mutationRate * 0.2f)) {
        int newTailType = static_cast<int>(tailType) + Random::rangeInt(-1, 1);
        tailType = static_cast<uint8_t>(std::clamp(newTailType, 0, 4));
    }

    // Flight musculature mutations
    if (Random::chance(mutationRate)) {
        breastMuscleRatio = std::clamp(breastMuscleRatio + Random::range(-mutationStrength * 0.03f, mutationStrength * 0.03f), 0.1f, 0.4f);
    }
    if (Random::chance(mutationRate)) {
        supracoracoideus = std::clamp(supracoracoideus + Random::range(-mutationStrength * 0.01f, mutationStrength * 0.01f), 0.02f, 0.1f);
    }
    if (Random::chance(mutationRate)) {
        muscleOxygenCapacity = std::clamp(muscleOxygenCapacity + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.5f, 1.5f);
    }
    if (Random::chance(mutationRate)) {
        anaerobicCapacity = std::clamp(anaerobicCapacity + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.3f, 1.0f);
    }

    // Body aerodynamics mutations
    if (Random::chance(mutationRate)) {
        bodyDragCoeff = std::clamp(bodyDragCoeff + Random::range(-mutationStrength * 0.01f, mutationStrength * 0.01f), 0.01f, 0.1f);
    }
    if (Random::chance(mutationRate)) {
        fuselageLength = std::clamp(fuselageLength + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.8f, 2.0f);
    }
    if (Random::chance(mutationRate)) {
        bodyDensity = std::clamp(bodyDensity + Random::range(-mutationStrength * 0.05f, mutationStrength * 0.05f), 0.8f, 1.2f);
    }

    // Specialized flight capability mutations
    if (Random::chance(mutationRate)) {
        hoveringAbility = std::clamp(hoveringAbility + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        divingSpeed = std::clamp(divingSpeed + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        maneuverability = std::clamp(maneuverability + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        thermalSensingAbility = std::clamp(thermalSensingAbility + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        windResistance = std::clamp(windResistance + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }

    // Flight behavior mutations
    if (Random::chance(mutationRate)) {
        flockingStrength = std::clamp(flockingStrength + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        territorialRadius = std::clamp(territorialRadius + Random::range(-mutationStrength * 5.0f, mutationStrength * 5.0f), 5.0f, 100.0f);
    }
    if (Random::chance(mutationRate)) {
        migrationInstinct = std::clamp(migrationInstinct + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        nocturnalFlight = std::clamp(nocturnalFlight + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }

    // Energy management mutations
    if (Random::chance(mutationRate)) {
        flightMetabolism = std::clamp(flightMetabolism + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.5f, 2.0f);
    }
    if (Random::chance(mutationRate)) {
        fatStorageCapacity = std::clamp(fatStorageCapacity + Random::range(-mutationStrength * 0.04f, mutationStrength * 0.04f), 0.1f, 0.5f);
    }
    if (Random::chance(mutationRate)) {
        restingRecoveryRate = std::clamp(restingRecoveryRate + Random::range(-mutationStrength * 0.04f, mutationStrength * 0.04f), 0.1f, 0.5f);
    }

    // Aquatic trait mutations
    if (Random::chance(mutationRate)) {
        finSize = std::clamp(finSize + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.3f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        tailSize = std::clamp(tailSize + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.5f, 1.2f);
    }
    if (Random::chance(mutationRate)) {
        swimFrequency = std::clamp(swimFrequency + Random::range(-mutationStrength * 0.5f, mutationStrength * 0.5f), 1.0f, 4.0f);
    }
    if (Random::chance(mutationRate)) {
        swimAmplitude = std::clamp(swimAmplitude + Random::range(-mutationStrength * 0.03f, mutationStrength * 0.03f), 0.1f, 0.3f);
    }
    if (Random::chance(mutationRate)) {
        preferredDepth = std::clamp(preferredDepth + Random::range(-mutationStrength * 0.05f, mutationStrength * 0.05f), 0.1f, 0.5f);
    }
    if (Random::chance(mutationRate)) {
        schoolingStrength = std::clamp(schoolingStrength + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.5f, 1.0f);
    }

    // Pattern trait mutations
    if (Random::chance(mutationRate)) {
        scaleSize = std::clamp(scaleSize + Random::range(-mutationStrength * 0.01f, mutationStrength * 0.01f), 0.01f, 0.1f);
    }
    if (Random::chance(mutationRate)) {
        scaleShininess = std::clamp(scaleShininess + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.2f, 0.9f);
    }
    if (Random::chance(mutationRate)) {
        patternFrequency = std::clamp(patternFrequency + Random::range(-mutationStrength * 1.0f, mutationStrength * 1.0f), 1.0f, 10.0f);
    }
    if (Random::chance(mutationRate * 0.2f)) {  // Rare pattern type change
        int newPatternType = static_cast<int>(patternType) + Random::rangeInt(-1, 1);
        patternType = static_cast<uint8_t>(std::clamp(newPatternType, 0, 4));
    }
    if (Random::chance(mutationRate)) {
        patternIntensity = std::clamp(patternIntensity + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        patternSecondaryHue = std::clamp(patternSecondaryHue + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        spotSize = std::clamp(spotSize + Random::range(-mutationStrength * 0.03f, mutationStrength * 0.03f), 0.02f, 0.3f);
    }
    if (Random::chance(mutationRate)) {
        int newStripeCount = static_cast<int>(stripeCount) + Random::rangeInt(-2, 2);
        stripeCount = static_cast<uint8_t>(std::clamp(newStripeCount, 3, 15));
    }
    if (Random::chance(mutationRate)) {
        gradientDirection = std::clamp(gradientDirection + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        markingContrast = std::clamp(markingContrast + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.1f, 1.0f);
    }

    // ==========================================
    // MORPHOLOGY DIVERSITY GENE MUTATIONS (Phase 7)
    // ==========================================

    // Helper lambda for heavy-tailed mutation (Cauchy-like distribution for rare extremes)
    auto heavyTailedMutation = [&](float value, float min, float max, float scale) -> float {
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            // Macro-mutation: jump to a new random value in range
            return Random::range(min, max);
        } else if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            // Extreme trait: push toward min or max
            float direction = Random::chance(0.5f) ? 1.0f : -1.0f;
            return std::clamp(value + direction * (max - min) * 0.4f, min, max);
        } else {
            // Normal mutation
            return std::clamp(value + Random::range(-mutationStrength * scale, mutationStrength * scale), min, max);
        }
    };

    // Body structure mutations
    if (Random::chance(mutationRate)) {
        int newSegmentCount = static_cast<int>(segmentCount) + Random::rangeInt(-1, 1);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            newSegmentCount = Random::rangeInt(1, 8);
        }
        segmentCount = static_cast<uint8_t>(std::clamp(newSegmentCount, 1, 8));
    }
    if (Random::chance(mutationRate)) {
        bodyAspect = heavyTailedMutation(bodyAspect, 0.3f, 3.0f, 0.2f);
    }
    if (Random::chance(mutationRate)) {
        bodyTaper = std::clamp(bodyTaper + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.5f, 1.5f);
    }

    // Fin count mutations (aquatic)
    if (Random::chance(mutationRate * 0.3f)) {
        int newDorsalFins = static_cast<int>(dorsalFinCount) + Random::rangeInt(-1, 1);
        dorsalFinCount = static_cast<uint8_t>(std::clamp(newDorsalFins, 0, 3));
    }
    if (Random::chance(mutationRate * 0.3f)) {
        int newPectoralFins = static_cast<int>(pectoralFinCount) + Random::rangeInt(-1, 1);
        pectoralFinCount = static_cast<uint8_t>(std::clamp(newPectoralFins, 0, 4));
    }
    if (Random::chance(mutationRate * 0.3f)) {
        int newVentralFins = static_cast<int>(ventralFinCount) + Random::rangeInt(-1, 1);
        ventralFinCount = static_cast<uint8_t>(std::clamp(newVentralFins, 0, 2));
    }
    if (Random::chance(mutationRate)) {
        finAspect = heavyTailedMutation(finAspect, 0.3f, 3.0f, 0.2f);
    }
    if (Random::chance(mutationRate)) {
        finRayCount = std::clamp(finRayCount + Random::range(-1.0f, 1.0f), 3.0f, 12.0f);
    }

    // Crest mutations
    if (Random::chance(mutationRate)) {
        crestHeight = heavyTailedMutation(crestHeight, 0.0f, 0.8f, 0.1f);
    }
    if (Random::chance(mutationRate)) {
        crestExtent = std::clamp(crestExtent + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate * 0.15f)) {  // Rare crest type change
        int newCrestType = static_cast<int>(crestType) + Random::rangeInt(-1, 1);
        crestType = static_cast<uint8_t>(std::clamp(newCrestType, 0, 4));
    }

    // Horn/antennae mutations
    if (Random::chance(mutationRate * 0.2f)) {  // Horn count changes are rare
        int newHornCount = static_cast<int>(hornCount) + Random::rangeInt(-1, 1);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            newHornCount = Random::rangeInt(0, 6);
        }
        hornCount = static_cast<uint8_t>(std::clamp(newHornCount, 0, 6));
    }
    if (Random::chance(mutationRate)) {
        hornLength = heavyTailedMutation(hornLength, 0.1f, 1.5f, 0.15f);
    }
    if (Random::chance(mutationRate)) {
        hornCurvature = std::clamp(hornCurvature + Random::range(-mutationStrength * 0.3f, mutationStrength * 0.3f), -1.0f, 1.0f);
    }
    if (Random::chance(mutationRate * 0.1f)) {
        int newHornType = static_cast<int>(hornType) + Random::rangeInt(-1, 1);
        hornType = static_cast<uint8_t>(std::clamp(newHornType, 0, 3));
    }
    if (Random::chance(mutationRate * 0.2f)) {
        int newAntennaeCount = static_cast<int>(antennaeCount) + Random::rangeInt(-1, 1);
        antennaeCount = static_cast<uint8_t>(std::clamp(newAntennaeCount, 0, 4));
    }
    if (Random::chance(mutationRate)) {
        antennaeLength = heavyTailedMutation(antennaeLength, 0.2f, 2.0f, 0.2f);
    }

    // Tail variant mutations
    if (Random::chance(mutationRate * 0.15f)) {  // Rare tail type change
        int newTailVariant = static_cast<int>(tailVariant) + Random::rangeInt(-1, 1);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            newTailVariant = Random::rangeInt(0, 6);
        }
        tailVariant = static_cast<uint8_t>(std::clamp(newTailVariant, 0, 6));
    }
    if (Random::chance(mutationRate)) {
        tailFinHeight = std::clamp(tailFinHeight + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.0f, 0.5f);
    }
    if (Random::chance(mutationRate)) {
        tailBulbSize = std::clamp(tailBulbSize + Random::range(-mutationStrength * 0.06f, mutationStrength * 0.06f), 0.0f, 0.4f);
    }

    // Jaw mutations
    if (Random::chance(mutationRate * 0.1f)) {
        int newJawType = static_cast<int>(jawType) + Random::rangeInt(-1, 1);
        jawType = static_cast<uint8_t>(std::clamp(newJawType, 0, 4));
    }
    if (Random::chance(mutationRate)) {
        jawProtrusion = std::clamp(jawProtrusion + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), -0.3f, 0.5f);
    }
    if (Random::chance(mutationRate)) {
        barbels = std::clamp(barbels + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Limb variation mutations
    if (Random::chance(mutationRate * 0.2f)) {
        int newLimbSegments = static_cast<int>(limbSegments) + Random::rangeInt(-1, 1);
        limbSegments = static_cast<uint8_t>(std::clamp(newLimbSegments, 2, 5));
    }
    if (Random::chance(mutationRate)) {
        limbTaper = std::clamp(limbTaper + Random::range(-mutationStrength * 0.1f, mutationStrength * 0.1f), 0.3f, 1.0f);
    }
    if (Random::chance(mutationRate)) {
        footSpread = heavyTailedMutation(footSpread, 0.3f, 2.0f, 0.15f);
    }
    if (Random::chance(mutationRate * 0.1f)) {
        hasClaws = !hasClaws;
    }
    if (Random::chance(mutationRate)) {
        clawLength = std::clamp(clawLength + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.0f, 0.5f);
    }

    // Spike mutations
    if (Random::chance(mutationRate * 0.2f)) {
        int newSpikeRows = static_cast<int>(spikeRows) + Random::rangeInt(-1, 1);
        spikeRows = static_cast<uint8_t>(std::clamp(newSpikeRows, 0, 4));
    }
    if (Random::chance(mutationRate)) {
        spikeLength = heavyTailedMutation(spikeLength, 0.0f, 0.6f, 0.08f);
    }
    if (Random::chance(mutationRate)) {
        spikeDensity = std::clamp(spikeDensity + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.1f, 1.0f);
    }

    // Shell/armor mutations
    if (Random::chance(mutationRate)) {
        shellCoverage = heavyTailedMutation(shellCoverage, 0.0f, 1.0f, 0.1f);
    }
    if (Random::chance(mutationRate)) {
        shellSegmentation = std::clamp(shellSegmentation + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }
    if (Random::chance(mutationRate * 0.15f)) {
        int newShellTexture = static_cast<int>(shellTexture) + Random::rangeInt(-1, 1);
        shellTexture = static_cast<uint8_t>(std::clamp(newShellTexture, 0, 3));
    }

    // Frill mutations
    if (Random::chance(mutationRate * 0.1f)) {
        hasNeckFrill = !hasNeckFrill;
    }
    if (Random::chance(mutationRate)) {
        frillSize = heavyTailedMutation(frillSize, 0.1f, 1.5f, 0.15f);
    }
    if (Random::chance(mutationRate * 0.1f)) {
        hasBodyFrills = !hasBodyFrills;
    }
    if (Random::chance(mutationRate)) {
        displayFeatherSize = std::clamp(displayFeatherSize + Random::range(-mutationStrength * 0.15f, mutationStrength * 0.15f), 0.0f, 1.0f);
    }

    // Eye diversity mutations
    if (Random::chance(mutationRate * 0.1f)) {
        int newEyeArrangement = static_cast<int>(eyeArrangement) + Random::rangeInt(-1, 1);
        eyeArrangement = static_cast<uint8_t>(std::clamp(newEyeArrangement, 0, 4));
    }
    if (Random::chance(mutationRate)) {
        eyeProtrusion = std::clamp(eyeProtrusion + Random::range(-mutationStrength * 0.08f, mutationStrength * 0.08f), 0.0f, 0.5f);
    }
    if (Random::chance(mutationRate * 0.05f)) {
        hasEyeSpots = !hasEyeSpots;
    }
    if (Random::chance(mutationRate * 0.2f)) {
        int newEyeSpotCount = static_cast<int>(eyeSpotCount) + Random::rangeInt(-1, 1);
        eyeSpotCount = static_cast<uint8_t>(std::clamp(newEyeSpotCount, 0, 8));
    }

    // ==========================================
    // BIOCHEMISTRY TRAIT MUTATIONS (Planet Chemistry)
    // ==========================================
    // These mutations tend to be slower and more constrained than physical traits,
    // representing fundamental biochemical changes that are harder to evolve.

    // Biopigment family mutation (rare - major biochemical shift)
    if (Random::chance(mutationRate * 0.1f)) {
        int newPigment = static_cast<int>(biopigmentFamily) + Random::rangeInt(-1, 1);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            newPigment = Random::rangeInt(0, 5);  // Rare jump to any pigment type
        }
        biopigmentFamily = static_cast<uint8_t>(std::clamp(newPigment, 0, 5));
    }

    // Membrane fluidity mutation
    if (Random::chance(mutationRate)) {
        membraneFluidity = heavyTailedMutation(membraneFluidity, 0.0f, 1.0f, 0.08f);
    }

    // Oxygen tolerance mutation
    if (Random::chance(mutationRate)) {
        oxygenTolerance = heavyTailedMutation(oxygenTolerance, 0.0f, 1.0f, 0.1f);
    }

    // Mineralization bias mutation
    if (Random::chance(mutationRate)) {
        mineralizationBias = heavyTailedMutation(mineralizationBias, 0.0f, 1.0f, 0.08f);
    }

    // Solvent affinity mutation (very rare - fundamental chemistry)
    if (Random::chance(mutationRate * 0.3f)) {
        solventAffinity = std::clamp(solventAffinity + Random::range(-mutationStrength * 0.05f, mutationStrength * 0.05f), 0.0f, 1.0f);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            // Rare major solvent adaptation shift
            solventAffinity = Random::range(0.0f, 1.0f);
        }
    }

    // Temperature tolerance mutation
    if (Random::chance(mutationRate)) {
        temperatureTolerance = heavyTailedMutation(temperatureTolerance, 5.0f, 50.0f, 3.0f);
    }

    // Radiation resistance mutation
    if (Random::chance(mutationRate)) {
        radiationResistance = heavyTailedMutation(radiationResistance, 0.0f, 1.0f, 0.08f);
    }

    // pH preference mutation
    if (Random::chance(mutationRate)) {
        pHPreference = heavyTailedMutation(pHPreference, 0.0f, 1.0f, 0.08f);
    }

    // Metabolic pathway mutation (rare - major change)
    if (Random::chance(mutationRate * 0.05f)) {
        int newPathway = static_cast<int>(metabolicPathway) + Random::rangeInt(-1, 1);
        if (Random::chance(MACRO_MUTATION_CHANCE)) {
            newPathway = Random::rangeInt(0, 3);  // Rare jump to any pathway
        }
        metabolicPathway = static_cast<uint8_t>(std::clamp(newPathway, 0, 3));
    }
}

void Genome::randomize() {
    // Original physical traits
    size = Random::range(0.5f, 2.0f);
    speed = Random::range(5.0f, 20.0f);
    visionRange = Random::range(10.0f, 50.0f);
    efficiency = Random::range(0.5f, 1.5f);

    color = glm::vec3(Random::value(), Random::value(), Random::value());

    neuralWeights.resize(NEURAL_WEIGHT_COUNT);
    for (int i = 0; i < NEURAL_WEIGHT_COUNT; i++) {
        neuralWeights[i] = Random::range(-1.0f, 1.0f);
    }

    // ==========================================
    // SENSORY TRAIT RANDOMIZATION
    // ==========================================

    // Vision traits
    visionFOV = Random::range(1.5f, 5.0f);        // π/2 to ~286 degrees
    visionAcuity = Random::range(0.2f, 0.8f);
    colorPerception = Random::range(0.1f, 0.7f);
    motionDetection = Random::range(0.3f, 0.8f);

    // Hearing traits
    hearingRange = Random::range(20.0f, 60.0f);
    hearingDirectionality = Random::range(0.3f, 0.7f);
    echolocationAbility = Random::range(0.0f, 0.2f);  // Most creatures start without echolocation

    // Smell traits
    smellRange = Random::range(30.0f, 80.0f);
    smellSensitivity = Random::range(0.3f, 0.7f);
    pheromoneProduction = Random::range(0.2f, 0.5f);

    // Touch traits
    touchRange = Random::range(1.0f, 4.0f);
    vibrationSensitivity = Random::range(0.2f, 0.6f);

    // Camouflage (prey may start with some)
    camouflageLevel = Random::range(0.0f, 0.3f);

    // Communication traits
    alarmCallVolume = Random::range(0.3f, 0.6f);
    displayIntensity = Random::range(0.2f, 0.5f);

    // Memory traits
    memoryCapacity = Random::range(0.3f, 0.7f);
    memoryRetention = Random::range(0.3f, 0.7f);

    // Flying traits (default values for non-flying creatures)
    wingSpan = 0.0f;
    flapFrequency = 0.0f;
    glideRatio = 0.0f;
    preferredAltitude = 0.0f;

    // Wing morphology defaults (non-flying)
    wingChord = 0.0f;
    wingAspectRatio = 0.0f;
    wingLoading = 0.0f;
    wingCamber = 0.0f;
    wingTaper = 0.0f;
    wingTwist = 0.0f;
    dihedralAngle = 0.0f;
    sweepAngle = 0.0f;
    wingType = 0;
    featherType = 0;

    // Tail defaults
    tailLength = 0.0f;
    tailSpan = 0.0f;
    tailType = 0;

    // Flight musculature defaults
    breastMuscleRatio = 0.0f;
    supracoracoideus = 0.0f;
    muscleOxygenCapacity = 0.0f;
    anaerobicCapacity = 0.0f;

    // Body aerodynamics defaults
    bodyDragCoeff = 0.05f;
    fuselageLength = 1.0f;
    bodyDensity = 1.0f;

    // Specialized flight capabilities defaults
    hoveringAbility = 0.0f;
    divingSpeed = 0.0f;
    maneuverability = 0.0f;
    thermalSensingAbility = 0.0f;
    windResistance = 0.0f;

    // Flight behavior defaults
    flockingStrength = 0.0f;
    territorialRadius = 0.0f;
    migrationInstinct = 0.0f;
    nocturnalFlight = 0.0f;

    // Energy management defaults
    flightMetabolism = 1.0f;
    fatStorageCapacity = 0.1f;
    restingRecoveryRate = 0.2f;

    // Aquatic traits (random values within specified ranges)
    finSize = Random::range(0.3f, 1.0f);
    tailSize = Random::range(0.5f, 1.2f);
    swimFrequency = Random::range(1.0f, 4.0f);
    swimAmplitude = Random::range(0.1f, 0.3f);
    preferredDepth = Random::range(0.1f, 0.5f);
    schoolingStrength = Random::range(0.5f, 1.0f);

    // Scale/skin pattern traits
    scaleSize = Random::range(0.01f, 0.1f);
    scaleShininess = Random::range(0.2f, 0.9f);
    patternFrequency = Random::range(1.0f, 10.0f);
    patternType = static_cast<uint8_t>(Random::rangeInt(0, 4));  // 0=solid, 1=stripes, 2=spots, 3=gradient, 4=counter-shading

    // Enhanced pattern parameters for visual diversity
    patternIntensity = Random::range(0.3f, 1.0f);
    patternSecondaryHue = Random::range(0.0f, 1.0f);
    spotSize = Random::range(0.02f, 0.3f);
    stripeCount = static_cast<uint8_t>(Random::rangeInt(3, 15));
    gradientDirection = Random::range(0.0f, 1.0f);
    markingContrast = Random::range(0.1f, 1.0f);

    // ==========================================
    // MORPHOLOGY DIVERSITY GENE RANDOMIZATION (Phase 7)
    // ==========================================

    // Body structure (with heavy-tailed distribution for diversity)
    segmentCount = static_cast<uint8_t>(Random::rangeInt(2, 5));  // Most creatures have 2-5 segments
    if (Random::chance(EXTREME_TRAIT_CHANCE)) {
        segmentCount = static_cast<uint8_t>(Random::rangeInt(1, 8));  // Rare extreme segmentation
    }

    bodyAspect = Random::range(0.6f, 1.8f);  // Normal range
    if (Random::chance(EXTREME_TRAIT_CHANCE)) {
        bodyAspect = Random::value() < 0.5f ? Random::range(0.3f, 0.5f) : Random::range(2.0f, 3.0f);  // Very compact or elongated
    }
    bodyTaper = Random::range(0.7f, 1.1f);

    // Fin diversity (mainly for aquatic)
    dorsalFinCount = static_cast<uint8_t>(Random::rangeInt(0, 2));
    pectoralFinCount = static_cast<uint8_t>(Random::rangeInt(0, 2));
    ventralFinCount = static_cast<uint8_t>(Random::rangeInt(0, 1));
    finAspect = Random::range(0.5f, 2.0f);
    finRayCount = Random::range(5.0f, 10.0f);

    // Crests (rare by default, more common in specific creature types)
    crestHeight = 0.0f;
    crestExtent = 0.0f;
    crestType = 0;
    if (Random::chance(0.15f)) {  // 15% chance of having a crest
        crestHeight = Random::range(0.1f, 0.4f);
        crestExtent = Random::range(0.2f, 0.6f);
        crestType = static_cast<uint8_t>(Random::rangeInt(1, 4));
        if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            crestHeight = Random::range(0.4f, 0.8f);  // Dramatic crest
        }
    }

    // Horns (rare)
    hornCount = 0;
    hornLength = 0.0f;
    hornCurvature = 0.0f;
    hornType = 0;
    if (Random::chance(0.12f)) {  // 12% chance of having horns
        hornCount = static_cast<uint8_t>(Random::rangeInt(1, 3));
        hornLength = Random::range(0.2f, 0.6f);
        hornCurvature = Random::range(-0.5f, 0.5f);
        hornType = static_cast<uint8_t>(Random::rangeInt(0, 3));
        if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            hornCount = static_cast<uint8_t>(Random::rangeInt(4, 6));  // Multi-horned
            hornLength = Random::range(0.8f, 1.5f);  // Long horns
        }
    }

    // Antennae (rare for non-insects)
    antennaeCount = 0;
    antennaeLength = 0.0f;
    if (Random::chance(0.08f)) {
        antennaeCount = static_cast<uint8_t>(Random::rangeInt(1, 2)) * 2;  // Always pairs
        antennaeLength = Random::range(0.3f, 1.0f);
    }

    // Tail variants
    tailVariant = 0;  // Standard tail
    tailFinHeight = 0.0f;
    tailBulbSize = 0.0f;
    if (Random::chance(0.25f)) {  // 25% chance of special tail
        tailVariant = static_cast<uint8_t>(Random::rangeInt(1, 6));
        if (tailVariant == 2) {  // Fan tail
            tailFinHeight = Random::range(0.1f, 0.4f);
        } else if (tailVariant == 1) {  // Club tail
            tailBulbSize = Random::range(0.1f, 0.3f);
        }
    }

    // Jaw type (most default to standard)
    jawType = 0;
    jawProtrusion = Random::range(-0.1f, 0.2f);
    barbels = 0.0f;
    if (Random::chance(0.2f)) {
        jawType = static_cast<uint8_t>(Random::rangeInt(1, 4));
    }
    if (Random::chance(0.1f)) {
        barbels = Random::range(0.2f, 0.8f);  // Whisker-like sensory organs
    }

    // Limb structure
    limbSegments = static_cast<uint8_t>(Random::rangeInt(2, 4));
    limbTaper = Random::range(0.5f, 0.9f);
    footSpread = Random::range(0.5f, 1.2f);
    hasClaws = Random::chance(0.3f);
    clawLength = hasClaws ? Random::range(0.05f, 0.25f) : 0.0f;

    // Spikes (rare)
    spikeRows = 0;
    spikeLength = 0.0f;
    spikeDensity = 0.0f;
    if (Random::chance(0.1f)) {
        spikeRows = static_cast<uint8_t>(Random::rangeInt(1, 3));
        spikeLength = Random::range(0.05f, 0.3f);
        spikeDensity = Random::range(0.3f, 0.7f);
        if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            spikeRows = static_cast<uint8_t>(Random::rangeInt(3, 4));
            spikeLength = Random::range(0.3f, 0.6f);
        }
    }

    // Shell/armor (rare)
    shellCoverage = 0.0f;
    shellSegmentation = 0.0f;
    shellTexture = 0;
    if (Random::chance(0.08f)) {
        shellCoverage = Random::range(0.2f, 0.7f);
        shellSegmentation = Random::range(0.3f, 0.8f);
        shellTexture = static_cast<uint8_t>(Random::rangeInt(0, 3));
        if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            shellCoverage = Random::range(0.8f, 1.0f);  // Full armor
        }
    }

    // Frills (rare display features)
    hasNeckFrill = false;
    frillSize = 0.0f;
    hasBodyFrills = false;
    displayFeatherSize = 0.0f;
    if (Random::chance(0.06f)) {
        hasNeckFrill = true;
        frillSize = Random::range(0.3f, 0.8f);
        if (Random::chance(EXTREME_TRAIT_CHANCE)) {
            frillSize = Random::range(1.0f, 1.5f);  // Dramatic frill
        }
    }
    if (Random::chance(0.04f)) {
        hasBodyFrills = true;
    }
    if (Random::chance(0.1f)) {
        displayFeatherSize = Random::range(0.2f, 0.6f);
    }

    // Eye diversity
    eyeArrangement = 0;  // Default paired
    eyeProtrusion = Random::range(0.0f, 0.2f);
    hasEyeSpots = false;
    eyeSpotCount = 0;
    if (Random::chance(0.15f)) {
        eyeArrangement = static_cast<uint8_t>(Random::rangeInt(1, 4));
    }
    if (Random::chance(0.08f)) {
        hasEyeSpots = true;
        eyeSpotCount = static_cast<uint8_t>(Random::rangeInt(1, 4)) * 2;  // Symmetrical
    }

    // ==========================================
    // BIOCHEMISTRY TRAIT RANDOMIZATION (Planet Chemistry)
    // ==========================================

    // Biopigment family (most start with chlorophyll-like or carotenoid)
    biopigmentFamily = static_cast<uint8_t>(Random::rangeInt(0, 2));  // 0-2 are most common
    if (Random::chance(0.15f)) {
        biopigmentFamily = static_cast<uint8_t>(Random::rangeInt(0, 5));  // Rare exotic pigments
    }

    // Membrane fluidity (Earth-like default: mid-range)
    membraneFluidity = Random::range(0.4f, 0.7f);
    if (Random::chance(EXTREME_TRAIT_CHANCE)) {
        // Extreme adaptation for hot or cold environments
        membraneFluidity = Random::value() < 0.5f ? Random::range(0.1f, 0.3f) : Random::range(0.8f, 0.95f);
    }

    // Oxygen tolerance (most start aerobic)
    oxygenTolerance = Random::range(0.5f, 0.85f);
    if (Random::chance(0.1f)) {
        oxygenTolerance = Random::range(0.0f, 0.3f);  // Rare anaerobic creatures
    }

    // Mineralization bias (soft-bodied to moderate by default)
    mineralizationBias = Random::range(0.2f, 0.6f);
    if (Random::chance(EXTREME_TRAIT_CHANCE)) {
        mineralizationBias = Random::range(0.8f, 1.0f);  // Heavily armored
    }

    // Solvent affinity (0.5 = water-adapted, Earth-like default)
    solventAffinity = Random::range(0.4f, 0.6f);
    if (Random::chance(0.05f)) {
        // Rare non-water solvent adaptation
        solventAffinity = Random::value() < 0.5f ? Random::range(0.0f, 0.2f) : Random::range(0.8f, 1.0f);
    }

    // Temperature tolerance (moderate generalist range)
    temperatureTolerance = Random::range(15.0f, 35.0f);
    if (Random::chance(EXTREME_TRAIT_CHANCE)) {
        temperatureTolerance = Random::value() < 0.5f ? Random::range(5.0f, 12.0f) : Random::range(40.0f, 50.0f);
    }

    // Radiation resistance (low to moderate by default)
    radiationResistance = Random::range(0.1f, 0.4f);
    if (Random::chance(0.1f)) {
        radiationResistance = Random::range(0.6f, 0.95f);  // Radiation-tolerant
    }

    // pH preference (neutral to slightly alkaline, Earth-like)
    pHPreference = Random::range(0.4f, 0.6f);
    if (Random::chance(0.1f)) {
        // Acidophile or alkaliphile
        pHPreference = Random::value() < 0.5f ? Random::range(0.0f, 0.25f) : Random::range(0.75f, 1.0f);
    }

    // Metabolic pathway (most start aerobic)
    metabolicPathway = 0;  // Aerobic respiration
    if (Random::chance(0.15f)) {
        metabolicPathway = static_cast<uint8_t>(Random::rangeInt(0, 3));
    }
}

void Genome::randomizeFlying() {
    // Start with base randomization
    randomize();

    // Override with flying-specific traits
    size = Random::range(0.4f, 0.8f);           // Smaller than land creatures
    speed = Random::range(15.0f, 25.0f);        // Faster in air
    visionRange = Random::range(40.0f, 60.0f);  // Better vision from above

    // Distinctive flying creature colors (blues, grays, whites)
    color = glm::vec3(
        Random::range(0.3f, 0.6f),   // Blues and grays
        Random::range(0.4f, 0.7f),
        Random::range(0.6f, 0.9f)
    );

    // Basic flying traits
    wingSpan = Random::range(0.8f, 1.5f);
    flapFrequency = Random::range(3.0f, 6.0f);
    glideRatio = Random::range(0.4f, 0.7f);
    preferredAltitude = Random::range(20.0f, 35.0f);

    // Wing morphology
    wingChord = Random::range(0.15f, 0.35f);
    wingAspectRatio = Random::range(5.0f, 12.0f);
    wingLoading = Random::range(20.0f, 60.0f);
    wingCamber = Random::range(0.03f, 0.1f);
    wingTaper = Random::range(0.4f, 0.8f);
    wingTwist = Random::range(-2.0f, 2.0f);
    dihedralAngle = Random::range(2.0f, 10.0f);
    sweepAngle = Random::range(0.0f, 15.0f);
    wingType = 0;  // Feathered by default
    featherType = 1;  // Primary feathers

    // Tail configuration
    tailLength = Random::range(0.4f, 0.9f);
    tailSpan = Random::range(0.3f, 0.6f);
    tailType = static_cast<uint8_t>(Random::rangeInt(0, 4));

    // Flight musculature
    breastMuscleRatio = Random::range(0.15f, 0.3f);
    supracoracoideus = Random::range(0.03f, 0.07f);
    muscleOxygenCapacity = Random::range(0.7f, 1.2f);
    anaerobicCapacity = Random::range(0.4f, 0.7f);

    // Body aerodynamics
    bodyDragCoeff = Random::range(0.02f, 0.06f);
    fuselageLength = Random::range(0.9f, 1.4f);
    bodyDensity = Random::range(0.85f, 1.0f);  // Lighter due to hollow bones

    // Specialized flight capabilities
    hoveringAbility = Random::range(0.0f, 0.3f);
    divingSpeed = Random::range(0.2f, 0.5f);
    maneuverability = Random::range(0.4f, 0.7f);
    thermalSensingAbility = Random::range(0.2f, 0.6f);
    windResistance = Random::range(0.3f, 0.6f);

    // Flight behavior
    flockingStrength = Random::range(0.3f, 0.7f);
    territorialRadius = Random::range(10.0f, 40.0f);
    migrationInstinct = Random::range(0.1f, 0.5f);
    nocturnalFlight = Random::range(0.0f, 0.2f);

    // Energy management
    flightMetabolism = Random::range(1.0f, 1.5f);
    fatStorageCapacity = Random::range(0.15f, 0.3f);
    restingRecoveryRate = Random::range(0.2f, 0.35f);

    // Enhanced motion detection for aerial hunting
    motionDetection = Random::range(0.6f, 0.9f);
    visionAcuity = Random::range(0.5f, 0.85f);
}

void Genome::randomizeBird() {
    // Start with base flying traits
    randomizeFlying();

    // Bird-specific adjustments
    size = Random::range(0.3f, 0.9f);           // Variable sizes (sparrow to crow)
    speed = Random::range(12.0f, 22.0f);        // Moderate flight speed

    // Bird colors (varied, often with browns and earth tones)
    float colorChoice = Random::value();
    if (colorChoice < 0.3f) {
        // Brown/tan (sparrows, wrens)
        color = glm::vec3(
            Random::range(0.4f, 0.6f),
            Random::range(0.3f, 0.5f),
            Random::range(0.2f, 0.4f)
        );
    } else if (colorChoice < 0.6f) {
        // Blue/gray (jays, mockingbirds)
        color = glm::vec3(
            Random::range(0.3f, 0.5f),
            Random::range(0.4f, 0.6f),
            Random::range(0.6f, 0.8f)
        );
    } else {
        // Black/white (crows, magpies)
        float intensity = Random::range(0.1f, 0.9f);
        color = glm::vec3(intensity, intensity, intensity);
    }

    // Bird flight characteristics
    wingSpan = Random::range(1.0f, 1.8f);       // Birds have proportionally larger wings
    flapFrequency = Random::range(3.0f, 8.0f);  // Variable flap rate
    glideRatio = Random::range(0.5f, 0.8f);     // Birds are excellent gliders
    preferredAltitude = Random::range(25.0f, 50.0f);  // Birds fly higher

    // Bird wing morphology
    wingChord = Random::range(0.2f, 0.4f);
    wingAspectRatio = Random::range(6.0f, 12.0f);
    wingLoading = Random::range(25.0f, 50.0f);
    wingCamber = Random::range(0.05f, 0.1f);
    wingTaper = Random::range(0.5f, 0.8f);
    wingTwist = Random::range(-3.0f, 0.0f);  // Washout for stability
    dihedralAngle = Random::range(3.0f, 8.0f);
    sweepAngle = Random::range(0.0f, 10.0f);
    wingType = 0;  // Feathered
    featherType = static_cast<uint8_t>(Random::rangeInt(0, 3));

    // Bird tail (forked or fan typically)
    tailLength = Random::range(0.5f, 1.0f);
    tailSpan = Random::range(0.4f, 0.7f);
    tailType = Random::chance(0.5f) ? 0 : 3;  // Forked or fan

    // Bird musculature
    breastMuscleRatio = Random::range(0.2f, 0.35f);
    supracoracoideus = Random::range(0.04f, 0.08f);
    muscleOxygenCapacity = Random::range(0.9f, 1.3f);
    anaerobicCapacity = Random::range(0.5f, 0.8f);

    // Body (hollow bones = lower density)
    bodyDragCoeff = Random::range(0.02f, 0.05f);
    fuselageLength = Random::range(1.0f, 1.5f);
    bodyDensity = Random::range(0.8f, 0.95f);

    // Bird flight capabilities
    hoveringAbility = Random::range(0.0f, 0.2f);
    divingSpeed = Random::range(0.3f, 0.6f);
    maneuverability = Random::range(0.5f, 0.8f);
    thermalSensingAbility = Random::range(0.3f, 0.7f);
    windResistance = Random::range(0.4f, 0.7f);

    // Bird behaviors
    flockingStrength = Random::range(0.4f, 0.9f);  // Many birds flock
    territorialRadius = Random::range(15.0f, 50.0f);
    migrationInstinct = Random::range(0.2f, 0.8f);
    nocturnalFlight = Random::range(0.0f, 0.1f);

    // Energy
    flightMetabolism = Random::range(1.1f, 1.6f);
    fatStorageCapacity = Random::range(0.2f, 0.4f);  // For migration
    restingRecoveryRate = Random::range(0.25f, 0.4f);

    // Enhanced vision (birds have excellent eyesight)
    visionRange = Random::range(50.0f, 80.0f);
    visionAcuity = Random::range(0.7f, 0.95f);
    colorPerception = Random::range(0.7f, 0.9f); // Birds see more colors than humans
    motionDetection = Random::range(0.6f, 0.85f);
}

void Genome::randomizeInsect() {
    // Start with base randomization
    randomize();

    // Insects are much smaller
    size = Random::range(0.1f, 0.4f);
    speed = Random::range(8.0f, 18.0f);         // Can be quite fast relative to size

    // Insect colors (often warning colors or camouflage)
    float colorChoice = Random::value();
    if (colorChoice < 0.3f) {
        // Yellow/black (bees, wasps)
        color = glm::vec3(
            Random::range(0.8f, 1.0f),
            Random::range(0.7f, 0.9f),
            Random::range(0.0f, 0.2f)
        );
    } else if (colorChoice < 0.5f) {
        // Iridescent blues/greens (dragonflies)
        color = glm::vec3(
            Random::range(0.0f, 0.3f),
            Random::range(0.5f, 0.9f),
            Random::range(0.6f, 1.0f)
        );
    } else if (colorChoice < 0.7f) {
        // Red/orange (ladybugs, butterflies)
        color = glm::vec3(
            Random::range(0.8f, 1.0f),
            Random::range(0.2f, 0.5f),
            Random::range(0.0f, 0.2f)
        );
    } else {
        // Earth tones (moths, beetles)
        float base = Random::range(0.3f, 0.5f);
        color = glm::vec3(base + 0.1f, base, base - 0.1f);
    }

    // Insect flight characteristics
    wingSpan = Random::range(0.3f, 0.8f);       // Smaller wings
    flapFrequency = Random::range(20.0f, 200.0f); // MUCH faster wing beats!
    glideRatio = Random::range(0.1f, 0.4f);     // Insects rarely glide
    preferredAltitude = Random::range(5.0f, 20.0f);  // Fly lower

    // Insect wing morphology (very different from birds)
    wingChord = Random::range(0.2f, 0.45f);
    wingAspectRatio = Random::range(3.0f, 8.0f);  // Lower aspect ratio
    wingLoading = Random::range(5.0f, 25.0f);     // Very low wing loading
    wingCamber = Random::range(0.01f, 0.05f);     // Less camber
    wingTaper = Random::range(0.6f, 1.0f);
    wingTwist = Random::range(0.0f, 3.0f);
    dihedralAngle = 0.0f;  // Insects don't use dihedral
    sweepAngle = Random::range(-5.0f, 15.0f);
    wingType = static_cast<uint8_t>(Random::rangeInt(2, 7));  // Insect wing types (2-7)
    featherType = 0;  // No feathers

    // Insect tail (minimal)
    tailLength = Random::range(0.1f, 0.3f);
    tailSpan = Random::range(0.0f, 0.1f);
    tailType = 2;  // Pointed/none

    // Insect flight muscles (different structure)
    breastMuscleRatio = Random::range(0.25f, 0.4f);  // High muscle ratio
    supracoracoideus = 0.0f;  // Different muscle arrangement
    muscleOxygenCapacity = Random::range(0.6f, 1.0f);
    anaerobicCapacity = Random::range(0.6f, 0.9f);

    // Insect body
    bodyDragCoeff = Random::range(0.03f, 0.08f);
    fuselageLength = Random::range(1.2f, 2.0f);  // Elongated bodies
    bodyDensity = Random::range(0.9f, 1.1f);

    // Insect flight capabilities
    hoveringAbility = Random::range(0.3f, 0.9f);  // Many insects hover well
    divingSpeed = Random::range(0.1f, 0.3f);
    maneuverability = Random::range(0.7f, 1.0f);  // Extremely maneuverable
    thermalSensingAbility = Random::range(0.0f, 0.2f);  // Don't use thermals much
    windResistance = Random::range(0.1f, 0.4f);  // Easily blown by wind

    // Insect behaviors
    flockingStrength = Random::range(0.0f, 0.8f);  // Varies by species
    territorialRadius = Random::range(2.0f, 15.0f);
    migrationInstinct = Random::range(0.0f, 0.4f);
    nocturnalFlight = Random::range(0.0f, 0.6f);  // Many are nocturnal

    // Energy
    flightMetabolism = Random::range(1.5f, 2.0f);  // High metabolism
    fatStorageCapacity = Random::range(0.1f, 0.2f);
    restingRecoveryRate = Random::range(0.3f, 0.5f);

    // Compound eye vision (different than vertebrates)
    visionRange = Random::range(15.0f, 35.0f);  // Shorter range
    visionFOV = Random::range(4.5f, 6.0f);      // Near 360-degree vision
    visionAcuity = Random::range(0.3f, 0.5f);   // Lower acuity but wide FOV
    motionDetection = Random::range(0.8f, 0.95f); // Excellent motion detection!
}

void Genome::randomizeAerialPredator() {
    // Start with bird traits
    randomizeBird();

    // Raptors are larger
    size = Random::range(0.7f, 1.3f);
    speed = Random::range(18.0f, 30.0f);        // Fast dive speed

    // Raptor colors (browns, grays, blacks)
    float colorChoice = Random::value();
    if (colorChoice < 0.5f) {
        // Brown (hawks, eagles)
        color = glm::vec3(
            Random::range(0.5f, 0.7f),
            Random::range(0.3f, 0.5f),
            Random::range(0.2f, 0.3f)
        );
    } else {
        // Gray/dark (falcons)
        float gray = Random::range(0.2f, 0.5f);
        color = glm::vec3(gray, gray, gray + 0.1f);
    }

    // Raptor flight characteristics
    wingSpan = Random::range(1.5f, 2.5f);       // Large wingspan
    flapFrequency = Random::range(2.0f, 4.0f);  // Slow, powerful flaps
    glideRatio = Random::range(0.7f, 0.95f);    // Excellent soaring
    preferredAltitude = Random::range(40.0f, 80.0f);  // Hunt from high altitude

    // Raptor wing morphology (optimized for soaring and diving)
    wingChord = Random::range(0.25f, 0.4f);
    wingAspectRatio = Random::range(8.0f, 15.0f);  // High for soaring
    wingLoading = Random::range(40.0f, 80.0f);     // Higher for speed
    wingCamber = Random::range(0.06f, 0.12f);
    wingTaper = Random::range(0.4f, 0.7f);
    wingTwist = Random::range(-4.0f, -1.0f);  // Strong washout
    dihedralAngle = Random::range(5.0f, 12.0f);
    sweepAngle = Random::range(10.0f, 30.0f);  // Swept for speed
    wingType = 0;  // Feathered
    featherType = 1;  // Primary flight feathers

    // Raptor tail (for maneuvering during hunting)
    tailLength = Random::range(0.6f, 1.1f);
    tailSpan = Random::range(0.5f, 0.8f);
    tailType = Random::chance(0.5f) ? 1 : 3;  // Rounded or fan for maneuverability

    // Powerful musculature
    breastMuscleRatio = Random::range(0.28f, 0.4f);
    supracoracoideus = Random::range(0.05f, 0.1f);
    muscleOxygenCapacity = Random::range(1.1f, 1.5f);
    anaerobicCapacity = Random::range(0.7f, 1.0f);  // High for burst attacks

    // Streamlined body
    bodyDragCoeff = Random::range(0.015f, 0.035f);
    fuselageLength = Random::range(1.1f, 1.6f);
    bodyDensity = Random::range(0.85f, 0.98f);

    // Specialized hunting flight
    hoveringAbility = Random::range(0.0f, 0.15f);
    divingSpeed = Random::range(0.8f, 1.0f);   // EXCEPTIONAL diving
    maneuverability = Random::range(0.6f, 0.9f);
    thermalSensingAbility = Random::range(0.7f, 0.95f);  // Masters of thermals
    windResistance = Random::range(0.6f, 0.9f);

    // Predator behaviors
    flockingStrength = Random::range(0.0f, 0.2f);  // Mostly solitary
    territorialRadius = Random::range(50.0f, 100.0f);  // Large territory
    migrationInstinct = Random::range(0.3f, 0.7f);
    nocturnalFlight = Random::range(0.0f, 0.15f);

    // High endurance for soaring
    flightMetabolism = Random::range(0.9f, 1.3f);  // Efficient when soaring
    fatStorageCapacity = Random::range(0.25f, 0.45f);
    restingRecoveryRate = Random::range(0.3f, 0.45f);

    // Exceptional vision (raptors have the best vision)
    visionRange = Random::range(80.0f, 120.0f);
    visionAcuity = Random::range(0.9f, 0.99f);  // Near-perfect acuity
    colorPerception = Random::range(0.8f, 0.95f);
    motionDetection = Random::range(0.85f, 0.95f);

    // Aggressive traits
    displayIntensity = Random::range(0.1f, 0.3f);  // Less social display
    alarmCallVolume = Random::range(0.5f, 0.8f);   // Loud calls
}

void Genome::randomizeHummingbird() {
    // Start with bird traits
    randomizeBird();

    // Hummingbirds are tiny
    size = Random::range(0.05f, 0.15f);
    speed = Random::range(20.0f, 35.0f);  // Very fast flight

    // Iridescent colors
    float colorChoice = Random::value();
    if (colorChoice < 0.4f) {
        // Green iridescent
        color = glm::vec3(
            Random::range(0.1f, 0.3f),
            Random::range(0.6f, 0.9f),
            Random::range(0.2f, 0.5f)
        );
    } else if (colorChoice < 0.7f) {
        // Ruby throated
        color = glm::vec3(
            Random::range(0.8f, 1.0f),
            Random::range(0.1f, 0.3f),
            Random::range(0.2f, 0.4f)
        );
    } else {
        // Blue/purple
        color = glm::vec3(
            Random::range(0.3f, 0.5f),
            Random::range(0.2f, 0.4f),
            Random::range(0.7f, 0.95f)
        );
    }

    // Hummingbird flight characteristics
    wingSpan = Random::range(0.4f, 0.7f);
    flapFrequency = Random::range(50.0f, 80.0f);  // Extremely fast!
    glideRatio = Random::range(0.0f, 0.1f);       // Almost never glide
    preferredAltitude = Random::range(5.0f, 15.0f);

    // Special wing morphology for hovering
    wingChord = Random::range(0.3f, 0.5f);  // Wide chord
    wingAspectRatio = Random::range(4.0f, 6.0f);  // Low aspect ratio
    wingLoading = Random::range(10.0f, 20.0f);    // Very low
    wingCamber = Random::range(0.08f, 0.12f);
    wingTaper = Random::range(0.7f, 0.95f);
    wingTwist = Random::range(0.0f, 2.0f);
    dihedralAngle = 0.0f;
    sweepAngle = Random::range(-5.0f, 5.0f);

    // Tail for stability during hover
    tailLength = Random::range(0.3f, 0.5f);
    tailSpan = Random::range(0.25f, 0.4f);
    tailType = 0;  // Forked

    // Exceptional hovering muscles
    breastMuscleRatio = Random::range(0.35f, 0.4f);  // Largest relative to body
    supracoracoideus = Random::range(0.08f, 0.1f);   // Powerful upstroke!
    muscleOxygenCapacity = Random::range(1.3f, 1.5f);
    anaerobicCapacity = Random::range(0.8f, 1.0f);

    // Specialized capabilities
    hoveringAbility = Random::range(0.95f, 1.0f);  // MASTERS of hovering
    divingSpeed = Random::range(0.2f, 0.4f);
    maneuverability = Random::range(0.9f, 1.0f);   // Incredible agility
    thermalSensingAbility = Random::range(0.0f, 0.1f);
    windResistance = Random::range(0.3f, 0.5f);

    // Behaviors
    flockingStrength = Random::range(0.0f, 0.1f);  // Solitary
    territorialRadius = Random::range(5.0f, 20.0f);
    migrationInstinct = Random::range(0.5f, 0.9f);  // Many migrate
    nocturnalFlight = 0.0f;

    // Extremely high metabolism
    flightMetabolism = Random::range(1.8f, 2.0f);
    fatStorageCapacity = Random::range(0.35f, 0.5f);  // Need fuel reserves
    restingRecoveryRate = Random::range(0.4f, 0.5f);
}

void Genome::randomizeOwl() {
    // Start with bird traits
    randomizeBird();

    // Owls are medium to large
    size = Random::range(0.4f, 1.0f);
    speed = Random::range(10.0f, 18.0f);  // Quiet flight, not super fast

    // Owl colors (camouflage browns, grays)
    float colorChoice = Random::value();
    if (colorChoice < 0.5f) {
        // Brown mottled
        color = glm::vec3(
            Random::range(0.45f, 0.6f),
            Random::range(0.35f, 0.5f),
            Random::range(0.25f, 0.4f)
        );
    } else if (colorChoice < 0.8f) {
        // Gray
        float gray = Random::range(0.35f, 0.55f);
        color = glm::vec3(gray, gray, gray + 0.05f);
    } else {
        // White (snowy owl)
        float white = Random::range(0.85f, 0.98f);
        color = glm::vec3(white, white, white);
    }

    // Owl flight characteristics (silent flight)
    wingSpan = Random::range(1.3f, 2.2f);
    flapFrequency = Random::range(2.0f, 4.0f);
    glideRatio = Random::range(0.6f, 0.85f);
    preferredAltitude = Random::range(10.0f, 30.0f);

    // Wing morphology for silent flight
    wingChord = Random::range(0.3f, 0.45f);  // Broad wings
    wingAspectRatio = Random::range(5.0f, 8.0f);  // Lower for maneuverability
    wingLoading = Random::range(20.0f, 40.0f);
    wingCamber = Random::range(0.07f, 0.12f);
    wingTaper = Random::range(0.5f, 0.75f);
    wingTwist = Random::range(-2.0f, 0.0f);
    dihedralAngle = Random::range(3.0f, 7.0f);
    sweepAngle = Random::range(5.0f, 15.0f);
    featherType = 2;  // Special silencing feathers

    // Round tail for quiet maneuvering
    tailLength = Random::range(0.5f, 0.8f);
    tailSpan = Random::range(0.45f, 0.65f);
    tailType = 1;  // Rounded

    // Musculature
    breastMuscleRatio = Random::range(0.22f, 0.32f);
    supracoracoideus = Random::range(0.04f, 0.07f);
    muscleOxygenCapacity = Random::range(0.9f, 1.2f);
    anaerobicCapacity = Random::range(0.6f, 0.85f);

    // Streamlined for silent approach
    bodyDragCoeff = Random::range(0.02f, 0.04f);
    fuselageLength = Random::range(0.9f, 1.2f);
    bodyDensity = Random::range(0.88f, 1.0f);

    // Specialized for nocturnal hunting
    hoveringAbility = Random::range(0.1f, 0.3f);
    divingSpeed = Random::range(0.5f, 0.75f);
    maneuverability = Random::range(0.7f, 0.9f);
    thermalSensingAbility = Random::range(0.1f, 0.3f);
    windResistance = Random::range(0.4f, 0.6f);

    // Behaviors
    flockingStrength = Random::range(0.0f, 0.1f);  // Solitary
    territorialRadius = Random::range(30.0f, 80.0f);
    migrationInstinct = Random::range(0.0f, 0.3f);  // Most don't migrate
    nocturnalFlight = Random::range(0.9f, 1.0f);   // NOCTURNAL specialists

    // Efficient metabolism for patient hunting
    flightMetabolism = Random::range(0.8f, 1.1f);
    fatStorageCapacity = Random::range(0.2f, 0.35f);
    restingRecoveryRate = Random::range(0.25f, 0.4f);

    // Exceptional night senses
    visionRange = Random::range(60.0f, 100.0f);
    visionAcuity = Random::range(0.85f, 0.98f);
    colorPerception = Random::range(0.2f, 0.4f);  // Less color, more light
    motionDetection = Random::range(0.9f, 0.98f);
    hearingRange = Random::range(70.0f, 100.0f);  // EXCEPTIONAL hearing
    hearingDirectionality = Random::range(0.9f, 0.98f);
}

void Genome::randomizeSeabird() {
    // Start with bird traits
    randomizeBird();

    // Seabirds vary in size (petrel to albatross)
    size = Random::range(0.5f, 1.5f);
    speed = Random::range(15.0f, 25.0f);

    // Seabird colors (whites, grays, black)
    float colorChoice = Random::value();
    if (colorChoice < 0.4f) {
        // White with gray
        color = glm::vec3(
            Random::range(0.85f, 0.98f),
            Random::range(0.85f, 0.98f),
            Random::range(0.88f, 1.0f)
        );
    } else if (colorChoice < 0.7f) {
        // Gray
        float gray = Random::range(0.45f, 0.7f);
        color = glm::vec3(gray, gray, gray + 0.05f);
    } else {
        // Dark brown/black
        float dark = Random::range(0.15f, 0.35f);
        color = glm::vec3(dark, dark, dark);
    }

    // Seabird flight (optimized for dynamic soaring)
    wingSpan = Random::range(1.5f, 3.0f);  // Large wingspan
    flapFrequency = Random::range(1.5f, 3.5f);
    glideRatio = Random::range(0.85f, 0.95f);  // EXCEPTIONAL gliders
    preferredAltitude = Random::range(5.0f, 30.0f);

    // Wing morphology for oceanic soaring
    wingChord = Random::range(0.15f, 0.25f);  // Narrow wings
    wingAspectRatio = Random::range(12.0f, 20.0f);  // VERY high aspect ratio
    wingLoading = Random::range(45.0f, 80.0f);
    wingCamber = Random::range(0.04f, 0.08f);
    wingTaper = Random::range(0.3f, 0.5f);
    wingTwist = Random::range(-4.0f, -1.0f);
    dihedralAngle = Random::range(3.0f, 8.0f);
    sweepAngle = Random::range(15.0f, 35.0f);

    // Short tail
    tailLength = Random::range(0.3f, 0.5f);
    tailSpan = Random::range(0.3f, 0.5f);
    tailType = Random::chance(0.5f) ? 0 : 2;  // Forked or pointed

    // Musculature for endurance
    breastMuscleRatio = Random::range(0.18f, 0.28f);
    supracoracoideus = Random::range(0.03f, 0.06f);
    muscleOxygenCapacity = Random::range(1.1f, 1.4f);
    anaerobicCapacity = Random::range(0.4f, 0.65f);

    // Streamlined
    bodyDragCoeff = Random::range(0.015f, 0.03f);
    fuselageLength = Random::range(1.2f, 1.8f);
    bodyDensity = Random::range(0.9f, 1.05f);

    // Dynamic soaring specialists
    hoveringAbility = Random::range(0.0f, 0.05f);
    divingSpeed = Random::range(0.4f, 0.7f);  // Dive for fish
    maneuverability = Random::range(0.4f, 0.6f);
    thermalSensingAbility = Random::range(0.2f, 0.5f);
    windResistance = Random::range(0.8f, 0.98f);  // EXCEPTIONAL wind handling

    // Behaviors
    flockingStrength = Random::range(0.3f, 0.7f);
    territorialRadius = Random::range(5.0f, 20.0f);
    migrationInstinct = Random::range(0.6f, 0.95f);  // Many migrate vast distances
    nocturnalFlight = Random::range(0.1f, 0.3f);

    // Long endurance
    flightMetabolism = Random::range(0.7f, 1.0f);  // Very efficient
    fatStorageCapacity = Random::range(0.35f, 0.5f);  // Long flights
    restingRecoveryRate = Random::range(0.2f, 0.35f);
}

void Genome::randomizeBat() {
    // Start with base flying traits
    randomizeFlying();

    // Bats vary in size
    size = Random::range(0.1f, 0.5f);
    speed = Random::range(10.0f, 22.0f);

    // Bat colors (dark browns, blacks, some with patterns)
    float colorChoice = Random::value();
    if (colorChoice < 0.6f) {
        // Dark brown
        color = glm::vec3(
            Random::range(0.25f, 0.4f),
            Random::range(0.18f, 0.3f),
            Random::range(0.12f, 0.22f)
        );
    } else if (colorChoice < 0.85f) {
        // Black
        float dark = Random::range(0.1f, 0.2f);
        color = glm::vec3(dark, dark, dark);
    } else {
        // Reddish (fruit bats)
        color = glm::vec3(
            Random::range(0.5f, 0.7f),
            Random::range(0.25f, 0.4f),
            Random::range(0.15f, 0.25f)
        );
    }

    // Bat flight characteristics
    wingSpan = Random::range(0.8f, 1.8f);
    flapFrequency = Random::range(8.0f, 18.0f);
    glideRatio = Random::range(0.3f, 0.6f);
    preferredAltitude = Random::range(5.0f, 25.0f);

    // Membrane wing morphology
    wingChord = Random::range(0.35f, 0.5f);  // Wide membrane
    wingAspectRatio = Random::range(4.0f, 8.0f);
    wingLoading = Random::range(15.0f, 35.0f);
    wingCamber = Random::range(0.08f, 0.15f);  // Flexible membrane
    wingTaper = Random::range(0.6f, 0.9f);
    wingTwist = Random::range(-1.0f, 2.0f);
    dihedralAngle = Random::range(0.0f, 5.0f);
    sweepAngle = Random::range(5.0f, 20.0f);
    wingType = 1;  // Membrane wing
    featherType = 0;  // No feathers

    // Tail (varies by species)
    tailLength = Random::range(0.2f, 0.6f);
    tailSpan = Random::range(0.1f, 0.3f);
    tailType = 2;  // Pointed/membrane

    // Flight muscles
    breastMuscleRatio = Random::range(0.2f, 0.32f);
    supracoracoideus = Random::range(0.04f, 0.08f);
    muscleOxygenCapacity = Random::range(0.9f, 1.3f);
    anaerobicCapacity = Random::range(0.6f, 0.85f);

    // Body
    bodyDragCoeff = Random::range(0.03f, 0.06f);
    fuselageLength = Random::range(0.9f, 1.3f);
    bodyDensity = Random::range(0.9f, 1.05f);

    // Specialized capabilities
    hoveringAbility = Random::range(0.3f, 0.6f);
    divingSpeed = Random::range(0.2f, 0.4f);
    maneuverability = Random::range(0.75f, 0.95f);  // Very maneuverable
    thermalSensingAbility = Random::range(0.0f, 0.2f);
    windResistance = Random::range(0.3f, 0.5f);

    // Bat behaviors
    flockingStrength = Random::range(0.4f, 0.9f);  // Many roost in colonies
    territorialRadius = Random::range(5.0f, 25.0f);
    migrationInstinct = Random::range(0.1f, 0.5f);
    nocturnalFlight = Random::range(0.9f, 1.0f);  // NOCTURNAL

    // Energy
    flightMetabolism = Random::range(1.2f, 1.6f);
    fatStorageCapacity = Random::range(0.15f, 0.3f);
    restingRecoveryRate = Random::range(0.25f, 0.4f);

    // Bat senses
    visionRange = Random::range(20.0f, 40.0f);  // Limited vision
    visionAcuity = Random::range(0.3f, 0.6f);
    colorPerception = Random::range(0.1f, 0.3f);
    motionDetection = Random::range(0.6f, 0.8f);

    // ECHOLOCATION
    echolocationAbility = Random::range(0.8f, 1.0f);
    hearingRange = Random::range(60.0f, 100.0f);
    hearingDirectionality = Random::range(0.85f, 0.98f);
}

void Genome::randomizeDragonfly() {
    // Start with insect traits
    randomizeInsect();

    // Dragonflies are larger insects
    size = Random::range(0.15f, 0.4f);
    speed = Random::range(18.0f, 35.0f);  // FAST flyers

    // Iridescent colors
    float colorChoice = Random::value();
    if (colorChoice < 0.4f) {
        // Blue iridescent
        color = glm::vec3(
            Random::range(0.1f, 0.3f),
            Random::range(0.4f, 0.7f),
            Random::range(0.7f, 1.0f)
        );
    } else if (colorChoice < 0.7f) {
        // Green iridescent
        color = glm::vec3(
            Random::range(0.1f, 0.3f),
            Random::range(0.6f, 0.9f),
            Random::range(0.3f, 0.5f)
        );
    } else {
        // Red/orange
        color = glm::vec3(
            Random::range(0.8f, 1.0f),
            Random::range(0.3f, 0.5f),
            Random::range(0.1f, 0.3f)
        );
    }

    // Dragonfly flight (4 independent wings)
    wingSpan = Random::range(0.6f, 1.2f);
    flapFrequency = Random::range(25.0f, 40.0f);
    glideRatio = Random::range(0.4f, 0.7f);  // Can glide!
    preferredAltitude = Random::range(3.0f, 15.0f);

    // Odonata wing morphology
    wingChord = Random::range(0.15f, 0.25f);
    wingAspectRatio = Random::range(8.0f, 14.0f);  // High aspect ratio
    wingLoading = Random::range(5.0f, 15.0f);
    wingCamber = Random::range(0.02f, 0.05f);
    wingTaper = Random::range(0.4f, 0.6f);
    wingTwist = 0.0f;
    dihedralAngle = 0.0f;
    sweepAngle = Random::range(-5.0f, 5.0f);
    wingType = 3;  // INSECT_DOUBLE (fore and hind wings)

    // Long tail (abdomen)
    tailLength = Random::range(1.2f, 2.0f);
    tailSpan = 0.0f;
    tailType = 2;  // Pointed

    // Powerful flight muscles
    breastMuscleRatio = Random::range(0.3f, 0.4f);
    muscleOxygenCapacity = Random::range(0.8f, 1.1f);
    anaerobicCapacity = Random::range(0.7f, 0.95f);

    // Exceptional aerial hunters
    hoveringAbility = Random::range(0.7f, 0.95f);
    divingSpeed = Random::range(0.4f, 0.6f);
    maneuverability = Random::range(0.9f, 1.0f);  // EXCEPTIONAL
    thermalSensingAbility = Random::range(0.0f, 0.1f);
    windResistance = Random::range(0.3f, 0.5f);

    // Predator behaviors
    flockingStrength = Random::range(0.0f, 0.2f);  // Usually solitary
    territorialRadius = Random::range(10.0f, 40.0f);
    migrationInstinct = Random::range(0.2f, 0.6f);  // Some migrate
    nocturnalFlight = Random::range(0.0f, 0.1f);

    // High metabolism
    flightMetabolism = Random::range(1.6f, 2.0f);

    // Compound eyes with exceptional vision
    visionRange = Random::range(30.0f, 50.0f);
    visionFOV = Random::range(5.5f, 6.0f);  // Nearly 360 degrees
    visionAcuity = Random::range(0.5f, 0.7f);
    motionDetection = Random::range(0.95f, 1.0f);  // Exceptional
}

void Genome::randomizeButterfly() {
    // Start with insect traits
    randomizeInsect();

    // Butterflies are medium insects
    size = Random::range(0.1f, 0.25f);
    speed = Random::range(5.0f, 12.0f);  // Relatively slow

    // Colorful patterns
    float colorChoice = Random::value();
    if (colorChoice < 0.25f) {
        // Orange monarch-style
        color = glm::vec3(
            Random::range(0.9f, 1.0f),
            Random::range(0.5f, 0.7f),
            Random::range(0.0f, 0.2f)
        );
    } else if (colorChoice < 0.5f) {
        // Blue morpho-style
        color = glm::vec3(
            Random::range(0.0f, 0.2f),
            Random::range(0.3f, 0.5f),
            Random::range(0.8f, 1.0f)
        );
    } else if (colorChoice < 0.75f) {
        // Yellow swallowtail-style
        color = glm::vec3(
            Random::range(0.9f, 1.0f),
            Random::range(0.85f, 1.0f),
            Random::range(0.2f, 0.4f)
        );
    } else {
        // White/pale
        color = glm::vec3(
            Random::range(0.9f, 1.0f),
            Random::range(0.9f, 1.0f),
            Random::range(0.85f, 0.95f)
        );
    }

    // Butterfly flight (coupled fore/hind wings)
    wingSpan = Random::range(0.5f, 1.0f);
    flapFrequency = Random::range(8.0f, 15.0f);  // Slower than other insects
    glideRatio = Random::range(0.5f, 0.8f);      // Good gliders
    preferredAltitude = Random::range(2.0f, 10.0f);

    // Lepidoptera wing morphology
    wingChord = Random::range(0.4f, 0.6f);  // Broad wings
    wingAspectRatio = Random::range(2.5f, 5.0f);  // Low aspect ratio
    wingLoading = Random::range(3.0f, 10.0f);     // Very low
    wingCamber = Random::range(0.03f, 0.08f);
    wingTaper = Random::range(0.6f, 0.9f);
    wingTwist = 0.0f;
    dihedralAngle = 0.0f;
    sweepAngle = Random::range(0.0f, 15.0f);
    wingType = 5;  // INSECT_COUPLED (lepidoptera)

    // Small tail (abdomen)
    tailLength = Random::range(0.4f, 0.7f);
    tailSpan = 0.0f;
    tailType = 2;

    // Weaker flight muscles (relative to body)
    breastMuscleRatio = Random::range(0.2f, 0.3f);
    muscleOxygenCapacity = Random::range(0.6f, 0.9f);
    anaerobicCapacity = Random::range(0.4f, 0.6f);

    // Flight capabilities
    hoveringAbility = Random::range(0.2f, 0.5f);
    divingSpeed = Random::range(0.1f, 0.25f);
    maneuverability = Random::range(0.5f, 0.75f);
    thermalSensingAbility = Random::range(0.3f, 0.6f);  // Use thermals
    windResistance = Random::range(0.1f, 0.3f);  // Easily blown by wind

    // Behaviors
    flockingStrength = Random::range(0.0f, 0.3f);
    territorialRadius = Random::range(2.0f, 10.0f);
    migrationInstinct = Random::range(0.3f, 0.9f);  // Many migrate
    nocturnalFlight = Random::range(0.0f, 0.2f);

    // Moderate metabolism
    flightMetabolism = Random::range(1.2f, 1.5f);
    fatStorageCapacity = Random::range(0.2f, 0.4f);  // For migration

    // Sense flowers (smell)
    smellRange = Random::range(50.0f, 100.0f);
    smellSensitivity = Random::range(0.7f, 0.95f);
}

void Genome::randomizeBee() {
    // Start with insect traits
    randomizeInsect();

    // Bees are small to medium
    size = Random::range(0.08f, 0.2f);
    speed = Random::range(10.0f, 18.0f);

    // Bee colors (yellow/black or fuzzy brown)
    float colorChoice = Random::value();
    if (colorChoice < 0.6f) {
        // Classic yellow/black (modeled as yellow)
        color = glm::vec3(
            Random::range(0.85f, 1.0f),
            Random::range(0.7f, 0.9f),
            Random::range(0.0f, 0.15f)
        );
    } else if (colorChoice < 0.85f) {
        // Fuzzy brown (bumblebee)
        color = glm::vec3(
            Random::range(0.5f, 0.7f),
            Random::range(0.4f, 0.55f),
            Random::range(0.2f, 0.35f)
        );
    } else {
        // Dark (carpenter bee)
        float dark = Random::range(0.1f, 0.25f);
        color = glm::vec3(dark, dark, dark + 0.05f);
    }

    // Bee flight (hymenoptera - coupled wings)
    wingSpan = Random::range(0.3f, 0.6f);
    flapFrequency = Random::range(130.0f, 200.0f);  // VERY fast
    glideRatio = Random::range(0.05f, 0.2f);
    preferredAltitude = Random::range(2.0f, 8.0f);

    // Hymenoptera wing morphology
    wingChord = Random::range(0.25f, 0.4f);
    wingAspectRatio = Random::range(5.0f, 9.0f);
    wingLoading = Random::range(15.0f, 30.0f);
    wingCamber = Random::range(0.02f, 0.05f);
    wingTaper = Random::range(0.5f, 0.8f);
    wingTwist = 0.0f;
    dihedralAngle = 0.0f;
    sweepAngle = Random::range(0.0f, 10.0f);
    wingType = 6;  // INSECT_COUPLED (hymenoptera)

    // Compact body
    tailLength = Random::range(0.3f, 0.5f);
    tailSpan = 0.0f;
    tailType = 2;

    // Strong flight muscles
    breastMuscleRatio = Random::range(0.28f, 0.38f);
    muscleOxygenCapacity = Random::range(0.8f, 1.1f);
    anaerobicCapacity = Random::range(0.6f, 0.85f);

    // Flight capabilities
    hoveringAbility = Random::range(0.6f, 0.9f);  // Good hoverers
    divingSpeed = Random::range(0.1f, 0.25f);
    maneuverability = Random::range(0.7f, 0.9f);
    thermalSensingAbility = Random::range(0.0f, 0.15f);
    windResistance = Random::range(0.3f, 0.5f);

    // Social behaviors
    flockingStrength = Random::range(0.5f, 0.9f);  // Social insects
    territorialRadius = Random::range(5.0f, 30.0f);
    migrationInstinct = Random::range(0.0f, 0.2f);
    nocturnalFlight = Random::range(0.0f, 0.1f);

    // High metabolism
    flightMetabolism = Random::range(1.6f, 2.0f);
    fatStorageCapacity = Random::range(0.1f, 0.2f);
    restingRecoveryRate = Random::range(0.35f, 0.5f);

    // Excellent navigation
    memoryCapacity = Random::range(0.7f, 0.9f);  // Remember flower locations
    memoryRetention = Random::range(0.6f, 0.85f);

    // Smell for flowers
    smellRange = Random::range(40.0f, 80.0f);
    smellSensitivity = Random::range(0.75f, 0.95f);
    pheromoneProduction = Random::range(0.6f, 0.9f);  // Communication
}

void Genome::randomizeDragon() {
    // Fantasy creature - start with base flying
    randomizeFlying();

    // Dragons are LARGE
    size = Random::range(2.0f, 8.0f);
    speed = Random::range(25.0f, 45.0f);  // Fast despite size

    // Dragon colors
    float colorChoice = Random::value();
    if (colorChoice < 0.2f) {
        // Red dragon
        color = glm::vec3(
            Random::range(0.7f, 0.95f),
            Random::range(0.1f, 0.25f),
            Random::range(0.05f, 0.15f)
        );
    } else if (colorChoice < 0.4f) {
        // Green dragon
        color = glm::vec3(
            Random::range(0.15f, 0.35f),
            Random::range(0.5f, 0.75f),
            Random::range(0.15f, 0.3f)
        );
    } else if (colorChoice < 0.6f) {
        // Black dragon
        float dark = Random::range(0.08f, 0.2f);
        color = glm::vec3(dark, dark, dark + 0.02f);
    } else if (colorChoice < 0.8f) {
        // Gold dragon
        color = glm::vec3(
            Random::range(0.85f, 1.0f),
            Random::range(0.7f, 0.85f),
            Random::range(0.2f, 0.4f)
        );
    } else {
        // Blue/silver
        color = glm::vec3(
            Random::range(0.5f, 0.7f),
            Random::range(0.6f, 0.8f),
            Random::range(0.75f, 0.95f)
        );
    }

    // Dragon flight (membrane wings like bats but larger)
    wingSpan = Random::range(3.0f, 8.0f);
    flapFrequency = Random::range(1.5f, 4.0f);
    glideRatio = Random::range(0.6f, 0.85f);
    preferredAltitude = Random::range(50.0f, 150.0f);

    // Dragon wing morphology
    wingChord = Random::range(0.35f, 0.5f);
    wingAspectRatio = Random::range(6.0f, 12.0f);
    wingLoading = Random::range(60.0f, 120.0f);
    wingCamber = Random::range(0.08f, 0.15f);
    wingTaper = Random::range(0.4f, 0.7f);
    wingTwist = Random::range(-3.0f, 0.0f);
    dihedralAngle = Random::range(5.0f, 12.0f);
    sweepAngle = Random::range(20.0f, 40.0f);
    wingType = 1;  // Membrane
    featherType = 0;

    // Long tail for balance
    tailLength = Random::range(1.0f, 1.8f);
    tailSpan = Random::range(0.3f, 0.6f);
    tailType = 2;  // Pointed (often with spade)

    // Massive musculature
    breastMuscleRatio = Random::range(0.35f, 0.45f);
    supracoracoideus = Random::range(0.08f, 0.12f);
    muscleOxygenCapacity = Random::range(1.3f, 1.8f);
    anaerobicCapacity = Random::range(0.8f, 1.0f);

    // Dense, armored body
    bodyDragCoeff = Random::range(0.04f, 0.08f);
    fuselageLength = Random::range(1.5f, 2.5f);
    bodyDensity = Random::range(1.05f, 1.25f);  // Heavy scales

    // Dragon flight capabilities
    hoveringAbility = Random::range(0.2f, 0.5f);
    divingSpeed = Random::range(0.7f, 0.95f);
    maneuverability = Random::range(0.5f, 0.75f);
    thermalSensingAbility = Random::range(0.6f, 0.9f);
    windResistance = Random::range(0.7f, 0.95f);

    // Dragon behaviors
    flockingStrength = Random::range(0.0f, 0.15f);  // Solitary
    territorialRadius = Random::range(100.0f, 500.0f);  // LARGE territory
    migrationInstinct = Random::range(0.1f, 0.4f);
    nocturnalFlight = Random::range(0.2f, 0.5f);

    // Dragon metabolism
    flightMetabolism = Random::range(1.0f, 1.4f);  // Efficient despite size
    fatStorageCapacity = Random::range(0.3f, 0.5f);
    restingRecoveryRate = Random::range(0.15f, 0.3f);

    // Exceptional senses
    visionRange = Random::range(100.0f, 200.0f);
    visionAcuity = Random::range(0.9f, 0.99f);
    colorPerception = Random::range(0.7f, 0.9f);
    motionDetection = Random::range(0.85f, 0.95f);
    smellRange = Random::range(80.0f, 150.0f);
    smellSensitivity = Random::range(0.75f, 0.95f);
}

void Genome::randomizeAquatic() {
    // Start with base randomization
    randomize();

    // Aquatic creatures - varied sizes
    size = Random::range(0.4f, 1.2f);
    speed = Random::range(8.0f, 18.0f);  // Swimming speed

    // Fish colors (silvers, blues, greens, some bright)
    float colorChoice = Random::value();
    if (colorChoice < 0.3f) {
        // Silver/blue (typical schooling fish)
        color = glm::vec3(
            Random::range(0.5f, 0.7f),
            Random::range(0.6f, 0.8f),
            Random::range(0.7f, 0.9f)
        );
    } else if (colorChoice < 0.5f) {
        // Green/brown (bottom feeders)
        color = glm::vec3(
            Random::range(0.3f, 0.5f),
            Random::range(0.4f, 0.6f),
            Random::range(0.2f, 0.4f)
        );
    } else if (colorChoice < 0.7f) {
        // Yellow/orange (tropical fish)
        color = glm::vec3(
            Random::range(0.8f, 1.0f),
            Random::range(0.5f, 0.8f),
            Random::range(0.0f, 0.3f)
        );
    } else {
        // Striped/patterned (varied)
        color = glm::vec3(
            Random::range(0.2f, 0.8f),
            Random::range(0.2f, 0.8f),
            Random::range(0.3f, 0.9f)
        );
    }

    // Aquatic-specific traits
    finSize = Random::range(0.4f, 0.9f);
    tailSize = Random::range(0.6f, 1.1f);
    swimFrequency = Random::range(1.5f, 3.5f);
    swimAmplitude = Random::range(0.12f, 0.25f);
    preferredDepth = Random::range(0.15f, 0.4f);
    schoolingStrength = Random::range(0.6f, 0.95f);  // Most fish school

    // Underwater senses (different from land)
    visionRange = Random::range(15.0f, 40.0f);  // Water limits vision
    visionAcuity = Random::range(0.4f, 0.7f);
    colorPerception = Random::range(0.3f, 0.6f);  // Limited colors underwater
    motionDetection = Random::range(0.6f, 0.85f); // Good motion detection

    // Lateral line sense (vibration sensitivity very important)
    vibrationSensitivity = Random::range(0.7f, 0.95f);
    touchRange = Random::range(2.0f, 6.0f);

    // Hearing underwater (different from air)
    hearingRange = Random::range(30.0f, 70.0f);  // Sound travels well in water
    hearingDirectionality = Random::range(0.5f, 0.8f);

    // Smell (very important underwater)
    smellRange = Random::range(50.0f, 120.0f);  // Smell carries far in water
    smellSensitivity = Random::range(0.6f, 0.9f);

    // No flying traits
    wingSpan = 0.0f;
    flapFrequency = 0.0f;
    glideRatio = 0.0f;
    preferredAltitude = 0.0f;
}

void Genome::randomizeAquaticPredator() {
    // Start with base aquatic traits
    randomizeAquatic();

    // Predatory fish are larger
    size = Random::range(0.8f, 1.5f);
    speed = Random::range(12.0f, 22.0f);  // Faster for hunting

    // Predator colors (darker, more camouflaged)
    float colorChoice = Random::value();
    if (colorChoice < 0.5f) {
        // Dark blue/gray (like pike, barracuda)
        color = glm::vec3(
            Random::range(0.2f, 0.4f),
            Random::range(0.3f, 0.5f),
            Random::range(0.4f, 0.6f)
        );
    } else {
        // Greenish brown (like bass)
        color = glm::vec3(
            Random::range(0.3f, 0.5f),
            Random::range(0.4f, 0.6f),
            Random::range(0.2f, 0.4f)
        );
    }

    // Predator traits
    tailSize = Random::range(0.9f, 1.2f);       // Strong tail for bursts
    swimFrequency = Random::range(2.0f, 4.0f);  // Can swim fast
    schoolingStrength = Random::range(0.2f, 0.5f);  // Mostly solitary

    // Enhanced senses for hunting
    visionRange = Random::range(30.0f, 50.0f);
    visionAcuity = Random::range(0.6f, 0.85f);
    motionDetection = Random::range(0.75f, 0.9f);
}

void Genome::randomizeShark() {
    // Start with predator traits
    randomizeAquaticPredator();

    // Sharks are large apex predators
    size = Random::range(1.2f, 2.0f);
    speed = Random::range(15.0f, 25.0f);

    // Shark colors (gray, sometimes with white belly)
    color = glm::vec3(
        Random::range(0.35f, 0.5f),
        Random::range(0.4f, 0.55f),
        Random::range(0.45f, 0.6f)
    );

    // Shark traits
    tailSize = Random::range(1.0f, 1.2f);
    finSize = Random::range(0.8f, 1.0f);
    swimFrequency = Random::range(1.0f, 2.5f);  // Slower, steady swimming
    swimAmplitude = Random::range(0.08f, 0.15f);  // Less body undulation
    preferredDepth = Random::range(0.2f, 0.45f);
    schoolingStrength = Random::range(0.0f, 0.2f);  // Solitary

    // Exceptional senses
    visionRange = Random::range(40.0f, 60.0f);
    motionDetection = Random::range(0.8f, 0.95f);

    // Electroreception (sharks have this - modeled as vibration sensitivity)
    vibrationSensitivity = Random::range(0.9f, 0.99f);
    touchRange = Random::range(5.0f, 8.0f);

    // Incredible sense of smell
    smellRange = Random::range(100.0f, 150.0f);
    smellSensitivity = Random::range(0.85f, 0.98f);
}

// Initialize new aquatic traits with default values
void initializeAquaticTraitsDefaults(Genome& g) {
    // Swimming morphology
    g.bodyStreamlining = 0.7f;

    // Depth behavior
    g.minDepthTolerance = 0.1f;
    g.maxDepthTolerance = 0.6f;
    g.pressureResistance = 1.0f;

    // Social behavior
    g.schoolingRadius = 5.0f;
    g.schoolingAlignment = 0.7f;

    // Respiration
    g.gillEfficiency = 1.0f;
    g.oxygenStorage = 0.0f;
    g.canBreathAir = false;

    // Buoyancy
    g.swimbladderSize = 0.8f;
    g.neutralBuoyancyDepth = 0.3f;

    // Special abilities - Bioluminescence (off by default)
    g.hasBioluminescence = false;
    g.biolumIntensity = 0.0f;
    g.biolumRed = 0.0f;
    g.biolumGreen = 0.5f;
    g.biolumBlue = 1.0f;
    g.biolumPattern = 0;

    // Special abilities - Echolocation (off by default)
    g.aquaticEcholocation = 0.0f;
    g.echolocationRange = 0.0f;
    g.echolocationPrecision = 0.0f;

    // Special abilities - Electroreception (off by default)
    g.electroreception = 0.0f;
    g.electroRange = 0.0f;

    // Special abilities - Lateral line
    g.lateralLineSensitivity = 0.5f;

    // Special abilities - Venom/Toxicity
    g.venomPotency = 0.0f;
    g.toxicity = 0.0f;

    // Special abilities - Camouflage
    g.aquaticCamouflage = 0.0f;
    g.colorChangeSpeed = 0.0f;

    // Special abilities - Ink defense
    g.inkCapacity = 0.0f;
    g.inkRechargeRate = 0.0f;

    // Special abilities - Electric discharge
    g.electricDischarge = 0.0f;
    g.electricRechargeRate = 0.0f;

    // Air-breathing behavior
    g.breathHoldDuration = 0.0f;
    g.surfaceBreathRate = 0.0f;

    // Fin configurations
    g.dorsalFinHeight = 0.25f;
    g.pectoralFinWidth = 0.35f;
    g.caudalFinType = 0.3f;
    g.analFinSize = 0.15f;
    g.pelvicFinSize = 0.15f;
    g.finCount = 5;

    // Scale/skin patterns
    g.scaleSize = 0.03f;
    g.scaleShininess = 0.5f;
    g.patternFrequency = 3.0f;
    g.patternType = 0;
}

void Genome::randomizeJellyfish() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Jellyfish are translucent, bell-shaped
    size = Random::range(0.1f, 0.8f);
    speed = Random::range(0.5f, 3.0f);  // Very slow

    // Translucent colors
    color = glm::vec3(
        Random::range(0.7f, 0.95f),
        Random::range(0.75f, 0.95f),
        Random::range(0.85f, 1.0f)
    );

    // Jellyfish traits - no real fins/tail
    finSize = 0.0f;
    tailSize = 0.0f;
    swimFrequency = Random::range(0.5f, 1.5f);  // Pulsing
    swimAmplitude = Random::range(0.3f, 0.6f);   // Large body movement
    bodyStreamlining = 0.3f;  // Not streamlined

    // Depth behavior - can go very deep
    preferredDepth = Random::range(0.1f, 0.6f);
    maxDepthTolerance = 0.9f;
    pressureResistance = 1.5f;  // Resistant to pressure

    // No real schooling for most jellyfish
    schoolingStrength = Random::range(0.0f, 0.3f);

    // Jellyfish don't have swim bladders
    swimbladderSize = 0.0f;

    // Bioluminescence is common in jellyfish
    hasBioluminescence = Random::chance(0.6f);
    if (hasBioluminescence) {
        biolumIntensity = Random::range(0.3f, 0.8f);
        biolumGreen = Random::range(0.3f, 0.8f);
        biolumBlue = Random::range(0.6f, 1.0f);
        biolumPattern = Random::chance(0.5f) ? 1 : 0;  // Pulsing or glow
    }

    // Venomous tentacles
    venomPotency = Random::range(0.2f, 0.9f);
    toxicity = Random::range(0.1f, 0.6f);

    // Very limited senses
    visionRange = Random::range(2.0f, 8.0f);
    visionAcuity = Random::range(0.05f, 0.2f);
    hearingRange = 0.0f;
    smellRange = Random::range(5.0f, 15.0f);

    // Pattern for procedural mesh
    patternType = 0;  // Translucent
    scaleShininess = 0.2f;
    scaleSize = 0.0f;  // No scales
}

void Genome::randomizeCrustacean() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Crabs/lobsters - armored bottom dwellers
    size = Random::range(0.2f, 0.6f);
    speed = Random::range(1.0f, 4.0f);  // Slow on bottom

    // Reds, oranges, browns
    float colorChoice = Random::value();
    if (colorChoice < 0.5f) {
        color = glm::vec3(
            Random::range(0.7f, 0.95f),
            Random::range(0.2f, 0.4f),
            Random::range(0.1f, 0.25f)
        );
    } else {
        color = glm::vec3(
            Random::range(0.4f, 0.6f),
            Random::range(0.3f, 0.5f),
            Random::range(0.2f, 0.4f)
        );
    }

    // Bottom dwellers - no swimming fins
    finSize = 0.0f;
    tailSize = Random::range(0.2f, 0.5f);  // Tail for escape
    swimFrequency = 0.0f;
    swimAmplitude = 0.0f;
    bodyStreamlining = 0.2f;  // Not streamlined

    // Prefer sea floor
    preferredDepth = Random::range(0.4f, 0.8f);
    maxDepthTolerance = 0.95f;
    pressureResistance = 2.0f;  // Very resistant

    // Not schooling
    schoolingStrength = 0.0f;

    // No swim bladder
    swimbladderSize = 0.0f;

    // No bioluminescence typically
    hasBioluminescence = false;

    // Armor instead of camouflage
    aquaticCamouflage = Random::range(0.1f, 0.4f);

    // Good touch/vibration senses
    vibrationSensitivity = Random::range(0.7f, 0.9f);
    touchRange = Random::range(3.0f, 6.0f);

    // Limited vision but wide FOV
    visionRange = Random::range(8.0f, 20.0f);
    visionFOV = Random::range(4.5f, 6.0f);  // Near 360 degrees

    // Smell for scavenging
    smellRange = Random::range(40.0f, 80.0f);
    smellSensitivity = Random::range(0.7f, 0.9f);

    // Shell pattern
    patternType = 0;  // Solid color
    scaleShininess = 0.3f;
    scaleSize = 0.0f;  // Shell, not scales
}

void Genome::randomizeEel() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Eels - long, serpentine
    size = Random::range(0.8f, 1.8f);  // Long
    speed = Random::range(4.0f, 10.0f);

    // Dark colors
    color = glm::vec3(
        Random::range(0.15f, 0.35f),
        Random::range(0.2f, 0.4f),
        Random::range(0.15f, 0.3f)
    );

    // Serpentine movement
    finSize = Random::range(0.1f, 0.3f);  // Small fins
    tailSize = Random::range(0.2f, 0.4f);
    swimFrequency = Random::range(1.0f, 2.5f);
    swimAmplitude = Random::range(0.4f, 0.7f);  // Large body waves
    bodyStreamlining = 0.9f;  // Very streamlined

    // Depth preferences
    preferredDepth = Random::range(0.2f, 0.5f);
    maxDepthTolerance = 0.8f;

    // Solitary
    schoolingStrength = 0.0f;

    // Small swim bladder
    swimbladderSize = Random::range(0.3f, 0.6f);

    // Moray eels hide in reefs
    aquaticCamouflage = Random::range(0.3f, 0.6f);

    // Good smell for hunting
    smellRange = Random::range(60.0f, 100.0f);
    smellSensitivity = Random::range(0.75f, 0.9f);

    // Electric eel capability (rare)
    if (Random::chance(0.3f)) {
        electricDischarge = Random::range(0.5f, 1.0f);
        electricRechargeRate = Random::range(0.08f, 0.15f);
    }

    // Fin configuration for eel
    dorsalFinHeight = Random::range(0.05f, 0.15f);
    pectoralFinWidth = Random::range(0.1f, 0.2f);
    caudalFinType = 0.0f;  // Rounded, continuous with body
    finCount = 2;

    patternType = Random::chance(0.5f) ? 1 : 0;  // Sometimes striped
}

void Genome::randomizeDolphin() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Dolphins - intelligent marine mammals
    size = Random::range(1.2f, 2.0f);
    speed = Random::range(12.0f, 20.0f);  // Fast swimmers

    // Gray coloring
    float gray = Random::range(0.4f, 0.6f);
    color = glm::vec3(gray, gray + 0.05f, gray + 0.1f);

    // Streamlined body
    finSize = Random::range(0.5f, 0.8f);
    tailSize = Random::range(0.8f, 1.1f);  // Strong fluke
    swimFrequency = Random::range(1.5f, 3.0f);
    swimAmplitude = Random::range(0.15f, 0.25f);
    bodyStreamlining = 0.95f;  // Very streamlined

    // Can go moderately deep
    preferredDepth = Random::range(0.1f, 0.3f);
    maxDepthTolerance = 0.5f;
    pressureResistance = 1.2f;

    // Dolphins are social
    schoolingStrength = Random::range(0.7f, 0.95f);
    schoolingRadius = Random::range(5.0f, 15.0f);
    schoolingAlignment = Random::range(0.6f, 0.9f);

    // Air breathing!
    canBreathAir = true;
    swimbladderSize = 0.0f;  // Mammals don't have swim bladders
    gillEfficiency = 0.0f;   // No gills
    oxygenStorage = Random::range(0.7f, 0.95f);  // Good oxygen storage
    breathHoldDuration = Random::range(8.0f, 15.0f);  // 8-15 minutes
    surfaceBreathRate = Random::range(1.0f, 2.0f);

    // Echolocation - dolphins are masters
    aquaticEcholocation = Random::range(0.8f, 0.98f);
    echolocationRange = Random::range(80.0f, 150.0f);
    echolocationPrecision = Random::range(0.8f, 0.95f);
    echolocationAbility = Random::range(0.8f, 0.98f);  // Legacy field

    // Good vision
    visionRange = Random::range(40.0f, 70.0f);
    visionAcuity = Random::range(0.6f, 0.8f);

    // Dolphins communicate
    alarmCallVolume = Random::range(0.7f, 0.95f);
    displayIntensity = Random::range(0.5f, 0.8f);

    // High intelligence
    memoryCapacity = Random::range(0.8f, 0.95f);
    memoryRetention = Random::range(0.8f, 0.95f);

    // Fin configuration
    dorsalFinHeight = Random::range(0.3f, 0.5f);
    pectoralFinWidth = Random::range(0.3f, 0.5f);
    caudalFinType = 0.8f;  // Horizontal fluke
    finCount = 4;

    patternType = 4;  // Counter-shading
    scaleShininess = 0.7f;
    scaleSize = 0.0f;  // Smooth skin
}

void Genome::randomizeWhale() {
    randomizeDolphin();  // Start with dolphin traits

    // Whales are MUCH larger
    size = Random::range(8.0f, 15.0f);
    speed = Random::range(5.0f, 12.0f);  // Slower than dolphins

    // Blue-gray coloring
    color = glm::vec3(
        Random::range(0.2f, 0.35f),
        Random::range(0.25f, 0.4f),
        Random::range(0.35f, 0.5f)
    );

    // Massive but streamlined
    bodyStreamlining = 0.85f;
    tailSize = Random::range(1.0f, 1.3f);

    // Whales can dive very deep
    preferredDepth = Random::range(0.1f, 0.4f);
    maxDepthTolerance = 0.8f;
    pressureResistance = 1.8f;

    // Extended breath hold
    oxygenStorage = Random::range(0.9f, 0.99f);
    breathHoldDuration = Random::range(30.0f, 90.0f);  // Up to 90 minutes!

    // Whales migrate alone or in small groups
    schoolingStrength = Random::range(0.2f, 0.5f);
    schoolingRadius = Random::range(20.0f, 50.0f);

    // Whale song communication
    alarmCallVolume = Random::range(0.9f, 1.0f);

    // Echolocation varies by whale type
    aquaticEcholocation = Random::range(0.3f, 0.9f);
    echolocationRange = Random::range(100.0f, 500.0f);  // Very long range

    dorsalFinHeight = Random::range(0.1f, 0.3f);  // Some whales have small/no dorsal
    finCount = 3;

    patternType = Random::chance(0.5f) ? 2 : 4;  // Spots or counter-shading
}

void Genome::randomizeOctopus() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Octopus - intelligent cephalopod
    size = Random::range(0.3f, 0.8f);
    speed = Random::range(3.0f, 8.0f);

    // Reddish-brown base (but can change!)
    color = glm::vec3(
        Random::range(0.5f, 0.8f),
        Random::range(0.3f, 0.5f),
        Random::range(0.2f, 0.4f)
    );

    // Jet propulsion, no fins
    finSize = 0.0f;
    tailSize = 0.0f;
    swimFrequency = 0.0f;
    swimAmplitude = 0.0f;
    bodyStreamlining = 0.6f;

    // Prefers reef/rocky bottom
    preferredDepth = Random::range(0.2f, 0.5f);
    maxDepthTolerance = 0.7f;

    // Solitary
    schoolingStrength = 0.0f;

    // Camouflage masters!
    aquaticCamouflage = Random::range(0.85f, 0.99f);
    colorChangeSpeed = Random::range(0.8f, 0.99f);

    // Ink defense
    inkCapacity = Random::range(0.7f, 1.0f);
    inkRechargeRate = Random::range(0.15f, 0.3f);

    // Excellent vision
    visionRange = Random::range(30.0f, 50.0f);
    visionAcuity = Random::range(0.7f, 0.9f);

    // Touch is important
    touchRange = Random::range(4.0f, 8.0f);
    vibrationSensitivity = Random::range(0.6f, 0.85f);

    // High intelligence
    memoryCapacity = Random::range(0.75f, 0.95f);
    memoryRetention = Random::range(0.6f, 0.85f);

    // No scales, soft body
    scaleSize = 0.0f;
    scaleShininess = 0.4f;
    patternType = 2;  // Can have spots
    finCount = 0;
}

void Genome::randomizeDeepSeaFish() {
    randomizeAquatic();
    initializeAquaticTraitsDefaults(*this);

    // Deep sea fish - adapted to extreme depths
    size = Random::range(0.2f, 1.0f);
    speed = Random::range(2.0f, 8.0f);  // Slower in deep water

    // Dark colors, often black
    float darkness = Random::range(0.1f, 0.3f);
    color = glm::vec3(darkness, darkness, darkness + 0.05f);

    // Prefer great depths
    preferredDepth = Random::range(0.6f, 0.9f);
    minDepthTolerance = 0.4f;  // Can't go too shallow
    maxDepthTolerance = 1.0f;
    pressureResistance = Random::range(1.5f, 2.5f);

    // Weak swim bladder or none
    swimbladderSize = Random::range(0.0f, 0.3f);

    // Most have bioluminescence
    hasBioluminescence = Random::chance(0.8f);
    if (hasBioluminescence) {
        biolumIntensity = Random::range(0.4f, 0.9f);
        biolumGreen = Random::range(0.2f, 0.6f);
        biolumBlue = Random::range(0.6f, 1.0f);

        // Anglerfish-style lure
        if (Random::chance(0.3f)) {
            biolumPattern = 3;  // Lure
        } else if (Random::chance(0.4f)) {
            biolumPattern = 4;  // Counter-illumination
        } else {
            biolumPattern = static_cast<uint8_t>(Random::rangeInt(0, 2));
        }
    }

    // Adapted eyes for low light
    visionRange = Random::range(10.0f, 30.0f);
    visionAcuity = Random::range(0.3f, 0.6f);

    // Enhanced lateral line
    lateralLineSensitivity = Random::range(0.8f, 0.98f);
    vibrationSensitivity = Random::range(0.85f, 0.98f);

    // Good smell in still water
    smellRange = Random::range(40.0f, 80.0f);

    // Often have large mouths relative to body
    patternType = 0;  // Usually no pattern
    scaleShininess = 0.2f;
}

void Genome::randomizePlankton() {
    randomize();
    initializeAquaticTraitsDefaults(*this);

    // Plankton - tiny drifters
    size = Random::range(0.01f, 0.05f);
    speed = Random::range(0.1f, 0.5f);  // Almost stationary

    // Translucent or greenish
    color = glm::vec3(
        Random::range(0.3f, 0.6f),
        Random::range(0.6f, 0.9f),
        Random::range(0.4f, 0.7f)
    );

    // No real swimming
    finSize = 0.0f;
    tailSize = 0.0f;
    swimFrequency = 0.0f;
    swimAmplitude = 0.0f;
    bodyStreamlining = 0.1f;

    // Drift with currents at all depths
    preferredDepth = Random::range(0.0f, 0.5f);
    maxDepthTolerance = 0.8f;

    // No swim bladder
    swimbladderSize = 0.0f;

    // Some plankton are bioluminescent
    hasBioluminescence = Random::chance(0.4f);
    if (hasBioluminescence) {
        biolumIntensity = Random::range(0.1f, 0.4f);
        biolumGreen = Random::range(0.4f, 0.8f);
        biolumBlue = Random::range(0.6f, 1.0f);
        biolumPattern = 2;  // Flash when disturbed
    }

    // Minimal senses
    visionRange = Random::range(0.5f, 2.0f);
    visionAcuity = Random::range(0.0f, 0.1f);
    hearingRange = 0.0f;
    smellRange = Random::range(0.5f, 3.0f);

    // No scales
    scaleSize = 0.0f;
    scaleShininess = 0.1f;
    patternType = 0;
    finCount = 0;
}

float Genome::calculateSensoryEnergyCost() const {
    float cost = 0.0f;

    // Vision: highest cost, scales with quality
    cost += (visionFOV / 6.28f) * 0.08f;           // FOV cost
    cost += (visionRange / 60.0f) * 0.15f;         // Range cost
    cost += visionAcuity * 0.25f;                  // Acuity is expensive
    cost += colorPerception * 0.15f;
    cost += motionDetection * 0.12f;

    // Hearing: moderate cost
    cost += (hearingRange / 100.0f) * 0.08f;
    cost += hearingDirectionality * 0.08f;
    cost += echolocationAbility * 0.35f;           // Echolocation is very expensive

    // Smell: low cost
    cost += (smellRange / 150.0f) * 0.04f;
    cost += smellSensitivity * 0.04f;
    cost += pheromoneProduction * 0.08f;

    // Touch: very low cost
    cost += (touchRange / 8.0f) * 0.02f;
    cost += vibrationSensitivity * 0.02f;

    // Camouflage: moderate cost (pigment production and behavior)
    cost += camouflageLevel * 0.12f;

    // Communication
    cost += alarmCallVolume * 0.05f;
    cost += displayIntensity * 0.08f;

    // Memory
    cost += memoryCapacity * 0.1f;
    cost += memoryRetention * 0.05f;

    return cost;
}

// ============================================================================
// CHEMISTRY-AWARE MUTATION
// ============================================================================
// Mutations that consider planet chemistry - creatures are more likely to
// mutate toward chemistry-compatible values, simulating selective pressure.

void Genome::mutateWithChemistry(float mutationRate, float mutationStrength, const PlanetChemistry& chemistry) {
    // First, apply standard mutations
    mutate(mutationRate, mutationStrength);

    // Then apply chemistry-biased corrections to biochemistry traits
    // This simulates selective pressure toward compatibility

    // Bias toward optimal solvent affinity for this planet
    if (Random::chance(mutationRate * 0.5f)) {
        float targetAffinity;
        switch (chemistry.solventType) {
            case SolventType::WATER:
                targetAffinity = 0.5f;
                break;
            case SolventType::AMMONIA:
            case SolventType::METHANE:
                targetAffinity = 0.15f;
                break;
            case SolventType::SULFURIC_ACID:
            case SolventType::ETHANOL:
                targetAffinity = 0.85f;
                break;
            default:
                targetAffinity = 0.5f;
        }
        // Drift toward target
        float drift = (targetAffinity - solventAffinity) * 0.1f * mutationStrength;
        solventAffinity = std::clamp(solventAffinity + drift, 0.0f, 1.0f);
    }

    // Bias oxygen tolerance toward atmospheric oxygen level
    if (Random::chance(mutationRate * 0.5f)) {
        float targetOxygenTolerance = chemistry.atmosphere.oxygen / 0.3f;  // Normalized to 0-1 range
        targetOxygenTolerance = std::clamp(targetOxygenTolerance, 0.0f, 1.0f);
        float drift = (targetOxygenTolerance - oxygenTolerance) * 0.1f * mutationStrength;
        oxygenTolerance = std::clamp(oxygenTolerance + drift, 0.0f, 1.0f);
    }

    // Bias membrane fluidity toward temperature optimum
    if (Random::chance(mutationRate * 0.5f)) {
        // Map temperature to fluidity: cold -> low, hot -> high
        float targetFluidity = (chemistry.temperatureBase + 50.0f) / 200.0f;  // -50 to +150 range
        targetFluidity = std::clamp(targetFluidity, 0.0f, 1.0f);
        float drift = (targetFluidity - membraneFluidity) * 0.08f * mutationStrength;
        membraneFluidity = std::clamp(membraneFluidity + drift, 0.0f, 1.0f);
    }

    // Bias radiation resistance toward environmental radiation
    if (Random::chance(mutationRate * 0.5f) && chemistry.radiationLevel > 1.0f) {
        float targetResistance = (chemistry.radiationLevel - 1.0f);
        targetResistance = std::clamp(targetResistance, 0.0f, 1.0f);
        float drift = (targetResistance - radiationResistance) * 0.1f * mutationStrength;
        radiationResistance = std::clamp(radiationResistance + drift, 0.0f, 1.0f);
    }

    // Bias pH preference toward environmental pH
    if (Random::chance(mutationRate * 0.5f)) {
        float targetpH;
        if (chemistry.acidity < 4.0f) {
            targetpH = 0.15f;  // Acidophile
        } else if (chemistry.acidity > 9.0f) {
            targetpH = 0.85f;  // Alkaliphile
        } else {
            targetpH = 0.5f;   // Neutral
        }
        float drift = (targetpH - pHPreference) * 0.08f * mutationStrength;
        pHPreference = std::clamp(pHPreference + drift, 0.0f, 1.0f);
    }

    // High acidity environments favor higher mineralization (protection)
    if (Random::chance(mutationRate * 0.3f) && chemistry.acidity < 5.0f) {
        float drift = 0.05f * mutationStrength;
        mineralizationBias = std::clamp(mineralizationBias + drift, 0.0f, 1.0f);
    }

    // High mineral availability allows higher mineralization
    float avgMineralAvail = (chemistry.minerals.calcium + chemistry.minerals.silicon +
                             chemistry.minerals.iron) / 3.0f;
    if (Random::chance(mutationRate * 0.3f) && mineralizationBias > avgMineralAvail * 1.2f) {
        // Mineral demand exceeds supply, drift toward lower mineralization
        float drift = -0.03f * mutationStrength;
        mineralizationBias = std::clamp(mineralizationBias + drift, 0.0f, 1.0f);
    }
}

// ============================================================================
// EVOLUTION PRESET INITIALIZATION
// ============================================================================

void Genome::initializeForPreset(EvolutionStartPreset preset,
                                  EvolutionGuidanceBias bias,
                                  const PlanetChemistry& chemistry) {
    // Start with base randomization
    randomize();

    // Apply preset modifications
    switch (preset) {
        case EvolutionStartPreset::PROTO:
            // Primordial: simple organisms, minimal traits
            size = Random::range(0.3f, 0.6f);
            speed = Random::range(2.0f, 8.0f);
            visionRange = Random::range(5.0f, 15.0f);
            efficiency = Random::range(0.8f, 1.2f);

            // Simple sensory systems
            visionFOV = Random::range(1.0f, 2.5f);
            visionAcuity = Random::range(0.05f, 0.3f);
            colorPerception = Random::range(0.0f, 0.2f);
            hearingRange = Random::range(5.0f, 20.0f);
            smellRange = Random::range(10.0f, 30.0f);

            // Minimal morphology
            segmentCount = 1;
            bodyAspect = Random::range(0.5f, 1.5f);
            dorsalFinCount = 0;
            pectoralFinCount = 0;
            hornCount = 0;
            shellCoverage = 0.0f;
            break;

        case EvolutionStartPreset::EARLY_LIMB:
            // Default randomize() values are good for this
            // Just ensure moderate complexity
            visionAcuity = Random::range(0.2f, 0.5f);
            segmentCount = static_cast<uint8_t>(Random::rangeInt(1, 3));
            break;

        case EvolutionStartPreset::COMPLEX:
            // More developed organisms
            size = Random::range(0.6f, 1.5f);
            speed = Random::range(8.0f, 18.0f);
            visionRange = Random::range(25.0f, 50.0f);

            // Enhanced sensory
            visionAcuity = Random::range(0.4f, 0.75f);
            colorPerception = Random::range(0.3f, 0.7f);
            motionDetection = Random::range(0.4f, 0.8f);
            hearingRange = Random::range(30.0f, 70.0f);

            // More complex morphology
            segmentCount = static_cast<uint8_t>(Random::rangeInt(2, 5));
            if (Random::chance(0.3f)) dorsalFinCount = static_cast<uint8_t>(Random::rangeInt(1, 2));
            if (Random::chance(0.2f)) hornCount = static_cast<uint8_t>(Random::rangeInt(1, 3));
            break;

        case EvolutionStartPreset::ADVANCED:
            // Sophisticated organisms
            size = Random::range(0.8f, 2.0f);
            speed = Random::range(12.0f, 22.0f);
            visionRange = Random::range(35.0f, 60.0f);
            efficiency = Random::range(0.6f, 1.0f);

            // High sensory capabilities
            visionAcuity = Random::range(0.6f, 0.9f);
            colorPerception = Random::range(0.5f, 0.9f);
            motionDetection = Random::range(0.6f, 0.95f);
            hearingRange = Random::range(50.0f, 90.0f);
            echolocationAbility = Random::range(0.0f, 0.4f);

            // Complex morphology
            segmentCount = static_cast<uint8_t>(Random::rangeInt(3, 7));
            dorsalFinCount = static_cast<uint8_t>(Random::rangeInt(0, 3));
            pectoralFinCount = static_cast<uint8_t>(Random::rangeInt(1, 3));
            if (Random::chance(0.4f)) hornCount = static_cast<uint8_t>(Random::rangeInt(1, 4));
            if (Random::chance(0.3f)) shellCoverage = Random::range(0.2f, 0.6f);

            // Better memory
            memoryCapacity = Random::range(0.5f, 0.9f);
            memoryRetention = Random::range(0.5f, 0.85f);
            break;

        default:
            break;
    }

    // Apply guidance bias
    switch (bias) {
        case EvolutionGuidanceBias::LAND:
            // Bias toward land locomotion
            size = std::clamp(size * 1.1f, 0.5f, 2.0f);
            speed = std::clamp(speed * 0.9f, 5.0f, 18.0f);  // Slightly slower on land
            bodyStreamlining = Random::range(0.3f, 0.6f);
            // Better vision, reduced aquatic traits
            visionRange *= 1.2f;
            gillEfficiency = 0.0f;
            canBreathAir = true;
            finSize = 0.0f;
            swimFrequency = 0.0f;
            break;

        case EvolutionGuidanceBias::AQUATIC:
            // Apply aquatic trait initialization
            randomizeAquatic();
            // Then restore preset-appropriate complexity
            if (preset == EvolutionStartPreset::PROTO) {
                visionAcuity = Random::range(0.1f, 0.3f);
                segmentCount = 1;
            }
            break;

        case EvolutionGuidanceBias::FLIGHT:
            // Initialize for flight potential
            size = Random::range(0.3f, 0.7f);  // Smaller for flight
            bodyDensity = Random::range(0.75f, 0.95f);  // Lighter
            wingSpan = Random::range(0.3f, 0.8f);  // Early wing development
            if (preset >= EvolutionStartPreset::COMPLEX) {
                randomizeFlying();
            }
            break;

        case EvolutionGuidanceBias::UNDERGROUND:
            // Bias toward burrowing
            size = Random::range(0.4f, 1.0f);
            bodyAspect = Random::range(1.5f, 2.5f);  // Elongated
            visionRange *= 0.5f;  // Reduced vision
            visionAcuity *= 0.6f;
            vibrationSensitivity = Random::range(0.6f, 0.95f);  // Enhanced
            touchRange = Random::range(3.0f, 7.0f);
            shellCoverage = Random::range(0.1f, 0.4f);  // Some armor
            hasClaws = true;
            clawLength = Random::range(0.2f, 0.4f);
            break;

        case EvolutionGuidanceBias::NONE:
        default:
            // No modifications
            break;
    }

    // Finally, adapt biochemistry to planet chemistry
    adaptToChemistry(chemistry);
}

void Genome::initializeForRegion(const RegionEvolutionConfig& config,
                                  const PlanetChemistry& chemistry) {
    initializeForPreset(config.preset, config.bias, chemistry);

    // Apply exotic biochemistry restrictions if needed
    if (!config.allowExoticBiochemistry) {
        // Keep biochemistry traits near standard values
        biopigmentFamily = std::clamp(biopigmentFamily, static_cast<uint8_t>(0), static_cast<uint8_t>(2));
        metabolicPathway = 0;  // Force aerobic
        solventAffinity = std::clamp(solventAffinity, 0.35f, 0.65f);  // Water-centered
    }
}

// ============================================================================
// VARIETY GENERATION (Phase 11 Agent 4)
// ============================================================================

EvolutionStartPreset Genome::selectPresetForBiome(BiomeType biome) {
    // Map biomes to appropriate evolutionary starting points
    switch (biome) {
        // Extreme environments = earlier life forms
        case BiomeType::DEEP_OCEAN:
        case BiomeType::VOLCANIC:
        case BiomeType::LAVA_FIELD:
        case BiomeType::GLACIER:
            return EvolutionStartPreset::PROTO;

        // Harsh but viable = early multi-cellular
        case BiomeType::DESERT_HOT:
        case BiomeType::DESERT_COLD:
        case BiomeType::TUNDRA:
        case BiomeType::CRATER_LAKE:
        case BiomeType::OCEAN:
            return EvolutionStartPreset::EARLY_LIMB;

        // Moderate environments = complex organisms
        case BiomeType::GRASSLAND:
        case BiomeType::SAVANNA:
        case BiomeType::TEMPERATE_FOREST:
        case BiomeType::BOREAL_FOREST:
        case BiomeType::SHALLOW_WATER:
        case BiomeType::BEACH_SANDY:
        case BiomeType::BEACH_ROCKY:
        case BiomeType::WETLAND:
        case BiomeType::SWAMP:
        case BiomeType::SHRUBLAND:
        case BiomeType::ALPINE_MEADOW:
        case BiomeType::ROCKY_HIGHLANDS:
            return EvolutionStartPreset::COMPLEX;

        // Rich ecosystems = advanced life
        case BiomeType::TROPICAL_RAINFOREST:
        case BiomeType::CORAL_REEF:
        case BiomeType::KELP_FOREST:
        case BiomeType::MANGROVE:
        case BiomeType::SALT_MARSH:
        case BiomeType::TIDAL_POOL:
        case BiomeType::MOUNTAIN_FOREST:
        case BiomeType::CAVE_ENTRANCE:
        case BiomeType::RIVER_BANK:
        case BiomeType::LAKE_SHORE:
            return EvolutionStartPreset::ADVANCED;

        default:
            return EvolutionStartPreset::COMPLEX;
    }
}

EvolutionGuidanceBias Genome::selectBiasForBiome(BiomeType biome, float varietySeed) {
    // Deterministic variety using seed for different niches in same biome
    uint32_t seed = static_cast<uint32_t>(varietySeed * 100.0f);

    // Water biomes = aquatic bias
    if (biome >= BiomeType::DEEP_OCEAN && biome <= BiomeType::KELP_FOREST) {
        return EvolutionGuidanceBias::AQUATIC;
    }

    // Coastal biomes = mix of aquatic and land
    if (biome >= BiomeType::BEACH_SANDY && biome <= BiomeType::SALT_MARSH) {
        return (seed % 3 == 0) ? EvolutionGuidanceBias::AQUATIC : EvolutionGuidanceBias::LAND;
    }

    // Special aquatic biomes
    if (biome == BiomeType::RIVER_BANK || biome == BiomeType::LAKE_SHORE) {
        return (seed % 2 == 0) ? EvolutionGuidanceBias::AQUATIC : EvolutionGuidanceBias::LAND;
    }

    // Highland biomes = mix of land and flight
    if (biome >= BiomeType::SHRUBLAND && biome <= BiomeType::MOUNTAIN_FOREST) {
        if (seed % 4 == 0) return EvolutionGuidanceBias::FLIGHT;
        return EvolutionGuidanceBias::LAND;
    }

    // Extreme cold = underground adaptations
    if (biome == BiomeType::TUNDRA || biome == BiomeType::GLACIER ||
        biome == BiomeType::DESERT_COLD || biome == BiomeType::CAVE_ENTRANCE) {
        return (seed % 3 == 0) ? EvolutionGuidanceBias::UNDERGROUND : EvolutionGuidanceBias::LAND;
    }

    // Most other biomes = primarily land with some flyers
    if (seed % 5 == 0) return EvolutionGuidanceBias::FLIGHT;
    if (seed % 7 == 0) return EvolutionGuidanceBias::UNDERGROUND;

    return EvolutionGuidanceBias::LAND;
}

void Genome::initializeForBiome(BiomeType biome,
                                const PlanetChemistry& chemistry,
                                float varietySeed) {
    // Select preset and bias based on biome
    EvolutionStartPreset preset = selectPresetForBiome(biome);
    EvolutionGuidanceBias bias = selectBiasForBiome(biome, varietySeed);

    // Initialize with selected configuration
    initializeForPreset(preset, bias, chemistry);

    // Apply additional variety mutations to prevent identical creatures
    applyVarietyMutations(0.3f + varietySeed * 0.2f);
}

void Genome::applyVarietyMutations(float varietyStrength) {
    // Apply cosmetic and morphological variety without drastically affecting fitness
    // This creates visual diversity while maintaining viability

    // Clamp variety strength to reasonable bounds
    varietyStrength = std::clamp(varietyStrength, 0.0f, 0.5f);

    // Color variety (purely cosmetic)
    color.r = std::clamp(color.r + Random::range(-varietyStrength, varietyStrength), 0.0f, 1.0f);
    color.g = std::clamp(color.g + Random::range(-varietyStrength, varietyStrength), 0.0f, 1.0f);
    color.b = std::clamp(color.b + Random::range(-varietyStrength, varietyStrength), 0.0f, 1.0f);

    // Size variety (affects balance but within safe range)
    float sizeVariation = Random::range(-0.15f, 0.15f) * varietyStrength;
    size = std::clamp(size + sizeVariation, 0.5f, 2.0f);

    // Speed variety (compensate with efficiency to maintain fitness)
    float speedVariation = Random::range(-2.0f, 2.0f) * varietyStrength;
    speed = std::clamp(speed + speedVariation, 5.0f, 20.0f);

    // Compensate speed changes with efficiency adjustments
    if (speedVariation > 0) {
        efficiency = std::clamp(efficiency - speedVariation * 0.01f, 0.5f, 1.5f);
    }

    // Morphology variety (body segments, limbs, etc.)
    if (Random::chance(varietyStrength)) {
        segmentCount = std::clamp(static_cast<uint8_t>(segmentCount + Random::rangeInt(-1, 1)),
                                  static_cast<uint8_t>(1), static_cast<uint8_t>(8));
    }

    if (Random::chance(varietyStrength)) {
        bodyAspect = std::clamp(bodyAspect + Random::range(-0.3f, 0.3f) * varietyStrength, 0.3f, 3.0f);
    }

    // Appendage variety
    if (Random::chance(varietyStrength * 0.5f)) {
        dorsalFinCount = std::clamp(static_cast<uint8_t>(dorsalFinCount + Random::rangeInt(-1, 1)),
                                    static_cast<uint8_t>(0), static_cast<uint8_t>(3));
    }

    if (Random::chance(varietyStrength * 0.5f)) {
        hornCount = std::clamp(static_cast<uint8_t>(hornCount + Random::rangeInt(-1, 2)),
                               static_cast<uint8_t>(0), static_cast<uint8_t>(6));
    }

    // Pattern variety (cosmetic)
    if (Random::chance(varietyStrength)) {
        patternType = Random::rangeInt(0, 4);
        patternIntensity = Random::range(0.2f, 0.8f);
        patternFrequency = Random::range(1.0f, 10.0f);
    }

    // Crest and display feature variety
    if (Random::chance(varietyStrength * 0.6f)) {
        crestHeight = Random::range(0.0f, 0.5f);
        crestType = Random::rangeInt(0, 4);
    }

    // Tail variety
    if (Random::chance(varietyStrength * 0.5f)) {
        tailVariant = Random::rangeInt(0, 6);
        tailLength = std::clamp(tailLength + Random::range(-0.2f, 0.2f) * varietyStrength, 0.2f, 1.5f);
    }

    // Eye arrangement variety (cosmetic but affects appearance significantly)
    if (Random::chance(varietyStrength * 0.4f)) {
        eyeArrangement = Random::rangeInt(0, 4);
    }

    // Shell/armor variety
    if (Random::chance(varietyStrength * 0.3f)) {
        shellCoverage = std::clamp(shellCoverage + Random::range(-0.2f, 0.2f) * varietyStrength, 0.0f, 1.0f);
    }
}

Genome::DiversityMetrics Genome::calculatePopulationDiversity(const std::vector<Genome>& population) {
    if (population.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    }

    DiversityMetrics metrics;

    // Calculate means
    float meanSize = 0.0f, meanSpeed = 0.0f;
    float meanColorR = 0.0f, meanColorG = 0.0f, meanColorB = 0.0f;
    float meanSegments = 0.0f, meanAspect = 0.0f, meanHorns = 0.0f;

    for (const auto& genome : population) {
        meanSize += genome.size;
        meanSpeed += genome.speed;
        meanColorR += genome.color.r;
        meanColorG += genome.color.g;
        meanColorB += genome.color.b;
        meanSegments += genome.segmentCount;
        meanAspect += genome.bodyAspect;
        meanHorns += genome.hornCount;
    }

    float n = static_cast<float>(population.size());
    meanSize /= n;
    meanSpeed /= n;
    meanColorR /= n;
    meanColorG /= n;
    meanColorB /= n;
    meanSegments /= n;
    meanAspect /= n;
    meanHorns /= n;

    // Calculate variances
    float varSize = 0.0f, varSpeed = 0.0f, varColor = 0.0f;
    float varSegments = 0.0f, varAspect = 0.0f, varHorns = 0.0f;

    for (const auto& genome : population) {
        float diffSize = genome.size - meanSize;
        float diffSpeed = genome.speed - meanSpeed;
        float diffColorR = genome.color.r - meanColorR;
        float diffColorG = genome.color.g - meanColorG;
        float diffColorB = genome.color.b - meanColorB;
        float diffSegments = genome.segmentCount - meanSegments;
        float diffAspect = genome.bodyAspect - meanAspect;
        float diffHorns = genome.hornCount - meanHorns;

        varSize += diffSize * diffSize;
        varSpeed += diffSpeed * diffSpeed;
        varColor += diffColorR * diffColorR + diffColorG * diffColorG + diffColorB * diffColorB;
        varSegments += diffSegments * diffSegments;
        varAspect += diffAspect * diffAspect;
        varHorns += diffHorns * diffHorns;
    }

    varSize /= n;
    varSpeed /= n;
    varColor /= (n * 3.0f);  // Average across RGB

    float varMorphology = (varSegments + varAspect + varHorns) / (n * 3.0f);

    // Normalize variances to 0-1 range (using expected max variances)
    metrics.sizeVariance = std::min(1.0f, varSize / 0.25f);  // Max variance ≈ 0.25 for size range 0.5-2.0
    metrics.speedVariance = std::min(1.0f, varSpeed / 25.0f);  // Max variance ≈ 25 for speed range 5-20
    metrics.colorVariance = std::min(1.0f, varColor);
    metrics.morphologyVariance = std::min(1.0f, varMorphology);

    // Overall diversity score (weighted average)
    metrics.overallDiversity = (
        metrics.sizeVariance * 0.2f +
        metrics.speedVariance * 0.2f +
        metrics.colorVariance * 0.3f +
        metrics.morphologyVariance * 0.3f
    );

    return metrics;
}

void Genome::adaptToChemistry(const PlanetChemistry& chemistry) {
    // Adapt solvent affinity to planet's primary solvent
    switch (chemistry.solventType) {
        case SolventType::WATER:
            solventAffinity = Random::range(0.4f, 0.6f);
            break;
        case SolventType::AMMONIA:
            solventAffinity = Random::range(0.1f, 0.25f);
            membraneFluidity = Random::range(0.15f, 0.35f);  // Rigid for cold
            break;
        case SolventType::METHANE:
            solventAffinity = Random::range(0.0f, 0.15f);
            membraneFluidity = Random::range(0.05f, 0.25f);  // Very rigid
            break;
        case SolventType::SULFURIC_ACID:
            solventAffinity = Random::range(0.75f, 0.95f);
            pHPreference = Random::range(0.0f, 0.25f);  // Acidophile
            mineralizationBias = Random::range(0.5f, 0.8f);  // Protection
            break;
        case SolventType::ETHANOL:
            solventAffinity = Random::range(0.7f, 0.9f);
            break;
        default:
            break;
    }

    // Adapt oxygen tolerance to atmospheric oxygen
    float atmOxygen = chemistry.atmosphere.oxygen;
    if (atmOxygen < 0.1f) {
        // Low oxygen - anaerobic preference
        oxygenTolerance = Random::range(0.0f, 0.3f);
        metabolicPathway = (Random::chance(0.7f)) ? 1 : 2;  // Anaerobic or chemosynthesis
    } else if (atmOxygen > 0.3f) {
        // High oxygen - highly aerobic
        oxygenTolerance = Random::range(0.7f, 1.0f);
        metabolicPathway = 0;  // Aerobic
    } else {
        // Normal oxygen range
        oxygenTolerance = Random::range(0.4f, 0.8f);
    }

    // Adapt membrane fluidity to temperature
    float tempNorm = (chemistry.temperatureBase + 50.0f) / 200.0f;
    tempNorm = std::clamp(tempNorm, 0.0f, 1.0f);
    membraneFluidity = std::clamp(tempNorm + Random::range(-0.15f, 0.15f), 0.0f, 1.0f);

    // Adapt temperature tolerance
    temperatureTolerance = Random::range(15.0f, 30.0f);
    if (chemistry.temperatureRange > 80.0f) {
        // High variability requires wider tolerance
        temperatureTolerance = Random::range(30.0f, 45.0f);
    }

    // Adapt radiation resistance
    if (chemistry.radiationLevel > 1.3f) {
        radiationResistance = Random::range(0.4f, 0.8f);
        // High radiation often selects for carotenoid/melanin pigments
        if (Random::chance(0.5f)) {
            biopigmentFamily = Random::chance(0.5f) ? 1 : 4;  // Carotenoid or melanin
        }
    } else {
        radiationResistance = Random::range(0.1f, 0.4f);
    }

    // Adapt pH preference to environmental acidity
    if (chemistry.acidity < 5.0f) {
        pHPreference = Random::range(0.0f, 0.3f);  // Acidophile
    } else if (chemistry.acidity > 9.0f) {
        pHPreference = Random::range(0.7f, 1.0f);  // Alkaliphile
    } else {
        pHPreference = Random::range(0.35f, 0.65f);  // Neutrophile
    }

    // Adapt mineralization based on mineral availability
    float mineralAvail = (chemistry.minerals.calcium + chemistry.minerals.silicon +
                          chemistry.minerals.iron + chemistry.minerals.phosphorus) / 4.0f;
    mineralizationBias = Random::range(0.1f, mineralAvail * 0.9f + 0.1f);

    // Select appropriate biopigment family based on chemistry
    if (chemistry.atmosphere.oxygen < 0.1f && chemistry.minerals.sulfur > 0.5f) {
        // Sulfur-rich, low oxygen: flavin pigments likely
        if (Random::chance(0.4f)) biopigmentFamily = 5;
    }
    if (chemistry.radiationLevel < 0.5f) {
        // Low light: phycocyanin or bacteriorhodopsin
        if (Random::chance(0.3f)) biopigmentFamily = Random::chance(0.5f) ? 2 : 3;
    }
}
