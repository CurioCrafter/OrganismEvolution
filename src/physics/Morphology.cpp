#include "Morphology.h"
#include <random>
#include <algorithm>

// =============================================================================
// MORPHOLOGY GENES IMPLEMENTATION
// =============================================================================

static std::mt19937& getGenerator() {
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    return gen;
}

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(getGenerator());
}

static int randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(getGenerator());
}

static float clamp(float value, float min, float max) {
    return std::max(min, std::min(max, value));
}

void MorphologyGenes::randomize() {
    // Body organization
    symmetry = static_cast<SymmetryType>(randomInt(0, 1)); // Mostly bilateral or radial
    segmentCount = randomInt(2, 6);
    segmentTaper = randomFloat(0.7f, 1.2f);
    bodyLength = randomFloat(0.5f, 2.0f);
    bodyWidth = randomFloat(0.3f, 0.8f);
    bodyHeight = randomFloat(0.3f, 0.8f);

    // Limbs
    legPairs = randomInt(0, 4);
    legSegments = randomInt(2, 4);
    legLength = randomFloat(0.5f, 1.2f);
    legThickness = randomFloat(0.08f, 0.25f);
    legSpread = randomFloat(0.4f, 1.0f);
    legAttachPoint = randomFloat(0.2f, 0.8f);

    // Arms
    armPairs = randomInt(0, 2);
    armSegments = randomInt(2, 4);
    armLength = randomFloat(0.4f, 0.9f);
    armThickness = randomFloat(0.05f, 0.15f);
    hasHands = randomFloat(0, 1) > 0.5f;

    // Wings (rare)
    wingPairs = randomFloat(0, 1) > 0.8f ? 1 : 0;
    wingSpan = randomFloat(1.5f, 3.0f);
    wingChord = randomFloat(0.3f, 0.6f);
    wingMembraneThickness = randomFloat(0.01f, 0.05f);
    canFly = wingPairs > 0 && randomFloat(0, 1) > 0.5f;

    // Tail
    hasTail = randomFloat(0, 1) > 0.3f;
    tailSegments = randomInt(3, 8);
    tailLength = randomFloat(0.3f, 1.5f);
    tailThickness = randomFloat(0.1f, 0.3f);
    tailTaper = randomFloat(0.3f, 0.8f);
    tailPrehensile = randomFloat(0, 1) > 0.8f;

    // Fins
    finCount = randomInt(0, 4);
    hasDorsalFin = randomFloat(0, 1) > 0.7f;
    hasPectoralFins = randomFloat(0, 1) > 0.6f;
    hasCaudalFin = randomFloat(0, 1) > 0.5f;
    finSize = randomFloat(0.2f, 0.5f);

    // Head
    headSize = randomFloat(0.2f, 0.5f);
    neckLength = randomFloat(0.1f, 0.4f);
    neckFlexibility = randomFloat(0.5f, 1.0f);
    eyeCount = randomInt(1, 4) * 2; // Even numbers
    eyeSize = randomFloat(0.05f, 0.2f);
    eyesSideFacing = randomFloat(0, 1) > 0.5f;

    // Joints
    primaryJointType = static_cast<JointType>(randomInt(1, 3));
    jointFlexibility = randomFloat(0.4f, 1.0f);
    jointStrength = randomFloat(0.3f, 0.8f);
    jointDamping = randomFloat(0.1f, 0.5f);

    // Special features
    primaryFeature = static_cast<FeatureType>(randomInt(0, 9));
    secondaryFeature = randomFloat(0, 1) > 0.7f ?
        static_cast<FeatureType>(randomInt(0, 9)) : FeatureType::NONE;
    featureSize = randomFloat(0.1f, 0.5f);
    armorCoverage = randomFloat(0, 1) > 0.8f ? randomFloat(0.3f, 0.8f) : 0.0f;

    // Allometry
    baseMass = randomFloat(0.5f, 3.0f);
    densityMultiplier = randomFloat(0.8f, 1.2f);
    metabolicMultiplier = randomFloat(0.7f, 1.3f);

    // Metamorphosis (rare)
    hasMetamorphosis = randomFloat(0, 1) > 0.85f;
    metamorphosisAge = randomFloat(10.0f, 30.0f);
    larvalSpeedBonus = randomFloat(1.1f, 1.5f);
    adultSizeMultiplier = randomFloat(1.2f, 2.0f);
}

void MorphologyGenes::mutate(float rate, float strength) {
    auto mutateFloat = [&](float& value, float min, float max) {
        if (randomFloat(0, 1) < rate) {
            value += randomFloat(-strength, strength) * (max - min);
            value = clamp(value, min, max);
        }
    };

    auto mutateInt = [&](int& value, int min, int max) {
        if (randomFloat(0, 1) < rate) {
            value += randomInt(-1, 1);
            value = std::max(min, std::min(max, value));
        }
    };

    // Body
    if (randomFloat(0, 1) < rate * 0.1f) { // Rare symmetry change
        symmetry = static_cast<SymmetryType>(randomInt(0, 1));
    }
    mutateInt(segmentCount, 2, 8);
    mutateFloat(segmentTaper, 0.5f, 1.5f);
    mutateFloat(bodyLength, 0.3f, 3.0f);
    mutateFloat(bodyWidth, 0.2f, 1.0f);
    mutateFloat(bodyHeight, 0.2f, 1.0f);

    // Legs
    mutateInt(legPairs, 0, 4);
    mutateInt(legSegments, 1, 4);
    mutateFloat(legLength, 0.3f, 1.5f);
    mutateFloat(legThickness, 0.05f, 0.3f);
    mutateFloat(legSpread, 0.3f, 1.0f);
    mutateFloat(legAttachPoint, 0.1f, 0.9f);

    // Arms
    mutateInt(armPairs, 0, 2);
    mutateInt(armSegments, 1, 4);
    mutateFloat(armLength, 0.2f, 1.2f);
    mutateFloat(armThickness, 0.03f, 0.2f);
    if (randomFloat(0, 1) < rate) hasHands = !hasHands;

    // Wings
    if (randomFloat(0, 1) < rate * 0.2f) { // Rare wing gain/loss
        wingPairs = wingPairs > 0 ? 0 : 1;
    }
    mutateFloat(wingSpan, 1.0f, 4.0f);
    mutateFloat(wingChord, 0.2f, 0.8f);
    if (randomFloat(0, 1) < rate) canFly = !canFly;

    // Tail
    if (randomFloat(0, 1) < rate * 0.3f) hasTail = !hasTail;
    mutateInt(tailSegments, 2, 10);
    mutateFloat(tailLength, 0.2f, 2.0f);
    mutateFloat(tailThickness, 0.05f, 0.4f);
    mutateFloat(tailTaper, 0.2f, 0.9f);

    // Fins
    mutateInt(finCount, 0, 6);
    if (randomFloat(0, 1) < rate) hasDorsalFin = !hasDorsalFin;
    if (randomFloat(0, 1) < rate) hasPectoralFins = !hasPectoralFins;
    if (randomFloat(0, 1) < rate) hasCaudalFin = !hasCaudalFin;
    mutateFloat(finSize, 0.1f, 0.6f);

    // Head
    mutateFloat(headSize, 0.15f, 0.6f);
    mutateFloat(neckLength, 0.05f, 0.6f);
    mutateFloat(neckFlexibility, 0.3f, 1.0f);
    mutateInt(eyeCount, 0, 8);
    eyeCount = (eyeCount / 2) * 2; // Keep even
    mutateFloat(eyeSize, 0.03f, 0.25f);
    if (randomFloat(0, 1) < rate) eyesSideFacing = !eyesSideFacing;

    // Joints
    if (randomFloat(0, 1) < rate * 0.3f) {
        primaryJointType = static_cast<JointType>(randomInt(1, 3));
    }
    mutateFloat(jointFlexibility, 0.2f, 1.0f);
    mutateFloat(jointStrength, 0.2f, 1.0f);
    mutateFloat(jointDamping, 0.05f, 0.6f);

    // Features
    if (randomFloat(0, 1) < rate * 0.4f) {
        primaryFeature = static_cast<FeatureType>(randomInt(0, 9));
    }
    if (randomFloat(0, 1) < rate * 0.4f) {
        secondaryFeature = static_cast<FeatureType>(randomInt(0, 9));
    }
    mutateFloat(featureSize, 0.05f, 0.6f);
    mutateFloat(armorCoverage, 0.0f, 1.0f);

    // Allometry
    mutateFloat(baseMass, 0.3f, 5.0f);
    mutateFloat(densityMultiplier, 0.6f, 1.4f);
    mutateFloat(metabolicMultiplier, 0.5f, 1.5f);

    // Metamorphosis
    if (randomFloat(0, 1) < rate * 0.1f) hasMetamorphosis = !hasMetamorphosis;
    mutateFloat(metamorphosisAge, 5.0f, 50.0f);
    mutateFloat(larvalSpeedBonus, 1.0f, 2.0f);
    mutateFloat(adultSizeMultiplier, 1.0f, 3.0f);
}

MorphologyGenes MorphologyGenes::crossover(const MorphologyGenes& p1, const MorphologyGenes& p2) {
    MorphologyGenes child;

    auto pick = [&]() { return randomFloat(0, 1) > 0.5f; };
    auto blend = [&](float a, float b) { return pick() ? a : b; };
    auto blendInt = [&](int a, int b) { return pick() ? a : b; };

    // Body
    child.symmetry = pick() ? p1.symmetry : p2.symmetry;
    child.segmentCount = blendInt(p1.segmentCount, p2.segmentCount);
    child.segmentTaper = blend(p1.segmentTaper, p2.segmentTaper);
    child.bodyLength = blend(p1.bodyLength, p2.bodyLength);
    child.bodyWidth = blend(p1.bodyWidth, p2.bodyWidth);
    child.bodyHeight = blend(p1.bodyHeight, p2.bodyHeight);

    // Legs
    child.legPairs = blendInt(p1.legPairs, p2.legPairs);
    child.legSegments = blendInt(p1.legSegments, p2.legSegments);
    child.legLength = blend(p1.legLength, p2.legLength);
    child.legThickness = blend(p1.legThickness, p2.legThickness);
    child.legSpread = blend(p1.legSpread, p2.legSpread);
    child.legAttachPoint = blend(p1.legAttachPoint, p2.legAttachPoint);

    // Arms
    child.armPairs = blendInt(p1.armPairs, p2.armPairs);
    child.armSegments = blendInt(p1.armSegments, p2.armSegments);
    child.armLength = blend(p1.armLength, p2.armLength);
    child.armThickness = blend(p1.armThickness, p2.armThickness);
    child.hasHands = pick() ? p1.hasHands : p2.hasHands;

    // Wings
    child.wingPairs = blendInt(p1.wingPairs, p2.wingPairs);
    child.wingSpan = blend(p1.wingSpan, p2.wingSpan);
    child.wingChord = blend(p1.wingChord, p2.wingChord);
    child.wingMembraneThickness = blend(p1.wingMembraneThickness, p2.wingMembraneThickness);
    child.canFly = pick() ? p1.canFly : p2.canFly;

    // Tail
    child.hasTail = pick() ? p1.hasTail : p2.hasTail;
    child.tailSegments = blendInt(p1.tailSegments, p2.tailSegments);
    child.tailLength = blend(p1.tailLength, p2.tailLength);
    child.tailThickness = blend(p1.tailThickness, p2.tailThickness);
    child.tailTaper = blend(p1.tailTaper, p2.tailTaper);
    child.tailPrehensile = pick() ? p1.tailPrehensile : p2.tailPrehensile;

    // Fins
    child.finCount = blendInt(p1.finCount, p2.finCount);
    child.hasDorsalFin = pick() ? p1.hasDorsalFin : p2.hasDorsalFin;
    child.hasPectoralFins = pick() ? p1.hasPectoralFins : p2.hasPectoralFins;
    child.hasCaudalFin = pick() ? p1.hasCaudalFin : p2.hasCaudalFin;
    child.finSize = blend(p1.finSize, p2.finSize);

    // Head
    child.headSize = blend(p1.headSize, p2.headSize);
    child.neckLength = blend(p1.neckLength, p2.neckLength);
    child.neckFlexibility = blend(p1.neckFlexibility, p2.neckFlexibility);
    child.eyeCount = blendInt(p1.eyeCount, p2.eyeCount);
    child.eyeSize = blend(p1.eyeSize, p2.eyeSize);
    child.eyesSideFacing = pick() ? p1.eyesSideFacing : p2.eyesSideFacing;

    // Joints
    child.primaryJointType = pick() ? p1.primaryJointType : p2.primaryJointType;
    child.jointFlexibility = blend(p1.jointFlexibility, p2.jointFlexibility);
    child.jointStrength = blend(p1.jointStrength, p2.jointStrength);
    child.jointDamping = blend(p1.jointDamping, p2.jointDamping);

    // Features
    child.primaryFeature = pick() ? p1.primaryFeature : p2.primaryFeature;
    child.secondaryFeature = pick() ? p1.secondaryFeature : p2.secondaryFeature;
    child.featureSize = blend(p1.featureSize, p2.featureSize);
    child.armorCoverage = blend(p1.armorCoverage, p2.armorCoverage);

    // Allometry
    child.baseMass = blend(p1.baseMass, p2.baseMass);
    child.densityMultiplier = blend(p1.densityMultiplier, p2.densityMultiplier);
    child.metabolicMultiplier = blend(p1.metabolicMultiplier, p2.metabolicMultiplier);

    // Metamorphosis
    child.hasMetamorphosis = pick() ? p1.hasMetamorphosis : p2.hasMetamorphosis;
    child.metamorphosisAge = blend(p1.metamorphosisAge, p2.metamorphosisAge);
    child.larvalSpeedBonus = blend(p1.larvalSpeedBonus, p2.larvalSpeedBonus);
    child.adultSizeMultiplier = blend(p1.adultSizeMultiplier, p2.adultSizeMultiplier);

    return child;
}

float MorphologyGenes::getExpectedMass() const {
    float volume = bodyLength * bodyWidth * bodyHeight;
    float limbVolume = legPairs * 2 * legLength * legThickness * legThickness * 0.5f;
    limbVolume += armPairs * 2 * armLength * armThickness * armThickness * 0.3f;
    limbVolume += wingPairs * 2 * wingSpan * wingChord * wingMembraneThickness;

    float tailVolume = hasTail ? tailLength * tailThickness * tailThickness * 0.3f : 0.0f;
    float headVolume = headSize * headSize * headSize * 0.5f;

    float totalVolume = volume + limbVolume + tailVolume + headVolume;
    return baseMass * totalVolume * densityMultiplier;
}

float MorphologyGenes::getMetabolicRate() const {
    float mass = getExpectedMass();
    return Allometry::metabolicRate(mass) * metabolicMultiplier;
}

float MorphologyGenes::getMaxSpeed() const {
    float mass = getExpectedMass();
    float baseSpeed = Allometry::maxSpeed(mass);

    // Modify by limb count - quadrupeds are generally faster
    float limbBonus = 1.0f;
    if (legPairs == 2) limbBonus = 1.1f;      // Quadrupeds
    else if (legPairs == 1) limbBonus = 0.9f;  // Bipeds
    else if (legPairs == 0) limbBonus = 0.5f;  // Serpentine
    else if (legPairs >= 3) limbBonus = 0.85f; // Many legs = stable but slower

    // Wings can increase speed
    if (canFly && wingPairs > 0) {
        baseSpeed *= 1.5f;
    }

    return baseSpeed * limbBonus;
}

float MorphologyGenes::getLimbFrequency() const {
    float mass = getExpectedMass();
    return Allometry::limbFrequency(mass);
}

// =============================================================================
// BODY PLAN IMPLEMENTATION
// =============================================================================

void BodyPlan::buildFromGenes(const MorphologyGenes& genes) {
    segments.clear();
    sourceGenes = genes;

    // Build in order: torso first, then head, tail, limbs
    addTorsoSegments(genes);
    addHead(genes);
    if (genes.hasTail) addTail(genes);
    if (genes.legPairs > 0) addLegs(genes);
    if (genes.armPairs > 0) addArms(genes);
    if (genes.wingPairs > 0) addWings(genes);
    if (genes.finCount > 0 || genes.hasDorsalFin || genes.hasPectoralFins || genes.hasCaudalFin) {
        addFins(genes);
    }
    addSpecialFeatures(genes);
}

void BodyPlan::addTorsoSegments(const MorphologyGenes& genes) {
    float segmentLength = genes.bodyLength / genes.segmentCount;
    float currentZ = -genes.bodyLength / 2.0f;
    float currentScale = 1.0f;

    for (int i = 0; i < genes.segmentCount; i++) {
        BodySegment seg;
        seg.name = "torso_" + std::to_string(i);

        float width = genes.bodyWidth * currentScale;
        float height = genes.bodyHeight * currentScale;

        seg.localPosition = glm::vec3(0, height / 2, currentZ + segmentLength / 2);
        seg.size = glm::vec3(width / 2, height / 2, segmentLength / 2);

        // First segment is root
        if (i == 0) {
            seg.parentIndex = -1;
        } else {
            seg.parentIndex = i - 1;
            seg.jointToParent.type = JointType::HINGE;
            seg.jointToParent.axis = glm::vec3(1, 0, 0);
            seg.jointToParent.minAngle = -0.3f * genes.jointFlexibility;
            seg.jointToParent.maxAngle = 0.3f * genes.jointFlexibility;
            seg.jointToParent.maxTorque = 50.0f * genes.jointStrength;
        }

        seg.mass = seg.size.x * seg.size.y * seg.size.z * 8.0f * genes.densityMultiplier;
        calculateInertia(seg);

        int idx = addSegment(seg);

        // Link to parent
        if (i > 0) {
            segments[i - 1].childIndices.push_back(idx);
        }

        currentZ += segmentLength;
        currentScale *= genes.segmentTaper;
    }
}

void BodyPlan::addHead(const MorphologyGenes& genes) {
    BodySegment head;
    head.name = "head";

    float headRadius = genes.headSize * genes.bodyWidth;
    head.size = glm::vec3(headRadius, headRadius, headRadius * 1.2f);

    // Position at front of body
    float frontZ = genes.bodyLength / 2.0f;
    head.localPosition = glm::vec3(0, genes.bodyHeight / 2, frontZ + genes.neckLength + head.size.z);

    // Attach to first (front) torso segment
    head.parentIndex = genes.segmentCount - 1; // Last torso segment is at front
    head.jointToParent.type = JointType::BALL_SOCKET;
    head.jointToParent.axis = glm::vec3(1, 0, 0);
    head.jointToParent.minAngle = -0.5f * genes.neckFlexibility;
    head.jointToParent.maxAngle = 0.5f * genes.neckFlexibility;
    head.jointToParent.secondaryAxis = glm::vec3(0, 1, 0);
    head.jointToParent.minAngle2 = -0.8f * genes.neckFlexibility;
    head.jointToParent.maxAngle2 = 0.8f * genes.neckFlexibility;

    head.mass = head.size.x * head.size.y * head.size.z * 8.0f * genes.densityMultiplier * 1.2f; // Heads are denser
    calculateInertia(head);

    int idx = addSegment(head);
    segments[genes.segmentCount - 1].childIndices.push_back(idx);
}

void BodyPlan::addTail(const MorphologyGenes& genes) {
    float segmentLength = genes.tailLength / genes.tailSegments;
    float currentZ = -genes.bodyLength / 2.0f - segmentLength;
    float currentThickness = genes.tailThickness * genes.bodyWidth;

    int parentIdx = 0; // First torso segment (rear)

    for (int i = 0; i < genes.tailSegments; i++) {
        BodySegment seg;
        seg.name = "tail_" + std::to_string(i);
        seg.appendageType = AppendageType::TAIL;
        seg.segmentIndexInLimb = i;
        seg.isTerminal = (i == genes.tailSegments - 1);

        seg.size = glm::vec3(currentThickness / 2, currentThickness / 2, segmentLength / 2);
        seg.localPosition = glm::vec3(0, genes.bodyHeight / 4, currentZ);

        seg.parentIndex = parentIdx;
        seg.jointToParent.type = genes.tailPrehensile ? JointType::BALL_SOCKET : JointType::HINGE;
        seg.jointToParent.axis = glm::vec3(1, 0, 0);
        seg.jointToParent.minAngle = -0.4f * genes.jointFlexibility;
        seg.jointToParent.maxAngle = 0.4f * genes.jointFlexibility;
        seg.jointToParent.maxTorque = 20.0f * genes.jointStrength;

        seg.mass = seg.size.x * seg.size.y * seg.size.z * 8.0f * genes.densityMultiplier;
        calculateInertia(seg);

        int idx = addSegment(seg);
        segments[parentIdx].childIndices.push_back(idx);

        parentIdx = idx;
        currentZ -= segmentLength;
        currentThickness *= genes.tailTaper;
    }
}

void BodyPlan::addLegs(const MorphologyGenes& genes) {
    // Determine which torso segments legs attach to
    int attachSegment = static_cast<int>(genes.legAttachPoint * (genes.segmentCount - 1));

    float segmentLength = genes.legLength * genes.bodyLength / genes.legSegments;
    float legSpacing = genes.bodyLength / (genes.legPairs + 1);

    for (int pair = 0; pair < genes.legPairs; pair++) {
        float zOffset = -genes.bodyLength / 2.0f + legSpacing * (pair + 1);

        // Find closest torso segment
        int torsoIdx = std::min(pair, genes.segmentCount - 1);

        for (int side = 0; side < 2; side++) { // Left and right
            float xDir = (side == 0) ? -1.0f : 1.0f;
            float xOffset = genes.bodyWidth / 2.0f * genes.legSpread * xDir;

            int parentIdx = torsoIdx;
            float currentThickness = genes.legThickness * genes.bodyWidth;

            for (int seg = 0; seg < genes.legSegments; seg++) {
                BodySegment legSeg;
                legSeg.name = "leg_" + std::to_string(pair) + "_" +
                              (side == 0 ? "L" : "R") + "_" + std::to_string(seg);
                legSeg.appendageType = AppendageType::LEG;
                legSeg.segmentIndexInLimb = seg;
                legSeg.isTerminal = (seg == genes.legSegments - 1);

                // First segment goes down and out, others go down
                glm::vec3 direction;
                if (seg == 0) {
                    direction = glm::normalize(glm::vec3(xDir * 0.5f, -1.0f, 0.0f));
                } else {
                    direction = glm::vec3(0, -1, 0);
                }

                legSeg.size = glm::vec3(currentThickness / 2, segmentLength / 2, currentThickness / 2);

                if (seg == 0) {
                    legSeg.localPosition = glm::vec3(xOffset, 0, zOffset) +
                                           direction * segmentLength / 2.0f;
                } else {
                    // Position relative to parent
                    legSeg.localPosition = direction * segmentLength;
                }

                legSeg.parentIndex = parentIdx;
                legSeg.jointToParent.type = (seg == 0) ? JointType::BALL_SOCKET : genes.primaryJointType;
                legSeg.jointToParent.axis = glm::vec3(0, 0, 1);
                legSeg.jointToParent.minAngle = -1.2f * genes.jointFlexibility;
                legSeg.jointToParent.maxAngle = 0.2f * genes.jointFlexibility;
                legSeg.jointToParent.maxTorque = 100.0f * genes.jointStrength;

                legSeg.mass = legSeg.size.x * legSeg.size.y * legSeg.size.z * 8.0f * genes.densityMultiplier;
                calculateInertia(legSeg);

                int idx = addSegment(legSeg);
                segments[parentIdx].childIndices.push_back(idx);

                parentIdx = idx;
                currentThickness *= 0.8f; // Taper
            }
        }
    }
}

void BodyPlan::addArms(const MorphologyGenes& genes) {
    if (genes.armPairs == 0) return;

    float segmentLength = genes.armLength * genes.bodyLength / genes.armSegments;

    // Arms attach near front of body
    int attachSegment = genes.segmentCount - 1;

    for (int pair = 0; pair < genes.armPairs; pair++) {
        for (int side = 0; side < 2; side++) {
            float xDir = (side == 0) ? -1.0f : 1.0f;
            float xOffset = genes.bodyWidth / 2.0f * xDir;

            int parentIdx = attachSegment;
            float currentThickness = genes.armThickness * genes.bodyWidth;

            for (int seg = 0; seg < genes.armSegments; seg++) {
                BodySegment armSeg;
                armSeg.name = "arm_" + std::to_string(pair) + "_" +
                              (side == 0 ? "L" : "R") + "_" + std::to_string(seg);
                armSeg.appendageType = AppendageType::ARM;
                armSeg.segmentIndexInLimb = seg;
                armSeg.isTerminal = (seg == genes.armSegments - 1);

                glm::vec3 direction;
                if (seg == 0) {
                    direction = glm::normalize(glm::vec3(xDir, -0.3f, 0.5f));
                } else {
                    direction = glm::normalize(glm::vec3(xDir * 0.3f, -0.5f, 0.5f));
                }

                armSeg.size = glm::vec3(currentThickness / 2, segmentLength / 2, currentThickness / 2);

                if (seg == 0) {
                    armSeg.localPosition = glm::vec3(xOffset, genes.bodyHeight / 3, genes.bodyLength / 3) +
                                           direction * segmentLength / 2.0f;
                } else {
                    armSeg.localPosition = direction * segmentLength;
                }

                armSeg.parentIndex = parentIdx;
                armSeg.jointToParent.type = (seg == 0) ? JointType::BALL_SOCKET : genes.primaryJointType;
                armSeg.jointToParent.axis = glm::vec3(0, 0, 1);
                armSeg.jointToParent.minAngle = -1.5f * genes.jointFlexibility;
                armSeg.jointToParent.maxAngle = 1.5f * genes.jointFlexibility;
                armSeg.jointToParent.maxTorque = 50.0f * genes.jointStrength;

                // Add hands to terminal segments
                if (armSeg.isTerminal && genes.hasHands) {
                    armSeg.feature = FeatureType::CLAWS; // Hands represented as claws for now
                }

                armSeg.mass = armSeg.size.x * armSeg.size.y * armSeg.size.z * 8.0f * genes.densityMultiplier;
                calculateInertia(armSeg);

                int idx = addSegment(armSeg);
                segments[parentIdx].childIndices.push_back(idx);

                parentIdx = idx;
                currentThickness *= 0.8f;
            }
        }
    }
}

void BodyPlan::addWings(const MorphologyGenes& genes) {
    if (genes.wingPairs == 0) return;

    // Wings attach to upper back
    int attachSegment = genes.segmentCount / 2;

    for (int side = 0; side < 2; side++) {
        float xDir = (side == 0) ? -1.0f : 1.0f;

        BodySegment wing;
        wing.name = std::string("wing_") + (side == 0 ? "L" : "R");
        wing.appendageType = AppendageType::WING;
        wing.isTerminal = true;

        float wingLength = genes.wingSpan * genes.bodyLength / 2.0f;
        wing.size = glm::vec3(wingLength / 2, genes.wingMembraneThickness, genes.wingChord * genes.bodyLength / 2);

        wing.localPosition = glm::vec3(
            xDir * (genes.bodyWidth / 2 + wingLength / 2),
            genes.bodyHeight / 2 + 0.1f,
            0
        );

        wing.parentIndex = attachSegment;
        wing.jointToParent.type = JointType::HINGE;
        wing.jointToParent.axis = glm::vec3(0, 0, 1);
        wing.jointToParent.minAngle = -0.3f;
        wing.jointToParent.maxAngle = 1.5f; // Wings can fold down but extend up
        wing.jointToParent.maxTorque = 200.0f * genes.jointStrength;

        wing.mass = wing.size.x * wing.size.y * wing.size.z * 2.0f * genes.densityMultiplier; // Wings are light
        calculateInertia(wing);

        int idx = addSegment(wing);
        segments[attachSegment].childIndices.push_back(idx);
    }
}

void BodyPlan::addFins(const MorphologyGenes& genes) {
    // Dorsal fin
    if (genes.hasDorsalFin) {
        int attachSegment = genes.segmentCount / 2;

        BodySegment fin;
        fin.name = "fin_dorsal";
        fin.appendageType = AppendageType::FIN;
        fin.isTerminal = true;

        float finHeight = genes.finSize * genes.bodyHeight;
        fin.size = glm::vec3(0.02f, finHeight / 2, genes.bodyLength / 4);
        fin.localPosition = glm::vec3(0, genes.bodyHeight / 2 + finHeight / 2, 0);

        fin.parentIndex = attachSegment;
        fin.jointToParent.type = JointType::FIXED;

        fin.mass = 0.1f * genes.densityMultiplier;
        calculateInertia(fin);

        int idx = addSegment(fin);
        segments[attachSegment].childIndices.push_back(idx);
    }

    // Pectoral fins
    if (genes.hasPectoralFins) {
        int attachSegment = genes.segmentCount - 1;

        for (int side = 0; side < 2; side++) {
            float xDir = (side == 0) ? -1.0f : 1.0f;

            BodySegment fin;
            fin.name = std::string("fin_pectoral_") + (side == 0 ? "L" : "R");
            fin.appendageType = AppendageType::FIN;
            fin.isTerminal = true;

            float finLength = genes.finSize * genes.bodyWidth;
            fin.size = glm::vec3(finLength / 2, 0.02f, genes.finSize * genes.bodyLength / 3);
            fin.localPosition = glm::vec3(
                xDir * (genes.bodyWidth / 2 + finLength / 2),
                0,
                genes.bodyLength / 4
            );

            fin.parentIndex = attachSegment;
            fin.jointToParent.type = JointType::HINGE;
            fin.jointToParent.axis = glm::vec3(0, 0, 1);
            fin.jointToParent.minAngle = -0.5f;
            fin.jointToParent.maxAngle = 0.5f;

            fin.mass = 0.1f * genes.densityMultiplier;
            calculateInertia(fin);

            int idx = addSegment(fin);
            segments[attachSegment].childIndices.push_back(idx);
        }
    }

    // Caudal (tail) fin
    if (genes.hasCaudalFin && genes.hasTail) {
        // Find last tail segment
        int lastTailIdx = -1;
        for (size_t i = 0; i < segments.size(); i++) {
            if (segments[i].appendageType == AppendageType::TAIL && segments[i].isTerminal) {
                lastTailIdx = static_cast<int>(i);
                break;
            }
        }

        if (lastTailIdx >= 0) {
            BodySegment fin;
            fin.name = "fin_caudal";
            fin.appendageType = AppendageType::FIN;
            fin.isTerminal = true;

            float finHeight = genes.finSize * genes.bodyHeight;
            fin.size = glm::vec3(0.02f, finHeight / 2, genes.finSize * genes.bodyLength / 4);
            fin.localPosition = glm::vec3(0, 0, -genes.finSize * genes.bodyLength / 4);

            fin.parentIndex = lastTailIdx;
            fin.jointToParent.type = JointType::FIXED;

            fin.mass = 0.1f * genes.densityMultiplier;
            calculateInertia(fin);

            int idx = addSegment(fin);
            segments[lastTailIdx].childIndices.push_back(idx);
        }
    }
}

void BodyPlan::addSpecialFeatures(const MorphologyGenes& genes) {
    // Add features to head
    int headIdx = -1;
    for (size_t i = 0; i < segments.size(); i++) {
        if (segments[i].name == "head") {
            headIdx = static_cast<int>(i);
            break;
        }
    }

    if (headIdx >= 0) {
        segments[headIdx].feature = genes.primaryFeature;
    }

    // Add armor to torso segments
    if (genes.armorCoverage > 0.0f) {
        for (auto& seg : segments) {
            if (seg.name.find("torso") != std::string::npos) {
                if (genes.armorCoverage > 0.5f) {
                    seg.feature = FeatureType::SHELL;
                }
                // Armor increases mass
                seg.mass *= (1.0f + genes.armorCoverage * 0.5f);
            }
        }
    }
}

int BodyPlan::addSegment(const BodySegment& segment) {
    segments.push_back(segment);
    return static_cast<int>(segments.size() - 1);
}

void BodyPlan::calculateInertia(BodySegment& segment) {
    // Approximate as box
    float w = segment.size.x * 2;
    float h = segment.size.y * 2;
    float d = segment.size.z * 2;
    float m = segment.mass;

    segment.inertia = glm::mat3(0.0f);
    segment.inertia[0][0] = m * (h*h + d*d) / 12.0f;
    segment.inertia[1][1] = m * (w*w + d*d) / 12.0f;
    segment.inertia[2][2] = m * (w*w + h*h) / 12.0f;
}

int BodyPlan::findRootSegment() const {
    for (size_t i = 0; i < segments.size(); i++) {
        if (segments[i].parentIndex < 0) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

std::vector<int> BodyPlan::findLimbRoots() const {
    std::vector<int> roots;
    for (size_t i = 0; i < segments.size(); i++) {
        const auto& seg = segments[i];
        if ((seg.appendageType == AppendageType::LEG ||
             seg.appendageType == AppendageType::ARM ||
             seg.appendageType == AppendageType::WING) &&
            seg.segmentIndexInLimb == 0) {
            roots.push_back(static_cast<int>(i));
        }
    }
    return roots;
}

std::vector<int> BodyPlan::findTerminalSegments() const {
    std::vector<int> terminals;
    for (size_t i = 0; i < segments.size(); i++) {
        if (segments[i].isTerminal) {
            terminals.push_back(static_cast<int>(i));
        }
    }
    return terminals;
}

float BodyPlan::getTotalMass() const {
    float total = 0.0f;
    for (const auto& seg : segments) {
        total += seg.mass;
    }
    return total;
}

glm::vec3 BodyPlan::getCenterOfMass() const {
    glm::vec3 com(0.0f);
    float totalMass = 0.0f;

    for (const auto& seg : segments) {
        com += seg.localPosition * seg.mass;
        totalMass += seg.mass;
    }

    if (totalMass > 0.0f) {
        com /= totalMass;
    }
    return com;
}

glm::mat3 BodyPlan::getTotalInertia() const {
    glm::mat3 total(0.0f);
    glm::vec3 com = getCenterOfMass();

    for (const auto& seg : segments) {
        // Add segment's own inertia
        total += seg.inertia;

        // Add parallel axis theorem contribution
        glm::vec3 r = seg.localPosition - com;
        float r2 = glm::dot(r, r);
        total[0][0] += seg.mass * (r2 - r.x * r.x);
        total[1][1] += seg.mass * (r2 - r.y * r.y);
        total[2][2] += seg.mass * (r2 - r.z * r.z);
    }

    return total;
}

void BodyPlan::getBounds(glm::vec3& minBound, glm::vec3& maxBound) const {
    if (segments.empty()) {
        minBound = maxBound = glm::vec3(0.0f);
        return;
    }

    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& seg : segments) {
        glm::vec3 segMin = seg.localPosition - seg.size;
        glm::vec3 segMax = seg.localPosition + seg.size;

        minBound = glm::min(minBound, segMin);
        maxBound = glm::max(maxBound, segMax);
    }
}
