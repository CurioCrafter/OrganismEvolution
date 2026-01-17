// test_pose.cpp - Unit tests for SkeletonPose and skinning
// Tests pose creation, blending, and matrix calculations

#include "../../src/animation/Skeleton.h"
#include "../../src/animation/Pose.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace animation;

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = 0.001f) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!approxEqual(a[i][j], b[i][j], epsilon)) {
                return false;
            }
        }
    }
    return true;
}

// Test SkinWeight
void testSkinWeight() {
    std::cout << "Testing SkinWeight..." << std::endl;

    SkinWeight weight;

    // Add influences
    weight.addInfluence(0, 0.5f);
    weight.addInfluence(1, 0.3f);
    weight.addInfluence(2, 0.2f);

    // Check influence count
    assert(weight.getInfluenceCount() == 3);

    // Normalize
    weight.normalize();

    // Check weights sum to 1.0
    float sum = weight.weights[0] + weight.weights[1] + weight.weights[2] + weight.weights[3];
    assert(approxEqual(sum, 1.0f));

    std::cout << "  SkinWeight tests passed!" << std::endl;
}

// Test SkeletonPose creation
void testPoseCreation() {
    std::cout << "Testing SkeletonPose creation..." << std::endl;

    // Create a simple skeleton
    Skeleton skeleton;
    skeleton.addBone("Root", -1, BoneTransform::identity());
    skeleton.addBone("Spine", 0, BoneTransform::identity());
    skeleton.addBone("Head", 1, BoneTransform::identity());

    // Create pose from skeleton
    SkeletonPose pose(skeleton);
    assert(pose.getBoneCount() == 3);

    // Check initial transforms are identity
    const BoneTransform& rootTransform = pose.getLocalTransform(0);
    assert(approxEqual(rootTransform.translation.x, 0.0f));
    assert(approxEqual(rootTransform.scale.x, 1.0f));

    std::cout << "  SkeletonPose creation tests passed!" << std::endl;
}

// Test bind pose
void testBindPose() {
    std::cout << "Testing bind pose..." << std::endl;

    // Create skeleton with non-identity bind poses
    Skeleton skeleton;

    BoneTransform rootPose;
    rootPose.translation = glm::vec3(0.0f, 1.0f, 0.0f);
    skeleton.addBone("Root", -1, rootPose);

    BoneTransform spinePose;
    spinePose.translation = glm::vec3(0.0f, 0.5f, 0.0f);
    skeleton.addBone("Spine", 0, spinePose);

    // Create pose and set to bind pose
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    // Check local transforms match bind poses
    assert(approxEqual(pose.getLocalTransform(0).translation.y, 1.0f));
    assert(approxEqual(pose.getLocalTransform(1).translation.y, 0.5f));

    std::cout << "  Bind pose tests passed!" << std::endl;
}

// Test global transform calculation
void testGlobalTransforms() {
    std::cout << "Testing global transform calculation..." << std::endl;

    // Create skeleton with offset bones
    Skeleton skeleton;

    BoneTransform rootPose;
    rootPose.translation = glm::vec3(0.0f, 1.0f, 0.0f);
    skeleton.addBone("Root", -1, rootPose);

    BoneTransform spinePose;
    spinePose.translation = glm::vec3(0.0f, 0.5f, 0.0f);
    skeleton.addBone("Spine", 0, spinePose);

    // Create and initialize pose
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);
    pose.calculateGlobalTransforms(skeleton);

    // Root global should equal local (no parent)
    const glm::mat4& rootGlobal = pose.getGlobalTransform(0);
    assert(approxEqual(rootGlobal[3][1], 1.0f)); // Y translation

    // Spine global should be root * local
    const glm::mat4& spineGlobal = pose.getGlobalTransform(1);
    // Should be 1.0 + 0.5 = 1.5
    assert(approxEqual(spineGlobal[3][1], 1.5f));

    std::cout << "  Global transform tests passed!" << std::endl;
}

// Test pose blending
void testPoseBlending() {
    std::cout << "Testing pose blending..." << std::endl;

    // Create skeleton
    Skeleton skeleton;
    skeleton.addBone("Root", -1, BoneTransform::identity());
    skeleton.addBone("Spine", 0, BoneTransform::identity());

    // Create two poses
    SkeletonPose poseA(skeleton);
    SkeletonPose poseB(skeleton);

    // Set different positions
    poseA.getLocalTransform(0).translation = glm::vec3(0.0f);
    poseB.getLocalTransform(0).translation = glm::vec3(10.0f, 10.0f, 10.0f);

    // Blend 50%
    SkeletonPose blended = SkeletonPose::lerp(poseA, poseB, 0.5f);

    // Check result is midpoint
    assert(approxEqual(blended.getLocalTransform(0).translation.x, 5.0f));
    assert(approxEqual(blended.getLocalTransform(0).translation.y, 5.0f));
    assert(approxEqual(blended.getLocalTransform(0).translation.z, 5.0f));

    std::cout << "  Pose blending tests passed!" << std::endl;
}

// Test masked blending
void testMaskedBlending() {
    std::cout << "Testing masked blending..." << std::endl;

    // Create skeleton
    Skeleton skeleton;
    skeleton.addBone("Root", -1, BoneTransform::identity());
    skeleton.addBone("Spine", 0, BoneTransform::identity());
    skeleton.addBone("Head", 1, BoneTransform::identity());

    // Create poses
    SkeletonPose base(skeleton);
    SkeletonPose overlay(skeleton);

    base.getLocalTransform(0).translation = glm::vec3(0.0f);
    base.getLocalTransform(1).translation = glm::vec3(0.0f);
    base.getLocalTransform(2).translation = glm::vec3(0.0f);

    overlay.getLocalTransform(0).translation = glm::vec3(10.0f);
    overlay.getLocalTransform(1).translation = glm::vec3(10.0f);
    overlay.getLocalTransform(2).translation = glm::vec3(10.0f);

    // Mask: only blend head (bone 2)
    std::vector<bool> mask = {false, false, true};

    base.blendMasked(overlay, 1.0f, mask);

    // Root and Spine should be unchanged
    assert(approxEqual(base.getLocalTransform(0).translation.x, 0.0f));
    assert(approxEqual(base.getLocalTransform(1).translation.x, 0.0f));

    // Head should be blended
    assert(approxEqual(base.getLocalTransform(2).translation.x, 10.0f));

    std::cout << "  Masked blending tests passed!" << std::endl;
}

// Test skinning matrix calculation
void testSkinningMatrices() {
    std::cout << "Testing skinning matrix calculation..." << std::endl;

    // Create skeleton
    Skeleton skeleton;
    BoneTransform rootPose;
    rootPose.translation = glm::vec3(0.0f, 1.0f, 0.0f);
    skeleton.addBone("Root", -1, rootPose);

    // Create pose
    SkeletonPose pose(skeleton);
    pose.setToBindPose(skeleton);

    // Calculate matrices
    pose.updateMatrices(skeleton);

    // Get skinning matrices
    const std::vector<glm::mat4>& skinMatrices = pose.getSkinningMatrices();
    assert(skinMatrices.size() == 1);

    // Skinning matrix = global * inverseBindMatrix
    // At bind pose, skinning matrix should be identity
    // (because global == bindPose, so global * inverse(bindPose) = identity)
    assert(approxEqual(skinMatrices[0], glm::mat4(1.0f)));

    std::cout << "  Skinning matrix tests passed!" << std::endl;
}

// Test SkinningUtils functions
void testSkinningUtils() {
    std::cout << "Testing SkinningUtils..." << std::endl;

    // Test skinned position calculation
    glm::vec3 position(1.0f, 0.0f, 0.0f);
    glm::uvec4 indices(0, 0, 0, 0);
    glm::vec4 weights(1.0f, 0.0f, 0.0f, 0.0f);

    // Identity skin matrix should leave position unchanged
    std::vector<glm::mat4> identityMatrices = {glm::mat4(1.0f)};
    glm::vec3 skinned = SkinningUtils::calculateSkinnedPosition(position, indices, weights, identityMatrices);
    assert(approxEqual(skinned.x, 1.0f));
    assert(approxEqual(skinned.y, 0.0f));
    assert(approxEqual(skinned.z, 0.0f));

    // Translation matrix should move position
    std::vector<glm::mat4> translatedMatrices = {glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f))};
    skinned = SkinningUtils::calculateSkinnedPosition(position, indices, weights, translatedMatrices);
    assert(approxEqual(skinned.x, 6.0f)); // 1 + 5

    std::cout << "  SkinningUtils tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Pose Unit Tests ===" << std::endl;

    testSkinWeight();
    testPoseCreation();
    testBindPose();
    testGlobalTransforms();
    testPoseBlending();
    testMaskedBlending();
    testSkinningMatrices();
    testSkinningUtils();

    std::cout << "\n=== All Pose tests passed! ===" << std::endl;
    return 0;
}
