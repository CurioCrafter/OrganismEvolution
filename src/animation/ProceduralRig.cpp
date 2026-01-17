#include "ProceduralRig.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace animation {

// =============================================================================
// RIG DEFINITION IMPLEMENTATION
// =============================================================================

bool RigDefinition::isValid() const {
    if (totalBones <= 0 || totalBones > static_cast<int>(MAX_BONES)) return false;
    if (spine.boneCount <= 0) return false;
    return true;
}

std::string RigDefinition::getDebugInfo() const {
    std::stringstream ss;
    ss << "RigDefinition:\n";
    ss << "  Category: " << static_cast<int>(category) << "\n";
    ss << "  Total Bones: " << totalBones << "\n";
    ss << "  Spine Bones: " << spine.boneCount << "\n";
    ss << "  Tail Bones: " << tail.boneCount << "\n";
    ss << "  Limbs: " << limbs.size() << "\n";
    for (size_t i = 0; i < limbs.size(); ++i) {
        ss << "    Limb " << i << ": type=" << static_cast<int>(limbs[i].type)
           << ", segments=" << limbs[i].segmentCount << "\n";
    }
    return ss.str();
}

// =============================================================================
// PROCEDURAL RIG GENERATOR IMPLEMENTATION
// =============================================================================

ProceduralRigGenerator::ProceduralRigGenerator() {
    // Default config is fine
}

RigDefinition::RigCategory ProceduralRigGenerator::categorizeFromMorphology(const MorphologyGenes& genes) {
    // Radial symmetry
    if (genes.symmetry == SymmetryType::RADIAL) {
        return RigDefinition::RigCategory::RADIAL;
    }

    // Aquatic without legs
    int totalLegs = genes.legPairs * 2;
    if (genes.finCount > 0 && totalLegs == 0) {
        return RigDefinition::RigCategory::AQUATIC;
    }

    // Serpentine (no legs, long body)
    if (totalLegs == 0 && genes.segmentCount >= 5) {
        return RigDefinition::RigCategory::SERPENTINE;
    }

    // Flying (has wings that allow flight)
    if (genes.canFly && genes.wingPairs > 0) {
        return RigDefinition::RigCategory::FLYING;
    }

    // Biped
    if (totalLegs == 2) {
        return RigDefinition::RigCategory::BIPED;
    }

    // Quadruped
    if (totalLegs == 4) {
        return RigDefinition::RigCategory::QUADRUPED;
    }

    // Hexapod
    if (totalLegs == 6) {
        return RigDefinition::RigCategory::HEXAPOD;
    }

    // Octopod
    if (totalLegs >= 8 || genes.tentacleCount >= 8) {
        return RigDefinition::RigCategory::OCTOPOD;
    }

    // Default to quadruped
    return RigDefinition::RigCategory::QUADRUPED;
}

RigDefinition ProceduralRigGenerator::generateRigDefinition(const MorphologyGenes& genes) const {
    RigDefinition rig;
    rig.sourceGenes = genes;
    rig.category = categorizeFromMorphology(genes);

    // Build rig based on category
    switch (rig.category) {
        case RigDefinition::RigCategory::BIPED:
            buildBipedRig(rig, genes);
            break;
        case RigDefinition::RigCategory::QUADRUPED:
            buildQuadrupedRig(rig, genes);
            break;
        case RigDefinition::RigCategory::HEXAPOD:
            buildHexapodRig(rig, genes);
            break;
        case RigDefinition::RigCategory::SERPENTINE:
            buildSerpentineRig(rig, genes);
            break;
        case RigDefinition::RigCategory::AQUATIC:
            buildAquaticRig(rig, genes);
            break;
        case RigDefinition::RigCategory::FLYING:
            buildFlyingRig(rig, genes);
            break;
        case RigDefinition::RigCategory::RADIAL:
            buildRadialRig(rig, genes);
            break;
        default:
            buildQuadrupedRig(rig, genes);
            break;
    }

    // Count total bones
    rig.totalBones = rig.spine.boneCount + rig.tail.boneCount + 1; // +1 for head
    if (rig.head.jawBone >= 0) rig.totalBones++;
    for (const auto& limb : rig.limbs) {
        rig.totalBones += limb.segmentCount;
    }
    // Add feature bones
    rig.totalBones += static_cast<int>(rig.head.hornBones.size());
    rig.totalBones += static_cast<int>(rig.head.antennaeBones.size());
    if (rig.head.crestBone >= 0) rig.totalBones++;
    if (rig.head.frillBone >= 0) rig.totalBones++;

    return rig;
}

Skeleton ProceduralRigGenerator::buildSkeleton(const RigDefinition& rig) const {
    Skeleton skeleton;

    // Add root bone
    BoneTransform rootTransform;
    rootTransform.translation = glm::vec3(0.0f);
    int32_t rootBone = skeleton.addBone(RigBoneNames::ROOT, -1, rootTransform);

    // Add spine
    int32_t lastSpineBone = addSpineBones(skeleton, rig.spine);

    // Add head to last spine bone (or neck)
    int32_t headParent = lastSpineBone;
    if (!rig.spine.boneIndices.empty()) {
        headParent = rig.spine.boneIndices.back();
    }

    // Use a const_cast here since we're building the skeleton and need to update bone indices
    HeadDefinition headCopy = rig.head;
    addHeadBones(skeleton, headCopy, headParent);

    // Add tail from pelvis/rear of spine
    int32_t tailParent = rootBone;
    if (rig.spine.hipBone >= 0) {
        tailParent = rig.spine.hipBone;
    } else if (!rig.spine.boneIndices.empty()) {
        tailParent = rig.spine.boneIndices[0];
    }

    TailDefinition tailCopy = rig.tail;
    addTailBones(skeleton, tailCopy, tailParent);

    // Add limbs
    for (size_t i = 0; i < rig.limbs.size(); ++i) {
        LimbDefinition limbCopy = rig.limbs[i];

        // Find attachment bone
        int32_t limbParent = rootBone;
        float attachPos = limbCopy.attachmentPosition;
        int spineIdx = static_cast<int>(attachPos * (rig.spine.boneCount - 1));
        spineIdx = std::clamp(spineIdx, 0, static_cast<int>(rig.spine.boneIndices.size()) - 1);
        if (!rig.spine.boneIndices.empty()) {
            limbParent = rig.spine.boneIndices[spineIdx];
        }

        addLimbBones(skeleton, limbCopy, limbParent);
    }

    // Calculate bone lengths and inverse bind matrices
    skeleton.calculateBoneLengths();

    return skeleton;
}

Skeleton ProceduralRigGenerator::generateSkeleton(const MorphologyGenes& genes) const {
    RigDefinition rig = generateRigDefinition(genes);
    return buildSkeleton(rig);
}

Skeleton ProceduralRigGenerator::generateSkeletonLOD(const MorphologyGenes& genes, int lodLevel) const {
    RigDefinition fullRig = generateRigDefinition(genes);
    RigDefinition lodRig = reduceLOD(fullRig, lodLevel);
    return buildSkeleton(lodRig);
}

// =============================================================================
// SPINE GENERATION
// =============================================================================

void ProceduralRigGenerator::generateSpine(RigDefinition& rig, const MorphologyGenes& genes) const {
    SpineDefinition& spine = rig.spine;

    // Calculate bone count based on segment count
    int baseSpineCount = genes.segmentCount + 1;
    spine.boneCount = std::clamp(baseSpineCount, 3, m_config.maxSpineBones);

    // Calculate total length
    spine.totalLength = genes.bodyLength * m_config.boneLengthScale;

    // Distribute bone lengths (tapered)
    spine.boneLengths.resize(spine.boneCount);
    float lengthSum = 0.0f;
    for (int i = 0; i < spine.boneCount; ++i) {
        // Slight taper toward head
        float t = static_cast<float>(i) / (spine.boneCount - 1);
        spine.boneLengths[i] = 1.0f - 0.2f * t;
        lengthSum += spine.boneLengths[i];
    }

    // Normalize to total length
    for (int i = 0; i < spine.boneCount; ++i) {
        spine.boneLengths[i] = (spine.boneLengths[i] / lengthSum) * spine.totalLength;
    }

    // Set width (for collision/mesh)
    spine.boneWidths.resize(spine.boneCount);
    for (int i = 0; i < spine.boneCount; ++i) {
        float t = static_cast<float>(i) / (spine.boneCount - 1);
        spine.boneWidths[i] = genes.bodyWidth * (1.0f - 0.3f * t) * m_config.boneLengthScale;
    }

    // Flexibility
    spine.neckFlexibility = genes.neckFlexibility * m_config.spineFlexibility;
    spine.torsoFlexibility = m_config.spineFlexibility * 0.5f;
    spine.pelvisFlexibility = m_config.spineFlexibility * 0.3f;
}

// =============================================================================
// TAIL GENERATION
// =============================================================================

void ProceduralRigGenerator::generateTail(RigDefinition& rig, const MorphologyGenes& genes) const {
    TailDefinition& tail = rig.tail;

    if (!genes.hasTail) {
        tail.boneCount = 0;
        return;
    }

    // Calculate bone count based on tail segments
    tail.boneCount = std::clamp(genes.tailSegments, 1, m_config.maxTailBones);
    tail.totalLength = genes.tailLength * genes.bodyLength * m_config.boneLengthScale;
    tail.baseTickness = genes.tailThickness * m_config.boneLengthScale;
    tail.tipThickness = tail.baseTickness * genes.tailTaper;
    tail.tailType = genes.tailType;

    // Motion parameters based on tail type
    switch (genes.tailType) {
        case TailType::STANDARD:
            tail.wagAmplitude = 0.3f;
            tail.wagFrequency = 2.0f;
            tail.flexibility = 0.7f;
            break;
        case TailType::CLUBBED:
            tail.wagAmplitude = 0.2f;
            tail.wagFrequency = 1.5f;
            tail.flexibility = 0.4f;
            break;
        case TailType::FAN:
            tail.wagAmplitude = 0.4f;
            tail.wagFrequency = 1.0f;
            tail.flexibility = 0.5f;
            break;
        case TailType::WHIP:
            tail.wagAmplitude = 0.5f;
            tail.wagFrequency = 3.0f;
            tail.flexibility = 0.9f;
            break;
        case TailType::PREHENSILE:
            tail.wagAmplitude = 0.2f;
            tail.wagFrequency = 0.5f;
            tail.flexibility = 0.95f;
            break;
        default:
            tail.wagAmplitude = 0.3f;
            tail.wagFrequency = 2.0f;
            tail.flexibility = 0.6f;
            break;
    }
}

// =============================================================================
// HEAD GENERATION
// =============================================================================

void ProceduralRigGenerator::generateHead(RigDefinition& rig, const MorphologyGenes& genes) const {
    HeadDefinition& head = rig.head;

    head.headSize = genes.headSize * m_config.boneLengthScale;
    head.jawRange = 0.5f; // ~30 degrees

    // Eyes
    if (genes.eyeCount >= 2) {
        // Left and right eye bones will be added
    }
}

// =============================================================================
// LIMB GENERATION
// =============================================================================

void ProceduralRigGenerator::generateLegs(RigDefinition& rig, const MorphologyGenes& genes) const {
    int legPairs = genes.legPairs;
    if (legPairs <= 0) return;

    float bodyLength = genes.bodyLength * m_config.boneLengthScale;
    float legLength = genes.legLength * bodyLength;

    for (int pair = 0; pair < legPairs; ++pair) {
        // Left leg
        LimbDefinition leftLeg;
        leftLeg.type = (pair == 0) ? LimbType::LEG_FRONT :
                       (pair == legPairs - 1) ? LimbType::LEG_REAR : LimbType::LEG_MIDDLE;
        leftLeg.side = LimbSide::LEFT;
        leftLeg.segmentCount = std::clamp(genes.legSegments, 2, m_config.maxLimbBones);
        leftLeg.totalLength = legLength;
        leftLeg.baseThickness = genes.legThickness * m_config.boneLengthScale;
        leftLeg.taperRatio = 0.7f;

        // Attachment position along body (0 = front, 1 = back)
        if (legPairs == 1) {
            leftLeg.attachmentPosition = 0.5f;
        } else {
            leftLeg.attachmentPosition = static_cast<float>(pair) / (legPairs - 1);
        }

        leftLeg.attachmentOffset = glm::vec3(-genes.bodyWidth * 0.5f * genes.legSpread, 0.0f, 0.0f);
        leftLeg.jointType = JointType::HINGE;
        leftLeg.jointFlexibility = genes.jointFlexibility;
        leftLeg.restRotation = glm::quat(1, 0, 0, 0);
        leftLeg.restSpread = genes.legSpread * 0.5f;

        rig.limbs.push_back(leftLeg);

        // Right leg (mirror)
        LimbDefinition rightLeg = leftLeg;
        rightLeg.side = LimbSide::RIGHT;
        rightLeg.attachmentOffset = glm::vec3(genes.bodyWidth * 0.5f * genes.legSpread, 0.0f, 0.0f);
        rig.limbs.push_back(rightLeg);
    }
}

void ProceduralRigGenerator::generateArms(RigDefinition& rig, const MorphologyGenes& genes) const {
    int armPairs = genes.armPairs;
    if (armPairs <= 0) return;

    float bodyLength = genes.bodyLength * m_config.boneLengthScale;
    float armLength = genes.armLength * bodyLength;

    for (int pair = 0; pair < armPairs; ++pair) {
        // Left arm
        LimbDefinition leftArm;
        leftArm.type = LimbType::ARM;
        leftArm.side = LimbSide::LEFT;
        leftArm.segmentCount = std::clamp(genes.armSegments, 2, m_config.maxLimbBones);
        leftArm.totalLength = armLength;
        leftArm.baseThickness = genes.armThickness * m_config.boneLengthScale;
        leftArm.taperRatio = 0.6f;

        // Arms attach near shoulders (front of body)
        leftArm.attachmentPosition = 0.8f + pair * 0.1f;
        leftArm.attachmentOffset = glm::vec3(-genes.bodyWidth * 0.5f, genes.bodyHeight * 0.3f, 0.0f);
        leftArm.jointType = JointType::BALL_SOCKET;
        leftArm.jointFlexibility = genes.jointFlexibility;
        leftArm.restRotation = glm::quat(1, 0, 0, 0);
        leftArm.restSpread = 0.3f;

        rig.limbs.push_back(leftArm);

        // Right arm (mirror)
        LimbDefinition rightArm = leftArm;
        rightArm.side = LimbSide::RIGHT;
        rightArm.attachmentOffset = glm::vec3(genes.bodyWidth * 0.5f, genes.bodyHeight * 0.3f, 0.0f);
        rig.limbs.push_back(rightArm);
    }
}

void ProceduralRigGenerator::generateWings(RigDefinition& rig, const MorphologyGenes& genes) const {
    if (genes.wingPairs <= 0) return;

    float wingspan = genes.wingSpan * genes.bodyLength * m_config.boneLengthScale;

    for (int pair = 0; pair < genes.wingPairs; ++pair) {
        // Left wing
        LimbDefinition leftWing;
        leftWing.type = LimbType::WING;
        leftWing.side = LimbSide::LEFT;
        leftWing.segmentCount = std::clamp(4, 3, m_config.maxWingBones); // shoulder, elbow, wrist, tip
        leftWing.totalLength = wingspan * 0.5f;
        leftWing.baseThickness = 0.1f * m_config.boneLengthScale;
        leftWing.taperRatio = 0.3f;

        leftWing.attachmentPosition = 0.7f;
        leftWing.attachmentOffset = glm::vec3(-genes.bodyWidth * 0.4f, genes.bodyHeight * 0.4f, 0.0f);
        leftWing.jointType = JointType::BALL_SOCKET;
        leftWing.jointFlexibility = 0.9f;
        leftWing.restRotation = glm::angleAxis(1.2f, glm::vec3(0, 0, 1)); // Folded
        leftWing.restSpread = 0.0f;

        rig.limbs.push_back(leftWing);

        // Right wing (mirror)
        LimbDefinition rightWing = leftWing;
        rightWing.side = LimbSide::RIGHT;
        rightWing.attachmentOffset = glm::vec3(genes.bodyWidth * 0.4f, genes.bodyHeight * 0.4f, 0.0f);
        rightWing.restRotation = glm::angleAxis(-1.2f, glm::vec3(0, 0, 1));
        rig.limbs.push_back(rightWing);
    }
}

void ProceduralRigGenerator::generateFins(RigDefinition& rig, const MorphologyGenes& genes) const {
    float finSize = genes.finSize * m_config.boneLengthScale;

    // Dorsal fins
    for (int i = 0; i < genes.dorsalFinCount; ++i) {
        LimbDefinition dorsalFin;
        dorsalFin.type = LimbType::FIN_DORSAL;
        dorsalFin.side = LimbSide::CENTER;
        dorsalFin.segmentCount = 2;
        dorsalFin.totalLength = finSize;
        dorsalFin.baseThickness = finSize * 0.1f;
        dorsalFin.taperRatio = 0.5f;

        // Position along back
        float pos = 0.3f + static_cast<float>(i) / (genes.dorsalFinCount) * 0.4f;
        dorsalFin.attachmentPosition = pos;
        dorsalFin.attachmentOffset = glm::vec3(0.0f, genes.bodyHeight * 0.5f, 0.0f);
        dorsalFin.jointType = JointType::HINGE;
        dorsalFin.jointFlexibility = 0.3f;

        rig.limbs.push_back(dorsalFin);
    }

    // Pectoral fins (sides)
    for (int pair = 0; pair < genes.pectoralFinPairs; ++pair) {
        LimbDefinition leftPec;
        leftPec.type = LimbType::FIN_PECTORAL;
        leftPec.side = LimbSide::LEFT;
        leftPec.segmentCount = 2;
        leftPec.totalLength = finSize * 1.2f;
        leftPec.baseThickness = finSize * 0.08f;
        leftPec.taperRatio = 0.4f;

        leftPec.attachmentPosition = 0.6f;
        leftPec.attachmentOffset = glm::vec3(-genes.bodyWidth * 0.45f, 0.0f, 0.0f);
        leftPec.jointType = JointType::BALL_SOCKET;
        leftPec.jointFlexibility = 0.5f;

        rig.limbs.push_back(leftPec);

        LimbDefinition rightPec = leftPec;
        rightPec.side = LimbSide::RIGHT;
        rightPec.attachmentOffset = glm::vec3(genes.bodyWidth * 0.45f, 0.0f, 0.0f);
        rig.limbs.push_back(rightPec);
    }

    // Caudal fin (tail fin)
    if (genes.hasCaudalFin) {
        LimbDefinition caudalFin;
        caudalFin.type = LimbType::FIN_CAUDAL;
        caudalFin.side = LimbSide::CENTER;
        caudalFin.segmentCount = 2;
        caudalFin.totalLength = finSize * 1.5f;
        caudalFin.baseThickness = finSize * 0.05f;
        caudalFin.taperRatio = 0.3f;

        caudalFin.attachmentPosition = 0.0f; // Attaches to tail end
        caudalFin.attachmentOffset = glm::vec3(0.0f);
        caudalFin.jointType = JointType::HINGE;
        caudalFin.jointFlexibility = 0.6f;

        rig.limbs.push_back(caudalFin);
    }
}

void ProceduralRigGenerator::generateTentacles(RigDefinition& rig, const MorphologyGenes& genes) const {
    if (genes.tentacleCount <= 0) return;

    float tentacleLength = genes.tentacleLength * genes.bodyLength * m_config.boneLengthScale;

    for (int i = 0; i < genes.tentacleCount; ++i) {
        LimbDefinition tentacle;
        tentacle.type = LimbType::TENTACLE;

        // Distribute around body
        float angle = static_cast<float>(i) / genes.tentacleCount * 2.0f * 3.14159f;
        if (i < genes.tentacleCount / 2) {
            tentacle.side = LimbSide::LEFT;
        } else {
            tentacle.side = LimbSide::RIGHT;
        }

        tentacle.segmentCount = 6; // Tentacles are flexible, need more bones
        tentacle.totalLength = tentacleLength;
        tentacle.baseThickness = 0.1f * m_config.boneLengthScale;
        tentacle.taperRatio = 0.2f;

        tentacle.attachmentPosition = 0.9f; // Near head
        float radius = genes.bodyWidth * 0.4f;
        tentacle.attachmentOffset = glm::vec3(
            std::cos(angle) * radius,
            -genes.bodyHeight * 0.3f,
            std::sin(angle) * radius
        );
        tentacle.jointType = JointType::BALL_SOCKET;
        tentacle.jointFlexibility = 0.95f;

        rig.limbs.push_back(tentacle);
    }
}

void ProceduralRigGenerator::generateFeatures(RigDefinition& rig, const MorphologyGenes& genes) const {
    // Horns
    for (int i = 0; i < genes.hornCount; ++i) {
        // Horn bones are tracked in head definition
        rig.head.hornBones.push_back(-1); // Will be assigned during skeleton build
    }

    // Antennae
    for (int i = 0; i < genes.antennaeCount; ++i) {
        rig.head.antennaeBones.push_back(-1);
    }

    // Crest
    if (genes.crestType != CrestType::NONE && genes.crestHeight > 0.1f) {
        rig.head.crestBone = -1; // Will be assigned
    }

    // Frill
    if (genes.hasNeckFrill && genes.frillSize > 0.1f) {
        rig.head.frillBone = -1; // Will be assigned
    }
}

// =============================================================================
// SKELETON BUILDING
// =============================================================================

int32_t ProceduralRigGenerator::addSpineBones(Skeleton& skeleton, const SpineDefinition& spine) const {
    int32_t parentBone = 0; // Root bone

    // Add pelvis/hip first
    BoneTransform pelvisTransform;
    pelvisTransform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    int32_t pelvisBone = skeleton.addBone(RigBoneNames::PELVIS, parentBone, pelvisTransform);

    int32_t lastBone = pelvisBone;
    float accumulatedLength = 0.0f;

    for (int i = 0; i < spine.boneCount; ++i) {
        BoneTransform transform;
        // Spine extends forward (positive Z)
        transform.translation = glm::vec3(0.0f, 0.0f, spine.boneLengths[i]);

        std::string boneName = RigBoneNames::makeSpineBone(i);
        lastBone = skeleton.addBone(boneName, lastBone, transform);

        accumulatedLength += spine.boneLengths[i];
    }

    return lastBone;
}

int32_t ProceduralRigGenerator::addTailBones(Skeleton& skeleton, const TailDefinition& tail, int32_t parentBone) const {
    if (tail.boneCount <= 0) return parentBone;

    float boneLength = tail.totalLength / tail.boneCount;
    int32_t lastBone = parentBone;

    for (int i = 0; i < tail.boneCount; ++i) {
        BoneTransform transform;
        // Tail extends backward (negative Z)
        transform.translation = glm::vec3(0.0f, 0.0f, -boneLength);

        // Slight downward curve
        float curveAngle = -0.05f * (i + 1);
        transform.rotation = glm::angleAxis(curveAngle, glm::vec3(1, 0, 0));

        std::string boneName = RigBoneNames::makeTailBone(i);
        lastBone = skeleton.addBone(boneName, lastBone, transform);
    }

    return lastBone;
}

int32_t ProceduralRigGenerator::addHeadBones(Skeleton& skeleton, const HeadDefinition& head, int32_t parentBone) const {
    BoneTransform headTransform;
    headTransform.translation = glm::vec3(0.0f, 0.0f, head.headSize);
    int32_t headBone = skeleton.addBone(RigBoneNames::HEAD, parentBone, headTransform);

    // Jaw bone
    BoneTransform jawTransform;
    jawTransform.translation = glm::vec3(0.0f, -head.headSize * 0.2f, head.headSize * 0.3f);
    skeleton.addBone(RigBoneNames::JAW, headBone, jawTransform);

    // Eye bones would be added here for eye tracking
    if (head.leftEyeBone >= 0 || head.rightEyeBone >= 0) {
        BoneTransform leftEyeTransform;
        leftEyeTransform.translation = glm::vec3(-head.headSize * 0.25f, head.headSize * 0.1f, head.headSize * 0.3f);
        skeleton.addBone(std::string(RigBoneNames::EYE_PREFIX) + "L", headBone, leftEyeTransform);

        BoneTransform rightEyeTransform;
        rightEyeTransform.translation = glm::vec3(head.headSize * 0.25f, head.headSize * 0.1f, head.headSize * 0.3f);
        skeleton.addBone(std::string(RigBoneNames::EYE_PREFIX) + "R", headBone, rightEyeTransform);
    }

    return headBone;
}

void ProceduralRigGenerator::addLimbBones(Skeleton& skeleton, LimbDefinition& limb, int32_t parentBone) const {
    float segmentLength = limb.totalLength / limb.segmentCount;
    int32_t lastBone = parentBone;

    limb.boneIndices.clear();

    const char* sideSuffix = (limb.side == LimbSide::LEFT) ? RigBoneNames::LEFT :
                             (limb.side == LimbSide::RIGHT) ? RigBoneNames::RIGHT :
                             RigBoneNames::CENTER;

    for (int i = 0; i < limb.segmentCount; ++i) {
        BoneTransform transform;

        // Direction based on limb type
        switch (limb.type) {
            case LimbType::LEG_FRONT:
            case LimbType::LEG_REAR:
            case LimbType::LEG_MIDDLE:
                // Legs point down initially, then outward
                if (i == 0) {
                    transform.translation = limb.attachmentOffset;
                } else {
                    transform.translation = glm::vec3(0.0f, -segmentLength, 0.0f);
                }
                break;

            case LimbType::ARM:
                // Arms point outward and slightly down
                if (i == 0) {
                    transform.translation = limb.attachmentOffset;
                } else {
                    float sideDir = (limb.side == LimbSide::LEFT) ? -1.0f : 1.0f;
                    transform.translation = glm::vec3(sideDir * segmentLength * 0.3f, -segmentLength * 0.7f, 0.0f);
                }
                break;

            case LimbType::WING:
                // Wings extend outward
                if (i == 0) {
                    transform.translation = limb.attachmentOffset;
                } else {
                    float sideDir = (limb.side == LimbSide::LEFT) ? -1.0f : 1.0f;
                    transform.translation = glm::vec3(sideDir * segmentLength, 0.0f, 0.0f);
                }
                break;

            case LimbType::FIN_DORSAL:
                // Fins point upward
                transform.translation = (i == 0) ? limb.attachmentOffset :
                    glm::vec3(0.0f, segmentLength, 0.0f);
                break;

            case LimbType::FIN_PECTORAL:
                // Side fins point outward
                if (i == 0) {
                    transform.translation = limb.attachmentOffset;
                } else {
                    float sideDir = (limb.side == LimbSide::LEFT) ? -1.0f : 1.0f;
                    transform.translation = glm::vec3(sideDir * segmentLength, 0.0f, 0.0f);
                }
                break;

            case LimbType::TENTACLE:
                // Tentacles are flexible, curve downward
                if (i == 0) {
                    transform.translation = limb.attachmentOffset;
                } else {
                    transform.translation = glm::vec3(0.0f, -segmentLength * 0.8f, -segmentLength * 0.2f);
                }
                break;

            default:
                transform.translation = glm::vec3(0.0f, -segmentLength, 0.0f);
                break;
        }

        // Generate bone name
        std::stringstream ss;
        switch (limb.type) {
            case LimbType::LEG_FRONT:
            case LimbType::LEG_REAR:
            case LimbType::LEG_MIDDLE:
                ss << RigBoneNames::LEG_PREFIX << static_cast<int>(limb.type) << "_" << i << sideSuffix;
                break;
            case LimbType::ARM:
                ss << RigBoneNames::ARM_PREFIX << i << sideSuffix;
                break;
            case LimbType::WING:
                ss << RigBoneNames::WING_PREFIX << i << sideSuffix;
                break;
            case LimbType::FIN_DORSAL:
                ss << RigBoneNames::FIN_DORSAL << "_" << i;
                break;
            case LimbType::FIN_PECTORAL:
                ss << RigBoneNames::FIN_PECTORAL_PREFIX << i << sideSuffix;
                break;
            case LimbType::TENTACLE:
                ss << RigBoneNames::TENTACLE_PREFIX << i << sideSuffix;
                break;
            default:
                ss << "limb_" << i << sideSuffix;
                break;
        }

        int32_t boneIndex = skeleton.addBone(ss.str(), lastBone, transform);
        limb.boneIndices.push_back(boneIndex);
        lastBone = boneIndex;
    }

    // Set IK target to last bone
    if (!limb.boneIndices.empty()) {
        limb.ikTargetBone = limb.boneIndices.back();
    }
}

// =============================================================================
// SPECIALIZED RIG BUILDERS
// =============================================================================

void ProceduralRigGenerator::buildBipedRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    generateSpine(rig, genes);
    generateTail(rig, genes);
    generateHead(rig, genes);

    // Force 2 legs for biped
    MorphologyGenes bipedGenes = genes;
    bipedGenes.legPairs = 1;
    generateLegs(rig, bipedGenes);
    generateArms(rig, genes);
    generateWings(rig, genes);
    generateFeatures(rig, genes);
}

void ProceduralRigGenerator::buildQuadrupedRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    generateSpine(rig, genes);
    generateTail(rig, genes);
    generateHead(rig, genes);

    // Force 4 legs for quadruped
    MorphologyGenes quadGenes = genes;
    quadGenes.legPairs = 2;
    generateLegs(rig, quadGenes);
    generateWings(rig, genes);
    generateFeatures(rig, genes);
}

void ProceduralRigGenerator::buildHexapodRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    generateSpine(rig, genes);
    generateTail(rig, genes);
    generateHead(rig, genes);

    // 6 legs
    MorphologyGenes hexGenes = genes;
    hexGenes.legPairs = 3;
    generateLegs(rig, hexGenes);
    generateWings(rig, genes);
    generateFeatures(rig, genes);
}

void ProceduralRigGenerator::buildSerpentineRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    // Serpentine has many spine segments, no limbs
    MorphologyGenes serpGenes = genes;
    serpGenes.segmentCount = std::max(10, genes.segmentCount);
    generateSpine(rig, serpGenes);
    generateHead(rig, genes);

    // Optional fins for aquatic snakes
    if (genes.finCount > 0) {
        generateFins(rig, genes);
    }
}

void ProceduralRigGenerator::buildAquaticRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    generateSpine(rig, genes);
    generateTail(rig, genes);
    generateHead(rig, genes);
    generateFins(rig, genes);
}

void ProceduralRigGenerator::buildFlyingRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    generateSpine(rig, genes);
    generateTail(rig, genes);
    generateHead(rig, genes);

    // Wings are essential
    MorphologyGenes flyGenes = genes;
    if (flyGenes.wingPairs < 1) flyGenes.wingPairs = 1;
    generateWings(rig, flyGenes);

    // Optional legs
    if (genes.legPairs > 0) {
        generateLegs(rig, genes);
    }

    generateFeatures(rig, genes);
}

void ProceduralRigGenerator::buildRadialRig(RigDefinition& rig, const MorphologyGenes& genes) const {
    // Radial creatures have a central body with arms radiating out
    generateSpine(rig, genes);

    // Tentacles instead of legs
    if (genes.tentacleCount > 0) {
        generateTentacles(rig, genes);
    } else {
        // Default to 5-arm starfish style
        MorphologyGenes radialGenes = genes;
        radialGenes.tentacleCount = 5;
        generateTentacles(rig, radialGenes);
    }
}

// =============================================================================
// LOD HELPERS
// =============================================================================

RigDefinition ProceduralRigGenerator::reduceLOD(const RigDefinition& rig, int lodLevel) const {
    RigDefinition lodRig = rig;

    // Reduce spine bones
    lodRig.spine.boneCount = calculateLODBoneCount(rig.spine.boneCount, lodLevel);

    // Reduce tail bones
    lodRig.tail.boneCount = calculateLODBoneCount(rig.tail.boneCount, lodLevel);

    // Reduce limb segments
    for (auto& limb : lodRig.limbs) {
        limb.segmentCount = std::max(2, calculateLODBoneCount(limb.segmentCount, lodLevel));
    }

    // Remove some features at high LOD
    if (lodLevel >= 2) {
        lodRig.head.hornBones.clear();
        lodRig.head.antennaeBones.clear();
        lodRig.head.crestBone = -1;
        lodRig.head.frillBone = -1;
    }

    return lodRig;
}

int ProceduralRigGenerator::calculateLODBoneCount(int originalCount, int lodLevel) const {
    float reductionFactor = 1.0f / (1.0f + lodLevel * 0.5f);
    return std::max(1, static_cast<int>(originalCount * reductionFactor));
}

// =============================================================================
// BONE NAMING HELPERS
// =============================================================================

namespace RigBoneNames {

std::string makeSpineBone(int index) {
    return std::string(SPINE_PREFIX) + std::to_string(index);
}

std::string makeNeckBone(int index) {
    return std::string(NECK_PREFIX) + std::to_string(index);
}

std::string makeTailBone(int index) {
    return std::string(TAIL_PREFIX) + std::to_string(index);
}

std::string makeLegBone(int pair, LimbSide side, const char* segment) {
    std::string sideSuffix = (side == LimbSide::LEFT) ? LEFT : RIGHT;
    return std::string(LEG_PREFIX) + std::to_string(pair) + segment + sideSuffix;
}

std::string makeArmBone(int pair, LimbSide side, const char* segment) {
    std::string sideSuffix = (side == LimbSide::LEFT) ? LEFT : RIGHT;
    return std::string(ARM_PREFIX) + std::to_string(pair) + segment + sideSuffix;
}

std::string makeWingBone(LimbSide side, const char* segment) {
    std::string sideSuffix = (side == LimbSide::LEFT) ? LEFT : RIGHT;
    return std::string(WING_PREFIX) + segment + sideSuffix;
}

std::string makeFinBone(const char* finType, LimbSide side, int segment) {
    std::string result = finType;
    if (side != LimbSide::CENTER) {
        result += (side == LimbSide::LEFT) ? LEFT : RIGHT;
    }
    if (segment >= 0) {
        result += "_" + std::to_string(segment);
    }
    return result;
}

std::string makeTentacleBone(int index, int segment) {
    return std::string(TENTACLE_PREFIX) + std::to_string(index) + "_" + std::to_string(segment);
}

std::string makeHornBone(int index) {
    return std::string(HORN_PREFIX) + std::to_string(index);
}

std::string makeAntennaBone(int index, int segment) {
    return std::string(ANTENNA_PREFIX) + std::to_string(index) + "_" + std::to_string(segment);
}

} // namespace RigBoneNames

// =============================================================================
// RIG PRESETS
// =============================================================================

namespace RigPresets {

RigDefinition createBipedRig(float height, bool hasArms, bool hasTail) {
    MorphologyGenes genes;
    genes.bodyLength = height * 0.4f;
    genes.bodyHeight = height * 0.2f;
    genes.bodyWidth = height * 0.15f;
    genes.legPairs = 1;
    genes.legLength = height * 0.5f;
    genes.legSegments = 3;
    genes.armPairs = hasArms ? 1 : 0;
    genes.armLength = height * 0.35f;
    genes.armSegments = 3;
    genes.hasTail = hasTail;
    genes.tailLength = height * 0.3f;
    genes.tailSegments = 5;
    genes.segmentCount = 3;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createQuadrupedRig(float length, float height, bool hasTail) {
    MorphologyGenes genes;
    genes.bodyLength = length;
    genes.bodyHeight = height * 0.5f;
    genes.bodyWidth = length * 0.3f;
    genes.legPairs = 2;
    genes.legLength = height;
    genes.legSegments = 3;
    genes.hasTail = hasTail;
    genes.tailLength = length * 0.6f;
    genes.tailSegments = 6;
    genes.segmentCount = 4;
    genes.neckLength = length * 0.2f;
    genes.headSize = length * 0.2f;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createHexapodRig(float length, bool hasWings, bool hasAntennae) {
    MorphologyGenes genes;
    genes.bodyLength = length;
    genes.bodyHeight = length * 0.2f;
    genes.bodyWidth = length * 0.25f;
    genes.legPairs = 3;
    genes.legLength = length * 0.5f;
    genes.legSegments = 3;
    genes.wingPairs = hasWings ? 1 : 0;
    genes.wingSpan = length * 2.0f;
    genes.canFly = hasWings;
    genes.antennaeCount = hasAntennae ? 2 : 0;
    genes.segmentCount = 3;
    genes.hasTail = false;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createSerpentineRig(float length, int segments) {
    MorphologyGenes genes;
    genes.bodyLength = length;
    genes.bodyHeight = length * 0.05f;
    genes.bodyWidth = length * 0.05f;
    genes.segmentCount = segments;
    genes.legPairs = 0;
    genes.hasTail = false; // Body IS the tail
    genes.neckLength = 0.0f;
    genes.headSize = length * 0.03f;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createAquaticRig(float length, bool hasLateralFins) {
    MorphologyGenes genes;
    genes.bodyLength = length;
    genes.bodyHeight = length * 0.25f;
    genes.bodyWidth = length * 0.15f;
    genes.segmentCount = 5;
    genes.legPairs = 0;
    genes.hasTail = true;
    genes.tailLength = length * 0.4f;
    genes.tailSegments = 5;
    genes.hasDorsalFin = true;
    genes.dorsalFinCount = 1;
    genes.hasPectoralFins = hasLateralFins;
    genes.pectoralFinPairs = hasLateralFins ? 1 : 0;
    genes.hasCaudalFin = true;
    genes.finSize = length * 0.15f;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createFlyingRig(float wingspan, bool hasTail, bool hasLegs) {
    MorphologyGenes genes;
    float bodyLength = wingspan * 0.3f;
    genes.bodyLength = bodyLength;
    genes.bodyHeight = bodyLength * 0.3f;
    genes.bodyWidth = bodyLength * 0.2f;
    genes.wingPairs = 1;
    genes.wingSpan = wingspan;
    genes.canFly = true;
    genes.legPairs = hasLegs ? 1 : 0;
    genes.legLength = bodyLength * 0.3f;
    genes.legSegments = 3;
    genes.hasTail = hasTail;
    genes.tailLength = bodyLength * 0.5f;
    genes.tailSegments = 4;
    genes.segmentCount = 3;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createRadialRig(float radius, int armCount) {
    MorphologyGenes genes;
    genes.symmetry = SymmetryType::RADIAL;
    genes.bodyLength = radius;
    genes.bodyHeight = radius * 0.3f;
    genes.bodyWidth = radius;
    genes.tentacleCount = armCount;
    genes.tentacleLength = radius * 2.0f;
    genes.segmentCount = 1;
    genes.legPairs = 0;
    genes.hasTail = false;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

RigDefinition createCephalopodRig(float bodySize, int tentacleCount) {
    MorphologyGenes genes;
    genes.bodyLength = bodySize;
    genes.bodyHeight = bodySize * 0.8f;
    genes.bodyWidth = bodySize * 0.6f;
    genes.tentacleCount = tentacleCount;
    genes.tentacleLength = bodySize * 3.0f;
    genes.segmentCount = 2;
    genes.legPairs = 0;
    genes.hasTail = false;
    genes.headSize = bodySize * 0.4f;
    genes.eyeCount = 2;
    genes.eyeSize = bodySize * 0.15f;

    ProceduralRigGenerator generator;
    return generator.generateRigDefinition(genes);
}

} // namespace RigPresets

// =============================================================================
// RIG VALIDATION
// =============================================================================

namespace RigValidation {

bool validateRig(const RigDefinition& rig, std::string& errorMessage) {
    if (rig.totalBones <= 0) {
        errorMessage = "Rig has no bones";
        return false;
    }

    if (rig.totalBones > static_cast<int>(MAX_BONES)) {
        errorMessage = "Rig exceeds maximum bone count (" + std::to_string(MAX_BONES) + ")";
        return false;
    }

    if (rig.spine.boneCount <= 0) {
        errorMessage = "Rig has no spine bones";
        return false;
    }

    errorMessage = "";
    return true;
}

bool validateSkeleton(const Skeleton& skeleton, const RigDefinition& rig, std::string& errorMessage) {
    if (skeleton.getBoneCount() == 0) {
        errorMessage = "Skeleton has no bones";
        return false;
    }

    if (!skeleton.isValid()) {
        errorMessage = "Skeleton is invalid";
        return false;
    }

    errorMessage = "";
    return true;
}

BoneStats calculateBoneStats(const RigDefinition& rig) {
    BoneStats stats;
    stats.totalBones = rig.totalBones;
    stats.spineBones = rig.spine.boneCount;
    stats.limbBones = 0;
    for (const auto& limb : rig.limbs) {
        stats.limbBones += limb.segmentCount;
    }
    stats.featureBones = static_cast<int>(rig.head.hornBones.size() + rig.head.antennaeBones.size());
    if (rig.head.crestBone >= 0) stats.featureBones++;
    if (rig.head.frillBone >= 0) stats.featureBones++;

    stats.maxChainLength = rig.spine.boneCount + rig.tail.boneCount;
    stats.totalBoneLength = rig.spine.totalLength + rig.tail.totalLength;

    return stats;
}

BoneStats calculateBoneStats(const Skeleton& skeleton) {
    BoneStats stats;
    stats.totalBones = skeleton.getBoneCount();
    stats.spineBones = 0;
    stats.limbBones = 0;
    stats.featureBones = 0;
    stats.maxChainLength = 0;
    stats.totalBoneLength = 0.0f;

    // Count bones by name prefix
    for (uint32_t i = 0; i < skeleton.getBoneCount(); ++i) {
        const auto& bone = skeleton.getBone(i);
        stats.totalBoneLength += bone.length;

        if (bone.name.find("spine") != std::string::npos ||
            bone.name.find("pelvis") != std::string::npos) {
            stats.spineBones++;
        } else if (bone.name.find("leg") != std::string::npos ||
                   bone.name.find("arm") != std::string::npos ||
                   bone.name.find("wing") != std::string::npos ||
                   bone.name.find("fin") != std::string::npos) {
            stats.limbBones++;
        } else if (bone.name.find("horn") != std::string::npos ||
                   bone.name.find("crest") != std::string::npos ||
                   bone.name.find("antenna") != std::string::npos) {
            stats.featureBones++;
        }
    }

    return stats;
}

std::string rigToString(const RigDefinition& rig) {
    return rig.getDebugInfo();
}

std::string skeletonToString(const Skeleton& skeleton) {
    std::stringstream ss;
    ss << "Skeleton (" << skeleton.getBoneCount() << " bones):\n";

    for (uint32_t i = 0; i < skeleton.getBoneCount(); ++i) {
        const auto& bone = skeleton.getBone(i);
        ss << "  [" << i << "] " << bone.name;
        if (bone.parentIndex >= 0) {
            ss << " (parent: " << bone.parentIndex << ")";
        }
        ss << " len=" << bone.length << "\n";
    }

    return ss.str();
}

} // namespace RigValidation

} // namespace animation
