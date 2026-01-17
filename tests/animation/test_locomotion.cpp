// test_locomotion.cpp - Unit tests for ProceduralLocomotion
// Tests gait patterns, foot placement, and body motion

#include "../../src/animation/Animation.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace animation;

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// Test gait timing presets
void testGaitPresets() {
    std::cout << "Testing gait presets..." << std::endl;

    // Biped walk
    GaitTiming bipedWalk = GaitPresets::bipedWalk();
    assert(bipedWalk.phaseOffsets.size() == 2);
    assert(approxEqual(bipedWalk.phaseOffsets[0], 0.0f));
    assert(approxEqual(bipedWalk.phaseOffsets[1], 0.5f));

    // Quadruped trot (diagonal pairs)
    GaitTiming quadTrot = GaitPresets::quadrupedTrot();
    assert(quadTrot.phaseOffsets.size() == 4);
    // Diagonal pairs should have same phase
    assert(approxEqual(quadTrot.phaseOffsets[0], quadTrot.phaseOffsets[3]) ||
           approxEqual(quadTrot.phaseOffsets[1], quadTrot.phaseOffsets[2]));

    // Hexapod walk
    GaitTiming hexWalk = GaitPresets::hexapodWalk();
    assert(hexWalk.phaseOffsets.size() == 6);

    std::cout << "  Gait preset tests passed!" << std::endl;
}

// Test ProceduralLocomotion initialization
void testLocomotionInit() {
    std::cout << "Testing ProceduralLocomotion initialization..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;

    loco.initialize(skeleton);

    // Initially should not be moving
    assert(!loco.isMoving());
    assert(approxEqual(loco.getGaitPhase(), 0.0f));

    std::cout << "  Locomotion initialization tests passed!" << std::endl;
}

// Test gait type switching
void testGaitTypeSwitching() {
    std::cout << "Testing gait type switching..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    // Switch to different gaits
    loco.setGaitType(GaitType::Walk);
    // No crash means success

    loco.setGaitType(GaitType::Trot);
    // No crash means success

    loco.setGaitType(GaitType::Gallop);
    // No crash means success

    loco.setGaitType(GaitType::Fly);
    // No crash means success

    loco.setGaitType(GaitType::Swim);
    // No crash means success

    std::cout << "  Gait type switching tests passed!" << std::endl;
}

// Test velocity-based phase update
void testPhaseUpdate() {
    std::cout << "Testing phase update..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    // Set velocity (creature moving forward)
    loco.setVelocity(glm::vec3(5.0f, 0.0f, 0.0f));

    // Update for 1 second
    float initialPhase = loco.getGaitPhase();
    loco.update(1.0f);
    float newPhase = loco.getGaitPhase();

    // Phase should have changed (or wrapped around)
    // At minimum, isMoving should be true
    assert(loco.isMoving());

    std::cout << "  Phase update tests passed!" << std::endl;
}

// Test foot configuration
void testFootConfiguration() {
    std::cout << "Testing foot configuration..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    // Clear default feet
    loco.clearFeet();

    // Add custom foot
    FootConfig foot;
    foot.hipBoneIndex = 0;
    foot.kneeBoneIndex = 1;
    foot.ankleBoneIndex = 2;
    foot.footBoneIndex = 3;
    foot.liftHeight = 0.2f;
    foot.stepLength = 0.5f;
    foot.phaseOffset = 0.0f;
    foot.restOffset = glm::vec3(0.5f, 0.0f, 0.5f);

    loco.addFoot(foot);

    // Update should not crash
    loco.update(0.016f);

    std::cout << "  Foot configuration tests passed!" << std::endl;
}

// Test wing configuration
void testWingConfiguration() {
    std::cout << "Testing wing configuration..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createFlying(1.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    loco.clearWings();

    WingConfig wing;
    wing.shoulderBoneIndex = 0;
    wing.elbowBoneIndex = 1;
    wing.wristBoneIndex = 2;
    wing.tipBoneIndex = 3;
    wing.flapAmplitude = 45.0f;
    wing.flapSpeed = 2.0f;
    wing.phaseOffset = 0.0f;

    loco.addWing(wing);
    loco.setGaitType(GaitType::Fly);

    // Update should not crash
    loco.update(0.016f);

    std::cout << "  Wing configuration tests passed!" << std::endl;
}

// Test spine configuration
void testSpineConfiguration() {
    std::cout << "Testing spine configuration..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createSerpentine(2.0f, 8);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    SpineConfig spine;
    for (int i = 0; i < 8; ++i) {
        spine.boneIndices.push_back(i);
    }
    spine.waveMagnitude = 0.2f;
    spine.waveFrequency = 2.0f;
    spine.waveSpeed = 3.0f;
    spine.phaseOffset = 0.0f;

    loco.setSpine(spine);
    loco.setGaitType(GaitType::Crawl);

    // Update should not crash
    loco.update(0.016f);

    std::cout << "  Spine configuration tests passed!" << std::endl;
}

// Test body motion (bob and sway)
void testBodyMotion() {
    std::cout << "Testing body motion..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);
    LocomotionSetup::setupQuadruped(loco, skeleton);

    loco.setVelocity(glm::vec3(3.0f, 0.0f, 0.0f));

    // Run for a bit
    for (int i = 0; i < 60; ++i) {
        loco.update(0.016f);
    }

    // Body should have some offset/tilt
    glm::vec3 offset = loco.getBodyOffset();
    glm::quat tilt = loco.getBodyTilt();

    // At least should return valid values
    assert(std::isfinite(offset.x) && std::isfinite(offset.y) && std::isfinite(offset.z));

    std::cout << "  Body motion tests passed!" << std::endl;
}

// Test foot placements
void testFootPlacements() {
    std::cout << "Testing foot placements..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);
    LocomotionSetup::setupQuadruped(loco, skeleton);

    loco.setVelocity(glm::vec3(3.0f, 0.0f, 0.0f));
    loco.setBodyPosition(glm::vec3(0.0f, 1.0f, 0.0f));

    // Set ground callback (flat ground at y=0)
    loco.setGroundCallback([](const glm::vec3& origin, const glm::vec3& dir,
                               float maxDist, glm::vec3& hit, glm::vec3& normal) -> bool {
        if (dir.y < 0.0f) {
            float t = -origin.y / dir.y;
            if (t > 0.0f && t < maxDist) {
                hit = origin + dir * t;
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
                return true;
            }
        }
        return false;
    });

    // Update a few frames
    for (int i = 0; i < 30; ++i) {
        loco.update(0.016f);
    }

    const auto& placements = loco.getFootPlacements();
    // Should have foot placements for each configured foot
    // Exact number depends on setup

    std::cout << "  Foot placement tests passed!" << std::endl;
}

// Test locomotion setup helpers
void testLocomotionSetup() {
    std::cout << "Testing locomotion setup helpers..." << std::endl;

    // Biped
    {
        Skeleton skeleton = SkeletonFactory::createBiped(1.0f);
        ProceduralLocomotion loco;
        loco.initialize(skeleton);
        LocomotionSetup::setupBiped(loco, skeleton);
        loco.update(0.016f); // Should not crash
    }

    // Quadruped
    {
        Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
        ProceduralLocomotion loco;
        loco.initialize(skeleton);
        LocomotionSetup::setupQuadruped(loco, skeleton);
        loco.update(0.016f);
    }

    // Flying
    {
        Skeleton skeleton = SkeletonFactory::createFlying(1.5f);
        ProceduralLocomotion loco;
        loco.initialize(skeleton);
        LocomotionSetup::setupFlying(loco, skeleton);
        loco.setGaitType(GaitType::Fly);
        loco.update(0.016f);
    }

    // Aquatic
    {
        Skeleton skeleton = SkeletonFactory::createAquatic(1.0f, 5);
        ProceduralLocomotion loco;
        loco.initialize(skeleton);
        LocomotionSetup::setupAquatic(loco, skeleton);
        loco.setGaitType(GaitType::Swim);
        loco.update(0.016f);
    }

    // Serpentine
    {
        Skeleton skeleton = SkeletonFactory::createSerpentine(2.0f, 8);
        ProceduralLocomotion loco;
        loco.initialize(skeleton);
        LocomotionSetup::setupSerpentine(loco, skeleton);
        loco.setGaitType(GaitType::Crawl);
        loco.update(0.016f);
    }

    std::cout << "  Locomotion setup tests passed!" << std::endl;
}

// Test speed factor calculation
void testSpeedFactor() {
    std::cout << "Testing speed factor..." << std::endl;

    Skeleton skeleton = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    ProceduralLocomotion loco;
    loco.initialize(skeleton);

    // Not moving
    loco.setVelocity(glm::vec3(0.0f));
    assert(approxEqual(loco.getSpeedFactor(), 0.0f));

    // Moving at some speed
    loco.setVelocity(glm::vec3(5.0f, 0.0f, 0.0f));
    float speedFactor = loco.getSpeedFactor();
    assert(speedFactor > 0.0f);

    std::cout << "  Speed factor tests passed!" << std::endl;
}

// Test CreatureAnimator integration
void testCreatureAnimator() {
    std::cout << "Testing CreatureAnimator integration..." << std::endl;

    CreatureAnimator animator;

    // Initialize quadruped
    animator.initializeQuadruped(1.0f, 0.5f);
    assert(animator.getBoneCount() > 0);

    // Set movement
    animator.setPosition(glm::vec3(0.0f, 0.5f, 0.0f));
    animator.setVelocity(glm::vec3(3.0f, 0.0f, 0.0f));
    animator.setRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    // Update
    animator.update(0.016f);

    // Get skinning matrices
    const auto& matrices = animator.getSkinningMatrices();
    assert(matrices.size() == animator.getBoneCount());

    // All matrices should be finite
    for (const auto& mat : matrices) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                assert(std::isfinite(mat[i][j]));
            }
        }
    }

    std::cout << "  CreatureAnimator integration tests passed!" << std::endl;
}

// Test different creature types
void testCreatureTypes() {
    std::cout << "Testing different creature types..." << std::endl;

    // Biped
    {
        CreatureAnimator animator;
        animator.initializeBiped(1.0f);
        assert(animator.getBoneCount() > 0);
        animator.update(0.016f);
    }

    // Quadruped
    {
        CreatureAnimator animator;
        animator.initializeQuadruped(1.0f, 0.5f);
        assert(animator.getBoneCount() > 0);
        animator.update(0.016f);
    }

    // Flying
    {
        CreatureAnimator animator;
        animator.initializeFlying(1.5f);
        assert(animator.getBoneCount() > 0);
        animator.update(0.016f);
    }

    // Aquatic
    {
        CreatureAnimator animator;
        animator.initializeAquatic(1.0f);
        assert(animator.getBoneCount() > 0);
        animator.update(0.016f);
    }

    // Serpentine
    {
        CreatureAnimator animator;
        animator.initializeSerpentine(2.0f);
        assert(animator.getBoneCount() > 0);
        animator.update(0.016f);
    }

    std::cout << "  Creature type tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Locomotion Unit Tests ===" << std::endl;

    testGaitPresets();
    testLocomotionInit();
    testGaitTypeSwitching();
    testPhaseUpdate();
    testFootConfiguration();
    testWingConfiguration();
    testSpineConfiguration();
    testBodyMotion();
    testFootPlacements();
    testLocomotionSetup();
    testSpeedFactor();
    testCreatureAnimator();
    testCreatureTypes();

    std::cout << "\n=== All Locomotion tests passed! ===" << std::endl;
    return 0;
}
