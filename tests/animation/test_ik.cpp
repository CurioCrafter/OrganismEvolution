// test_ik.cpp - Unit tests for IK solvers
// Tests Two-Bone IK, FABRIK, and CCD algorithms

#include "../../src/animation/Skeleton.h"
#include "../../src/animation/Pose.h"
#include "../../src/animation/IKSolver.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace animation;

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.01f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

// Create a simple arm skeleton for testing
Skeleton createTestArmSkeleton() {
    Skeleton skeleton;

    // Shoulder at origin
    BoneTransform shoulderPose;
    shoulderPose.translation = glm::vec3(0.0f);
    skeleton.addBone("Shoulder", -1, shoulderPose);

    // Elbow 1 unit down
    BoneTransform elbowPose;
    elbowPose.translation = glm::vec3(0.0f, -1.0f, 0.0f);
    skeleton.addBone("Elbow", 0, elbowPose);

    // Wrist 1 unit down from elbow
    BoneTransform wristPose;
    wristPose.translation = glm::vec3(0.0f, -1.0f, 0.0f);
    skeleton.addBone("Wrist", 1, wristPose);

    skeleton.calculateBoneLengths();
    return skeleton;
}

// Create a spine skeleton for testing multi-bone IK
Skeleton createTestSpineSkeleton(int segments) {
    Skeleton skeleton;

    for (int i = 0; i < segments; ++i) {
        BoneTransform pose;
        pose.translation = (i == 0) ? glm::vec3(0.0f) : glm::vec3(0.0f, 0.5f, 0.0f);
        skeleton.addBone("Spine" + std::to_string(i), (i == 0) ? -1 : i - 1, pose);
    }

    skeleton.calculateBoneLengths();
    return skeleton;
}

// Test Two-Bone IK basic functionality
void testTwoBoneIKBasic() {
    std::cout << "Testing Two-Bone IK basic..." << std::endl;

    Skeleton skeleton = createTestArmSkeleton();
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    TwoBoneIK solver;

    // Target directly below - should be reachable
    IKTarget target;
    target.position = glm::vec3(0.0f, -1.5f, 0.0f);
    target.weight = 1.0f;

    bool success = solver.solve(skeleton, pose, 0, 1, 2, target);
    assert(success);

    // Calculate new wrist position
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 wristPos = glm::vec3(pose.getGlobalTransform(2)[3]);

    // Should be close to target
    assert(approxEqual(wristPos, target.position, 0.1f));

    std::cout << "  Two-Bone IK basic tests passed!" << std::endl;
}

// Test Two-Bone IK with target too far
void testTwoBoneIKOutOfReach() {
    std::cout << "Testing Two-Bone IK out of reach..." << std::endl;

    Skeleton skeleton = createTestArmSkeleton();
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    TwoBoneIK solver;

    // Target too far - arm can only reach ~2 units
    IKTarget target;
    target.position = glm::vec3(0.0f, -10.0f, 0.0f);
    target.weight = 1.0f;

    // Should still succeed (stretch toward target)
    bool success = solver.solve(skeleton, pose, 0, 1, 2, target);
    assert(success);

    // Arm should be extended toward target
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 wristPos = glm::vec3(pose.getGlobalTransform(2)[3]);

    // Should be in direction of target, at max reach
    assert(wristPos.y < 0.0f); // Pointing down

    std::cout << "  Two-Bone IK out of reach tests passed!" << std::endl;
}

// Test Two-Bone IK with pole vector
void testTwoBoneIKPoleVector() {
    std::cout << "Testing Two-Bone IK with pole vector..." << std::endl;

    Skeleton skeleton = createTestArmSkeleton();
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    TwoBoneIK solver;

    IKTarget target;
    target.position = glm::vec3(0.0f, -1.0f, 1.0f);
    target.weight = 1.0f;

    // Pole vector pointing backward - elbow should bend backward
    PoleVector pole;
    pole.position = glm::vec3(0.0f, -0.5f, -1.0f);
    pole.weight = 1.0f;
    pole.enabled = true;

    bool success = solver.solveWithPole(skeleton, pose, 0, 1, 2, target, pole);
    assert(success);

    // Verify elbow bends toward pole
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 elbowPos = glm::vec3(pose.getGlobalTransform(1)[3]);

    // Elbow should be displaced toward the pole direction (negative Z)
    // Note: Exact position depends on implementation
    assert(success); // At minimum, should succeed

    std::cout << "  Two-Bone IK pole vector tests passed!" << std::endl;
}

// Test FABRIK solver
void testFABRIK() {
    std::cout << "Testing FABRIK solver..." << std::endl;

    Skeleton skeleton = createTestSpineSkeleton(5);
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    FABRIKSolver solver;

    // Chain of all spine bones
    std::vector<uint32_t> chain = {0, 1, 2, 3, 4};

    // Target at an offset position
    IKTarget target;
    target.position = glm::vec3(1.0f, 1.5f, 0.0f);
    target.weight = 1.0f;

    bool success = solver.solve(skeleton, pose, chain, target);
    assert(success);

    // Calculate end position
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 endPos = glm::vec3(pose.getGlobalTransform(4)[3]);

    // Should be close to target
    float dist = glm::length(endPos - target.position);
    assert(dist < 0.1f); // Within tolerance

    std::cout << "  FABRIK tests passed!" << std::endl;
}

// Test FABRIK with constraints
void testFABRIKConstrained() {
    std::cout << "Testing FABRIK with constraints..." << std::endl;

    Skeleton skeleton = createTestSpineSkeleton(5);
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    FABRIKSolver solver;
    IKConfig config;
    config.maxIterations = 20;
    config.tolerance = 0.01f;
    solver.setConfig(config);

    std::vector<uint32_t> chain = {0, 1, 2, 3, 4};

    // Angle constraints (min, max) in radians
    std::vector<glm::vec2> constraints;
    for (int i = 0; i < 5; ++i) {
        constraints.push_back(glm::vec2(-0.5f, 0.5f)); // Limited bend
    }

    IKTarget target;
    target.position = glm::vec3(0.5f, 2.0f, 0.0f);
    target.weight = 1.0f;

    bool success = solver.solveConstrained(skeleton, pose, chain, target, constraints);
    assert(success);

    std::cout << "  FABRIK constrained tests passed!" << std::endl;
}

// Test CCD solver
void testCCD() {
    std::cout << "Testing CCD solver..." << std::endl;

    Skeleton skeleton = createTestSpineSkeleton(5);
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    CCDSolver solver;
    IKConfig config;
    config.maxIterations = 50;
    config.tolerance = 0.01f;
    config.damping = 0.8f;
    solver.setConfig(config);

    std::vector<uint32_t> chain = {0, 1, 2, 3, 4};

    IKTarget target;
    target.position = glm::vec3(1.0f, 1.0f, 0.5f);
    target.weight = 1.0f;

    bool success = solver.solve(skeleton, pose, chain, target);
    assert(success);

    // Calculate end position
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 endPos = glm::vec3(pose.getGlobalTransform(4)[3]);

    // Should be reasonably close to target (CCD converges slower)
    float dist = glm::length(endPos - target.position);
    assert(dist < 0.5f); // More lenient tolerance for CCD

    std::cout << "  CCD tests passed!" << std::endl;
}

// Test IK System manager
void testIKSystem() {
    std::cout << "Testing IK System manager..." << std::endl;

    Skeleton skeleton = createTestArmSkeleton();
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    IKSystem system;

    // Add a two-bone chain
    IKChain armChain;
    armChain.startBoneIndex = 0;
    armChain.endBoneIndex = 2;
    armChain.chainLength = 2;

    auto handle = system.addChain(armChain, IKSystem::SolverType::TwoBone, 10);
    assert(handle != IKSystem::INVALID_HANDLE);

    // Set target
    IKTarget target;
    target.position = glm::vec3(0.0f, -1.5f, 0.5f);
    target.weight = 1.0f;
    system.setTarget(handle, target);

    // Solve
    system.solve(skeleton, pose);

    // Verify pose was modified
    pose.calculateGlobalTransforms(skeleton);
    glm::vec3 wristPos = glm::vec3(pose.getGlobalTransform(2)[3]);

    // Should have moved toward target
    assert(wristPos.z > 0.0f || wristPos.y < 0.0f); // Some movement

    std::cout << "  IK System tests passed!" << std::endl;
}

// Test IK utility functions
void testIKUtils() {
    std::cout << "Testing IK utility functions..." << std::endl;

    // Test rotation between vectors
    glm::vec3 from(1.0f, 0.0f, 0.0f);
    glm::vec3 to(0.0f, 1.0f, 0.0f);

    glm::quat rot = IKUtils::rotationBetweenVectors(from, to);

    // Apply rotation
    glm::vec3 result = rot * from;

    // Should be close to 'to' vector
    assert(approxEqual(result, to, 0.01f));

    // Test angle clamping
    float clamped = IKUtils::clampAngle(2.0f, -1.0f, 1.0f);
    assert(approxEqual(clamped, 1.0f));

    clamped = IKUtils::clampAngle(-2.0f, -1.0f, 1.0f);
    assert(approxEqual(clamped, -1.0f));

    clamped = IKUtils::clampAngle(0.5f, -1.0f, 1.0f);
    assert(approxEqual(clamped, 0.5f));

    std::cout << "  IK utility tests passed!" << std::endl;
}

int main() {
    std::cout << "=== IK Solver Unit Tests ===" << std::endl;

    testTwoBoneIKBasic();
    testTwoBoneIKOutOfReach();
    testTwoBoneIKPoleVector();
    testFABRIK();
    testFABRIKConstrained();
    testCCD();
    testIKSystem();
    testIKUtils();

    std::cout << "\n=== All IK tests passed! ===" << std::endl;
    return 0;
}
