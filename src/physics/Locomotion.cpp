#include "Locomotion.h"
#include <algorithm>
#include <cmath>

// =============================================================================
// CENTRAL PATTERN GENERATOR IMPLEMENTATION
// =============================================================================

void CentralPatternGenerator::initialize(const BodyPlan& bodyPlan, GaitType gait) {
    currentGait = gait;
    globalPhase = 0.0f;

    // Count limbs (legs, arms used for locomotion)
    limbCount = 0;
    for (const auto& seg : bodyPlan.getSegments()) {
        if ((seg.appendageType == AppendageType::LEG ||
             seg.appendageType == AppendageType::ARM) &&
            seg.segmentIndexInLimb == 0) {
            limbCount++;
        }
    }

    limbPhaseOffsets.resize(limbCount);
    dutyFactors.resize(limbCount);

    calculatePhaseOffsets();
}

void CentralPatternGenerator::update(float deltaTime, float speedMultiplier) {
    // Advance global phase
    globalPhase += baseFrequency * speedMultiplier * deltaTime;
    if (globalPhase >= 1.0f) {
        globalPhase -= 1.0f;
    }
}

float CentralPatternGenerator::getLimbPhase(int limbIndex) const {
    if (limbIndex < 0 || limbIndex >= limbCount) return 0.0f;

    float phase = globalPhase + limbPhaseOffsets[limbIndex];
    if (phase >= 1.0f) phase -= 1.0f;
    return phase;
}

bool CentralPatternGenerator::isLimbInStance(int limbIndex) const {
    if (limbIndex < 0 || limbIndex >= limbCount) return true;

    float phase = getLimbPhase(limbIndex);
    float duty = dutyFactors[limbIndex];

    // Stance phase is from 0 to duty factor
    return phase < duty;
}

void CentralPatternGenerator::setGaitType(GaitType gait) {
    currentGait = gait;
    calculatePhaseOffsets();
}

void CentralPatternGenerator::calculatePhaseOffsets() {
    // Set up phase offsets based on gait type and limb count
    std::fill(limbPhaseOffsets.begin(), limbPhaseOffsets.end(), 0.0f);
    std::fill(dutyFactors.begin(), dutyFactors.end(), GaitPatterns::getDutyFactor(currentGait));

    if (limbCount == 0) return;

    std::vector<float> offsets;

    switch (limbCount) {
        case 2: // Biped
            offsets = GaitPatterns::bipedWalk();
            break;

        case 4: // Quadruped
            switch (currentGait) {
                case GaitType::WALK:   offsets = GaitPatterns::quadrupedWalk(); break;
                case GaitType::TROT:   offsets = GaitPatterns::quadrupedTrot(); break;
                case GaitType::GALLOP: offsets = GaitPatterns::quadrupedGallop(); break;
                default:               offsets = GaitPatterns::quadrupedWalk(); break;
            }
            break;

        case 6: // Hexapod
            switch (currentGait) {
                case GaitType::TRIPOD: offsets = GaitPatterns::hexapodTripod(); break;
                case GaitType::WAVE:   offsets = GaitPatterns::hexapodWave(); break;
                default:               offsets = GaitPatterns::hexapodTripod(); break;
            }
            break;

        case 8: // Octopod - use wave pattern
            for (int i = 0; i < 8; i++) {
                offsets.push_back(static_cast<float>(i) / 8.0f);
            }
            break;

        default:
            // Generic multi-leg: distribute phases evenly
            for (int i = 0; i < limbCount; i++) {
                offsets.push_back(static_cast<float>(i) / limbCount);
            }
            break;
    }

    // Copy offsets to member
    for (size_t i = 0; i < std::min(offsets.size(), limbPhaseOffsets.size()); i++) {
        limbPhaseOffsets[i] = offsets[i];
    }
}

// =============================================================================
// IK SOLVER IMPLEMENTATION
// =============================================================================

bool IKSolver::solveLimb(
    const std::vector<BodySegment>& segments,
    const std::vector<int>& limbSegmentIndices,
    const glm::vec3& targetPosition,
    std::vector<float>& outJointAngles,
    int maxIterations)
{
    if (limbSegmentIndices.empty()) return false;

    // Build joint positions from segments
    std::vector<glm::vec3> jointPositions;
    std::vector<float> segmentLengths;

    glm::vec3 basePos = segments[limbSegmentIndices[0]].localPosition;
    jointPositions.push_back(basePos);

    for (size_t i = 0; i < limbSegmentIndices.size(); i++) {
        const auto& seg = segments[limbSegmentIndices[i]];
        float length = seg.size.y * 2.0f; // Assuming Y is the long axis
        segmentLengths.push_back(length);

        // Add joint at end of segment
        glm::vec3 endPos = jointPositions.back() + glm::vec3(0, -length, 0);
        jointPositions.push_back(endPos);
    }

    // Solve using FABRIK
    bool solved = solveFABRIK(jointPositions, segmentLengths, targetPosition,
                              basePos, 0.01f, maxIterations);

    if (solved) {
        // Extract joint angles from solved positions
        outJointAngles.resize(limbSegmentIndices.size());
        for (size_t i = 0; i < limbSegmentIndices.size(); i++) {
            glm::vec3 dir = jointPositions[i + 1] - jointPositions[i];
            outJointAngles[i] = std::atan2(dir.z, -dir.y);
        }
    }

    return solved;
}

bool IKSolver::solveFABRIK(
    std::vector<glm::vec3>& jointPositions,
    const std::vector<float>& segmentLengths,
    const glm::vec3& target,
    const glm::vec3& basePosition,
    float tolerance,
    int maxIterations)
{
    if (jointPositions.size() < 2) return false;

    int n = static_cast<int>(jointPositions.size());

    // Calculate total reach
    float totalLength = 0.0f;
    for (float len : segmentLengths) {
        totalLength += len;
    }

    // Check if target is reachable
    float targetDist = glm::length(target - basePosition);
    if (targetDist > totalLength) {
        // Target unreachable - stretch towards it
        glm::vec3 dir = (targetDist > 0.0001f) ?
            (target - basePosition) / targetDist : glm::vec3(0, -1, 0);
        jointPositions[0] = basePosition;
        for (int i = 0; i < n - 1; i++) {
            jointPositions[i + 1] = jointPositions[i] + dir * segmentLengths[i];
        }
        return false;
    }

    // FABRIK iterations
    for (int iter = 0; iter < maxIterations; iter++) {
        // Check convergence
        float endError = glm::length(jointPositions[n - 1] - target);
        if (endError < tolerance) {
            return true;
        }

        // Forward pass: from end to base
        jointPositions[n - 1] = target;
        for (int i = n - 2; i >= 0; i--) {
            glm::vec3 diff = jointPositions[i] - jointPositions[i + 1];
            float length = glm::length(diff);
            glm::vec3 dir = (length > 0.0001f) ? diff / length : glm::vec3(0, 1, 0);
            jointPositions[i] = jointPositions[i + 1] + dir * segmentLengths[i];
        }

        // Backward pass: from base to end
        jointPositions[0] = basePosition;
        for (int i = 0; i < n - 1; i++) {
            glm::vec3 diff = jointPositions[i + 1] - jointPositions[i];
            float length = glm::length(diff);
            glm::vec3 dir = (length > 0.0001f) ? diff / length : glm::vec3(0, -1, 0);
            jointPositions[i + 1] = jointPositions[i] + dir * segmentLengths[i];
        }
    }

    return glm::length(jointPositions[n - 1] - target) < tolerance;
}

bool IKSolver::solve2Bone(
    float upperLength,
    float lowerLength,
    const glm::vec3& target,
    const glm::vec3& poleVector,
    float& outUpperAngle,
    float& outLowerAngle)
{
    float targetDist = glm::length(target);

    // Check reachability
    if (targetDist > upperLength + lowerLength) {
        // Fully extended
        outUpperAngle = std::atan2(target.z, target.y);
        outLowerAngle = 0.0f;
        return false;
    }

    if (targetDist < std::abs(upperLength - lowerLength)) {
        // Too close
        outUpperAngle = 0.0f;
        outLowerAngle = 3.14159f;
        return false;
    }

    // Law of cosines for elbow angle
    float cosElbow = (upperLength * upperLength + lowerLength * lowerLength - targetDist * targetDist) /
                     (2.0f * upperLength * lowerLength);
    cosElbow = std::max(-1.0f, std::min(1.0f, cosElbow));
    outLowerAngle = std::acos(cosElbow);

    // Shoulder angle
    float cosShoulderOffset = (upperLength * upperLength + targetDist * targetDist - lowerLength * lowerLength) /
                               (2.0f * upperLength * targetDist);
    cosShoulderOffset = std::max(-1.0f, std::min(1.0f, cosShoulderOffset));
    float shoulderOffset = std::acos(cosShoulderOffset);

    float targetAngle = std::atan2(target.z, -target.y);
    outUpperAngle = targetAngle + shoulderOffset;

    return true;
}

// =============================================================================
// LOCOMOTION CONTROLLER IMPLEMENTATION
// =============================================================================

void LocomotionController::initialize(const BodyPlan& plan, const MorphologyGenes& g) {
    bodyPlan = &plan;
    genes = g;

    // Initialize CPG
    GaitType defaultGait = GaitType::WALK;
    if (genes.legPairs == 0) defaultGait = GaitType::SLITHER;
    else if (genes.legPairs == 1) defaultGait = GaitType::HOP;
    else if (genes.legPairs == 3) defaultGait = GaitType::TRIPOD;

    cpg.initialize(*bodyPlan, defaultGait);
    cpg.setFrequency(genes.getLimbFrequency());

    // Build limb chains
    limbChains.clear();
    const auto& segments = bodyPlan->getSegments();

    // Find all limb root segments and trace chains
    for (size_t i = 0; i < segments.size(); i++) {
        const auto& seg = segments[i];
        if (seg.appendageType == AppendageType::LEG && seg.segmentIndexInLimb == 0) {
            // Trace this limb chain
            std::vector<int> chain;
            int current = static_cast<int>(i);

            while (current >= 0 && static_cast<size_t>(current) < segments.size()) {
                chain.push_back(current);
                if (segments[current].isTerminal) break;

                // Find child in same limb
                int nextChild = -1;
                for (int childIdx : segments[current].childIndices) {
                    if (segments[childIdx].appendageType == AppendageType::LEG) {
                        nextChild = childIdx;
                        break;
                    }
                }
                current = nextChild;
            }

            if (!chain.empty()) {
                limbChains.push_back(chain);
            }
        }
    }

    // Initialize limb states
    limbStates.resize(limbChains.size());
    for (size_t i = 0; i < limbStates.size(); i++) {
        limbStates[i].limbIndex = static_cast<int>(i);
    }

    // Initialize joint states
    jointStates.resize(segments.size());
    for (size_t i = 0; i < segments.size(); i++) {
        jointStates[i].segmentIndex = static_cast<int>(i);
    }
}

void LocomotionController::update(float deltaTime, const glm::vec3& desiredVelocity, float groundHeight) {
    if (!bodyPlan) return;

    float speed = glm::length(desiredVelocity);

    // Update CPG
    updateCPG(deltaTime, speed);

    // Update limb targets based on CPG
    updateLimbTargets(groundHeight);

    // Solve IK for each limb
    solveIK();

    // Calculate joint torques to achieve target angles
    calculateTorques(deltaTime);

    // Update balance tracking
    updateBalance();

    // Calculate energy expenditure
    calculateEnergyExpenditure(deltaTime);

    // Update velocity based on torques and stance/swing
    if (speed > 0.01f) {
        glm::vec3 moveDir = glm::normalize(desiredVelocity);
        float maxSpeed = genes.getMaxSpeed();
        velocity = moveDir * std::min(speed, maxSpeed);
        position += velocity * deltaTime;
    } else {
        velocity *= 0.9f; // Damping when not moving
    }
}

void LocomotionController::updateCPG(float deltaTime, float speed) {
    // Adjust gait based on speed
    GaitType optimalGait = selectOptimalGait(speed);
    if (optimalGait != cpg.getGaitType()) {
        cpg.setGaitType(optimalGait);
    }

    // Update frequency based on speed
    float baseFreq = genes.getLimbFrequency();
    float speedRatio = speed / genes.getMaxSpeed();
    cpg.setFrequency(baseFreq * (0.5f + speedRatio * 1.5f));

    cpg.update(deltaTime, 1.0f);
}

void LocomotionController::updateLimbTargets(float groundHeight) {
    for (size_t i = 0; i < limbStates.size(); i++) {
        float phase = cpg.getLimbPhase(static_cast<int>(i));
        bool inStance = cpg.isLimbInStance(static_cast<int>(i));

        limbStates[i].phase = phase;
        limbStates[i].inStance = inStance;
        limbStates[i].footTarget = calculateFootTarget(static_cast<int>(i), phase, groundHeight);
    }
}

void LocomotionController::solveIK() {
    const auto& segments = bodyPlan->getSegments();

    for (size_t i = 0; i < limbChains.size(); i++) {
        std::vector<float> angles;
        bool solved = IKSolver::solveLimb(
            segments,
            limbChains[i],
            limbStates[i].footTarget,
            angles
        );

        if (solved) {
            // Update joint states with solved angles
            for (size_t j = 0; j < limbChains[i].size() && j < angles.size(); j++) {
                int segIdx = limbChains[i][j];
                jointStates[segIdx].targetAngle = angles[j];
            }
        }
    }
}

void LocomotionController::calculateTorques(float deltaTime) {
    const auto& segments = bodyPlan->getSegments();

    for (auto& js : jointStates) {
        const auto& seg = segments[js.segmentIndex];

        // PD controller for joint angle
        float error = js.targetAngle - js.currentAngle;
        float kp = seg.jointToParent.stiffness;
        float kd = seg.jointToParent.damping;

        float torque = kp * error - kd * js.angularVelocity;

        // Clamp to max torque
        float maxTorque = seg.jointToParent.maxTorque;
        torque = std::max(-maxTorque, std::min(maxTorque, torque));

        js.appliedTorque = torque;

        // Integrate angular velocity
        float angularAccel = torque / seg.mass; // Simplified
        js.angularVelocity += angularAccel * deltaTime;

        // PHASE 11 - Agent 9: Increased angular damping (prevents spinning)
        // Old: 0.98f (2% damping) - too weak, allowed spinning to persist
        // New: 0.85f (15% damping) - strong enough to stop oscillations
        js.angularVelocity *= 0.85f;

        // Integrate angle
        js.currentAngle += js.angularVelocity * deltaTime;

        // Clamp to joint limits
        js.currentAngle = std::max(seg.jointToParent.minAngle,
                         std::min(seg.jointToParent.maxAngle, js.currentAngle));
    }
}

void LocomotionController::updateBalance() {
    // Count limbs in stance
    int stanceCount = 0;
    for (const auto& ls : limbStates) {
        if (ls.inStance) stanceCount++;
    }

    // Calculate support polygon (simplified)
    // Stable if at least 3 feet on ground, or center of mass over support
    if (genes.legPairs >= 2) {
        stable = stanceCount >= 3;
    } else if (genes.legPairs == 1) {
        stable = stanceCount >= 1; // Biped is stable with 1 foot
    } else {
        stable = true; // Serpentine is always "stable"
    }
}

void LocomotionController::calculateEnergyExpenditure(float deltaTime) {
    energyExpenditure = 0.0f;

    // Energy from joint torques
    for (const auto& js : jointStates) {
        // Power = torque * angular velocity
        float power = std::abs(js.appliedTorque * js.angularVelocity);
        energyExpenditure += power * deltaTime;
    }

    // Scale by metabolic rate
    energyExpenditure *= genes.metabolicMultiplier;

    // Base metabolic cost
    energyExpenditure += genes.getMetabolicRate() * 0.01f * deltaTime;
}

GaitType LocomotionController::selectOptimalGait(float speed) const {
    float maxSpeed = genes.getMaxSpeed();
    float speedRatio = speed / maxSpeed;

    if (genes.legPairs == 0) {
        return GaitType::SLITHER;
    } else if (genes.legPairs == 1) {
        return speedRatio > 0.5f ? GaitType::HOP : GaitType::WALK;
    } else if (genes.legPairs == 2) {
        if (speedRatio < 0.3f) return GaitType::WALK;
        else if (speedRatio < 0.7f) return GaitType::TROT;
        else return GaitType::GALLOP;
    } else if (genes.legPairs == 3) {
        return speedRatio > 0.3f ? GaitType::TRIPOD : GaitType::WAVE;
    } else {
        return GaitType::WAVE;
    }
}

glm::vec3 LocomotionController::calculateFootTarget(int limbIndex, float phase, float groundHeight) const {
    if (limbIndex < 0 || limbIndex >= static_cast<int>(limbChains.size())) {
        return glm::vec3(0, groundHeight, 0);
    }

    // Get limb root position
    const auto& segments = bodyPlan->getSegments();
    int rootIdx = limbChains[limbIndex][0];
    glm::vec3 hipPos = segments[rootIdx].localPosition + position;

    // Calculate foot position based on gait phase
    bool inStance = phase < GaitPatterns::getDutyFactor(cpg.getGaitType());
    float stepLength = calculateStepLength(limbIndex);
    float stepHeight = calculateStepHeight(limbIndex);

    glm::vec3 footPos;

    if (inStance) {
        // Stance phase: foot on ground, moving backward relative to body
        float stancePhase = phase / GaitPatterns::getDutyFactor(cpg.getGaitType());
        float zOffset = stepLength * (0.5f - stancePhase);

        footPos = hipPos + glm::vec3(0, groundHeight - hipPos.y, zOffset);
    } else {
        // Swing phase: lift foot and move forward
        float dutyFactor = GaitPatterns::getDutyFactor(cpg.getGaitType());
        float swingPhase = (phase - dutyFactor) / (1.0f - dutyFactor);

        // Parabolic trajectory for swing
        float height = stepHeight * 4.0f * swingPhase * (1.0f - swingPhase);
        float zOffset = stepLength * (-0.5f + swingPhase);

        footPos = hipPos + glm::vec3(0, groundHeight - hipPos.y + height, zOffset);
    }

    return footPos;
}

float LocomotionController::calculateStepHeight(int limbIndex) const {
    // Step height based on leg length and speed
    float legLength = genes.legLength * genes.bodyLength;
    float speedRatio = glm::length(velocity) / genes.getMaxSpeed();

    return legLength * 0.15f * (1.0f + speedRatio);
}

float LocomotionController::calculateStepLength(int limbIndex) const {
    // Step length based on leg length and gait
    float legLength = genes.legLength * genes.bodyLength;
    float speedRatio = glm::length(velocity) / genes.getMaxSpeed();

    float baseStepLength = legLength * 0.8f;

    // Longer steps at higher speeds
    return baseStepLength * (0.5f + speedRatio * 1.0f);
}

void LocomotionController::setGaitType(GaitType gait) {
    cpg.setGaitType(gait);
}

// =============================================================================
// PHYSICS BODY IMPLEMENTATION
// =============================================================================

void PhysicsBody::initialize(const BodyPlan& plan) {
    bodyPlan = &plan;

    int n = bodyPlan->getSegmentCount();
    segmentPositions.resize(n);
    segmentOrientations.resize(n, glm::quat(1, 0, 0, 0));
    segmentVelocities.resize(n, glm::vec3(0.0f));
    segmentAngularVelocities.resize(n, glm::vec3(0.0f));
    forces.resize(n, glm::vec3(0.0f));
    torques.resize(n, glm::vec3(0.0f));
    jointAngles.resize(n, 0.0f);
    jointVelocities.resize(n, 0.0f);

    // Initialize positions from body plan
    const auto& segments = bodyPlan->getSegments();
    for (int i = 0; i < n; i++) {
        segmentPositions[i] = segments[i].localPosition;
    }
}

void PhysicsBody::update(float deltaTime) {
    if (!bodyPlan) return;

    // Apply gravity
    const auto& segments = bodyPlan->getSegments();
    for (size_t i = 0; i < segments.size(); i++) {
        forces[i] += glm::vec3(0, -gravity * segments[i].mass, 0);
    }

    // Integrate
    integrateVelocities(deltaTime);
    integratePositions(deltaTime);

    // Resolve constraints
    solveJointConstraints();
    applyJointLimits();
    resolveGroundContact();

    clearForces();
}

void PhysicsBody::applyForce(const glm::vec3& force, const glm::vec3& point) {
    // Find nearest segment
    float minDist = std::numeric_limits<float>::max();
    int nearest = 0;

    for (size_t i = 0; i < segmentPositions.size(); i++) {
        float dist = glm::length(point - segmentPositions[i]);
        if (dist < minDist) {
            minDist = dist;
            nearest = static_cast<int>(i);
        }
    }

    forces[nearest] += force;

    // Apply torque from offset
    glm::vec3 offset = point - segmentPositions[nearest];
    torques[nearest] += glm::cross(offset, force);
}

void PhysicsBody::applyTorque(int segmentIndex, const glm::vec3& torque) {
    if (segmentIndex >= 0 && segmentIndex < static_cast<int>(torques.size())) {
        torques[segmentIndex] += torque;
    }
}

void PhysicsBody::applyJointTorque(int segmentIndex, float torque) {
    if (!bodyPlan || segmentIndex < 0 || segmentIndex >= bodyPlan->getSegmentCount()) {
        return;
    }

    const auto& seg = bodyPlan->getSegments()[segmentIndex];

    // Apply torque around joint axis
    glm::vec3 axis = seg.jointToParent.axis;
    glm::vec3 worldAxis = segmentOrientations[segmentIndex] * axis;

    applyTorque(segmentIndex, worldAxis * torque);
}

void PhysicsBody::resolveGroundContact() {
    for (size_t i = 0; i < segmentPositions.size(); i++) {
        const auto& seg = bodyPlan->getSegments()[i];
        float bottom = segmentPositions[i].y - seg.size.y;

        if (bottom < groundHeight) {
            // Push up
            segmentPositions[i].y = groundHeight + seg.size.y;

            // Kill downward velocity
            if (segmentVelocities[i].y < 0) {
                segmentVelocities[i].y *= -0.3f; // Bounce
            }

            // Friction
            segmentVelocities[i].x *= 0.9f;
            segmentVelocities[i].z *= 0.9f;
        }
    }
}

glm::vec3 PhysicsBody::getCenterOfMass() const {
    glm::vec3 com(0.0f);
    float totalMass = 0.0f;

    const auto& segments = bodyPlan->getSegments();
    for (size_t i = 0; i < segments.size(); i++) {
        com += segmentPositions[i] * segments[i].mass;
        totalMass += segments[i].mass;
    }

    if (totalMass > 0.0f) {
        com /= totalMass;
    }
    return com;
}

glm::vec3 PhysicsBody::getVelocity() const {
    glm::vec3 vel(0.0f);
    float totalMass = 0.0f;

    const auto& segments = bodyPlan->getSegments();
    for (size_t i = 0; i < segments.size(); i++) {
        vel += segmentVelocities[i] * segments[i].mass;
        totalMass += segments[i].mass;
    }

    if (totalMass > 0.0f) {
        vel /= totalMass;
    }
    return vel;
}

float PhysicsBody::getKineticEnergy() const {
    float ke = 0.0f;
    const auto& segments = bodyPlan->getSegments();

    for (size_t i = 0; i < segments.size(); i++) {
        float v2 = glm::dot(segmentVelocities[i], segmentVelocities[i]);
        ke += 0.5f * segments[i].mass * v2;

        // Rotational energy (simplified)
        float w2 = glm::dot(segmentAngularVelocities[i], segmentAngularVelocities[i]);
        ke += 0.5f * segments[i].inertia[0][0] * w2;
    }

    return ke;
}

void PhysicsBody::setRootPosition(const glm::vec3& pos) {
    if (segmentPositions.empty()) return;

    glm::vec3 offset = pos - segmentPositions[0];
    for (auto& p : segmentPositions) {
        p += offset;
    }
}

void PhysicsBody::setRootOrientation(const glm::quat& orient) {
    if (segmentOrientations.empty()) return;
    segmentOrientations[0] = orient;
}

void PhysicsBody::setRootVelocity(const glm::vec3& vel) {
    if (segmentVelocities.empty()) return;
    glm::vec3 offset = vel - segmentVelocities[0];
    for (auto& v : segmentVelocities) {
        v += offset;
    }
}

void PhysicsBody::integrateVelocities(float deltaTime) {
    const auto& segments = bodyPlan->getSegments();

    for (size_t i = 0; i < segments.size(); i++) {
        // Linear acceleration
        glm::vec3 accel = forces[i] / segments[i].mass;
        segmentVelocities[i] += accel * deltaTime;

        // Angular acceleration (simplified - using first diagonal of inertia)
        float I = segments[i].inertia[0][0];
        if (I > 0.0001f) {
            glm::vec3 angAccel = torques[i] / I;
            segmentAngularVelocities[i] += angAccel * deltaTime;
        }

        // Damping
        segmentVelocities[i] *= 0.99f;
        segmentAngularVelocities[i] *= 0.95f;
    }
}

void PhysicsBody::integratePositions(float deltaTime) {
    for (size_t i = 0; i < segmentPositions.size(); i++) {
        segmentPositions[i] += segmentVelocities[i] * deltaTime;

        // Orientation (simplified - just integrate around Y for now)
        if (glm::length(segmentAngularVelocities[i]) > 0.001f) {
            float angle = glm::length(segmentAngularVelocities[i]) * deltaTime;
            glm::vec3 axis = glm::normalize(segmentAngularVelocities[i]);
            glm::quat dq = glm::angleAxis(angle, axis);
            segmentOrientations[i] = dq * segmentOrientations[i];
        }
    }
}

void PhysicsBody::solveJointConstraints() {
    const auto& segments = bodyPlan->getSegments();

    // Multiple iterations for stability
    for (int iter = 0; iter < 4; iter++) {
        for (size_t i = 1; i < segments.size(); i++) {
            int parent = segments[i].parentIndex;
            if (parent < 0) continue;

            // Calculate expected position based on parent
            glm::vec3 localOffset = segments[i].localPosition - segments[parent].localPosition;
            glm::vec3 expectedPos = segmentPositions[parent] +
                                    segmentOrientations[parent] * localOffset;

            // Move towards constraint
            glm::vec3 error = expectedPos - segmentPositions[i];
            segmentPositions[i] += error * 0.5f;
            segmentPositions[parent] -= error * 0.5f;
        }
    }
}

void PhysicsBody::applyJointLimits() {
    const auto& segments = bodyPlan->getSegments();

    for (size_t i = 1; i < segments.size(); i++) {
        const auto& joint = segments[i].jointToParent;

        // Clamp joint angle
        jointAngles[i] = std::max(joint.minAngle,
                        std::min(joint.maxAngle, jointAngles[i]));

        // Clamp joint velocity if at limits
        if (jointAngles[i] <= joint.minAngle && jointVelocities[i] < 0) {
            jointVelocities[i] = 0;
        }
        if (jointAngles[i] >= joint.maxAngle && jointVelocities[i] > 0) {
            jointVelocities[i] = 0;
        }
    }
}

void PhysicsBody::clearForces() {
    for (auto& f : forces) f = glm::vec3(0.0f);
    for (auto& t : torques) t = glm::vec3(0.0f);
}
