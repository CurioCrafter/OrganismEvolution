#include "ProceduralLocomotion.h"
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <cmath>

namespace animation {

void ProceduralLocomotion::initialize(const Skeleton& skeleton) {
    m_feet.clear();
    m_footPlacements.clear();
    m_footTargets.clear();
    m_footPreviousTargets.clear();
    m_wings.clear();
    m_hasSpine = false;
    m_gaitPhase = 0.0f;
    m_time = 0.0f;
}

void ProceduralLocomotion::addFoot(const FootConfig& config) {
    m_feet.push_back(config);
    m_footPlacements.push_back(FootPlacement{});
    m_footTargets.push_back(glm::vec3(0.0f));
    m_footPreviousTargets.push_back(glm::vec3(0.0f));

    // Update gait timing phases if needed
    if (m_gaitTiming.phaseOffsets.size() < m_feet.size()) {
        m_gaitTiming.phaseOffsets.push_back(config.phaseOffset);
    }
}

void ProceduralLocomotion::addWing(const WingConfig& config) {
    m_wings.push_back(config);
}

void ProceduralLocomotion::setSpine(const SpineConfig& config) {
    m_spine = config;
    m_hasSpine = true;
}

void ProceduralLocomotion::setGaitType(GaitType type) {
    m_gaitType = type;

    // Set default timing based on gait type
    switch (type) {
        case GaitType::Walk:
            if (m_feet.size() == 2) {
                m_gaitTiming = GaitPresets::bipedWalk();
            } else if (m_feet.size() == 4) {
                m_gaitTiming = GaitPresets::quadrupedWalk();
            }
            break;
        case GaitType::Trot:
            m_gaitTiming = GaitPresets::quadrupedTrot();
            break;
        case GaitType::Gallop:
            m_gaitTiming = GaitPresets::quadrupedGallop();
            break;
        default:
            break;
    }
}

void ProceduralLocomotion::setGaitTiming(const GaitTiming& timing) {
    m_gaitTiming = timing;
}

void ProceduralLocomotion::update(float deltaTime) {
    m_time += deltaTime;

    updateGaitPhase(deltaTime);
    updateFootPlacements();
    updateBodyMotion();
}

void ProceduralLocomotion::applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem) {
    // Apply foot IK
    for (size_t i = 0; i < m_feet.size(); ++i) {
        const FootConfig& foot = m_feet[i];
        const FootPlacement& placement = m_footPlacements[i];

        IKTarget target;
        target.position = placement.targetPosition;
        target.weight = placement.blendWeight;

        // Set up IK chain if not already added
        IKChain chain;
        chain.startBoneIndex = foot.hipBoneIndex;
        chain.endBoneIndex = foot.footBoneIndex;
        chain.chainLength = 4;

        // Apply two-bone IK for leg
        TwoBoneIK solver;
        solver.solve(skeleton, pose, foot.hipBoneIndex, foot.kneeBoneIndex, foot.ankleBoneIndex, target);
    }

    // Update wings
    if (!m_wings.empty()) {
        updateWings(0.0f, pose); // deltaTime already accumulated in m_time
    }

    // Update spine
    if (m_hasSpine) {
        updateSpine(0.0f, pose);
    }
}

float ProceduralLocomotion::getSpeedFactor() const {
    float speed = glm::length(m_velocity);
    return glm::clamp(speed / 5.0f, 0.0f, 1.0f); // Normalize to 5 m/s max
}

void ProceduralLocomotion::updateGaitPhase(float deltaTime) {
    float speed = glm::length(m_velocity);
    if (speed < 0.01f) {
        return; // Standing still
    }

    // Adjust cycle time based on speed
    float adjustedCycleTime = m_gaitTiming.cycleTime / (speed + 0.1f);
    adjustedCycleTime = glm::clamp(adjustedCycleTime, 0.2f, 2.0f);

    m_gaitPhase += deltaTime / adjustedCycleTime;
    while (m_gaitPhase >= 1.0f) {
        m_gaitPhase -= 1.0f;
    }
}

void ProceduralLocomotion::updateFootPlacements() {
    glm::vec3 forward = m_bodyRotation * glm::vec3(0, 0, 1);
    glm::vec3 right = m_bodyRotation * glm::vec3(1, 0, 0);

    for (size_t i = 0; i < m_feet.size(); ++i) {
        const FootConfig& foot = m_feet[i];
        FootPlacement& placement = m_footPlacements[i];

        float footPhase = getFootPhase(i);
        bool isSwinging = footPhase < (1.0f - m_gaitTiming.dutyFactor);

        // Calculate rest position
        glm::vec3 restPos = m_bodyPosition + m_bodyRotation * foot.restOffset;

        // Add stride offset based on velocity
        glm::vec3 strideOffset = glm::normalize(m_velocity + glm::vec3(0.0001f)) * foot.stepLength;

        if (isSwinging) {
            // Foot is in swing phase
            float swingProgress = footPhase / (1.0f - m_gaitTiming.dutyFactor);
            placement.stepProgress = swingProgress;

            // Calculate target for next footfall
            glm::vec3 nextTarget = restPos + strideOffset * 0.5f;

            // Raycast to find ground
            glm::vec3 hitPoint, hitNormal;
            if (raycastGround(nextTarget + glm::vec3(0, 1, 0), hitPoint, hitNormal)) {
                placement.groundNormal = hitNormal;
                placement.isGrounded = false;

                // Interpolate from current to next with arc
                float arcPhase = std::sin(swingProgress * 3.14159f);
                glm::vec3 liftOffset = glm::vec3(0, foot.liftHeight * arcPhase, 0);

                placement.targetPosition = glm::mix(m_footPreviousTargets[i], hitPoint, swingProgress) + liftOffset;
                m_footTargets[i] = hitPoint;
            } else {
                float arcHeight = std::sin(swingProgress * 3.14159f) * foot.liftHeight;
                placement.targetPosition = restPos + glm::vec3(0, arcHeight, 0);
                placement.isGrounded = false;
            }

            placement.blendWeight = 1.0f;
        } else {
            // Foot is in stance phase (on ground)
            float stanceProgress = (footPhase - (1.0f - m_gaitTiming.dutyFactor)) / m_gaitTiming.dutyFactor;
            placement.stepProgress = stanceProgress;

            // Keep foot planted, update previous target
            placement.targetPosition = m_footTargets[i];
            placement.isGrounded = true;
            placement.blendWeight = 1.0f;

            // At end of stance, save for next swing
            if (stanceProgress > 0.95f) {
                m_footPreviousTargets[i] = m_footTargets[i];
            }
        }
    }
}

void ProceduralLocomotion::updateBodyMotion() {
    float speed = glm::length(m_velocity);
    if (speed < 0.01f) {
        m_bodyOffset = glm::vec3(0.0f);
        m_bodyTilt = glm::quat(1, 0, 0, 0);
        return;
    }

    // Vertical bob synchronized with gait
    float bobFrequency = m_feet.size() == 2 ? 2.0f : 1.0f; // Bipeds bob twice per cycle
    float bob = std::sin(m_gaitPhase * 3.14159f * 2.0f * bobFrequency) * 0.02f * getSpeedFactor();
    m_bodyOffset.y = bob;

    // Side-to-side sway
    float sway = std::sin(m_gaitPhase * 3.14159f * 2.0f) * 0.01f * getSpeedFactor();
    m_bodyOffset.x = sway;

    // Forward lean based on acceleration
    glm::vec3 acceleration = m_velocity * 0.1f; // Simplified
    float forwardLean = glm::clamp(acceleration.z * 0.05f, -0.1f, 0.1f);
    m_bodyTilt = glm::angleAxis(forwardLean, glm::vec3(1, 0, 0));
}

void ProceduralLocomotion::updateWings(float deltaTime, SkeletonPose& pose) {
    for (const WingConfig& wing : m_wings) {
        float phase = m_time * wing.flapSpeed * 3.14159f * 2.0f + wing.phaseOffset;

        // Base flapping motion
        float flapAngle = std::sin(phase) * glm::radians(wing.flapAmplitude);

        // Apply to shoulder (main flap)
        BoneTransform& shoulder = pose.getLocalTransform(wing.shoulderBoneIndex);
        glm::quat flapRot = glm::angleAxis(flapAngle, glm::vec3(0, 0, 1));
        shoulder.rotation = flapRot * shoulder.rotation;

        // Secondary motion at elbow (fold)
        float elbowFold = std::sin(phase + 0.5f) * glm::radians(wing.flapAmplitude * 0.3f);
        BoneTransform& elbow = pose.getLocalTransform(wing.elbowBoneIndex);
        glm::quat elbowRot = glm::angleAxis(-elbowFold, glm::vec3(0, 0, 1));
        elbow.rotation = elbowRot * elbow.rotation;

        // Wrist flex
        float wristFlex = std::sin(phase + 1.0f) * glm::radians(wing.flapAmplitude * 0.2f);
        BoneTransform& wrist = pose.getLocalTransform(wing.wristBoneIndex);
        glm::quat wristRot = glm::angleAxis(wristFlex, glm::vec3(0, 0, 1));
        wrist.rotation = wristRot * wrist.rotation;
    }
}

void ProceduralLocomotion::updateSpine(float deltaTime, SkeletonPose& pose) {
    if (!m_hasSpine || m_spine.boneIndices.empty()) {
        return;
    }

    float speed = glm::length(m_velocity);
    float wavePhase = m_time * m_spine.waveSpeed + m_spine.phaseOffset;

    for (size_t i = 0; i < m_spine.boneIndices.size(); ++i) {
        uint32_t boneIndex = m_spine.boneIndices[i];
        float segmentPhase = wavePhase + static_cast<float>(i) * m_spine.waveFrequency;

        // Horizontal wave (for swimming/slithering)
        float horizontalWave = std::sin(segmentPhase) * m_spine.waveMagnitude;

        // Vertical wave (smaller amplitude)
        float verticalWave = std::sin(segmentPhase * 0.5f) * m_spine.waveMagnitude * 0.3f;

        BoneTransform& transform = pose.getLocalTransform(boneIndex);

        // Apply rotations
        glm::quat horizontalRot = glm::angleAxis(horizontalWave * speed, glm::vec3(0, 1, 0));
        glm::quat verticalRot = glm::angleAxis(verticalWave * speed, glm::vec3(1, 0, 0));

        transform.rotation = horizontalRot * verticalRot * transform.rotation;
    }
}

glm::vec3 ProceduralLocomotion::calculateFootPosition(const FootConfig& foot, float phase,
                                                       const FootPlacement& placement) {
    // Arc trajectory for swing phase
    float arcHeight = std::sin(phase * 3.14159f) * foot.liftHeight;
    return placement.targetPosition + glm::vec3(0, arcHeight, 0);
}

bool ProceduralLocomotion::raycastGround(const glm::vec3& origin, glm::vec3& hitPoint, glm::vec3& hitNormal) {
    if (m_groundCallback) {
        return m_groundCallback(origin, glm::vec3(0, -1, 0), 10.0f, hitPoint, hitNormal);
    }

    // Default: flat ground at y=0
    hitPoint = glm::vec3(origin.x, 0.0f, origin.z);
    hitNormal = glm::vec3(0, 1, 0);
    return true;
}

float ProceduralLocomotion::getFootPhase(size_t footIndex) const {
    if (footIndex >= m_gaitTiming.phaseOffsets.size()) {
        return m_gaitPhase;
    }

    float phase = m_gaitPhase + m_gaitTiming.phaseOffsets[footIndex];
    while (phase >= 1.0f) phase -= 1.0f;
    while (phase < 0.0f) phase += 1.0f;
    return phase;
}

// ============================================================================
// Gait Presets
// ============================================================================

namespace GaitPresets {

GaitTiming bipedWalk() {
    GaitTiming timing;
    timing.cycleTime = 1.0f;
    timing.dutyFactor = 0.6f;
    timing.phaseOffsets = {0.0f, 0.5f}; // Left, Right - opposite phases
    return timing;
}

GaitTiming bipedRun() {
    GaitTiming timing;
    timing.cycleTime = 0.5f;
    timing.dutyFactor = 0.3f;
    timing.phaseOffsets = {0.0f, 0.5f};
    return timing;
}

GaitTiming quadrupedWalk() {
    GaitTiming timing;
    timing.cycleTime = 1.2f;
    timing.dutyFactor = 0.75f;
    // FL, FR, BL, BR - lateral sequence walk
    timing.phaseOffsets = {0.0f, 0.5f, 0.75f, 0.25f};
    return timing;
}

GaitTiming quadrupedTrot() {
    GaitTiming timing;
    timing.cycleTime = 0.6f;
    timing.dutyFactor = 0.5f;
    // Diagonal pairs: FL+BR and FR+BL
    timing.phaseOffsets = {0.0f, 0.5f, 0.5f, 0.0f};
    return timing;
}

GaitTiming quadrupedGallop() {
    GaitTiming timing;
    timing.cycleTime = 0.4f;
    timing.dutyFactor = 0.25f;
    // Rotary gallop: front together, back together, with offset
    timing.phaseOffsets = {0.0f, 0.1f, 0.5f, 0.6f};
    return timing;
}

GaitTiming hexapodWalk() {
    GaitTiming timing;
    timing.cycleTime = 0.8f;
    timing.dutyFactor = 0.5f;
    // Tripod gait: alternating triangles
    // L1, R1, L2, R2, L3, R3
    timing.phaseOffsets = {0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f};
    return timing;
}

GaitTiming octopodWalk() {
    GaitTiming timing;
    timing.cycleTime = 1.0f;
    timing.dutyFactor = 0.75f;
    // Wave gait: sequential leg movement
    timing.phaseOffsets = {0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f};
    return timing;
}

} // namespace GaitPresets

// ============================================================================
// Locomotion Setup Helpers
// ============================================================================

namespace LocomotionSetup {

void setupBiped(ProceduralLocomotion& loco, const Skeleton& skeleton) {
    loco.initialize(skeleton);

    // Left leg
    int32_t lHip = skeleton.findBoneIndex("hip_l");
    int32_t lKnee = skeleton.findBoneIndex("knee_l");
    int32_t lAnkle = skeleton.findBoneIndex("ankle_l");
    int32_t lFoot = skeleton.findBoneIndex("foot_l");

    if (lHip >= 0 && lKnee >= 0 && lAnkle >= 0 && lFoot >= 0) {
        FootConfig leftFoot;
        leftFoot.hipBoneIndex = lHip;
        leftFoot.kneeBoneIndex = lKnee;
        leftFoot.ankleBoneIndex = lAnkle;
        leftFoot.footBoneIndex = lFoot;
        leftFoot.restOffset = glm::vec3(-0.1f, 0.0f, 0.0f);
        leftFoot.phaseOffset = 0.0f;
        loco.addFoot(leftFoot);
    }

    // Right leg
    int32_t rHip = skeleton.findBoneIndex("hip_r");
    int32_t rKnee = skeleton.findBoneIndex("knee_r");
    int32_t rAnkle = skeleton.findBoneIndex("ankle_r");
    int32_t rFoot = skeleton.findBoneIndex("foot_r");

    if (rHip >= 0 && rKnee >= 0 && rAnkle >= 0 && rFoot >= 0) {
        FootConfig rightFoot;
        rightFoot.hipBoneIndex = rHip;
        rightFoot.kneeBoneIndex = rKnee;
        rightFoot.ankleBoneIndex = rAnkle;
        rightFoot.footBoneIndex = rFoot;
        rightFoot.restOffset = glm::vec3(0.1f, 0.0f, 0.0f);
        rightFoot.phaseOffset = 0.5f;
        loco.addFoot(rightFoot);
    }

    loco.setGaitType(GaitType::Walk);
}

void setupQuadruped(ProceduralLocomotion& loco, const Skeleton& skeleton) {
    loco.initialize(skeleton);

    // Front left
    int32_t flShoulder = skeleton.findBoneIndex("shoulder_fl");
    int32_t flElbow = skeleton.findBoneIndex("elbow_fl");
    int32_t flWrist = skeleton.findBoneIndex("wrist_fl");
    int32_t flFoot = skeleton.findBoneIndex("foot_fl");

    if (flShoulder >= 0) {
        FootConfig fl;
        fl.hipBoneIndex = flShoulder;
        fl.kneeBoneIndex = flElbow;
        fl.ankleBoneIndex = flWrist;
        fl.footBoneIndex = flFoot;
        fl.restOffset = glm::vec3(-0.15f, 0.0f, 0.3f);
        fl.phaseOffset = 0.0f;
        loco.addFoot(fl);
    }

    // Front right
    int32_t frShoulder = skeleton.findBoneIndex("shoulder_fr");
    int32_t frElbow = skeleton.findBoneIndex("elbow_fr");
    int32_t frWrist = skeleton.findBoneIndex("wrist_fr");
    int32_t frFoot = skeleton.findBoneIndex("foot_fr");

    if (frShoulder >= 0) {
        FootConfig fr;
        fr.hipBoneIndex = frShoulder;
        fr.kneeBoneIndex = frElbow;
        fr.ankleBoneIndex = frWrist;
        fr.footBoneIndex = frFoot;
        fr.restOffset = glm::vec3(0.15f, 0.0f, 0.3f);
        fr.phaseOffset = 0.5f;
        loco.addFoot(fr);
    }

    // Back left
    int32_t blHip = skeleton.findBoneIndex("hip_bl");
    int32_t blKnee = skeleton.findBoneIndex("knee_bl");
    int32_t blAnkle = skeleton.findBoneIndex("ankle_bl");
    int32_t blFoot = skeleton.findBoneIndex("foot_bl");

    if (blHip >= 0) {
        FootConfig bl;
        bl.hipBoneIndex = blHip;
        bl.kneeBoneIndex = blKnee;
        bl.ankleBoneIndex = blAnkle;
        bl.footBoneIndex = blFoot;
        bl.restOffset = glm::vec3(-0.15f, 0.0f, -0.3f);
        bl.phaseOffset = 0.75f;
        loco.addFoot(bl);
    }

    // Back right
    int32_t brHip = skeleton.findBoneIndex("hip_br");
    int32_t brKnee = skeleton.findBoneIndex("knee_br");
    int32_t brAnkle = skeleton.findBoneIndex("ankle_br");
    int32_t brFoot = skeleton.findBoneIndex("foot_br");

    if (brHip >= 0) {
        FootConfig br;
        br.hipBoneIndex = brHip;
        br.kneeBoneIndex = brKnee;
        br.ankleBoneIndex = brAnkle;
        br.footBoneIndex = brFoot;
        br.restOffset = glm::vec3(0.15f, 0.0f, -0.3f);
        br.phaseOffset = 0.25f;
        loco.addFoot(br);
    }

    loco.setGaitType(GaitType::Walk);
}

void setupFlying(ProceduralLocomotion& loco, const Skeleton& skeleton) {
    loco.initialize(skeleton);

    // Left wing
    int32_t lWing1 = skeleton.findBoneIndex("wing_l_1");
    int32_t lWing2 = skeleton.findBoneIndex("wing_l_2");
    int32_t lWing3 = skeleton.findBoneIndex("wing_l_3");
    int32_t lWingTip = skeleton.findBoneIndex("wing_l_tip");

    if (lWing1 >= 0) {
        WingConfig leftWing;
        leftWing.shoulderBoneIndex = lWing1;
        leftWing.elbowBoneIndex = lWing2;
        leftWing.wristBoneIndex = lWing3;
        leftWing.tipBoneIndex = lWingTip;
        leftWing.flapAmplitude = 45.0f;
        leftWing.phaseOffset = 0.0f;
        loco.addWing(leftWing);
    }

    // Right wing
    int32_t rWing1 = skeleton.findBoneIndex("wing_r_1");
    int32_t rWing2 = skeleton.findBoneIndex("wing_r_2");
    int32_t rWing3 = skeleton.findBoneIndex("wing_r_3");
    int32_t rWingTip = skeleton.findBoneIndex("wing_r_tip");

    if (rWing1 >= 0) {
        WingConfig rightWing;
        rightWing.shoulderBoneIndex = rWing1;
        rightWing.elbowBoneIndex = rWing2;
        rightWing.wristBoneIndex = rWing3;
        rightWing.tipBoneIndex = rWingTip;
        rightWing.flapAmplitude = 45.0f;
        rightWing.phaseOffset = 0.0f; // Same phase for symmetry
        loco.addWing(rightWing);
    }

    loco.setGaitType(GaitType::Fly);
}

void setupAquatic(ProceduralLocomotion& loco, const Skeleton& skeleton) {
    loco.initialize(skeleton);

    // Find all body segments
    SpineConfig spine;
    for (int i = 0; i < 10; ++i) {
        std::string name = "body_" + std::to_string(i);
        int32_t boneIndex = skeleton.findBoneIndex(name);
        if (boneIndex >= 0) {
            spine.boneIndices.push_back(boneIndex);
        }
    }

    // Add tail
    int32_t tailBase = skeleton.findBoneIndex("tail_base");
    int32_t tailFin = skeleton.findBoneIndex("tail_fin");
    if (tailBase >= 0) spine.boneIndices.push_back(tailBase);
    if (tailFin >= 0) spine.boneIndices.push_back(tailFin);

    if (!spine.boneIndices.empty()) {
        spine.waveMagnitude = 0.15f;
        spine.waveFrequency = 0.8f;
        spine.waveSpeed = 3.0f;
        loco.setSpine(spine);
    }

    loco.setGaitType(GaitType::Swim);
}

void setupSerpentine(ProceduralLocomotion& loco, const Skeleton& skeleton) {
    loco.initialize(skeleton);

    // Find all segments
    SpineConfig spine;
    for (int i = 0; i < 20; ++i) {
        std::string name = "segment_" + std::to_string(i);
        int32_t boneIndex = skeleton.findBoneIndex(name);
        if (boneIndex >= 0) {
            spine.boneIndices.push_back(boneIndex);
        }
    }

    if (!spine.boneIndices.empty()) {
        spine.waveMagnitude = 0.3f;
        spine.waveFrequency = 0.5f;
        spine.waveSpeed = 2.5f;
        loco.setSpine(spine);
    }

    loco.setGaitType(GaitType::Crawl);
}

} // namespace LocomotionSetup

// =============================================================================
// MORPHOLOGY LOCOMOTION IMPLEMENTATION
// =============================================================================

MorphologyLocomotion::MorphologyLocomotion()
    : m_velocity(0.0f)
    , m_bodyPosition(0.0f)
    , m_bodyRotation(1.0f, 0.0f, 0.0f, 0.0f)
{
}

void MorphologyLocomotion::initializeFromMorphology(const MorphologyGenes& genes, const Skeleton& skeleton) {
    m_legCount = genes.legPairs * 2;
    m_hasWings = genes.wingPairs > 0;
    m_hasTail = genes.hasTail;
    m_hasSpine = genes.segmentCount > 1;

    // Initialize the gait generator from morphology
    m_gaitGenerator.initializeFromMorphology(genes);

    // Set up legacy locomotion for wing/spine animation
    m_legacyLoco.initialize(skeleton);

    if (m_hasWings) {
        LocomotionSetup::setupFlying(m_legacyLoco, skeleton);
    }

    if (m_hasSpine && m_legCount == 0) {
        LocomotionSetup::setupSerpentine(m_legacyLoco, skeleton);
    } else if (m_hasSpine) {
        LocomotionSetup::setupAquatic(m_legacyLoco, skeleton);
    }
}

void MorphologyLocomotion::setGroundCallback(GroundCallback callback) {
    m_gaitGenerator.setGroundCallback([callback](const glm::vec3& origin, const glm::vec3& dir,
                                                  float maxDist, glm::vec3& hit, glm::vec3& normal) {
        return callback(origin, dir, maxDist, hit, normal);
    });
    m_legacyLoco.setGroundCallback(callback);
}

void MorphologyLocomotion::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
    m_gaitGenerator.setVelocity(velocity);
    m_legacyLoco.setVelocity(velocity);
}

void MorphologyLocomotion::setAngularVelocity(float omega) {
    m_angularVelocity = omega;
    m_gaitGenerator.setTurnRate(omega);
    m_legacyLoco.setAngularVelocity(omega);
}

void MorphologyLocomotion::setBodyTransform(const glm::vec3& position, const glm::quat& rotation) {
    m_bodyPosition = position;
    m_bodyRotation = rotation;
    m_gaitGenerator.setBodyTransform(position, rotation);
    m_legacyLoco.setBodyPosition(position);
    m_legacyLoco.setBodyRotation(rotation);
}

void MorphologyLocomotion::setTerrainSlope(float slopeAngle) {
    m_terrainSlope = slopeAngle;
}

void MorphologyLocomotion::setTerrainRoughness(float roughness) {
    m_terrainRoughness = roughness;
}

void MorphologyLocomotion::setIsSwimming(bool swimming) {
    m_isSwimming = swimming;
    if (swimming) {
        m_legacyLoco.setGaitType(GaitType::Swim);
    }
}

void MorphologyLocomotion::setIsFlying(bool flying) {
    m_isFlying = flying;
    if (flying) {
        m_legacyLoco.setGaitType(GaitType::Fly);
    }
}

void MorphologyLocomotion::update(float deltaTime) {
    // Select gait based on conditions
    float speed = glm::length(m_velocity);

    if (m_isSwimming) {
        m_gaitGenerator.requestGait(GaitPattern::SWIMMING_FISH);
    } else if (m_isFlying) {
        m_gaitGenerator.requestGait(GaitPattern::FLIGHT_FLAPPING);
    } else {
        // Let gait generator auto-select based on speed
        m_gaitGenerator.setTargetSpeed(speed);
    }

    // Update gait generator
    m_gaitGenerator.update(deltaTime);

    // Update legacy locomotion (for wings/spine)
    m_legacyLoco.update(deltaTime);
}

void MorphologyLocomotion::applyToPose(const Skeleton& skeleton, SkeletonPose& pose, IKSystem& ikSystem) {
    // Apply leg IK from gait generator
    const GaitState& state = m_gaitGenerator.getState();

    for (size_t i = 0; i < state.legs.size(); ++i) {
        const GaitState::LegState& leg = state.legs[i];

        IKTarget target;
        target.position = leg.currentTarget;
        target.weight = leg.blendWeight;

        // Use two-bone IK for legs (simplified - actual implementation would use bone indices)
        // This is a placeholder that should be connected to actual skeleton bone indices
    }

    // Apply wing/spine animation from legacy system
    if (m_hasWings || m_hasSpine) {
        m_legacyLoco.applyToPose(skeleton, pose, ikSystem);
    }
}

GaitPattern MorphologyLocomotion::getCurrentGait() const {
    return m_gaitGenerator.getCurrentGait();
}

float MorphologyLocomotion::getGaitPhase() const {
    return m_gaitGenerator.getGaitPhase();
}

glm::vec3 MorphologyLocomotion::getBodyOffset() const {
    return m_gaitGenerator.getBodyOffset();
}

glm::quat MorphologyLocomotion::getBodyTilt() const {
    return m_gaitGenerator.getBodyTilt();
}

bool MorphologyLocomotion::isInMotion() const {
    return glm::length(m_velocity) > 0.01f;
}

bool MorphologyLocomotion::isInTransition() const {
    return m_gaitGenerator.isTransitioning();
}

// =============================================================================
// LOCOMOTION STYLES IMPLEMENTATION
// =============================================================================

namespace LocomotionStyles {

void applyBipedStyle(MorphologyLocomotion& loco, const BipedStyle& style) {
    GaitParameters walkParams = GaitGenerator::createBipedWalk();
    walkParams.speedMax = style.walkSpeed;
    walkParams.bodySwayAmplitude = style.hipSwayAmount;
    walkParams.bodyRollAmplitude = style.shoulderRollAmount;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::BIPED_WALK, walkParams);

    GaitParameters runParams = GaitGenerator::createBipedRun();
    runParams.speedMax = style.runSpeed;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::BIPED_RUN, runParams);
}

void applyQuadrupedStyle(MorphologyLocomotion& loco, const QuadrupedStyle& style) {
    GaitParameters walkParams = GaitGenerator::createQuadrupedWalk();
    walkParams.speedMax = style.walkSpeed;
    walkParams.bodySwayAmplitude = style.shoulderDropAmount;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::QUADRUPED_WALK, walkParams);

    GaitParameters trotParams = GaitGenerator::createQuadrupedTrot();
    trotParams.speedMin = style.walkSpeed;
    trotParams.speedMax = style.trotSpeed;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::QUADRUPED_TROT, trotParams);

    GaitParameters gallopParams = GaitGenerator::createQuadrupedGallop();
    gallopParams.speedMin = style.trotSpeed;
    gallopParams.speedMax = style.gallopSpeed;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::QUADRUPED_GALLOP, gallopParams);
}

void applyHexapodStyle(MorphologyLocomotion& loco, const HexapodStyle& style) {
    GaitParameters tripodParams = GaitGenerator::createHexapodTripod();
    tripodParams.speedMax = style.runSpeed;
    tripodParams.stepHeight = style.legLiftHeight;
    tripodParams.bodyPitchAmplitude = style.bodyPitchAmount;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::HEXAPOD_TRIPOD, tripodParams);

    GaitParameters rippleParams = GaitGenerator::createHexapodRipple();
    rippleParams.speedMax = style.walkSpeed;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::HEXAPOD_RIPPLE, rippleParams);
}

void applyOctopodStyle(MorphologyLocomotion& loco, const OctopodStyle& style) {
    GaitParameters waveParams = GaitGenerator::createOctopodWave();
    waveParams.speedMax = style.runSpeed;
    waveParams.bodyBobAmplitude = style.bodyLowerAmount;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::OCTOPOD_WAVE, waveParams);
}

void applySerpentineStyle(MorphologyLocomotion& loco, const SerpentineStyle& style) {
    GaitParameters lateralParams = GaitGenerator::createSerpentineLateral();
    lateralParams.speedMax = style.crawlSpeed;
    lateralParams.bodySwayAmplitude = style.waveAmplitude;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::SERPENTINE_LATERAL, lateralParams);

    GaitParameters rectiParams = GaitGenerator::createSerpentineRectilinear();
    rectiParams.speedMax = style.crawlSpeed * 0.3f;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::SERPENTINE_RECTILINEAR, rectiParams);
}

void applyAvianStyle(MorphologyLocomotion& loco, const AvianStyle& style) {
    GaitParameters walkParams = GaitGenerator::createBipedWalk();
    walkParams.speedMax = style.walkSpeed;
    walkParams.bodyBobAmplitude = style.headBobAmount;
    walkParams.stepHeight = style.hopHeight;
    loco.getGaitGenerator().setGaitParameters(GaitPattern::BIPED_WALK, walkParams);
}

void applyAquaticStyle(MorphologyLocomotion& loco, const AquaticStyle& style) {
    // Aquatic creatures use swim animation, not gaits
    // Configure via SwimAnimator instead
}

void autoApplyStyle(MorphologyLocomotion& loco, const MorphologyGenes& genes) {
    int legCount = genes.legPairs * 2;

    if (legCount == 0) {
        SerpentineStyle style;
        style.waveAmplitude = genes.bodyLength * 0.15f;
        applySerpentineStyle(loco, style);
    } else if (legCount == 2) {
        if (genes.wingPairs > 0) {
            AvianStyle style;
            applyAvianStyle(loco, style);
        } else {
            BipedStyle style;
            applyBipedStyle(loco, style);
        }
    } else if (legCount == 4) {
        QuadrupedStyle style;
        applyQuadrupedStyle(loco, style);
    } else if (legCount == 6) {
        HexapodStyle style;
        applyHexapodStyle(loco, style);
    } else if (legCount >= 8) {
        OctopodStyle style;
        applyOctopodStyle(loco, style);
    }
}

} // namespace LocomotionStyles

} // namespace animation
