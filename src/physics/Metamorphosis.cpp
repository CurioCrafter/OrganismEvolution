#include "Metamorphosis.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// =============================================================================
// LIFE STAGE CONTROLLER IMPLEMENTATION
// =============================================================================

void LifeStageController::initialize(const MorphologyGenes& genes) {
    baseGenes = genes;

    // Determine metamorphosis type based on genes
    if (!genes.hasMetamorphosis) {
        metamorphosisType = MetamorphosisType::NONE;
        currentStage = LifeStage::ADULT;
    } else {
        // Choose metamorphosis type based on morphology
        if (genes.wingPairs > 0 && genes.legPairs >= 3) {
            // Insect-like: complete metamorphosis
            metamorphosisType = MetamorphosisType::COMPLETE;
        } else if (genes.finCount > 0 && genes.legPairs > 0) {
            // Amphibian-like: aquatic to land
            metamorphosisType = MetamorphosisType::AQUATIC_TO_LAND;
        } else if (genes.wingPairs > 0) {
            // Gradual wing development
            metamorphosisType = MetamorphosisType::GRADUAL;
        } else {
            metamorphosisType = MetamorphosisType::GRADUAL;
        }

        currentStage = LifeStage::LARVAL;
    }

    // Generate larval morphology
    adultMorphology = genes;
    switch (metamorphosisType) {
        case MetamorphosisType::COMPLETE:
            larvalMorphology = LarvalMorphology::generateCompleteLarval(genes);
            break;
        case MetamorphosisType::AQUATIC_TO_LAND:
            larvalMorphology = LarvalMorphology::generateAquaticLarval(genes);
            break;
        case MetamorphosisType::GRADUAL:
            larvalMorphology = LarvalMorphology::generateNymphMorphology(genes);
            break;
        default:
            larvalMorphology = genes;
            break;
    }

    // Set transformation duration based on adult size
    transformationDuration = 5.0f + genes.baseMass * 5.0f;

    calculateStageInfo();
}

void LifeStageController::update(float deltaTime, float energy, float health) {
    age += deltaTime;

    // Check for stage transitions
    if (!isMetamorphosing()) {
        checkForTransition(energy);
    }

    // Update transformation if in progress
    if (isMetamorphosing()) {
        updateTransformation(deltaTime);
    }

    // Update stage info
    stageInfo.ageInStage += deltaTime;
    calculateStageInfo();
}

MorphologyGenes LifeStageController::getCurrentMorphology() const {
    if (metamorphosisType == MetamorphosisType::NONE) {
        return baseGenes;
    }

    switch (currentStage) {
        case LifeStage::EGG:
            return larvalMorphology; // Preparing larval form

        case LifeStage::LARVAL:
            if (isMetamorphosing()) {
                return MorphologyInterpolator::interpolate(
                    larvalMorphology, adultMorphology, transformationProgress);
            }
            return larvalMorphology;

        case LifeStage::JUVENILE:
            // Intermediate form
            return MorphologyInterpolator::interpolate(
                larvalMorphology, adultMorphology, 0.5f);

        case LifeStage::ADULT:
        case LifeStage::ELDER:
        default:
            return adultMorphology;
    }
}

void LifeStageController::setStage(LifeStage stage) {
    LifeStage oldStage = currentStage;
    currentStage = stage;
    stageInfo.ageInStage = 0.0f;

    if (onStageChange && oldStage != stage) {
        onStageChange(oldStage, stage);
    }

    calculateStageInfo();
}

void LifeStageController::checkForTransition(float energy) {
    // Don't transition during metamorphosis
    if (isMetamorphosing()) return;

    switch (currentStage) {
        case LifeStage::EGG:
            // Hatch when enough time passes
            if (stageInfo.ageInStage > 5.0f) {
                transitionToStage(LifeStage::LARVAL);
            }
            break;

        case LifeStage::LARVAL:
            // Metamorphose when old enough and have enough energy
            if (baseGenes.hasMetamorphosis &&
                age >= baseGenes.metamorphosisAge &&
                energy > 50.0f) {
                // Start metamorphosis
                transformationStage = TransformationStage::INITIATING;
                transformationProgress = 0.0f;
            }
            // Direct transition if no metamorphosis
            else if (!baseGenes.hasMetamorphosis && stageInfo.ageInStage > 20.0f) {
                transitionToStage(LifeStage::JUVENILE);
            }
            break;

        case LifeStage::JUVENILE:
            // Become adult
            if (stageInfo.ageInStage > 30.0f) {
                transitionToStage(LifeStage::ADULT);
            }
            break;

        case LifeStage::ADULT:
            // Become elder at old age
            {
                float maxAge = 100.0f + baseGenes.baseMass * 50.0f;
                if (age > maxAge * 0.7f) {
                    transitionToStage(LifeStage::ELDER);
                }
            }
            break;

        case LifeStage::ELDER:
            // Stay elder until death
            break;
    }
}

void LifeStageController::updateTransformation(float deltaTime) {
    transformationProgress += deltaTime / transformationDuration;

    // Update transformation stage based on progress
    if (transformationProgress < 0.2f) {
        transformationStage = TransformationStage::INITIATING;
    } else if (transformationProgress < 0.6f) {
        transformationStage = TransformationStage::REORGANIZING;
    } else if (transformationProgress < 0.8f) {
        transformationStage = TransformationStage::EMERGING;
    } else if (transformationProgress < 1.0f) {
        transformationStage = TransformationStage::HARDENING;
    } else {
        // Transformation complete
        transformationStage = TransformationStage::COMPLETE;
        transformationProgress = 1.0f;
        transitionToStage(LifeStage::ADULT);
    }
}

void LifeStageController::calculateStageInfo() {
    stageInfo.stage = currentStage;

    switch (currentStage) {
        case LifeStage::EGG:
            stageInfo.sizeMultiplier = 0.1f;
            stageInfo.speedMultiplier = 0.0f;
            stageInfo.strengthMultiplier = 0.0f;
            stageInfo.canReproduce = false;
            break;

        case LifeStage::LARVAL:
            if (isMetamorphosing()) {
                // During metamorphosis, capabilities reduced
                stageInfo.sizeMultiplier = 0.3f + transformationProgress * 0.4f;
                stageInfo.speedMultiplier = 0.2f * (1.0f - transformationProgress * 0.8f);
                stageInfo.strengthMultiplier = 0.2f;
            } else {
                stageInfo.sizeMultiplier = baseGenes.hasMetamorphosis ?
                    1.0f / baseGenes.adultSizeMultiplier : 0.5f;
                stageInfo.speedMultiplier = baseGenes.larvalSpeedBonus;
                stageInfo.strengthMultiplier = 0.3f;
            }
            stageInfo.canReproduce = false;
            break;

        case LifeStage::JUVENILE:
            stageInfo.sizeMultiplier = 0.7f;
            stageInfo.speedMultiplier = 1.1f; // Young and fast
            stageInfo.strengthMultiplier = 0.6f;
            stageInfo.canReproduce = false;
            break;

        case LifeStage::ADULT:
            stageInfo.sizeMultiplier = 1.0f;
            stageInfo.speedMultiplier = 1.0f;
            stageInfo.strengthMultiplier = 1.0f;
            stageInfo.canReproduce = true;
            break;

        case LifeStage::ELDER:
            stageInfo.sizeMultiplier = 1.0f;
            stageInfo.speedMultiplier = 0.8f;
            stageInfo.strengthMultiplier = 0.7f;
            stageInfo.canReproduce = false; // Or reduced fertility
            break;
    }
}

MorphologyGenes LifeStageController::generateLarvalMorphology(const MorphologyGenes& adult) {
    switch (metamorphosisType) {
        case MetamorphosisType::COMPLETE:
            return LarvalMorphology::generateCompleteLarval(adult);
        case MetamorphosisType::AQUATIC_TO_LAND:
            return LarvalMorphology::generateAquaticLarval(adult);
        case MetamorphosisType::GRADUAL:
            return LarvalMorphology::generateNymphMorphology(adult);
        default:
            return adult;
    }
}

void LifeStageController::transitionToStage(LifeStage newStage) {
    LifeStage oldStage = currentStage;
    currentStage = newStage;
    stageInfo.ageInStage = 0.0f;

    // Reset transformation state
    if (newStage == LifeStage::ADULT) {
        transformationStage = TransformationStage::COMPLETE;
    }

    if (onStageChange) {
        onStageChange(oldStage, newStage);
    }
}

// =============================================================================
// MORPHOLOGY INTERPOLATOR IMPLEMENTATION
// =============================================================================

MorphologyGenes MorphologyInterpolator::interpolate(
    const MorphologyGenes& from,
    const MorphologyGenes& to,
    float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    float smoothT = easeInOut(t);

    MorphologyGenes result;

    // Body organization
    result.symmetry = (t < 0.5f) ? from.symmetry : to.symmetry;
    result.segmentCount = lerpInt(from.segmentCount, to.segmentCount, smoothT);
    result.segmentTaper = lerp(from.segmentTaper, to.segmentTaper, smoothT);
    result.bodyLength = lerp(from.bodyLength, to.bodyLength, smoothT);
    result.bodyWidth = lerp(from.bodyWidth, to.bodyWidth, smoothT);
    result.bodyHeight = lerp(from.bodyHeight, to.bodyHeight, smoothT);

    // Limbs - these appear gradually
    float limbT = std::max(0.0f, (t - 0.3f) / 0.7f); // Limbs appear after 30%
    result.legPairs = lerpInt(from.legPairs, to.legPairs, limbT);
    result.legSegments = lerpInt(from.legSegments, to.legSegments, limbT);
    result.legLength = lerp(from.legLength, to.legLength, limbT);
    result.legThickness = lerp(from.legThickness, to.legThickness, limbT);
    result.legSpread = lerp(from.legSpread, to.legSpread, limbT);

    // Arms
    result.armPairs = lerpInt(from.armPairs, to.armPairs, limbT);
    result.armSegments = lerpInt(from.armSegments, to.armSegments, limbT);
    result.armLength = lerp(from.armLength, to.armLength, limbT);
    result.armThickness = lerp(from.armThickness, to.armThickness, limbT);
    result.hasHands = (limbT > 0.5f) ? to.hasHands : from.hasHands;

    // Wings - appear late
    float wingT = std::max(0.0f, (t - 0.6f) / 0.4f);
    result.wingPairs = lerpInt(from.wingPairs, to.wingPairs, wingT);
    result.wingSpan = lerp(from.wingSpan, to.wingSpan, wingT);
    result.wingChord = lerp(from.wingChord, to.wingChord, wingT);
    result.canFly = (wingT > 0.8f) && to.canFly;

    // Tail - changes throughout
    result.hasTail = (t < 0.5f) ? from.hasTail : to.hasTail;
    result.tailSegments = lerpInt(from.tailSegments, to.tailSegments, smoothT);
    result.tailLength = lerp(from.tailLength, to.tailLength, smoothT);
    result.tailThickness = lerp(from.tailThickness, to.tailThickness, smoothT);
    result.tailTaper = lerp(from.tailTaper, to.tailTaper, smoothT);

    // Fins - disappear early for land forms
    float finT = std::max(0.0f, 1.0f - t * 2.0f); // Fins disappear first
    if (to.finCount == 0 && from.finCount > 0) {
        result.finCount = static_cast<int>(from.finCount * finT);
        result.finSize = from.finSize * finT;
    } else {
        result.finCount = lerpInt(from.finCount, to.finCount, smoothT);
        result.finSize = lerp(from.finSize, to.finSize, smoothT);
    }
    result.hasDorsalFin = (finT > 0.5f) ? from.hasDorsalFin : to.hasDorsalFin;
    result.hasPectoralFins = (finT > 0.5f) ? from.hasPectoralFins : to.hasPectoralFins;
    result.hasCaudalFin = (t < 0.7f) ? from.hasCaudalFin : to.hasCaudalFin;

    // Head - gradual change
    result.headSize = lerp(from.headSize, to.headSize, smoothT);
    result.neckLength = lerp(from.neckLength, to.neckLength, smoothT);
    result.neckFlexibility = lerp(from.neckFlexibility, to.neckFlexibility, smoothT);
    result.eyeCount = lerpInt(from.eyeCount, to.eyeCount, smoothT);
    result.eyeSize = lerp(from.eyeSize, to.eyeSize, smoothT);
    result.eyesSideFacing = (t < 0.5f) ? from.eyesSideFacing : to.eyesSideFacing;

    // Joints
    result.primaryJointType = (t < 0.5f) ? from.primaryJointType : to.primaryJointType;
    result.jointFlexibility = lerp(from.jointFlexibility, to.jointFlexibility, smoothT);
    result.jointStrength = lerp(from.jointStrength, to.jointStrength, smoothT);
    result.jointDamping = lerp(from.jointDamping, to.jointDamping, smoothT);

    // Features - appear late
    float featureT = std::max(0.0f, (t - 0.7f) / 0.3f);
    result.primaryFeature = (featureT > 0.5f) ? to.primaryFeature : from.primaryFeature;
    result.secondaryFeature = (featureT > 0.7f) ? to.secondaryFeature : from.secondaryFeature;
    result.featureSize = lerp(from.featureSize, to.featureSize, featureT);
    result.armorCoverage = lerp(from.armorCoverage, to.armorCoverage, smoothT);

    // Allometry
    result.baseMass = lerp(from.baseMass, to.baseMass, smoothT);
    result.densityMultiplier = lerp(from.densityMultiplier, to.densityMultiplier, smoothT);
    result.metabolicMultiplier = lerp(from.metabolicMultiplier, to.metabolicMultiplier, smoothT);

    // Metamorphosis settings (keep adult's)
    result.hasMetamorphosis = to.hasMetamorphosis;
    result.metamorphosisAge = to.metamorphosisAge;
    result.larvalSpeedBonus = to.larvalSpeedBonus;
    result.adultSizeMultiplier = to.adultSizeMultiplier;

    return result;
}

float MorphologyInterpolator::easeInOut(float t) {
    return t * t * (3.0f - 2.0f * t);
}

float MorphologyInterpolator::easeOutElastic(float t) {
    if (t == 0 || t == 1) return t;

    float p = 0.3f;
    float s = p / 4.0f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * 3.14159f) / p) + 1.0f;
}

float MorphologyInterpolator::lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

int MorphologyInterpolator::lerpInt(int a, int b, float t) {
    return static_cast<int>(std::round(a + (b - a) * t));
}

// =============================================================================
// INSTAR CONTROLLER IMPLEMENTATION
// =============================================================================

void InstarController::initialize(const MorphologyGenes& genes, int numInstars) {
    adultGenes = genes;
    info.totalInstars = numInstars;
    info.currentInstar = 1;
    info.sizeMultiplier = calculateSizeForInstar(1);
    info.isFinalInstar = (numInstars == 1);

    // Energy needed to molt increases with each instar
    info.nextMoltEnergy = 30.0f + info.currentInstar * 20.0f;
}

bool InstarController::canMolt(float currentEnergy) const {
    if (info.isFinalInstar) return false;
    return currentEnergy >= info.nextMoltEnergy;
}

void InstarController::molt() {
    if (info.isFinalInstar) return;

    info.currentInstar++;
    info.sizeMultiplier = calculateSizeForInstar(info.currentInstar);
    info.isFinalInstar = (info.currentInstar >= info.totalInstars);
    info.nextMoltEnergy = 30.0f + info.currentInstar * 20.0f;
}

MorphologyGenes InstarController::getCurrentMorphology() const {
    // Nymph morphology scaled by instar
    MorphologyGenes morph = LarvalMorphology::generateNymphMorphology(adultGenes);

    float progress = static_cast<float>(info.currentInstar) / info.totalInstars;

    // Interpolate towards adult
    morph = MorphologyInterpolator::interpolate(morph, adultGenes, progress);

    // Apply size multiplier
    morph.baseMass *= info.sizeMultiplier;
    morph.bodyLength *= std::pow(info.sizeMultiplier, 0.33f);
    morph.bodyWidth *= std::pow(info.sizeMultiplier, 0.33f);
    morph.bodyHeight *= std::pow(info.sizeMultiplier, 0.33f);

    return morph;
}

float InstarController::calculateSizeForInstar(int instar) const {
    // Exponential growth through instars
    // First instar is ~20% of adult size, final is ~80%
    float progress = static_cast<float>(instar) / info.totalInstars;
    return 0.2f + 0.6f * progress * progress; // Quadratic growth
}

// =============================================================================
// AMPHIBIOUS TRANSITION CONTROLLER IMPLEMENTATION (PHASE 7 - Agent 3)
// =============================================================================

void AmphibiousTransitionController::initialize(
    float initialLungCapacity,
    float initialLimbStrength,
    AmphibiousStage startingStage)
{
    m_state.reset();
    m_state.currentStage = startingStage;
    m_state.lungCapacity = std::clamp(initialLungCapacity, 0.0f, 1.0f);
    m_state.limbStrength = std::clamp(initialLimbStrength, 0.0f, 1.0f);

    // Set initial traits based on starting stage
    switch (startingStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            m_state.skinMoisture = 1.0f;
            m_state.aquaticAffinity = 1.0f;
            break;
        case AmphibiousStage::TRANSITIONING:
            m_state.skinMoisture = 0.8f;
            m_state.aquaticAffinity = 0.7f;
            m_state.transitionProgress = 0.5f;
            break;
        case AmphibiousStage::AMPHIBIOUS:
            m_state.skinMoisture = 0.5f;
            m_state.aquaticAffinity = 0.5f;
            m_state.lungCapacity = std::max(m_state.lungCapacity, 0.5f);
            m_state.limbStrength = std::max(m_state.limbStrength, 0.4f);
            break;
        case AmphibiousStage::LAND_ADAPTED:
            m_state.skinMoisture = 0.2f;
            m_state.aquaticAffinity = 0.2f;
            m_state.lungCapacity = std::max(m_state.lungCapacity, 0.8f);
            m_state.limbStrength = std::max(m_state.limbStrength, 0.7f);
            break;
    }
}

void AmphibiousTransitionController::initializeAsAmphibian() {
    // Amphibians start in the AMPHIBIOUS stage with balanced traits
    initialize(0.6f, 0.5f, AmphibiousStage::AMPHIBIOUS);
    m_state.transitionProgress = 0.5f;  // Middle of stage
}

AmphibiousUpdateResult AmphibiousTransitionController::update(
    float deltaTime,
    EnvironmentZone currentZone,
    float waterDepth,
    float distanceToShore,
    float oxygenAvailability,
    float totalAge)
{
    AmphibiousUpdateResult result;
    result.previousStage = m_state.currentStage;

    // Update cooldown timer
    if (m_state.stageChangeCooldown > 0.0f) {
        m_state.stageChangeCooldown -= deltaTime;
    }

    // Update exposure timers based on current zone
    updateExposureTimers(deltaTime, currentZone);

    // Develop traits based on environment (gradual adaptation)
    updateTraitDevelopment(deltaTime, currentZone);

    // Calculate environmental penalties (stress, energy drain, damage)
    calculateEnvironmentalPenalties(deltaTime, currentZone, result);

    // Check for stage transitions (only if cooldown expired)
    if (m_state.stageChangeCooldown <= 0.0f) {
        bool advanced = checkTransitionToNextStage();
        if (!advanced) {
            checkRegressionToPreviousStage();
        }
    }

    // Check if stage changed
    if (m_state.currentStage != result.previousStage) {
        result.stageChanged = true;
        result.newStage = m_state.currentStage;
        m_state.stageChangeCooldown = 60.0f;  // 1 minute cooldown between changes
        m_state.lastStageChangeTime = totalAge;

        if (m_state.debugLogEnabled) {
            result.debugMessage = "[AMPHIBIOUS] Stage change: " +
                std::string(getAmphibiousStageName(result.previousStage)) + " -> " +
                std::string(getAmphibiousStageName(result.newStage)) +
                " (Progress: " + std::to_string(m_state.transitionProgress) + ")";
        }
    }

    return result;
}

void AmphibiousTransitionController::updateExposureTimers(float deltaTime, EnvironmentZone zone) {
    // Reset continuous timers when changing zones
    switch (zone) {
        case EnvironmentZone::DEEP_WATER:
            m_state.timeInDeepWater += deltaTime;
            m_state.timeSubmerged += deltaTime;
            m_state.timeOnLand = 0.0f;  // Reset land timer
            break;

        case EnvironmentZone::SHALLOW_WATER:
            m_state.timeNearShore += deltaTime;
            m_state.timeSubmerged += deltaTime;
            m_state.timeOnLand = 0.0f;
            break;

        case EnvironmentZone::SHORE:
            m_state.timeNearShore += deltaTime;
            // Shore counts partially for both
            m_state.timeSubmerged = std::max(0.0f, m_state.timeSubmerged - deltaTime * 0.5f);
            m_state.timeOnLand += deltaTime * 0.5f;
            break;

        case EnvironmentZone::LAND:
            m_state.timeOnLand += deltaTime;
            m_state.timeSubmerged = 0.0f;
            // Slowly decay near-shore time when fully on land
            m_state.timeNearShore = std::max(0.0f, m_state.timeNearShore - deltaTime * 0.1f);
            break;
    }
}

void AmphibiousTransitionController::updateTraitDevelopment(float deltaTime, EnvironmentZone zone) {
    // Trait development rates (per second)
    const float lungDevelopmentRate = 0.001f;   // Slow lung development
    const float limbDevelopmentRate = 0.0015f;  // Slightly faster limb development
    const float moistureAdaptRate = 0.002f;     // Skin moisture adaptation
    const float affinityChangeRate = 0.001f;    // Very slow affinity change

    switch (zone) {
        case EnvironmentZone::DEEP_WATER:
            // Deep water maintains aquatic traits, regresses land adaptations
            m_state.skinMoisture = std::min(1.0f, m_state.skinMoisture + moistureAdaptRate * deltaTime);
            m_state.aquaticAffinity = std::min(1.0f, m_state.aquaticAffinity + affinityChangeRate * deltaTime);
            // Slight regression of land traits if not used
            m_state.limbStrength = std::max(0.0f, m_state.limbStrength - limbDevelopmentRate * 0.5f * deltaTime);
            break;

        case EnvironmentZone::SHALLOW_WATER:
            // Shallow water allows gradual adaptation
            // Can develop lungs by surfacing
            m_state.lungCapacity = std::min(1.0f, m_state.lungCapacity + lungDevelopmentRate * 0.5f * deltaTime);
            // Maintains existing traits
            break;

        case EnvironmentZone::SHORE:
            // Shore is the optimal zone for transition development
            m_state.lungCapacity = std::min(1.0f, m_state.lungCapacity + lungDevelopmentRate * deltaTime);
            m_state.limbStrength = std::min(1.0f, m_state.limbStrength + limbDevelopmentRate * deltaTime);
            m_state.skinMoisture = std::max(0.0f, m_state.skinMoisture - moistureAdaptRate * 0.5f * deltaTime);
            m_state.aquaticAffinity = std::max(0.0f, m_state.aquaticAffinity - affinityChangeRate * deltaTime);
            break;

        case EnvironmentZone::LAND:
            // Land forces rapid adaptation (or death for unadapted)
            if (m_state.currentStage != AmphibiousStage::FULLY_AQUATIC) {
                m_state.lungCapacity = std::min(1.0f, m_state.lungCapacity + lungDevelopmentRate * 2.0f * deltaTime);
                m_state.limbStrength = std::min(1.0f, m_state.limbStrength + limbDevelopmentRate * 2.0f * deltaTime);
                m_state.skinMoisture = std::max(0.0f, m_state.skinMoisture - moistureAdaptRate * deltaTime);
                m_state.aquaticAffinity = std::max(0.0f, m_state.aquaticAffinity - affinityChangeRate * 2.0f * deltaTime);
            }
            break;
    }

    // Update transition progress based on trait development
    float targetProgress = 0.0f;
    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            // Progress toward transitioning based on lung/limb development
            targetProgress = (m_state.lungCapacity + m_state.limbStrength) / 2.0f /
                            m_thresholds.lungCapacityToTransition;
            break;

        case AmphibiousStage::TRANSITIONING:
            targetProgress = (m_state.lungCapacity - m_thresholds.lungCapacityToTransition) /
                            (m_thresholds.lungCapacityToAmphibious - m_thresholds.lungCapacityToTransition);
            break;

        case AmphibiousStage::AMPHIBIOUS:
            targetProgress = (m_state.lungCapacity - m_thresholds.lungCapacityToAmphibious) /
                            (m_thresholds.lungCapacityToLand - m_thresholds.lungCapacityToAmphibious);
            break;

        case AmphibiousStage::LAND_ADAPTED:
            targetProgress = 1.0f - m_state.aquaticAffinity;
            break;
    }

    // Smooth progress changes
    m_state.transitionProgress = std::clamp(targetProgress, 0.0f, 1.0f);
}

void AmphibiousTransitionController::calculateEnvironmentalPenalties(
    float deltaTime,
    EnvironmentZone zone,
    AmphibiousUpdateResult& result)
{
    result.energyDrain = 0.0f;
    result.speedPenalty = 0.0f;
    result.healthDamage = 0.0f;

    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            if (zone == EnvironmentZone::LAND) {
                // Fully aquatic creatures die quickly on land
                float landTime = m_state.timeOnLand;
                float survivalRatio = landTime / m_thresholds.maxLandTimeAquatic;

                m_state.environmentalStress = std::min(1.0f, survivalRatio);
                result.energyDrain = 5.0f * deltaTime * survivalRatio;  // Heavy energy drain
                result.speedPenalty = 0.8f;  // 80% speed reduction

                if (survivalRatio > 0.5f) {
                    result.healthDamage = 10.0f * deltaTime * (survivalRatio - 0.5f) * 2.0f;
                }
            } else if (zone == EnvironmentZone::SHORE) {
                // Some stress at shore but survivable
                m_state.environmentalStress = 0.3f;
                result.energyDrain = 1.0f * deltaTime;
                result.speedPenalty = 0.3f;
            } else {
                m_state.environmentalStress = 0.0f;
            }
            break;

        case AmphibiousStage::TRANSITIONING:
            if (zone == EnvironmentZone::LAND) {
                // Can survive briefly on land with penalties
                float landTime = m_state.timeOnLand;
                float maxTime = m_thresholds.maxLandTimeAquatic * 3.0f;  // 3x survival time
                float survivalRatio = landTime / maxTime;

                m_state.environmentalStress = std::min(1.0f, survivalRatio);
                result.energyDrain = 2.0f * deltaTime * survivalRatio;
                result.speedPenalty = 0.5f;  // 50% speed reduction

                if (survivalRatio > 0.7f) {
                    result.healthDamage = 5.0f * deltaTime * (survivalRatio - 0.7f) * 3.0f;
                }
            } else if (zone == EnvironmentZone::DEEP_WATER) {
                // Slight inefficiency in deep water as adapting to surface
                result.energyDrain = 0.5f * deltaTime;
            }
            break;

        case AmphibiousStage::AMPHIBIOUS:
            // Amphibious can handle both environments with minor penalties
            if (zone == EnvironmentZone::LAND) {
                // Needs to stay moist
                if (m_state.skinMoisture > 0.5f) {
                    result.energyDrain = 1.0f * deltaTime;  // Extra energy to maintain moisture
                }
            } else if (zone == EnvironmentZone::DEEP_WATER) {
                // Less efficient swimmer than fully aquatic
                result.speedPenalty = 0.2f;
            }
            m_state.environmentalStress = 0.1f;  // Low baseline stress
            break;

        case AmphibiousStage::LAND_ADAPTED:
            if (zone == EnvironmentZone::DEEP_WATER || zone == EnvironmentZone::SHALLOW_WATER) {
                // Land creatures struggle underwater
                float submergedTime = m_state.timeSubmerged;
                float survivalRatio = submergedTime / m_thresholds.maxWaterTimeLand;

                m_state.environmentalStress = std::min(1.0f, survivalRatio);
                result.speedPenalty = 0.4f;  // 40% speed reduction
                result.energyDrain = 2.0f * deltaTime * survivalRatio;

                if (survivalRatio > 0.5f) {
                    // Drowning damage
                    result.healthDamage = 8.0f * deltaTime * (survivalRatio - 0.5f) * 2.0f;
                }
            } else {
                m_state.environmentalStress = 0.0f;
            }
            break;
    }
}

bool AmphibiousTransitionController::checkTransitionToNextStage() {
    if (!meetsTraitThresholds(static_cast<AmphibiousStage>(static_cast<int>(m_state.currentStage) + 1))) {
        return false;
    }

    if (!meetsExposureThresholds(static_cast<AmphibiousStage>(static_cast<int>(m_state.currentStage) + 1))) {
        return false;
    }

    // Advance to next stage
    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            m_state.currentStage = AmphibiousStage::TRANSITIONING;
            m_state.transitionProgress = 0.0f;
            return true;

        case AmphibiousStage::TRANSITIONING:
            m_state.currentStage = AmphibiousStage::AMPHIBIOUS;
            m_state.transitionProgress = 0.0f;
            return true;

        case AmphibiousStage::AMPHIBIOUS:
            m_state.currentStage = AmphibiousStage::LAND_ADAPTED;
            m_state.transitionProgress = 0.0f;
            return true;

        case AmphibiousStage::LAND_ADAPTED:
            // Already at final stage
            return false;
    }

    return false;
}

bool AmphibiousTransitionController::checkRegressionToPreviousStage() {
    // Regression happens when spending too long in wrong environment
    // and traits regress below threshold

    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            // Cannot regress further
            return false;

        case AmphibiousStage::TRANSITIONING:
            // Regress if lung capacity drops too low
            if (m_state.lungCapacity < m_thresholds.lungCapacityToTransition * 0.8f &&
                m_state.timeInDeepWater > 600.0f) {  // 10 minutes in deep water
                m_state.currentStage = AmphibiousStage::FULLY_AQUATIC;
                m_state.transitionProgress = 1.0f;  // Full progress in previous stage
                return true;
            }
            break;

        case AmphibiousStage::AMPHIBIOUS:
            // Regress if spending too much time fully submerged
            if (m_state.limbStrength < m_thresholds.limbStrengthToAmphibious * 0.8f &&
                m_state.timeSubmerged > 1200.0f) {  // 20 minutes submerged
                m_state.currentStage = AmphibiousStage::TRANSITIONING;
                m_state.transitionProgress = 1.0f;
                return true;
            }
            break;

        case AmphibiousStage::LAND_ADAPTED:
            // Regress if spending too much time in water
            if (m_state.aquaticAffinity > m_thresholds.aquaticAffinityMaxLand * 1.5f &&
                m_state.timeSubmerged > 600.0f) {  // 10 minutes in water
                m_state.currentStage = AmphibiousStage::AMPHIBIOUS;
                m_state.transitionProgress = 1.0f;
                return true;
            }
            break;
    }

    return false;
}

bool AmphibiousTransitionController::meetsTraitThresholds(AmphibiousStage targetStage) const {
    switch (targetStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            return true;  // Always meets threshold for first stage

        case AmphibiousStage::TRANSITIONING:
            return m_state.lungCapacity >= m_thresholds.lungCapacityToTransition &&
                   m_state.limbStrength >= m_thresholds.limbStrengthToTransition;

        case AmphibiousStage::AMPHIBIOUS:
            return m_state.lungCapacity >= m_thresholds.lungCapacityToAmphibious &&
                   m_state.limbStrength >= m_thresholds.limbStrengthToAmphibious &&
                   m_state.skinMoisture <= (1.0f - m_thresholds.skinMoistureMinAmphibious);

        case AmphibiousStage::LAND_ADAPTED:
            return m_state.lungCapacity >= m_thresholds.lungCapacityToLand &&
                   m_state.limbStrength >= m_thresholds.limbStrengthToLand &&
                   m_state.skinMoisture <= m_thresholds.skinMoistureMinLand &&
                   m_state.aquaticAffinity <= m_thresholds.aquaticAffinityMaxLand;

        default:
            return false;
    }
}

bool AmphibiousTransitionController::meetsExposureThresholds(AmphibiousStage targetStage) const {
    switch (targetStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            return true;

        case AmphibiousStage::TRANSITIONING:
            return m_state.timeNearShore >= m_thresholds.shoreExposureToTransition;

        case AmphibiousStage::AMPHIBIOUS:
            return m_state.timeOnLand >= m_thresholds.landExposureToAmphibious;

        case AmphibiousStage::LAND_ADAPTED:
            return m_state.timeOnLand >= m_thresholds.landExposureToLandAdapted;

        default:
            return false;
    }
}

bool AmphibiousTransitionController::canSurviveInZone(EnvironmentZone zone) const {
    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            return zone != EnvironmentZone::LAND;

        case AmphibiousStage::TRANSITIONING:
            return true;  // Can survive anywhere briefly

        case AmphibiousStage::AMPHIBIOUS:
            return true;  // Can survive anywhere

        case AmphibiousStage::LAND_ADAPTED:
            // Can survive in water briefly but not indefinitely
            return true;

        default:
            return false;
    }
}

float AmphibiousTransitionController::getSpeedMultiplier(EnvironmentZone zone) const {
    float swimSkill = m_state.aquaticAffinity;
    float walkSkill = m_state.limbStrength;

    switch (zone) {
        case EnvironmentZone::DEEP_WATER:
        case EnvironmentZone::SHALLOW_WATER:
            // Swim speed based on aquatic affinity
            return 0.5f + swimSkill * 0.5f;

        case EnvironmentZone::SHORE:
            // Blended speed
            return 0.5f + (swimSkill + walkSkill) * 0.25f;

        case EnvironmentZone::LAND:
            // Walk speed based on limb strength
            return 0.3f + walkSkill * 0.7f;

        default:
            return 1.0f;
    }
}

float AmphibiousTransitionController::getEnergyCostMultiplier(EnvironmentZone zone) const {
    // Movement in unfamiliar environments costs more energy
    switch (m_state.currentStage) {
        case AmphibiousStage::FULLY_AQUATIC:
            if (zone == EnvironmentZone::LAND) return 3.0f;
            if (zone == EnvironmentZone::SHORE) return 1.5f;
            return 1.0f;

        case AmphibiousStage::TRANSITIONING:
            if (zone == EnvironmentZone::LAND) return 2.0f;
            if (zone == EnvironmentZone::DEEP_WATER) return 1.3f;
            return 1.0f;

        case AmphibiousStage::AMPHIBIOUS:
            // Most efficient at shore, slightly less elsewhere
            if (zone == EnvironmentZone::SHORE) return 0.9f;
            return 1.1f;

        case AmphibiousStage::LAND_ADAPTED:
            if (zone == EnvironmentZone::DEEP_WATER) return 2.5f;
            if (zone == EnvironmentZone::SHALLOW_WATER) return 1.8f;
            return 1.0f;

        default:
            return 1.0f;
    }
}

void AmphibiousTransitionController::forceStage(AmphibiousStage stage) {
    AmphibiousStage oldStage = m_state.currentStage;
    m_state.currentStage = stage;
    m_state.transitionProgress = 0.5f;
    m_state.stageChangeCooldown = 0.0f;  // No cooldown for forced changes

    // Adjust traits to match forced stage
    switch (stage) {
        case AmphibiousStage::FULLY_AQUATIC:
            m_state.lungCapacity = 0.0f;
            m_state.limbStrength = 0.0f;
            m_state.skinMoisture = 1.0f;
            m_state.aquaticAffinity = 1.0f;
            break;

        case AmphibiousStage::TRANSITIONING:
            m_state.lungCapacity = std::max(m_state.lungCapacity, m_thresholds.lungCapacityToTransition);
            m_state.limbStrength = std::max(m_state.limbStrength, m_thresholds.limbStrengthToTransition);
            m_state.skinMoisture = 0.8f;
            m_state.aquaticAffinity = 0.7f;
            break;

        case AmphibiousStage::AMPHIBIOUS:
            m_state.lungCapacity = std::max(m_state.lungCapacity, m_thresholds.lungCapacityToAmphibious);
            m_state.limbStrength = std::max(m_state.limbStrength, m_thresholds.limbStrengthToAmphibious);
            m_state.skinMoisture = 0.5f;
            m_state.aquaticAffinity = 0.5f;
            break;

        case AmphibiousStage::LAND_ADAPTED:
            m_state.lungCapacity = std::max(m_state.lungCapacity, m_thresholds.lungCapacityToLand);
            m_state.limbStrength = std::max(m_state.limbStrength, m_thresholds.limbStrengthToLand);
            m_state.skinMoisture = 0.2f;
            m_state.aquaticAffinity = 0.2f;
            break;
    }

    if (m_state.debugLogEnabled) {
        std::cout << "[AMPHIBIOUS DEBUG] Forced stage change: "
                  << getAmphibiousStageName(oldStage) << " -> "
                  << getAmphibiousStageName(stage) << std::endl;
    }
}
