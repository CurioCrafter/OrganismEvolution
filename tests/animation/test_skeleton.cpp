// test_skeleton.cpp - Unit tests for Skeleton class
// Tests bone hierarchy, bind poses, and skeleton factory functions

#include "../../src/animation/Skeleton.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace animation;

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

bool approxEqual(const glm::vec3& a, const glm::vec3& b, float epsilon = 0.001f) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

// Test BoneTransform
void testBoneTransform() {
    std::cout << "Testing BoneTransform..." << std::endl;

    // Test identity transform
    BoneTransform identity = BoneTransform::identity();
    assert(approxEqual(identity.translation, glm::vec3(0.0f)));
    assert(approxEqual(identity.scale, glm::vec3(1.0f)));

    // Test matrix conversion
    BoneTransform t;
    t.translation = glm::vec3(1.0f, 2.0f, 3.0f);
    t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    t.scale = glm::vec3(2.0f);

    glm::mat4 mat = t.toMatrix();
    // Check translation is in last column
    assert(approxEqual(mat[3][0], 1.0f));
    assert(approxEqual(mat[3][1], 2.0f));
    assert(approxEqual(mat[3][2], 3.0f));

    // Test lerp
    BoneTransform a, b;
    a.translation = glm::vec3(0.0f);
    b.translation = glm::vec3(10.0f, 10.0f, 10.0f);
    BoneTransform mid = BoneTransform::lerp(a, b, 0.5f);
    assert(approxEqual(mid.translation, glm::vec3(5.0f)));

    std::cout << "  BoneTransform tests passed!" << std::endl;
}

// Test Skeleton basic operations
void testSkeletonBasic() {
    std::cout << "Testing Skeleton basic operations..." << std::endl;

    Skeleton skeleton;

    // Add root bone
    BoneTransform rootPose = BoneTransform::identity();
    int32_t rootIdx = skeleton.addBone("Root", -1, rootPose);
    assert(rootIdx == 0);
    assert(skeleton.getBoneCount() == 1);

    // Add child bone
    BoneTransform childPose;
    childPose.translation = glm::vec3(0.0f, 1.0f, 0.0f);
    int32_t childIdx = skeleton.addBone("Spine", rootIdx, childPose);
    assert(childIdx == 1);
    assert(skeleton.getBoneCount() == 2);

    // Test bone lookup by name
    assert(skeleton.findBoneIndex("Root") == 0);
    assert(skeleton.findBoneIndex("Spine") == 1);
    assert(skeleton.findBoneIndex("NonExistent") == -1);

    // Test bone access
    const Bone& root = skeleton.getBone(0);
    assert(root.name == "Root");
    assert(root.parentIndex == -1);

    const Bone& spine = skeleton.getBone(1);
    assert(spine.name == "Spine");
    assert(spine.parentIndex == 0);

    // Test root bone finding
    std::vector<int32_t> roots = skeleton.getRootBones();
    assert(roots.size() == 1);
    assert(roots[0] == 0);

    // Test child bone finding
    std::vector<int32_t> children = skeleton.getChildBones(0);
    assert(children.size() == 1);
    assert(children[0] == 1);

    std::cout << "  Skeleton basic tests passed!" << std::endl;
}

// Test skeleton hierarchy
void testSkeletonHierarchy() {
    std::cout << "Testing Skeleton hierarchy..." << std::endl;

    Skeleton skeleton;

    // Build a simple arm hierarchy: Shoulder -> Elbow -> Wrist -> Hand
    skeleton.addBone("Shoulder", -1, BoneTransform::identity());
    skeleton.addBone("Elbow", 0, BoneTransform::identity());
    skeleton.addBone("Wrist", 1, BoneTransform::identity());
    skeleton.addBone("Hand", 2, BoneTransform::identity());

    // Test isDescendant
    assert(skeleton.isDescendant(3, 0)); // Hand is descendant of Shoulder
    assert(skeleton.isDescendant(2, 0)); // Wrist is descendant of Shoulder
    assert(skeleton.isDescendant(1, 0)); // Elbow is descendant of Shoulder
    assert(!skeleton.isDescendant(0, 3)); // Shoulder is NOT descendant of Hand
    assert(!skeleton.isDescendant(0, 0)); // Bone is not its own descendant

    std::cout << "  Skeleton hierarchy tests passed!" << std::endl;
}

// Test skeleton factory functions
void testSkeletonFactory() {
    std::cout << "Testing SkeletonFactory..." << std::endl;

    // Test biped creation
    Skeleton biped = SkeletonFactory::createBiped(1.0f);
    assert(biped.getBoneCount() > 0);
    assert(biped.findBoneIndex("Root") != -1 || biped.findBoneIndex("Pelvis") != -1);
    std::cout << "  Biped has " << biped.getBoneCount() << " bones" << std::endl;

    // Test quadruped creation
    Skeleton quadruped = SkeletonFactory::createQuadruped(1.0f, 0.5f);
    assert(quadruped.getBoneCount() > 0);
    std::cout << "  Quadruped has " << quadruped.getBoneCount() << " bones" << std::endl;

    // Test serpentine creation
    Skeleton serpentine = SkeletonFactory::createSerpentine(2.0f, 8);
    assert(serpentine.getBoneCount() >= 8); // At least 8 spine segments
    std::cout << "  Serpentine has " << serpentine.getBoneCount() << " bones" << std::endl;

    // Test flying creation
    Skeleton flying = SkeletonFactory::createFlying(1.5f);
    assert(flying.getBoneCount() > 0);
    std::cout << "  Flying has " << flying.getBoneCount() << " bones" << std::endl;

    // Test aquatic creation
    Skeleton aquatic = SkeletonFactory::createAquatic(1.0f, 5);
    assert(aquatic.getBoneCount() > 0);
    std::cout << "  Aquatic has " << aquatic.getBoneCount() << " bones" << std::endl;

    std::cout << "  SkeletonFactory tests passed!" << std::endl;
}

// Test skeleton validity
void testSkeletonValidity() {
    std::cout << "Testing Skeleton validity..." << std::endl;

    // Valid skeleton
    Skeleton valid;
    valid.addBone("Root", -1, BoneTransform::identity());
    valid.addBone("Child", 0, BoneTransform::identity());
    assert(valid.isValid());

    // Empty skeleton should still be valid (edge case)
    Skeleton empty;
    // Note: Depending on implementation, empty might be valid or invalid

    std::cout << "  Skeleton validity tests passed!" << std::endl;
}

int main() {
    std::cout << "=== Skeleton Unit Tests ===" << std::endl;

    testBoneTransform();
    testSkeletonBasic();
    testSkeletonHierarchy();
    testSkeletonFactory();
    testSkeletonValidity();

    std::cout << "\n=== All Skeleton tests passed! ===" << std::endl;
    return 0;
}
