#include "SmallCreatures.h"
#include "ColonyBehavior.h"
#include "SmallCreaturePhysics.h"
#include "../../environment/Terrain.h"
#include "../../utils/SpatialGrid.h"
#include <algorithm>
#include <cmath>

namespace small {

// Static member initialization
std::atomic<uint32_t> SmallCreatureID::nextID{1};

// =============================================================================
// SmallCreatureGenome Implementation
// =============================================================================

SmallCreatureGenome::SmallCreatureGenome() {
    // Default values
    size = 1.0f;
    speed = 1.0f;
    metabolism = 1.0f;
    reproduction = 1.0f;
    lifespan = 1.0f;
    aggression = 0.5f;

    visionRange = 1.0f;
    visionAngle = 1.0f;
    smellRange = 1.0f;
    hearingRange = 1.0f;
    touchSensitivity = 1.0f;
    antennaeLength = 1.0f;

    socialStrength = 0.5f;
    territoriality = 0.5f;
    curiosity = 0.5f;
    fearResponse = 0.5f;
    nestingDrive = 0.5f;

    for (int i = 0; i < 8; ++i) {
        neuralWeights[i] = 0.0f;
    }

    colorR = 0.5f;
    colorG = 0.5f;
    colorB = 0.5f;
    flags = 0;
}

SmallCreatureGenome SmallCreatureGenome::crossover(const SmallCreatureGenome& a,
                                                    const SmallCreatureGenome& b,
                                                    std::mt19937& rng) {
    SmallCreatureGenome child;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    // 50/50 inheritance for each trait
    child.size = dist(rng) < 0.5f ? a.size : b.size;
    child.speed = dist(rng) < 0.5f ? a.speed : b.speed;
    child.metabolism = dist(rng) < 0.5f ? a.metabolism : b.metabolism;
    child.reproduction = dist(rng) < 0.5f ? a.reproduction : b.reproduction;
    child.lifespan = dist(rng) < 0.5f ? a.lifespan : b.lifespan;
    child.aggression = dist(rng) < 0.5f ? a.aggression : b.aggression;

    child.visionRange = dist(rng) < 0.5f ? a.visionRange : b.visionRange;
    child.visionAngle = dist(rng) < 0.5f ? a.visionAngle : b.visionAngle;
    child.smellRange = dist(rng) < 0.5f ? a.smellRange : b.smellRange;
    child.hearingRange = dist(rng) < 0.5f ? a.hearingRange : b.hearingRange;
    child.touchSensitivity = dist(rng) < 0.5f ? a.touchSensitivity : b.touchSensitivity;
    child.antennaeLength = dist(rng) < 0.5f ? a.antennaeLength : b.antennaeLength;

    child.socialStrength = dist(rng) < 0.5f ? a.socialStrength : b.socialStrength;
    child.territoriality = dist(rng) < 0.5f ? a.territoriality : b.territoriality;
    child.curiosity = dist(rng) < 0.5f ? a.curiosity : b.curiosity;
    child.fearResponse = dist(rng) < 0.5f ? a.fearResponse : b.fearResponse;
    child.nestingDrive = dist(rng) < 0.5f ? a.nestingDrive : b.nestingDrive;

    for (int i = 0; i < 8; ++i) {
        child.neuralWeights[i] = dist(rng) < 0.5f ? a.neuralWeights[i] : b.neuralWeights[i];
    }

    // Blend colors
    child.colorR = (a.colorR + b.colorR) * 0.5f;
    child.colorG = (a.colorG + b.colorG) * 0.5f;
    child.colorB = (a.colorB + b.colorB) * 0.5f;

    child.flags = dist(rng) < 0.5f ? a.flags : b.flags;

    return child;
}

void SmallCreatureGenome::mutate(float rate, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::normal_distribution<float> mutation(0.0f, 0.1f);

    auto maybeMutate = [&](float& value, float min, float max) {
        if (dist(rng) < rate) {
            value += mutation(rng);
            value = std::clamp(value, min, max);
        }
    };

    maybeMutate(size, 0.5f, 2.0f);
    maybeMutate(speed, 0.5f, 2.0f);
    maybeMutate(metabolism, 0.5f, 2.0f);
    maybeMutate(reproduction, 0.5f, 2.0f);
    maybeMutate(lifespan, 0.5f, 2.0f);
    maybeMutate(aggression, 0.0f, 1.0f);

    maybeMutate(visionRange, 0.1f, 3.0f);
    maybeMutate(visionAngle, 0.3f, 1.5f);
    maybeMutate(smellRange, 0.1f, 3.0f);
    maybeMutate(hearingRange, 0.1f, 3.0f);
    maybeMutate(touchSensitivity, 0.1f, 2.0f);
    maybeMutate(antennaeLength, 0.1f, 2.0f);

    maybeMutate(socialStrength, 0.0f, 1.0f);
    maybeMutate(territoriality, 0.0f, 1.0f);
    maybeMutate(curiosity, 0.0f, 1.0f);
    maybeMutate(fearResponse, 0.0f, 1.0f);
    maybeMutate(nestingDrive, 0.0f, 1.0f);

    for (int i = 0; i < 8; ++i) {
        maybeMutate(neuralWeights[i], -2.0f, 2.0f);
    }

    maybeMutate(colorR, 0.0f, 1.0f);
    maybeMutate(colorG, 0.0f, 1.0f);
    maybeMutate(colorB, 0.0f, 1.0f);
}

void SmallCreatureGenome::initializeForType(SmallCreatureType type, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::normal_distribution<float> norm(0.0f, 0.1f);

    auto props = getProperties(type);

    // Base size from properties with variation
    float sizeRange = props.maxSize - props.minSize;
    size = props.minSize + dist(rng) * sizeRange;

    // Speed based on creature properties
    speed = props.baseSpeed * (0.8f + dist(rng) * 0.4f);

    // Metabolism
    metabolism = props.energyRate * (0.9f + dist(rng) * 0.2f);

    // Type-specific initializations
    if (isAnt(type)) {
        socialStrength = 0.9f + dist(rng) * 0.1f;
        smellRange = 2.0f + dist(rng);
        territoriality = 0.7f;
        colorR = 0.1f + dist(rng) * 0.2f;
        colorG = 0.05f + dist(rng) * 0.1f;
        colorB = 0.02f;
    }
    else if (isBee(type)) {
        socialStrength = 0.85f + dist(rng) * 0.15f;
        visionRange = 2.0f + dist(rng);
        colorR = 0.9f + dist(rng) * 0.1f;
        colorG = 0.7f + dist(rng) * 0.2f;
        colorB = 0.0f;
    }
    else if (isSpider(type)) {
        socialStrength = 0.1f + dist(rng) * 0.2f;
        touchSensitivity = 2.0f + dist(rng);
        aggression = 0.6f + dist(rng) * 0.3f;
        colorR = 0.2f + dist(rng) * 0.3f;
        colorG = 0.15f + dist(rng) * 0.2f;
        colorB = 0.1f + dist(rng) * 0.1f;
    }
    else if (isSmallMammal(type)) {
        fearResponse = 0.7f + dist(rng) * 0.3f;
        hearingRange = 2.0f + dist(rng);
        if (type == SmallCreatureType::SQUIRREL_TREE) {
            colorR = 0.6f + dist(rng) * 0.2f;
            colorG = 0.4f + dist(rng) * 0.2f;
            colorB = 0.2f + dist(rng) * 0.1f;
        } else if (type == SmallCreatureType::MOUSE || type == SmallCreatureType::RAT) {
            colorR = 0.5f + dist(rng) * 0.2f;
            colorG = 0.4f + dist(rng) * 0.2f;
            colorB = 0.3f + dist(rng) * 0.2f;
        }
    }
    else if (isAmphibian(type)) {
        if (type == SmallCreatureType::FROG || type == SmallCreatureType::TREE_FROG) {
            colorR = 0.1f + dist(rng) * 0.3f;
            colorG = 0.5f + dist(rng) * 0.4f;
            colorB = 0.1f + dist(rng) * 0.2f;
        }
    }

    // Initialize neural weights
    std::normal_distribution<float> weightDist(0.0f, 0.5f);
    for (int i = 0; i < 8; ++i) {
        neuralWeights[i] = weightDist(rng);
    }
}

// =============================================================================
// SmallCreatureProperties Lookup Table
// =============================================================================

namespace {
    SmallCreatureProperties propertiesTable[static_cast<int>(SmallCreatureType::COUNT)];
    bool propertiesInitialized = false;

    void initializeProperties() {
        if (propertiesInitialized) return;

        // ANT_WORKER
        auto& antWorker = propertiesTable[static_cast<int>(SmallCreatureType::ANT_WORKER)];
        antWorker.sizeCategory = SizeCategory::TINY;
        antWorker.primaryLocomotion = LocomotionType::WALKING;
        antWorker.secondaryLocomotion = LocomotionType::CLIMBING;
        antWorker.primaryHabitat = HabitatType::GROUND_SURFACE;
        antWorker.socialType = SocialType::COLONY;
        antWorker.minSize = 0.002f; antWorker.maxSize = 0.005f;
        antWorker.baseSpeed = 0.03f; antWorker.energyRate = 0.01f;
        antWorker.reproductionRate = 0.0f; // Only queen reproduces
        antWorker.canClimb = true; antWorker.hasExoskeleton = true;
        antWorker.isColonial = true;

        // ANT_SOLDIER (similar to worker but larger)
        auto& antSoldier = propertiesTable[static_cast<int>(SmallCreatureType::ANT_SOLDIER)];
        antSoldier = antWorker;
        antSoldier.minSize = 0.004f; antSoldier.maxSize = 0.008f;
        antSoldier.baseSpeed = 0.04f;

        // ANT_QUEEN
        auto& antQueen = propertiesTable[static_cast<int>(SmallCreatureType::ANT_QUEEN)];
        antQueen = antWorker;
        antQueen.minSize = 0.008f; antQueen.maxSize = 0.015f;
        antQueen.baseSpeed = 0.01f;
        antQueen.reproductionRate = 0.1f;

        // BEETLE_GROUND
        auto& beetle = propertiesTable[static_cast<int>(SmallCreatureType::BEETLE_GROUND)];
        beetle.sizeCategory = SizeCategory::TINY;
        beetle.primaryLocomotion = LocomotionType::WALKING;
        beetle.primaryHabitat = HabitatType::GROUND_SURFACE;
        beetle.socialType = SocialType::SOLITARY;
        beetle.minSize = 0.005f; beetle.maxSize = 0.03f;
        beetle.baseSpeed = 0.05f; beetle.energyRate = 0.02f;
        beetle.hasExoskeleton = true; beetle.canFly = true;

        // BEETLE_DUNG
        auto& dungBeetle = propertiesTable[static_cast<int>(SmallCreatureType::BEETLE_DUNG)];
        dungBeetle = beetle;
        dungBeetle.isDecomposer = true;

        // BEETLE_CARRION
        auto& carrionBeetle = propertiesTable[static_cast<int>(SmallCreatureType::BEETLE_CARRION)];
        carrionBeetle = beetle;
        carrionBeetle.isDecomposer = true;

        // EARTHWORM
        auto& worm = propertiesTable[static_cast<int>(SmallCreatureType::EARTHWORM)];
        worm.sizeCategory = SizeCategory::SMALL;
        worm.primaryLocomotion = LocomotionType::CRAWLING;
        worm.secondaryLocomotion = LocomotionType::BURROWING;
        worm.primaryHabitat = HabitatType::UNDERGROUND;
        worm.socialType = SocialType::SOLITARY;
        worm.minSize = 0.05f; worm.maxSize = 0.2f;
        worm.baseSpeed = 0.01f; worm.energyRate = 0.005f;
        worm.canBurrow = true; worm.isDecomposer = true;

        // CENTIPEDE
        auto& centipede = propertiesTable[static_cast<int>(SmallCreatureType::CENTIPEDE)];
        centipede.sizeCategory = SizeCategory::SMALL;
        centipede.primaryLocomotion = LocomotionType::CRAWLING;
        centipede.primaryHabitat = HabitatType::GROUND_SURFACE;
        centipede.socialType = SocialType::SOLITARY;
        centipede.minSize = 0.02f; centipede.maxSize = 0.15f;
        centipede.baseSpeed = 0.1f; centipede.energyRate = 0.03f;
        centipede.isPredator = true; centipede.isVenomous = true;
        centipede.hasExoskeleton = true;

        // MILLIPEDE
        auto& millipede = propertiesTable[static_cast<int>(SmallCreatureType::MILLIPEDE)];
        millipede = centipede;
        millipede.isPredator = false;
        millipede.isVenomous = false;
        millipede.isPoisonous = true;
        millipede.isDecomposer = true;
        millipede.baseSpeed = 0.03f;

        // CRICKET
        auto& cricket = propertiesTable[static_cast<int>(SmallCreatureType::CRICKET)];
        cricket.sizeCategory = SizeCategory::TINY;
        cricket.primaryLocomotion = LocomotionType::JUMPING;
        cricket.primaryHabitat = HabitatType::GRASS;
        cricket.socialType = SocialType::SOLITARY;
        cricket.minSize = 0.01f; cricket.maxSize = 0.03f;
        cricket.baseSpeed = 0.05f; cricket.energyRate = 0.02f;
        cricket.canJump = true; cricket.hasExoskeleton = true;
        cricket.isNocturnal = true;

        // GRASSHOPPER
        auto& grasshopper = propertiesTable[static_cast<int>(SmallCreatureType::GRASSHOPPER)];
        grasshopper = cricket;
        grasshopper.minSize = 0.02f; grasshopper.maxSize = 0.05f;
        grasshopper.canFly = true;

        // COCKROACH
        auto& roach = propertiesTable[static_cast<int>(SmallCreatureType::COCKROACH)];
        roach.sizeCategory = SizeCategory::TINY;
        roach.primaryLocomotion = LocomotionType::WALKING;
        roach.primaryHabitat = HabitatType::GROUND_SURFACE;
        roach.socialType = SocialType::SOLITARY;
        roach.minSize = 0.01f; roach.maxSize = 0.04f;
        roach.baseSpeed = 0.15f; roach.energyRate = 0.01f;
        roach.isNocturnal = true; roach.hasExoskeleton = true;
        roach.isDecomposer = true;

        // TERMITE_WORKER
        auto& termiteWorker = propertiesTable[static_cast<int>(SmallCreatureType::TERMITE_WORKER)];
        termiteWorker.sizeCategory = SizeCategory::TINY;
        termiteWorker.primaryLocomotion = LocomotionType::WALKING;
        termiteWorker.primaryHabitat = HabitatType::UNDERGROUND;
        termiteWorker.socialType = SocialType::COLONY;
        termiteWorker.minSize = 0.003f; termiteWorker.maxSize = 0.006f;
        termiteWorker.baseSpeed = 0.02f; termiteWorker.energyRate = 0.01f;
        termiteWorker.hasExoskeleton = true; termiteWorker.isColonial = true;
        termiteWorker.isDecomposer = true;

        // Flying insects
        // FLY
        auto& fly = propertiesTable[static_cast<int>(SmallCreatureType::FLY)];
        fly.sizeCategory = SizeCategory::TINY;
        fly.primaryLocomotion = LocomotionType::FLYING;
        fly.primaryHabitat = HabitatType::AERIAL;
        fly.socialType = SocialType::SWARM;
        fly.minSize = 0.003f; fly.maxSize = 0.01f;
        fly.baseSpeed = 0.5f; fly.energyRate = 0.03f;
        fly.canFly = true; fly.hasExoskeleton = true;
        fly.isDecomposer = true;

        // MOSQUITO
        auto& mosquito = propertiesTable[static_cast<int>(SmallCreatureType::MOSQUITO)];
        mosquito = fly;
        mosquito.minSize = 0.002f; mosquito.maxSize = 0.006f;
        mosquito.isParasite = true;

        // BUTTERFLY
        auto& butterfly = propertiesTable[static_cast<int>(SmallCreatureType::BUTTERFLY)];
        butterfly.sizeCategory = SizeCategory::SMALL;
        butterfly.primaryLocomotion = LocomotionType::FLYING;
        butterfly.primaryHabitat = HabitatType::AERIAL;
        butterfly.socialType = SocialType::SOLITARY;
        butterfly.minSize = 0.02f; butterfly.maxSize = 0.1f;
        butterfly.baseSpeed = 0.3f; butterfly.energyRate = 0.02f;
        butterfly.canFly = true; butterfly.hasExoskeleton = true;
        butterfly.isPollinator = true; butterfly.undergoesMorphosis = true;

        // MOTH
        auto& moth = propertiesTable[static_cast<int>(SmallCreatureType::MOTH)];
        moth = butterfly;
        moth.isNocturnal = true;

        // BEE_WORKER
        auto& beeWorker = propertiesTable[static_cast<int>(SmallCreatureType::BEE_WORKER)];
        beeWorker.sizeCategory = SizeCategory::TINY;
        beeWorker.primaryLocomotion = LocomotionType::FLYING;
        beeWorker.primaryHabitat = HabitatType::AERIAL;
        beeWorker.socialType = SocialType::COLONY;
        beeWorker.minSize = 0.01f; beeWorker.maxSize = 0.02f;
        beeWorker.baseSpeed = 0.6f; beeWorker.energyRate = 0.04f;
        beeWorker.canFly = true; beeWorker.hasExoskeleton = true;
        beeWorker.isColonial = true; beeWorker.isPollinator = true;
        beeWorker.isVenomous = true;

        // DRAGONFLY
        auto& dragonfly = propertiesTable[static_cast<int>(SmallCreatureType::DRAGONFLY)];
        dragonfly.sizeCategory = SizeCategory::SMALL;
        dragonfly.primaryLocomotion = LocomotionType::FLYING;
        dragonfly.primaryHabitat = HabitatType::AERIAL;
        dragonfly.socialType = SocialType::SOLITARY;
        dragonfly.minSize = 0.03f; dragonfly.maxSize = 0.1f;
        dragonfly.baseSpeed = 1.0f; dragonfly.energyRate = 0.05f;
        dragonfly.canFly = true; dragonfly.hasExoskeleton = true;
        dragonfly.isPredator = true;

        // Arachnids
        // SPIDER_ORB_WEAVER
        auto& orbWeaver = propertiesTable[static_cast<int>(SmallCreatureType::SPIDER_ORB_WEAVER)];
        orbWeaver.sizeCategory = SizeCategory::TINY;
        orbWeaver.primaryLocomotion = LocomotionType::WALKING;
        orbWeaver.secondaryLocomotion = LocomotionType::CLIMBING;
        orbWeaver.primaryHabitat = HabitatType::BUSH;
        orbWeaver.socialType = SocialType::SOLITARY;
        orbWeaver.minSize = 0.005f; orbWeaver.maxSize = 0.03f;
        orbWeaver.baseSpeed = 0.05f; orbWeaver.energyRate = 0.01f;
        orbWeaver.canClimb = true; orbWeaver.hasExoskeleton = true;
        orbWeaver.isPredator = true;

        // SPIDER_JUMPING
        auto& jumpingSpider = propertiesTable[static_cast<int>(SmallCreatureType::SPIDER_JUMPING)];
        jumpingSpider = orbWeaver;
        jumpingSpider.primaryLocomotion = LocomotionType::JUMPING;
        jumpingSpider.baseSpeed = 0.1f;
        jumpingSpider.canJump = true;

        // SPIDER_WOLF
        auto& wolfSpider = propertiesTable[static_cast<int>(SmallCreatureType::SPIDER_WOLF)];
        wolfSpider = orbWeaver;
        wolfSpider.primaryHabitat = HabitatType::GROUND_SURFACE;
        wolfSpider.minSize = 0.01f; wolfSpider.maxSize = 0.04f;
        wolfSpider.baseSpeed = 0.15f;

        // SCORPION
        auto& scorpion = propertiesTable[static_cast<int>(SmallCreatureType::SCORPION)];
        scorpion.sizeCategory = SizeCategory::SMALL;
        scorpion.primaryLocomotion = LocomotionType::WALKING;
        scorpion.primaryHabitat = HabitatType::GROUND_SURFACE;
        scorpion.socialType = SocialType::SOLITARY;
        scorpion.minSize = 0.03f; scorpion.maxSize = 0.15f;
        scorpion.baseSpeed = 0.1f; scorpion.energyRate = 0.02f;
        scorpion.hasExoskeleton = true; scorpion.isPredator = true;
        scorpion.isVenomous = true; scorpion.isNocturnal = true;

        // Small Mammals
        // MOUSE
        auto& mouse = propertiesTable[static_cast<int>(SmallCreatureType::MOUSE)];
        mouse.sizeCategory = SizeCategory::SMALL;
        mouse.primaryLocomotion = LocomotionType::WALKING;
        mouse.secondaryLocomotion = LocomotionType::CLIMBING;
        mouse.primaryHabitat = HabitatType::GROUND_SURFACE;
        mouse.socialType = SocialType::FAMILY;
        mouse.minSize = 0.05f; mouse.maxSize = 0.1f;
        mouse.baseSpeed = 0.5f; mouse.energyRate = 0.1f;
        mouse.canClimb = true; mouse.canJump = true;
        mouse.isNocturnal = true;

        // RAT
        auto& rat = propertiesTable[static_cast<int>(SmallCreatureType::RAT)];
        rat = mouse;
        rat.minSize = 0.1f; rat.maxSize = 0.25f;
        rat.baseSpeed = 0.6f;

        // SHREW
        auto& shrew = propertiesTable[static_cast<int>(SmallCreatureType::SHREW)];
        shrew = mouse;
        shrew.minSize = 0.03f; shrew.maxSize = 0.08f;
        shrew.isPredator = true; // Insectivore
        shrew.energyRate = 0.2f; // Very high metabolism

        // MOLE
        auto& mole = propertiesTable[static_cast<int>(SmallCreatureType::MOLE)];
        mole.sizeCategory = SizeCategory::SMALL;
        mole.primaryLocomotion = LocomotionType::BURROWING;
        mole.primaryHabitat = HabitatType::UNDERGROUND;
        mole.socialType = SocialType::SOLITARY;
        mole.minSize = 0.1f; mole.maxSize = 0.2f;
        mole.baseSpeed = 0.1f; mole.energyRate = 0.15f;
        mole.canBurrow = true; mole.isPredator = true;

        // SQUIRREL_TREE
        auto& squirrel = propertiesTable[static_cast<int>(SmallCreatureType::SQUIRREL_TREE)];
        squirrel.sizeCategory = SizeCategory::MEDIUM;
        squirrel.primaryLocomotion = LocomotionType::CLIMBING;
        squirrel.secondaryLocomotion = LocomotionType::JUMPING;
        squirrel.primaryHabitat = HabitatType::CANOPY;
        squirrel.socialType = SocialType::SOLITARY;
        squirrel.minSize = 0.15f; squirrel.maxSize = 0.3f;
        squirrel.baseSpeed = 0.8f; squirrel.energyRate = 0.12f;
        squirrel.canClimb = true; squirrel.canJump = true;

        // SQUIRREL_GROUND
        auto& groundSquirrel = propertiesTable[static_cast<int>(SmallCreatureType::SQUIRREL_GROUND)];
        groundSquirrel = squirrel;
        groundSquirrel.primaryLocomotion = LocomotionType::WALKING;
        groundSquirrel.primaryHabitat = HabitatType::GROUND_SURFACE;
        groundSquirrel.canBurrow = true;

        // RABBIT
        auto& rabbit = propertiesTable[static_cast<int>(SmallCreatureType::RABBIT)];
        rabbit.sizeCategory = SizeCategory::MEDIUM;
        rabbit.primaryLocomotion = LocomotionType::JUMPING;
        rabbit.primaryHabitat = HabitatType::GRASS;
        rabbit.socialType = SocialType::HERD;
        rabbit.minSize = 0.2f; rabbit.maxSize = 0.4f;
        rabbit.baseSpeed = 1.0f; rabbit.energyRate = 0.1f;
        rabbit.canJump = true; rabbit.canBurrow = true;

        // BAT_SMALL
        auto& batSmall = propertiesTable[static_cast<int>(SmallCreatureType::BAT_SMALL)];
        batSmall.sizeCategory = SizeCategory::SMALL;
        batSmall.primaryLocomotion = LocomotionType::FLYING;
        batSmall.primaryHabitat = HabitatType::AERIAL;
        batSmall.socialType = SocialType::COLONY;
        batSmall.minSize = 0.03f; batSmall.maxSize = 0.08f;
        batSmall.baseSpeed = 1.0f; batSmall.energyRate = 0.15f;
        batSmall.canFly = true; batSmall.isColonial = true;
        batSmall.isPredator = true; batSmall.isNocturnal = true;

        // Reptiles
        // LIZARD_SMALL
        auto& lizard = propertiesTable[static_cast<int>(SmallCreatureType::LIZARD_SMALL)];
        lizard.sizeCategory = SizeCategory::SMALL;
        lizard.primaryLocomotion = LocomotionType::WALKING;
        lizard.primaryHabitat = HabitatType::GROUND_SURFACE;
        lizard.socialType = SocialType::SOLITARY;
        lizard.minSize = 0.05f; lizard.maxSize = 0.15f;
        lizard.baseSpeed = 0.5f; lizard.energyRate = 0.03f;
        lizard.canClimb = true; lizard.isPredator = true;

        // GECKO
        auto& gecko = propertiesTable[static_cast<int>(SmallCreatureType::GECKO)];
        gecko = lizard;
        gecko.primaryHabitat = HabitatType::TREE_TRUNK;
        gecko.isNocturnal = true;

        // SNAKE_SMALL
        auto& smallSnake = propertiesTable[static_cast<int>(SmallCreatureType::SNAKE_SMALL)];
        smallSnake.sizeCategory = SizeCategory::SMALL;
        smallSnake.primaryLocomotion = LocomotionType::CRAWLING;
        smallSnake.primaryHabitat = HabitatType::GRASS;
        smallSnake.socialType = SocialType::SOLITARY;
        smallSnake.minSize = 0.2f; smallSnake.maxSize = 0.5f;
        smallSnake.baseSpeed = 0.3f; smallSnake.energyRate = 0.02f;
        smallSnake.isPredator = true; smallSnake.canSwim = true;

        // Amphibians
        // FROG
        auto& frog = propertiesTable[static_cast<int>(SmallCreatureType::FROG)];
        frog.sizeCategory = SizeCategory::SMALL;
        frog.primaryLocomotion = LocomotionType::JUMPING;
        frog.secondaryLocomotion = LocomotionType::SWIMMING;
        frog.primaryHabitat = HabitatType::WATER_SURFACE;
        frog.socialType = SocialType::SOLITARY;
        frog.minSize = 0.03f; frog.maxSize = 0.12f;
        frog.baseSpeed = 0.4f; frog.energyRate = 0.05f;
        frog.canJump = true; frog.canSwim = true;
        frog.isPredator = true; frog.undergoesMorphosis = true;

        // TREE_FROG
        auto& treeFrog = propertiesTable[static_cast<int>(SmallCreatureType::TREE_FROG)];
        treeFrog = frog;
        treeFrog.primaryHabitat = HabitatType::CANOPY;
        treeFrog.canClimb = true;
        treeFrog.isPoisonous = true;

        // TOAD
        auto& toad = propertiesTable[static_cast<int>(SmallCreatureType::TOAD)];
        toad = frog;
        toad.primaryHabitat = HabitatType::GROUND_SURFACE;
        toad.canSwim = false;
        toad.isPoisonous = true;

        // SALAMANDER
        auto& salamander = propertiesTable[static_cast<int>(SmallCreatureType::SALAMANDER)];
        salamander.sizeCategory = SizeCategory::SMALL;
        salamander.primaryLocomotion = LocomotionType::CRAWLING;
        salamander.primaryHabitat = HabitatType::UNDERGROUND;
        salamander.socialType = SocialType::SOLITARY;
        salamander.minSize = 0.05f; salamander.maxSize = 0.2f;
        salamander.baseSpeed = 0.1f; salamander.energyRate = 0.03f;
        salamander.canSwim = true; salamander.isPredator = true;
        salamander.isPoisonous = true;

        // Other invertebrates
        // SNAIL
        auto& snail = propertiesTable[static_cast<int>(SmallCreatureType::SNAIL)];
        snail.sizeCategory = SizeCategory::TINY;
        snail.primaryLocomotion = LocomotionType::CRAWLING;
        snail.primaryHabitat = HabitatType::GROUND_SURFACE;
        snail.socialType = SocialType::SOLITARY;
        snail.minSize = 0.01f; snail.maxSize = 0.05f;
        snail.baseSpeed = 0.001f; snail.energyRate = 0.005f;
        snail.hasExoskeleton = true;

        propertiesInitialized = true;
    }
}

const SmallCreatureProperties& getProperties(SmallCreatureType type) {
    initializeProperties();
    return propertiesTable[static_cast<int>(type)];
}

std::string getTypeName(SmallCreatureType type) {
    switch (type) {
        case SmallCreatureType::ANT_WORKER: return "Ant Worker";
        case SmallCreatureType::ANT_SOLDIER: return "Ant Soldier";
        case SmallCreatureType::ANT_QUEEN: return "Ant Queen";
        case SmallCreatureType::BEETLE_GROUND: return "Ground Beetle";
        case SmallCreatureType::BEETLE_DUNG: return "Dung Beetle";
        case SmallCreatureType::BEETLE_CARRION: return "Carrion Beetle";
        case SmallCreatureType::EARTHWORM: return "Earthworm";
        case SmallCreatureType::CENTIPEDE: return "Centipede";
        case SmallCreatureType::MILLIPEDE: return "Millipede";
        case SmallCreatureType::CRICKET: return "Cricket";
        case SmallCreatureType::GRASSHOPPER: return "Grasshopper";
        case SmallCreatureType::COCKROACH: return "Cockroach";
        case SmallCreatureType::TERMITE_WORKER: return "Termite Worker";
        case SmallCreatureType::TERMITE_SOLDIER: return "Termite Soldier";
        case SmallCreatureType::TERMITE_QUEEN: return "Termite Queen";
        case SmallCreatureType::FLY: return "Fly";
        case SmallCreatureType::MOSQUITO: return "Mosquito";
        case SmallCreatureType::GNAT: return "Gnat";
        case SmallCreatureType::BUTTERFLY: return "Butterfly";
        case SmallCreatureType::MOTH: return "Moth";
        case SmallCreatureType::BEE_WORKER: return "Bee Worker";
        case SmallCreatureType::BEE_DRONE: return "Bee Drone";
        case SmallCreatureType::BEE_QUEEN: return "Bee Queen";
        case SmallCreatureType::WASP: return "Wasp";
        case SmallCreatureType::HORNET: return "Hornet";
        case SmallCreatureType::DRAGONFLY: return "Dragonfly";
        case SmallCreatureType::DAMSELFLY: return "Damselfly";
        case SmallCreatureType::FIREFLY: return "Firefly";
        case SmallCreatureType::CICADA: return "Cicada";
        case SmallCreatureType::LOCUST: return "Locust";
        case SmallCreatureType::SPIDER_ORB_WEAVER: return "Orb Weaver Spider";
        case SmallCreatureType::SPIDER_JUMPING: return "Jumping Spider";
        case SmallCreatureType::SPIDER_WOLF: return "Wolf Spider";
        case SmallCreatureType::SPIDER_TRAPDOOR: return "Trapdoor Spider";
        case SmallCreatureType::SCORPION: return "Scorpion";
        case SmallCreatureType::TICK: return "Tick";
        case SmallCreatureType::MITE: return "Mite";
        case SmallCreatureType::HARVESTMAN: return "Harvestman";
        case SmallCreatureType::MOUSE: return "Mouse";
        case SmallCreatureType::RAT: return "Rat";
        case SmallCreatureType::SHREW: return "Shrew";
        case SmallCreatureType::VOLE: return "Vole";
        case SmallCreatureType::MOLE: return "Mole";
        case SmallCreatureType::SQUIRREL_GROUND: return "Ground Squirrel";
        case SmallCreatureType::SQUIRREL_TREE: return "Tree Squirrel";
        case SmallCreatureType::CHIPMUNK: return "Chipmunk";
        case SmallCreatureType::RABBIT: return "Rabbit";
        case SmallCreatureType::HARE: return "Hare";
        case SmallCreatureType::BAT_SMALL: return "Small Bat";
        case SmallCreatureType::BAT_LARGE: return "Large Bat";
        case SmallCreatureType::HEDGEHOG: return "Hedgehog";
        case SmallCreatureType::LIZARD_SMALL: return "Small Lizard";
        case SmallCreatureType::GECKO: return "Gecko";
        case SmallCreatureType::SKINK: return "Skink";
        case SmallCreatureType::CHAMELEON: return "Chameleon";
        case SmallCreatureType::SNAKE_SMALL: return "Small Snake";
        case SmallCreatureType::SNAKE_MEDIUM: return "Medium Snake";
        case SmallCreatureType::TURTLE_SMALL: return "Small Turtle";
        case SmallCreatureType::FROG: return "Frog";
        case SmallCreatureType::TOAD: return "Toad";
        case SmallCreatureType::TREE_FROG: return "Tree Frog";
        case SmallCreatureType::SALAMANDER: return "Salamander";
        case SmallCreatureType::NEWT: return "Newt";
        case SmallCreatureType::SNAIL: return "Snail";
        case SmallCreatureType::SLUG: return "Slug";
        case SmallCreatureType::CRAB_SMALL: return "Small Crab";
        case SmallCreatureType::CRAYFISH: return "Crayfish";
        default: return "Unknown";
    }
}

int getTrophicLevel(SmallCreatureType type) {
    auto props = getProperties(type);
    if (props.isDecomposer) return 1;
    if (props.isPredator) return 2;
    if (isSmallMammal(type)) {
        if (type == SmallCreatureType::SHREW) return 2;
        return 1; // Most are herbivores
    }
    return 1; // Default to primary consumer
}

bool canEat(SmallCreatureType predator, SmallCreatureType prey) {
    auto predProps = getProperties(predator);
    auto preyProps = getProperties(prey);

    if (!predProps.isPredator) return false;

    // Spiders eat insects
    if (isSpider(predator) && isInsect(prey)) return true;

    // Centipedes eat smaller invertebrates
    if (predator == SmallCreatureType::CENTIPEDE) {
        return isInsect(prey) || prey == SmallCreatureType::EARTHWORM;
    }

    // Scorpions eat insects and spiders
    if (predator == SmallCreatureType::SCORPION) {
        return isInsect(prey) || isSpider(prey);
    }

    // Dragonflies eat flying insects
    if (predator == SmallCreatureType::DRAGONFLY) {
        return isFlyingInsect(prey) && prey != SmallCreatureType::DRAGONFLY;
    }

    // Shrews eat insects
    if (predator == SmallCreatureType::SHREW) {
        return isInsect(prey);
    }

    // Lizards/geckos eat insects
    if (predator == SmallCreatureType::LIZARD_SMALL ||
        predator == SmallCreatureType::GECKO) {
        return isInsect(prey);
    }

    // Frogs eat insects
    if (isAmphibian(predator)) {
        return isInsect(prey);
    }

    // Small snakes eat mice, frogs
    if (predator == SmallCreatureType::SNAKE_SMALL) {
        return prey == SmallCreatureType::MOUSE ||
               prey == SmallCreatureType::FROG ||
               isInsect(prey);
    }

    // Bats eat flying insects
    if (predator == SmallCreatureType::BAT_SMALL) {
        return isFlyingInsect(prey);
    }

    return false;
}

bool isPreyForLargeCreatures(SmallCreatureType type) {
    // Most small creatures can be eaten by larger predators
    if (isSmallMammal(type)) return true;
    if (isReptile(type)) return true;
    if (isAmphibian(type)) return true;
    if (type == SmallCreatureType::DRAGONFLY) return true;
    if (type == SmallCreatureType::BUTTERFLY) return true;
    if (type == SmallCreatureType::MOTH) return true;
    return false;
}

// =============================================================================
// MicroSpatialGrid Implementation
// =============================================================================

MicroSpatialGrid::MicroSpatialGrid(float worldSize, float cellSize)
    : worldSize_(worldSize), cellSize_(cellSize) {
    gridDimension_ = static_cast<int>(std::ceil(worldSize / cellSize));
    cells_.resize(gridDimension_ * gridDimension_);
}

MicroSpatialGrid::~MicroSpatialGrid() = default;

void MicroSpatialGrid::clear() {
    for (auto& cell : cells_) {
        cell.creatures.clear();
        cell.food.clear();
        cell.pheromones.clear();
    }
}

int MicroSpatialGrid::getCellIndex(float x, float z) const {
    int cx = static_cast<int>((x + worldSize_ * 0.5f) / cellSize_);
    int cz = static_cast<int>((z + worldSize_ * 0.5f) / cellSize_);
    cx = std::clamp(cx, 0, gridDimension_ - 1);
    cz = std::clamp(cz, 0, gridDimension_ - 1);
    return cz * gridDimension_ + cx;
}

void MicroSpatialGrid::getCellRange(const XMFLOAT3& pos, float radius,
                                     int& minX, int& maxX, int& minZ, int& maxZ) const {
    float halfWorld = worldSize_ * 0.5f;
    minX = static_cast<int>((pos.x - radius + halfWorld) / cellSize_);
    maxX = static_cast<int>((pos.x + radius + halfWorld) / cellSize_);
    minZ = static_cast<int>((pos.z - radius + halfWorld) / cellSize_);
    maxZ = static_cast<int>((pos.z + radius + halfWorld) / cellSize_);

    minX = std::clamp(minX, 0, gridDimension_ - 1);
    maxX = std::clamp(maxX, 0, gridDimension_ - 1);
    minZ = std::clamp(minZ, 0, gridDimension_ - 1);
    maxZ = std::clamp(maxZ, 0, gridDimension_ - 1);
}

void MicroSpatialGrid::insert(SmallCreature* creature) {
    int idx = getCellIndex(creature->position.x, creature->position.z);
    cells_[idx].creatures.push_back(creature);
}

void MicroSpatialGrid::insertFood(MicroFood* food) {
    int idx = getCellIndex(food->position.x, food->position.z);
    cells_[idx].food.push_back(food);
}

void MicroSpatialGrid::insertPheromone(PheromonePoint* pheromone) {
    int idx = getCellIndex(pheromone->position.x, pheromone->position.z);
    cells_[idx].pheromones.push_back(pheromone);
}

std::vector<SmallCreature*> MicroSpatialGrid::queryCreatures(const XMFLOAT3& pos, float radius) const {
    std::vector<SmallCreature*> result;
    float radiusSq = radius * radius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, radius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* c : cell.creatures) {
                float dx = c->position.x - pos.x;
                float dz = c->position.z - pos.z;
                if (dx * dx + dz * dz <= radiusSq) {
                    result.push_back(c);
                }
            }
        }
    }
    return result;
}

std::vector<SmallCreature*> MicroSpatialGrid::queryByType(const XMFLOAT3& pos, float radius,
                                                          SmallCreatureType type) const {
    std::vector<SmallCreature*> result;
    float radiusSq = radius * radius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, radius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* c : cell.creatures) {
                if (c->type != type) continue;
                float dx = c->position.x - pos.x;
                float dz = c->position.z - pos.z;
                if (dx * dx + dz * dz <= radiusSq) {
                    result.push_back(c);
                }
            }
        }
    }
    return result;
}

std::vector<MicroFood*> MicroSpatialGrid::queryFood(const XMFLOAT3& pos, float radius) const {
    std::vector<MicroFood*> result;
    float radiusSq = radius * radius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, radius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* f : cell.food) {
                float dx = f->position.x - pos.x;
                float dz = f->position.z - pos.z;
                if (dx * dx + dz * dz <= radiusSq) {
                    result.push_back(f);
                }
            }
        }
    }
    return result;
}

std::vector<PheromonePoint*> MicroSpatialGrid::queryPheromones(const XMFLOAT3& pos, float radius,
                                                               PheromonePoint::Type filterType) const {
    std::vector<PheromonePoint*> result;
    float radiusSq = radius * radius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, radius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* p : cell.pheromones) {
                if (filterType != static_cast<PheromonePoint::Type>(-1) && p->type != filterType)
                    continue;
                float dx = p->position.x - pos.x;
                float dz = p->position.z - pos.z;
                if (dx * dx + dz * dz <= radiusSq) {
                    result.push_back(p);
                }
            }
        }
    }
    return result;
}

SmallCreature* MicroSpatialGrid::findNearest(const XMFLOAT3& pos, float maxRadius,
                                              std::function<bool(SmallCreature*)> filter) const {
    SmallCreature* nearest = nullptr;
    float nearestDistSq = maxRadius * maxRadius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, maxRadius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* c : cell.creatures) {
                if (filter && !filter(c)) continue;
                float dx = c->position.x - pos.x;
                float dz = c->position.z - pos.z;
                float distSq = dx * dx + dz * dz;
                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    nearest = c;
                }
            }
        }
    }
    return nearest;
}

int MicroSpatialGrid::countInRadius(const XMFLOAT3& pos, float radius) const {
    int count = 0;
    float radiusSq = radius * radius;

    int minX, maxX, minZ, maxZ;
    getCellRange(pos, radius, minX, maxX, minZ, maxZ);

    for (int z = minZ; z <= maxZ; ++z) {
        for (int x = minX; x <= maxX; ++x) {
            const auto& cell = cells_[z * gridDimension_ + x];
            for (auto* c : cell.creatures) {
                float dx = c->position.x - pos.x;
                float dz = c->position.z - pos.z;
                if (dx * dx + dz * dz <= radiusSq) {
                    ++count;
                }
            }
        }
    }
    return count;
}

// =============================================================================
// SmallCreatureManager Implementation
// =============================================================================

SmallCreatureManager::SmallCreatureManager(float worldSize)
    : worldSize_(worldSize)
    , largeCreatureGrid_(nullptr)
    , terrain_(nullptr)
    , rng_(std::random_device{}()) {
    spatialGrid_ = std::make_unique<MicroSpatialGrid>(worldSize, 2.0f);
    pheromoneSystem_ = std::make_unique<PheromoneSystem>(worldSize);
}

SmallCreatureManager::~SmallCreatureManager() = default;

void SmallCreatureManager::initialize(Terrain* terrain, int initialPopulation) {
    terrain_ = terrain;
    creatures_.reserve(maxCreatures_);
    genomes_.reserve(maxCreatures_);
    foodSources_.reserve(10000);

    std::uniform_real_distribution<float> posDist(-worldSize_ * 0.4f, worldSize_ * 0.4f);

    // Spawn initial population with variety
    int remaining = initialPopulation;

    // Ants (30%)
    int antCount = static_cast<int>(initialPopulation * 0.3f);
    for (int i = 0; i < antCount / 50; ++i) {
        XMFLOAT3 colonyPos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            colonyPos.y = terrain_->getHeight(colonyPos.x, colonyPos.z);
        }
        spawnColony(SmallCreatureType::ANT_WORKER, colonyPos, 50);
    }
    remaining -= antCount;

    // Beetles (10%)
    int beetleCount = static_cast<int>(initialPopulation * 0.1f);
    for (int i = 0; i < beetleCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }
        spawn(SmallCreatureType::BEETLE_GROUND, pos);
    }
    remaining -= beetleCount;

    // Butterflies/moths (5%)
    int butterflyCount = static_cast<int>(initialPopulation * 0.05f);
    for (int i = 0; i < butterflyCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 5.0f + posDist(rng_) * 0.1f, posDist(rng_) };
        spawn(SmallCreatureType::BUTTERFLY, pos);
    }
    remaining -= butterflyCount;

    // Spiders (8%)
    int spiderCount = static_cast<int>(initialPopulation * 0.08f);
    for (int i = 0; i < spiderCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }
        SmallCreatureType spiderType = (i % 3 == 0) ? SmallCreatureType::SPIDER_JUMPING :
                                       (i % 3 == 1) ? SmallCreatureType::SPIDER_WOLF :
                                                      SmallCreatureType::SPIDER_ORB_WEAVER;
        spawn(spiderType, pos);
    }
    remaining -= spiderCount;

    // Mice (7%)
    int miceCount = static_cast<int>(initialPopulation * 0.07f);
    for (int i = 0; i < miceCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }
        spawn(SmallCreatureType::MOUSE, pos);
    }
    remaining -= miceCount;

    // Squirrels (5%)
    int squirrelCount = static_cast<int>(initialPopulation * 0.05f);
    for (int i = 0; i < squirrelCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 3.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z) + 3.0f;
        }
        spawn(SmallCreatureType::SQUIRREL_TREE, pos);
    }
    remaining -= squirrelCount;

    // Frogs (5%)
    int frogCount = static_cast<int>(initialPopulation * 0.05f);
    for (int i = 0; i < frogCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }
        spawn(SmallCreatureType::FROG, pos);
    }
    remaining -= frogCount;

    // Earthworms (10%)
    int wormCount = static_cast<int>(initialPopulation * 0.1f);
    for (int i = 0; i < wormCount; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z) - 0.1f;
        }
        spawn(SmallCreatureType::EARTHWORM, pos);
    }
    remaining -= wormCount;

    // Fill rest with various insects
    for (int i = 0; i < remaining; ++i) {
        XMFLOAT3 pos = { posDist(rng_), 0.0f, posDist(rng_) };
        if (terrain_) {
            pos.y = terrain_->getHeight(pos.x, pos.z);
        }
        SmallCreatureType type;
        int r = i % 10;
        if (r < 3) type = SmallCreatureType::CRICKET;
        else if (r < 5) type = SmallCreatureType::GRASSHOPPER;
        else if (r < 7) type = SmallCreatureType::FLY;
        else if (r < 8) type = SmallCreatureType::DRAGONFLY;
        else if (r < 9) type = SmallCreatureType::CENTIPEDE;
        else type = SmallCreatureType::COCKROACH;
        spawn(type, pos);
    }
}

SmallCreature* SmallCreatureManager::spawn(SmallCreatureType type, const XMFLOAT3& position) {
    SmallCreatureGenome genome;
    genome.initializeForType(type, rng_);
    return spawnWithGenome(type, position, genome);
}

SmallCreature* SmallCreatureManager::spawnWithGenome(SmallCreatureType type, const XMFLOAT3& position,
                                                      const SmallCreatureGenome& genome) {
    if (creatures_.size() >= maxCreatures_) {
        // Reuse dead slot if available
        if (!deadIndices_.empty()) {
            size_t idx = deadIndices_.back();
            deadIndices_.pop_back();

            genomes_[idx] = genome;
            SmallCreature& c = creatures_[idx];
            c.id = SmallCreatureID::generate();
            c.type = type;
            c.stage = getProperties(type).undergoesMorphosis ? LifeStage::EGG : LifeStage::JUVENILE;
            c.flags = SmallCreature::FLAG_ALIVE;
            c.position = position;
            c.velocity = { 0, 0, 0 };
            c.targetPosition = position;
            c.rotation = 0.0f;
            c.energy = 100.0f;
            c.age = 0.0f;
            c.health = 100.0f;
            c.fear = 0.0f;
            c.hunger = 0.0f;
            c.matingUrge = 0.0f;
            c.colonyID = 0;
            c.nestID = 0;
            c.animationTime = 0.0f;
            c.animationSpeed = 1.0f;
            c.genome = &genomes_[idx];

            // Random sex
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            c.setMale(dist(rng_) < 0.5f);

            return &c;
        }
        return nullptr; // At capacity
    }

    genomes_.push_back(genome);
    creatures_.emplace_back();

    SmallCreature& c = creatures_.back();
    c.id = SmallCreatureID::generate();
    c.type = type;
    c.stage = getProperties(type).undergoesMorphosis ? LifeStage::EGG : LifeStage::JUVENILE;
    c.flags = SmallCreature::FLAG_ALIVE;
    c.position = position;
    c.velocity = { 0, 0, 0 };
    c.targetPosition = position;
    c.rotation = 0.0f;
    c.energy = 100.0f;
    c.age = 0.0f;
    c.health = 100.0f;
    c.fear = 0.0f;
    c.hunger = 0.0f;
    c.matingUrge = 0.0f;
    c.colonyID = 0;
    c.nestID = 0;
    c.animationTime = 0.0f;
    c.animationSpeed = 1.0f;
    c.genome = &genomes_.back();

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    c.setMale(dist(rng_) < 0.5f);

    return &c;
}

void SmallCreatureManager::spawnColony(SmallCreatureType type, const XMFLOAT3& position, int size) {
    // Create a new colony
    auto colony = std::make_unique<Colony>(type, position);
    uint32_t colonyID = colony->getID();
    colonies_.push_back(std::move(colony));

    std::uniform_real_distribution<float> offsetDist(-0.5f, 0.5f);

    // Spawn queen first
    SmallCreatureType queenType = type;
    if (type == SmallCreatureType::ANT_WORKER) queenType = SmallCreatureType::ANT_QUEEN;
    else if (type == SmallCreatureType::BEE_WORKER) queenType = SmallCreatureType::BEE_QUEEN;
    else if (type == SmallCreatureType::TERMITE_WORKER) queenType = SmallCreatureType::TERMITE_QUEEN;

    SmallCreature* queen = spawn(queenType, position);
    if (queen) {
        queen->colonyID = colonyID;
        colonies_.back()->setQueen(queen);
    }

    // Spawn workers
    for (int i = 0; i < size - 1; ++i) {
        XMFLOAT3 offset = { offsetDist(rng_), 0.0f, offsetDist(rng_) };
        XMFLOAT3 pos = { position.x + offset.x, position.y, position.z + offset.z };

        SmallCreature* worker = spawn(type, pos);
        if (worker) {
            worker->colonyID = colonyID;
        }
    }
}

void SmallCreatureManager::spawnSwarm(SmallCreatureType type, const XMFLOAT3& center,
                                       float radius, int count) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> radiusDist(0.0f, radius);
    std::uniform_real_distribution<float> heightDist(-1.0f, 1.0f);

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(rng_);
        float r = radiusDist(rng_);
        XMFLOAT3 pos = {
            center.x + cosf(angle) * r,
            center.y + heightDist(rng_),
            center.z + sinf(angle) * r
        };
        spawn(type, pos);
    }
}

void SmallCreatureManager::update(float deltaTime, Terrain* terrain) {
    terrain_ = terrain;

    // Clear and rebuild spatial grid
    spatialGrid_->clear();
    for (auto& creature : creatures_) {
        if (creature.isAlive()) {
            spatialGrid_->insert(&creature);
        }
    }

    // Update pheromone system
    pheromoneSystem_->update(deltaTime);

    // Insert pheromones into spatial grid for queries
    for (auto& point : pheromoneSystem_->getPoints()) {
        spatialGrid_->insertPheromone(&point);
    }

    // Insert food sources
    for (auto& food : foodSources_) {
        spatialGrid_->insertFood(&food);
    }

    // Update colonies
    for (auto& colony : colonies_) {
        colony->update(deltaTime, *this);
    }

    // Update creatures by category for cache efficiency
    updateInsects(deltaTime, terrain);
    updateArachnids(deltaTime, terrain);
    updateSmallMammals(deltaTime, terrain);
    updateReptiles(deltaTime, terrain);
    updateAmphibians(deltaTime, terrain);

    // Cleanup dead creatures periodically
    static float cleanupTimer = 0.0f;
    cleanupTimer += deltaTime;
    if (cleanupTimer > 5.0f) {
        cleanupDead();
        cleanupTimer = 0.0f;
    }
}

void SmallCreatureManager::updateInsects(float deltaTime, Terrain* terrain) {
    for (auto& creature : creatures_) {
        if (!creature.isAlive() || !isInsect(creature.type)) continue;

        // Update based on specific type
        if (isAnt(creature.type)) {
            updateAntBehavior(creature, deltaTime);
        } else if (isBee(creature.type)) {
            updateBeeBehavior(creature, deltaTime);
        } else {
            // Generic insect behavior
            // Wander and seek food
            auto foods = spatialGrid_->queryFood(creature.position, creature.genome->smellRange);
            if (!foods.empty()) {
                creature.targetPosition = foods[0]->position;
                creature.setHunting(true);
            } else {
                // Random wandering
                std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
                if (creature.animationTime > 2.0f) {
                    creature.targetPosition.x = creature.position.x + dist(rng_) * 2.0f;
                    creature.targetPosition.z = creature.position.z + dist(rng_) * 2.0f;
                    creature.animationTime = 0.0f;
                }
            }
        }

        // Apply physics
        SmallCreaturePhysics::update(creature, deltaTime, terrain, *spatialGrid_);

        // Update life cycle
        updateMetamorphosis(creature, deltaTime);
        updateReproduction(creature, deltaTime);
        checkDeath(creature);

        creature.animationTime += deltaTime;
    }
}

void SmallCreatureManager::updateArachnids(float deltaTime, Terrain* terrain) {
    for (auto& creature : creatures_) {
        if (!creature.isAlive() || !isArachnid(creature.type)) continue;

        if (isSpider(creature.type)) {
            updateSpiderBehavior(creature, deltaTime);
        } else {
            // Generic arachnid (scorpion, etc)
            // Hunt for prey
            auto prey = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
                [&creature](SmallCreature* c) {
                    return c->isAlive() && canEat(creature.type, c->type);
                });
            if (prey) {
                creature.targetPosition = prey->position;
                creature.setHunting(true);
            }
        }

        SmallCreaturePhysics::update(creature, deltaTime, terrain, *spatialGrid_);
        updateReproduction(creature, deltaTime);
        checkDeath(creature);
        creature.animationTime += deltaTime;
    }
}

void SmallCreatureManager::updateSmallMammals(float deltaTime, Terrain* terrain) {
    for (auto& creature : creatures_) {
        if (!creature.isAlive() || !isSmallMammal(creature.type)) continue;

        if (creature.type == SmallCreatureType::MOUSE ||
            creature.type == SmallCreatureType::RAT) {
            updateMouseBehavior(creature, deltaTime);
        } else if (creature.type == SmallCreatureType::SQUIRREL_TREE ||
                   creature.type == SmallCreatureType::CHIPMUNK) {
            updateSquirrelBehavior(creature, deltaTime);
        } else {
            // Generic small mammal behavior
            // Forage for food
            auto foods = spatialGrid_->queryFood(creature.position, creature.genome->smellRange * 2.0f);
            if (!foods.empty() && creature.hunger > 30.0f) {
                creature.targetPosition = foods[0]->position;
            }
        }

        SmallCreaturePhysics::update(creature, deltaTime, terrain, *spatialGrid_);
        updateReproduction(creature, deltaTime);
        checkDeath(creature);
        creature.animationTime += deltaTime;
    }
}

void SmallCreatureManager::updateReptiles(float deltaTime, Terrain* terrain) {
    for (auto& creature : creatures_) {
        if (!creature.isAlive() || !isReptile(creature.type)) continue;

        // Hunt for insects
        auto prey = spatialGrid_->findNearest(creature.position, creature.genome->visionRange * 1.5f,
            [&creature](SmallCreature* c) {
                return c->isAlive() && canEat(creature.type, c->type);
            });

        if (prey && creature.hunger > 40.0f) {
            creature.targetPosition = prey->position;
            creature.setHunting(true);

            // Check if close enough to catch
            float dx = prey->position.x - creature.position.x;
            float dz = prey->position.z - creature.position.z;
            float distSq = dx * dx + dz * dz;
            if (distSq < 0.01f) {
                // Catch prey
                prey->setAlive(false);
                creature.energy += 30.0f;
                creature.hunger = 0.0f;
                creature.setHunting(false);
            }
        } else {
            // Bask or wander slowly
            creature.setHunting(false);
            creature.setResting(creature.energy > 70.0f);
        }

        SmallCreaturePhysics::update(creature, deltaTime, terrain, *spatialGrid_);
        updateReproduction(creature, deltaTime);
        checkDeath(creature);
        creature.animationTime += deltaTime;
    }
}

void SmallCreatureManager::updateAmphibians(float deltaTime, Terrain* terrain) {
    for (auto& creature : creatures_) {
        if (!creature.isAlive() || !isAmphibian(creature.type)) continue;

        updateFrogBehavior(creature, deltaTime);

        SmallCreaturePhysics::update(creature, deltaTime, terrain, *spatialGrid_);
        updateMetamorphosis(creature, deltaTime);
        updateReproduction(creature, deltaTime);
        checkDeath(creature);
        creature.animationTime += deltaTime;
    }
}

void SmallCreatureManager::updateAntBehavior(SmallCreature& creature, float deltaTime) {
    if (creature.colonyID == 0) return;

    Colony* colony = getColony(creature.colonyID);
    if (!colony) return;

    // Queens stay in nest
    if (creature.type == SmallCreatureType::ANT_QUEEN) {
        creature.setInNest(true);
        return;
    }

    // Follow pheromone trails
    auto pheromones = spatialGrid_->queryPheromones(
        creature.position, creature.genome->smellRange, PheromonePoint::Type::FOOD_TRAIL);

    if (!pheromones.empty() && creature.hunger > 20.0f && !creature.isCarryingFood()) {
        // Find strongest pheromone
        PheromonePoint* strongest = nullptr;
        float maxStrength = 0.0f;
        for (auto* p : pheromones) {
            if (p->colonyID == creature.colonyID && p->strength > maxStrength) {
                maxStrength = p->strength;
                strongest = p;
            }
        }
        if (strongest) {
            creature.targetPosition = strongest->position;
        }
    } else if (creature.isCarryingFood()) {
        // Return to nest, lay trail
        creature.targetPosition = colony->getNestPosition();

        // Lay pheromone trail
        pheromoneSystem_->addPheromone(
            creature.position, creature.colonyID,
            PheromonePoint::Type::FOOD_TRAIL, 1.0f);

        // Check if at nest
        float dx = creature.position.x - colony->getNestPosition().x;
        float dz = creature.position.z - colony->getNestPosition().z;
        if (dx * dx + dz * dz < 0.5f) {
            creature.setCarryingFood(false);
            colony->addFood(10.0f);
            creature.energy += 5.0f;
        }
    } else {
        // Search for food
        auto foods = spatialGrid_->queryFood(creature.position, creature.genome->smellRange);
        if (!foods.empty()) {
            MicroFood* closest = foods[0];
            float minDist = FLT_MAX;
            for (auto* f : foods) {
                float dx = f->position.x - creature.position.x;
                float dz = f->position.z - creature.position.z;
                float dist = dx * dx + dz * dz;
                if (dist < minDist) {
                    minDist = dist;
                    closest = f;
                }
            }

            if (minDist < 0.01f) {
                // Pick up food
                creature.setCarryingFood(true);
                closest->amount -= 1.0f;
            } else {
                creature.targetPosition = closest->position;
            }
        } else {
            // Random exploration with bias towards unexplored areas
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            if (creature.animationTime > 3.0f) {
                creature.targetPosition.x = creature.position.x + dist(rng_) * 5.0f;
                creature.targetPosition.z = creature.position.z + dist(rng_) * 5.0f;
                creature.animationTime = 0.0f;
            }
        }
    }
}

void SmallCreatureManager::updateBeeBehavior(SmallCreature& creature, float deltaTime) {
    if (creature.type == SmallCreatureType::BEE_QUEEN) {
        creature.setInNest(true);
        return;
    }

    Colony* colony = creature.colonyID ? getColony(creature.colonyID) : nullptr;

    // Bees seek nectar from flowers (represented as specific food types)
    auto foods = spatialGrid_->queryFood(creature.position, creature.genome->visionRange * 2.0f);
    MicroFood* targetFlower = nullptr;
    float minDist = FLT_MAX;

    for (auto* f : foods) {
        if (f->type == MicroFood::Type::NECTAR || f->type == MicroFood::Type::POLLEN) {
            float dx = f->position.x - creature.position.x;
            float dz = f->position.z - creature.position.z;
            float dist = dx * dx + dz * dz;
            if (dist < minDist) {
                minDist = dist;
                targetFlower = f;
            }
        }
    }

    if (creature.isCarryingFood() && colony) {
        // Return to hive
        creature.targetPosition = colony->getNestPosition();
        creature.targetPosition.y = colony->getNestPosition().y + 2.0f; // Fly above ground

        float dx = creature.position.x - colony->getNestPosition().x;
        float dz = creature.position.z - colony->getNestPosition().z;
        if (dx * dx + dz * dz < 1.0f) {
            creature.setCarryingFood(false);
            colony->addFood(15.0f);
            creature.energy += 10.0f;

            // Do waggle dance to communicate food location (pheromone)
            pheromoneSystem_->addPheromone(
                colony->getNestPosition(), creature.colonyID,
                PheromonePoint::Type::FOOD_TRAIL, 2.0f);
        }
    } else if (targetFlower) {
        creature.targetPosition = targetFlower->position;
        creature.targetPosition.y += 0.5f; // Hover above flower

        if (minDist < 0.1f) {
            creature.setCarryingFood(true);
            targetFlower->amount -= 0.5f;
        }
    } else {
        // Random flight pattern
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        if (creature.animationTime > 2.0f) {
            creature.targetPosition.x = creature.position.x + dist(rng_) * 10.0f;
            creature.targetPosition.y = creature.position.y + dist(rng_) * 2.0f;
            creature.targetPosition.z = creature.position.z + dist(rng_) * 10.0f;
            creature.targetPosition.y = std::max(creature.targetPosition.y, 1.0f);
            creature.animationTime = 0.0f;
        }
    }
}

void SmallCreatureManager::updateSpiderBehavior(SmallCreature& creature, float deltaTime) {
    // Orb weavers wait in webs
    if (creature.type == SmallCreatureType::SPIDER_ORB_WEAVER) {
        creature.setResting(true);
        // TODO: Web mechanics - detect prey in web via vibration

        // Occasionally check for prey caught in web
        auto prey = spatialGrid_->findNearest(creature.position, 0.5f,
            [](SmallCreature* c) {
                return c->isAlive() && isFlyingInsect(c->type);
            });
        if (prey) {
            creature.energy += 20.0f;
            prey->setAlive(false);
        }
    }
    // Jumping spiders actively hunt
    else if (creature.type == SmallCreatureType::SPIDER_JUMPING) {
        auto prey = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
            [&creature](SmallCreature* c) {
                return c->isAlive() && canEat(creature.type, c->type);
            });

        if (prey) {
            float dx = prey->position.x - creature.position.x;
            float dz = prey->position.z - creature.position.z;
            float dist = sqrtf(dx * dx + dz * dz);

            if (dist < 0.3f) {
                // Jump attack!
                creature.velocity.x = dx * 5.0f;
                creature.velocity.z = dz * 5.0f;
                creature.velocity.y = 0.5f;

                if (dist < 0.05f) {
                    prey->setAlive(false);
                    creature.energy += 25.0f;
                }
            } else {
                creature.targetPosition = prey->position;
            }
        } else {
            // Slow patrol
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            if (creature.animationTime > 4.0f) {
                creature.targetPosition.x = creature.position.x + dist(rng_) * 1.0f;
                creature.targetPosition.z = creature.position.z + dist(rng_) * 1.0f;
                creature.animationTime = 0.0f;
            }
        }
    }
    // Wolf spiders chase prey
    else if (creature.type == SmallCreatureType::SPIDER_WOLF) {
        auto prey = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
            [&creature](SmallCreature* c) {
                return c->isAlive() && canEat(creature.type, c->type);
            });

        if (prey) {
            creature.targetPosition = prey->position;
            creature.setHunting(true);

            float dx = prey->position.x - creature.position.x;
            float dz = prey->position.z - creature.position.z;
            if (dx * dx + dz * dz < 0.02f) {
                prey->setAlive(false);
                creature.energy += 20.0f;
                creature.setHunting(false);
            }
        }
    }
}

void SmallCreatureManager::updateMouseBehavior(SmallCreature& creature, float deltaTime) {
    // Check for predators (snakes, owls, larger creatures)
    bool predatorNearby = false;

    // Check main creature grid for large predators
    if (largeCreatureGrid_) {
        // Would query for predators here
    }

    // Also check small predators
    auto threat = spatialGrid_->findNearest(creature.position, creature.genome->hearingRange,
        [&creature](SmallCreature* c) {
            return c->isAlive() && canEat(c->type, creature.type);
        });

    if (threat) {
        predatorNearby = true;
        creature.fear = 1.0f;
        creature.setFleeing(true);

        // Run away from predator
        float dx = creature.position.x - threat->position.x;
        float dz = creature.position.z - threat->position.z;
        float dist = sqrtf(dx * dx + dz * dz);
        if (dist > 0.001f) {
            creature.targetPosition.x = creature.position.x + (dx / dist) * 5.0f;
            creature.targetPosition.z = creature.position.z + (dz / dist) * 5.0f;
        }
    } else {
        creature.fear *= 0.95f;
        creature.setFleeing(false);

        // Forage for food
        if (creature.hunger > 30.0f) {
            auto foods = spatialGrid_->queryFood(creature.position, creature.genome->smellRange);
            if (!foods.empty()) {
                MicroFood* best = nullptr;
                float minDist = FLT_MAX;
                for (auto* f : foods) {
                    if (f->type == MicroFood::Type::SEEDS || f->type == MicroFood::Type::PLANT_MATTER) {
                        float dx = f->position.x - creature.position.x;
                        float dz = f->position.z - creature.position.z;
                        float dist = dx * dx + dz * dz;
                        if (dist < minDist) {
                            minDist = dist;
                            best = f;
                        }
                    }
                }
                if (best) {
                    creature.targetPosition = best->position;
                    if (minDist < 0.01f) {
                        creature.energy += 15.0f;
                        creature.hunger = 0.0f;
                        best->amount -= 1.0f;
                    }
                }
            }
        } else {
            // Random exploration, prefer cover
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            if (creature.animationTime > 2.0f) {
                creature.targetPosition.x = creature.position.x + dist(rng_) * 2.0f;
                creature.targetPosition.z = creature.position.z + dist(rng_) * 2.0f;
                creature.animationTime = 0.0f;
            }
        }
    }

    // Update hunger
    creature.hunger += deltaTime * creature.genome->metabolism * 5.0f;
}

void SmallCreatureManager::updateSquirrelBehavior(SmallCreature& creature, float deltaTime) {
    // Squirrels are tree dwellers - handled more in TreeDwellers system
    // Basic behavior: gather nuts, climb trees, evade predators

    bool predatorNearby = false;
    auto threat = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
        [&creature](SmallCreature* c) {
            return c->isAlive() &&
                   (c->type == SmallCreatureType::SNAKE_SMALL ||
                    c->type == SmallCreatureType::SNAKE_MEDIUM);
        });

    if (threat) {
        predatorNearby = true;
        creature.fear = 1.0f;
        creature.setFleeing(true);

        // Run up tree (increase Y)
        creature.targetPosition = creature.position;
        creature.targetPosition.y += 5.0f;
    } else {
        creature.fear *= 0.9f;
        creature.setFleeing(false);

        // Gather food (nuts, seeds)
        if (creature.hunger > 25.0f || creature.isCarryingFood()) {
            if (!creature.isCarryingFood()) {
                auto foods = spatialGrid_->queryFood(creature.position, creature.genome->visionRange);
                MicroFood* nut = nullptr;
                float minDist = FLT_MAX;
                for (auto* f : foods) {
                    if (f->type == MicroFood::Type::SEEDS) {
                        float dx = f->position.x - creature.position.x;
                        float dz = f->position.z - creature.position.z;
                        float dist = dx * dx + dz * dz;
                        if (dist < minDist) {
                            minDist = dist;
                            nut = f;
                        }
                    }
                }
                if (nut) {
                    creature.targetPosition = nut->position;
                    if (minDist < 0.05f) {
                        creature.setCarryingFood(true);
                        nut->amount -= 1.0f;
                    }
                }
            } else {
                // Return to cache/nest (for now, just consume)
                creature.setCarryingFood(false);
                creature.energy += 20.0f;
                creature.hunger = 0.0f;
            }
        } else {
            // Play / explore tree
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            if (creature.animationTime > 1.5f) {
                creature.targetPosition.x = creature.position.x + dist(rng_) * 3.0f;
                creature.targetPosition.y = creature.position.y + dist(rng_) * 2.0f;
                creature.targetPosition.z = creature.position.z + dist(rng_) * 3.0f;
                creature.targetPosition.y = std::max(creature.targetPosition.y, 0.5f);
                creature.animationTime = 0.0f;
            }
        }
    }

    creature.hunger += deltaTime * creature.genome->metabolism * 3.0f;
}

void SmallCreatureManager::updateFrogBehavior(SmallCreature& creature, float deltaTime) {
    // Frogs jump to catch flying insects
    auto prey = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
        [](SmallCreature* c) {
            return c->isAlive() && isFlyingInsect(c->type);
        });

    if (prey && creature.hunger > 30.0f) {
        float dx = prey->position.x - creature.position.x;
        float dy = prey->position.y - creature.position.y;
        float dz = prey->position.z - creature.position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < 2.0f && dy < 1.0f) {
            // Tongue strike range - jump and catch!
            if (distSq < 0.5f) {
                // Catch with tongue
                prey->setAlive(false);
                creature.energy += 15.0f;
                creature.hunger = 0.0f;
            } else {
                // Jump towards prey
                creature.velocity.x = dx * 2.0f;
                creature.velocity.z = dz * 2.0f;
                creature.velocity.y = 1.0f + dy;
            }
        } else {
            // Hop towards prey area
            creature.targetPosition = prey->position;
            creature.targetPosition.y = creature.position.y;
        }
    } else {
        // Sit and wait, occasionally hop
        creature.setResting(true);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(rng_) < 0.01f) {
            // Random hop
            creature.velocity.x = (dist(rng_) - 0.5f) * 2.0f;
            creature.velocity.z = (dist(rng_) - 0.5f) * 2.0f;
            creature.velocity.y = 0.5f;
        }
    }

    creature.hunger += deltaTime * creature.genome->metabolism * 2.0f;
}

void SmallCreatureManager::updateMetamorphosis(SmallCreature& creature, float deltaTime) {
    auto props = getProperties(creature.type);
    if (!props.undergoesMorphosis) {
        // Direct development
        if (creature.stage == LifeStage::JUVENILE && creature.age > 10.0f) {
            creature.stage = LifeStage::MATURE;
        }
        return;
    }

    // Metamorphosis timeline (age-based)
    switch (creature.stage) {
        case LifeStage::EGG:
            if (creature.age > 2.0f) {
                creature.stage = LifeStage::LARVA;
            }
            break;
        case LifeStage::LARVA:
            if (creature.age > 8.0f) {
                creature.stage = LifeStage::PUPA;
            }
            break;
        case LifeStage::PUPA:
            if (creature.age > 12.0f) {
                creature.stage = LifeStage::ADULT;
            }
            break;
        default:
            break;
    }
}

void SmallCreatureManager::updateReproduction(SmallCreature& creature, float deltaTime) {
    if (creature.stage != LifeStage::ADULT && creature.stage != LifeStage::MATURE) {
        return;
    }

    creature.matingUrge += deltaTime * creature.genome->reproduction * 0.1f;

    if (creature.matingUrge > 1.0f && creature.energy > 60.0f) {
        // Find mate
        auto mate = spatialGrid_->findNearest(creature.position, creature.genome->visionRange,
            [&creature](SmallCreature* c) {
                return c->isAlive() &&
                       c->type == creature.type &&
                       c->isMale() != creature.isMale() &&
                       c->matingUrge > 0.5f &&
                       (c->stage == LifeStage::ADULT || c->stage == LifeStage::MATURE);
            });

        if (mate) {
            float dx = mate->position.x - creature.position.x;
            float dz = mate->position.z - creature.position.z;
            float distSq = dx * dx + dz * dz;

            if (distSq < 0.1f) {
                // Mate!
                if (!creature.isMale()) {
                    // Female produces offspring
                    SmallCreatureGenome childGenome = SmallCreatureGenome::crossover(
                        *creature.genome, *mate->genome, rng_);
                    childGenome.mutate(0.05f, rng_);

                    // Spawn near parent
                    std::uniform_real_distribution<float> offsetDist(-0.2f, 0.2f);
                    XMFLOAT3 childPos = creature.position;
                    childPos.x += offsetDist(rng_);
                    childPos.z += offsetDist(rng_);

                    spawnWithGenome(creature.type, childPos, childGenome);

                    creature.energy -= 30.0f;
                }

                creature.matingUrge = 0.0f;
                mate->matingUrge = 0.0f;
            } else {
                creature.targetPosition = mate->position;
            }
        }
    }
}

void SmallCreatureManager::checkDeath(SmallCreature& creature) {
    auto props = getProperties(creature.type);

    // Age death
    float maxAge = 30.0f * creature.genome->lifespan;
    if (props.hasExoskeleton) maxAge *= 0.3f; // Insects live shorter

    if (creature.age > maxAge) {
        creature.setAlive(false);
        return;
    }

    // Starvation
    if (creature.energy <= 0.0f) {
        creature.setAlive(false);
        return;
    }

    // Health death
    if (creature.health <= 0.0f) {
        creature.setAlive(false);
        return;
    }

    // Energy decay
    creature.energy -= creature.genome->metabolism * props.energyRate * 0.016f;
    creature.age += 0.016f;
}

void SmallCreatureManager::cleanupDead() {
    for (size_t i = 0; i < creatures_.size(); ++i) {
        if (!creatures_[i].isAlive()) {
            // Mark as available for reuse
            bool alreadyDead = std::find(deadIndices_.begin(), deadIndices_.end(), i) != deadIndices_.end();
            if (!alreadyDead) {
                deadIndices_.push_back(i);

                // Convert to carrion food source
                if (isSmallMammal(creatures_[i].type) ||
                    isReptile(creatures_[i].type) ||
                    isAmphibian(creatures_[i].type)) {
                    addFood(creatures_[i].position, 20.0f, MicroFood::Type::CARRION);
                } else if (isInsect(creatures_[i].type)) {
                    addFood(creatures_[i].position, 5.0f, MicroFood::Type::INSECT);
                }
            }
        }
    }
}

void SmallCreatureManager::addFood(const XMFLOAT3& position, float amount, MicroFood::Type type) {
    foodSources_.push_back({ position, amount, type });
}

Colony* SmallCreatureManager::getColony(uint32_t id) {
    for (auto& colony : colonies_) {
        if (colony->getID() == id) {
            return colony.get();
        }
    }
    return nullptr;
}

size_t SmallCreatureManager::getAliveCount() const {
    size_t count = 0;
    for (const auto& c : creatures_) {
        if (c.isAlive()) ++count;
    }
    return count;
}

SmallCreatureManager::Stats SmallCreatureManager::getStats() const {
    Stats stats = {};
    stats.totalCreatures = creatures_.size();
    stats.colonyCount = colonies_.size();

    float totalEnergy = 0.0f;
    float totalAge = 0.0f;

    for (const auto& c : creatures_) {
        if (c.isAlive()) {
            ++stats.aliveCreatures;
            totalEnergy += c.energy;
            totalAge += c.age;

            if (isInsect(c.type)) ++stats.insectCount;
            else if (isArachnid(c.type)) ++stats.arachnidCount;
            else if (isSmallMammal(c.type)) ++stats.mammalCount;
            else if (isReptile(c.type)) ++stats.reptileCount;
            else if (isAmphibian(c.type)) ++stats.amphibianCount;
        } else {
            ++stats.deadCreatures;
        }
    }

    if (stats.aliveCreatures > 0) {
        stats.averageEnergy = totalEnergy / stats.aliveCreatures;
        stats.averageAge = totalAge / stats.aliveCreatures;
    }

    return stats;
}

} // namespace small
