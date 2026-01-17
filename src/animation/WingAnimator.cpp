#include "WingAnimator.h"
#include "../physics/Morphology.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace animation {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI * 0.5f;
constexpr float GRAVITY = 9.81f;

// This file provides additional non-inline implementations for WingAnimator
// Most functionality is in the header as inline methods for performance

// Advanced wing animation calculations that benefit from being in a .cpp file

// Calculate realistic feather deformation based on air pressure
void calculateFeatherDeformation(WingPose& pose, float airSpeed, float angleOfAttack, bool isDownstroke) {
    // Feathers spread on upstroke to reduce drag
    if (!isDownstroke) {
        pose.featherSpread = 0.7f;
    } else {
        // Feathers close on downstroke for maximum thrust
        pose.featherSpread = 0.0f;
    }

    // Wing tip bends backward under air pressure
    // More bend at higher speeds
    float pressureFactor = airSpeed * 0.02f;
    if (isDownstroke) {
        pose.wingTipBend = -pressureFactor * 15.0f;  // Bend backward
    } else {
        pose.wingTipBend = pressureFactor * 5.0f;    // Slight forward on recovery
    }
}

// Calculate insect wing deformation (membrane wings flex differently)
void calculateInsectWingDeformation(WingPose& pose, float phase, float frequency) {
    // Insect wings are rigid but rotate (pronation/supination)
    // No feather spread - they're membrane
    pose.featherSpread = 0.0f;

    // High-frequency oscillations cause subtle membrane vibration
    float vibration = std::sin(phase * 6.28318f * 4.0f) * 0.02f;
    pose.wingTipBend = vibration * frequency;
}

// Calculate bat wing membrane stretch
void calculateBatWingStretch(WingPose& pose, float phase, bool isDownstroke) {
    // Bat wings are highly flexible membrane
    // They can change shape dramatically during flight

    if (isDownstroke) {
        // Membrane taut during downstroke for lift
        pose.featherSpread = 0.0f;  // Represents membrane tension
        pose.wingTipBend = -10.0f;
    } else {
        // Membrane relaxes on upstroke
        pose.featherSpread = 0.3f;  // Slight relaxation
        pose.wingTipBend = 5.0f;

        // Fingers fold slightly to reduce surface area
        float foldAmount = std::sin(phase * 3.14159f) * 0.2f;
        pose.wristRotation = glm::angleAxis(foldAmount, glm::vec3(0, 1, 0)) * pose.wristRotation;
    }
}

// Extended wing animator for more complex behaviors
class ExtendedWingAnimator {
public:
    // Murmuration-style synchronized flapping
    static void synchronizeFlapping(WingAnimator& animator, float neighborPhase, float syncStrength) {
        // Gradually sync phase with neighbors
        float currentPhase = animator.getPhase();
        float phaseDiff = neighborPhase - currentPhase;

        // Normalize phase difference to [-0.5, 0.5]
        while (phaseDiff > 0.5f) phaseDiff -= 1.0f;
        while (phaseDiff < -0.5f) phaseDiff += 1.0f;

        // Apply sync (this would require modifying WingAnimator to have a setPhase method)
        // For now, this demonstrates the concept
    }

    // Tired flight animation (reduced amplitude, irregular rhythm)
    static void applyFatigue(WingAnimator& animator, float fatigueLevel) {
        WingAnimConfig& config = animator.getConfig();

        // Reduce amplitude when tired
        float originalAmplitude = config.flapAmplitude;
        config.flapAmplitude = originalAmplitude * (1.0f - fatigueLevel * 0.3f);

        // Add irregularity (would need to modify base frequency)
        // This demonstrates the concept
    }

    // Injury compensation (asymmetric flight)
    static void applyWingDamage(WingAnimator& animator, bool isLeftWing, float damageLevel) {
        // Damaged wing has reduced range of motion
        // The other wing compensates by working harder
        // This would need deeper integration with WingAnimator
    }
};

// Procedural feather animation helpers
namespace FeatherAnimation {

    // Calculate individual feather rotation based on position along wing
    glm::quat calculateFeatherRotation(int featherIndex, int totalFeathers,
                                        float wingAngle, float airSpeed, bool isUpstroke) {
        // Feathers at tip rotate more than at root
        float positionFactor = static_cast<float>(featherIndex) / totalFeathers;

        // Base rotation follows wing angle
        float baseAngle = wingAngle * positionFactor;

        // Air pressure causes additional rotation
        float pressureAngle = airSpeed * 0.01f * positionFactor;

        // Feathers spread on upstroke
        float spreadAngle = isUpstroke ? 10.0f * positionFactor : 0.0f;

        // Combine rotations
        glm::quat rotation = glm::angleAxis(glm::radians(baseAngle), glm::vec3(0, 0, 1));
        rotation = glm::angleAxis(glm::radians(spreadAngle), glm::vec3(0, 1, 0)) * rotation;

        return rotation;
    }

    // Calculate primary feather positions for detailed wing rendering
    void calculatePrimaryFeathers(std::vector<glm::vec3>& featherPositions,
                                   std::vector<glm::quat>& featherRotations,
                                   const WingPose& pose, float wingSpan, int featherCount) {
        featherPositions.resize(featherCount);
        featherRotations.resize(featherCount);

        for (int i = 0; i < featherCount; ++i) {
            float t = static_cast<float>(i) / (featherCount - 1);

            // Position along wing trailing edge
            float x = wingSpan * 0.5f * t;
            float y = 0.0f;
            float z = -0.1f * wingSpan * std::sqrt(t);  // Curved trailing edge

            featherPositions[i] = glm::vec3(x, y, z);

            // Rotation based on position and feather spread
            float spreadAngle = pose.featherSpread * 15.0f * t;
            featherRotations[i] = glm::angleAxis(glm::radians(spreadAngle), glm::vec3(0, 1, 0));
        }
    }
}

// Wing sound generation parameters (for audio system integration)
namespace WingSounds {

    struct WingAudioParams {
        float flappingVolume;
        float flappingPitch;
        float wooshVolume;
        float featherRustleVolume;
    };

    WingAudioParams calculateAudioParams(WingType type, float flapFrequency, float airSpeed,
                                          float phase, bool isDownstroke) {
        WingAudioParams params;

        // Base volumes depend on wing type
        switch (type) {
            case WingType::FEATHERED:
                params.flappingVolume = 0.3f;
                params.featherRustleVolume = 0.4f;
                break;
            case WingType::MEMBRANE:
                params.flappingVolume = 0.2f;
                params.featherRustleVolume = 0.0f;  // No feathers
                break;
            case WingType::INSECT_SINGLE:
            case WingType::INSECT_DOUBLE:
            case WingType::INSECT_COUPLED:
                params.flappingVolume = 0.1f;  // Buzzing instead
                params.featherRustleVolume = 0.0f;
                break;
            default:
                params.flappingVolume = 0.2f;
                params.featherRustleVolume = 0.1f;
                break;
        }

        // Frequency affects pitch
        params.flappingPitch = 0.5f + flapFrequency * 0.1f;

        // Speed affects woosh volume
        params.wooshVolume = std::min(1.0f, airSpeed * 0.05f);

        // Downstroke is louder
        if (isDownstroke) {
            params.flappingVolume *= 1.5f;
        }

        // Modulate by phase for rhythmic sound
        float phaseModulation = 0.5f + 0.5f * std::sin(phase * 6.28318f);
        params.flappingVolume *= phaseModulation;

        return params;
    }
}

// Wing IK solver for procedural perching/landing
namespace WingIK {

    // Solve wing fold to reach a perch point
    void solvePerchPose(WingPose& pose, const glm::vec3& shoulderPos, const glm::vec3& perchPoint,
                        float upperArmLength, float forearmLength, float handLength) {
        // Calculate total reach
        glm::vec3 toPerch = perchPoint - shoulderPos;
        float distance = glm::length(toPerch);
        float totalReach = upperArmLength + forearmLength + handLength;

        if (distance > totalReach) {
            // Can't reach - extend toward point
            glm::vec3 dir = glm::normalize(toPerch);
            pose.shoulderRotation = glm::rotation(glm::vec3(1, 0, 0), dir);
            pose.elbowRotation = glm::quat(1, 0, 0, 0);
            pose.wristRotation = glm::quat(1, 0, 0, 0);
        } else {
            // Solve two-bone IK for upper arm and forearm
            // Then orient hand to grasp

            // Simplified: fold wing to body
            pose.shoulderRotation = glm::angleAxis(glm::radians(-80.0f), glm::vec3(0, 0, 1));
            pose.elbowRotation = glm::angleAxis(glm::radians(100.0f), glm::vec3(0, 1, 0));
            pose.wristRotation = glm::angleAxis(glm::radians(-20.0f), glm::vec3(0, 1, 0));
        }

        pose.featherSpread = 0.0f;
        pose.wingTipBend = 0.0f;
    }

    // Solve wing pose for landing flare
    void solveLandingFlarePose(WingPose& pose, float flareAmount, bool isLeft) {
        float side = isLeft ? 1.0f : -1.0f;

        // Wings spread wide and angled back for maximum drag
        float shoulderAngle = glm::radians(30.0f + flareAmount * 30.0f);
        float backAngle = glm::radians(flareAmount * 45.0f);

        pose.shoulderRotation = glm::angleAxis(shoulderAngle, glm::vec3(0, 0, side));
        pose.shoulderRotation = glm::angleAxis(backAngle, glm::vec3(1, 0, 0)) * pose.shoulderRotation;

        // Elbows slightly bent
        pose.elbowRotation = glm::angleAxis(glm::radians(15.0f * side), glm::vec3(0, 1, 0));

        // Wrists extended
        pose.wristRotation = glm::quat(1, 0, 0, 0);

        // Feathers spread for maximum surface area
        pose.featherSpread = flareAmount;

        // Wing tips curl up from pressure
        pose.wingTipBend = flareAmount * 20.0f;
    }
}

// Utility functions for wing animation blending
namespace WingBlending {

    // Blend between two wing poses
    WingPose blend(const WingPose& a, const WingPose& b, float t) {
        WingPose result;
        result.shoulderRotation = glm::slerp(a.shoulderRotation, b.shoulderRotation, t);
        result.elbowRotation = glm::slerp(a.elbowRotation, b.elbowRotation, t);
        result.wristRotation = glm::slerp(a.wristRotation, b.wristRotation, t);
        result.tipRotation = glm::slerp(a.tipRotation, b.tipRotation, t);
        result.featherSpread = glm::mix(a.featherSpread, b.featherSpread, t);
        result.wingTipBend = glm::mix(a.wingTipBend, b.wingTipBend, t);
        return result;
    }

    // Additive blend (for layering animations)
    WingPose additive(const WingPose& base, const WingPose& add, float weight) {
        WingPose result = base;

        // Extract axis-angle from additive and apply weighted
        glm::vec3 shoulderAxis;
        float shoulderAngle;
        if (glm::length(glm::axis(add.shoulderRotation)) > 0.0f) {
            shoulderAxis = glm::axis(add.shoulderRotation);
            shoulderAngle = glm::angle(add.shoulderRotation) * weight;
            result.shoulderRotation = glm::angleAxis(shoulderAngle, shoulderAxis) * base.shoulderRotation;
        }

        // Similar for other joints...
        result.featherSpread = base.featherSpread + add.featherSpread * weight;
        result.wingTipBend = base.wingTipBend + add.wingTipBend * weight;

        return result;
    }

    // Masked blend (only blend specific joints)
    WingPose maskedBlend(const WingPose& a, const WingPose& b, float t,
                          bool blendShoulder, bool blendElbow, bool blendWrist, bool blendTip) {
        WingPose result = a;

        if (blendShoulder) {
            result.shoulderRotation = glm::slerp(a.shoulderRotation, b.shoulderRotation, t);
        }
        if (blendElbow) {
            result.elbowRotation = glm::slerp(a.elbowRotation, b.elbowRotation, t);
        }
        if (blendWrist) {
            result.wristRotation = glm::slerp(a.wristRotation, b.wristRotation, t);
        }
        if (blendTip) {
            result.tipRotation = glm::slerp(a.tipRotation, b.tipRotation, t);
        }

        return result;
    }
}

// Procedural wing turbulence (for realistic flight in wind)
namespace WingTurbulence {

    // Apply procedural noise to wing pose
    void applyTurbulence(WingPose& pose, float time, float intensity, uint32_t seed) {
        // Simple noise function
        auto noise = [](float t, uint32_t s) {
            float v = std::sin(t * 12.9898f + s * 78.233f);
            v = v * 43758.5453f;
            return v - std::floor(v);
        };

        // Apply to each joint
        float shoulderNoise = (noise(time, seed) - 0.5f) * intensity;
        float elbowNoise = (noise(time * 1.3f, seed + 1) - 0.5f) * intensity;
        float wristNoise = (noise(time * 1.7f, seed + 2) - 0.5f) * intensity;

        pose.shoulderRotation = glm::angleAxis(shoulderNoise * 0.1f, glm::vec3(0, 0, 1)) * pose.shoulderRotation;
        pose.elbowRotation = glm::angleAxis(elbowNoise * 0.05f, glm::vec3(0, 1, 0)) * pose.elbowRotation;
        pose.wristRotation = glm::angleAxis(wristNoise * 0.03f, glm::vec3(0, 1, 0)) * pose.wristRotation;
    }

    // Calculate gust response
    void applyGust(WingPose& pose, const glm::vec3& gustDirection, float gustStrength) {
        // Wings get pushed by gusts
        float upwardGust = gustDirection.y * gustStrength;
        float sidewaysGust = gustDirection.x * gustStrength;

        // Upward gusts lift the wings
        pose.shoulderRotation = glm::angleAxis(upwardGust * 0.1f, glm::vec3(1, 0, 0)) * pose.shoulderRotation;

        // Sideways gusts cause asymmetric response (handled at caller level)
    }
}

// =============================================================================
// FLIGHT SEQUENCE IMPLEMENTATION
// =============================================================================

void FlightSequence::addKeyframe(const FlightKeyframe& keyframe) {
    m_keyframes.push_back(keyframe);
    // Keep sorted by time
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const FlightKeyframe& a, const FlightKeyframe& b) {
            return a.time < b.time;
        });
}

void FlightSequence::clear() {
    m_keyframes.clear();
}

FlightKeyframe FlightSequence::evaluate(float time) const {
    if (m_keyframes.empty()) {
        return FlightKeyframe{};
    }

    if (m_keyframes.size() == 1 || time <= m_keyframes.front().time) {
        return m_keyframes.front();
    }

    if (time >= m_keyframes.back().time) {
        return m_keyframes.back();
    }

    // Find surrounding keyframes
    size_t i = 0;
    while (i < m_keyframes.size() - 1 && m_keyframes[i + 1].time < time) {
        ++i;
    }

    const FlightKeyframe& a = m_keyframes[i];
    const FlightKeyframe& b = m_keyframes[i + 1];

    float t = (time - a.time) / (b.time - a.time);

    // Interpolate
    FlightKeyframe result;
    result.time = time;
    result.state = t < 0.5f ? a.state : b.state;
    result.flapAmplitude = glm::mix(a.flapAmplitude, b.flapAmplitude, t);
    result.flapFrequency = glm::mix(a.flapFrequency, b.flapFrequency, t);
    result.bankAngle = glm::mix(a.bankAngle, b.bankAngle, t);
    result.pitchAngle = glm::mix(a.pitchAngle, b.pitchAngle, t);
    result.velocity = glm::mix(a.velocity, b.velocity, t);
    result.bodyRoll = glm::mix(a.bodyRoll, b.bodyRoll, t);

    return result;
}

float FlightSequence::getDuration() const {
    if (m_keyframes.empty()) return 0.0f;
    return m_keyframes.back().time;
}

bool FlightSequence::isComplete(float time) const {
    return time >= getDuration();
}

// =============================================================================
// PREDEFINED FLIGHT SEQUENCES
// =============================================================================

namespace FlightSequences {

FlightSequence createTakeoffSequence(WingType type, float mass, float wingSpan) {
    FlightSequence seq;

    // Calculate parameters based on physical properties
    float baseFlapFreq = 3.0f / std::sqrt(mass);  // Smaller = faster
    float amplitudeBoost = 1.3f;  // Extra power during takeoff

    if (type == WingType::INSECT_SINGLE || type == WingType::INSECT_DOUBLE) {
        // Insects: vertical takeoff
        seq.addKeyframe({0.0f, FlightAnimState::GROUNDED, 0.0f, 0.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});
        seq.addKeyframe({0.1f, FlightAnimState::TAKING_OFF, 60.0f, baseFlapFreq * 1.5f, 0.0f, -10.0f, {0, 2, 0}, 0.0f});
        seq.addKeyframe({0.3f, FlightAnimState::HOVERING, 55.0f, baseFlapFreq * 1.3f, 0.0f, 0.0f, {0, 3, 0}, 0.0f});
        seq.addKeyframe({0.5f, FlightAnimState::FLAPPING, 50.0f, baseFlapFreq, 0.0f, 5.0f, {0, 2, 3}, 0.0f});
    } else {
        // Birds/bats: running takeoff
        seq.addKeyframe({0.0f, FlightAnimState::GROUNDED, 0.0f, 0.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});
        seq.addKeyframe({0.2f, FlightAnimState::TAKING_OFF, 70.0f * amplitudeBoost, baseFlapFreq * 1.3f, 0.0f, -15.0f, {0, 0, 2}, 0.0f});
        seq.addKeyframe({0.5f, FlightAnimState::TAKING_OFF, 75.0f * amplitudeBoost, baseFlapFreq * 1.4f, 0.0f, -25.0f, {0, 1, 4}, 0.0f});
        seq.addKeyframe({0.8f, FlightAnimState::FLAPPING, 65.0f, baseFlapFreq * 1.2f, 0.0f, -10.0f, {0, 3, 6}, 0.0f});
        seq.addKeyframe({1.2f, FlightAnimState::FLAPPING, 60.0f, baseFlapFreq, 0.0f, 0.0f, {0, 4, 8}, 0.0f});
    }

    return seq;
}

FlightSequence createLandingSequence(WingType type, float approachSpeed) {
    FlightSequence seq;

    if (type == WingType::INSECT_SINGLE || type == WingType::INSECT_DOUBLE) {
        // Insects: hover then land
        seq.addKeyframe({0.0f, FlightAnimState::FLAPPING, 50.0f, 20.0f, 0.0f, 0.0f, {0, 0, approachSpeed}, 0.0f});
        seq.addKeyframe({0.3f, FlightAnimState::HOVERING, 55.0f, 25.0f, 0.0f, 10.0f, {0, -1, approachSpeed * 0.3f}, 0.0f});
        seq.addKeyframe({0.6f, FlightAnimState::LANDING, 45.0f, 20.0f, 0.0f, 15.0f, {0, -2, 0}, 0.0f});
        seq.addKeyframe({1.0f, FlightAnimState::GROUNDED, 0.0f, 0.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});
    } else {
        // Birds: glide approach then flare
        seq.addKeyframe({0.0f, FlightAnimState::GLIDING, 5.0f, 0.5f, 0.0f, -5.0f, {0, -2, approachSpeed}, 0.0f});
        seq.addKeyframe({0.5f, FlightAnimState::LANDING, 40.0f, 2.0f, 0.0f, 15.0f, {0, -3, approachSpeed * 0.5f}, 0.0f});
        seq.addKeyframe({0.8f, FlightAnimState::LANDING, 80.0f, 3.0f, 0.0f, 35.0f, {0, -2, approachSpeed * 0.2f}, 0.0f});  // Flare
        seq.addKeyframe({1.2f, FlightAnimState::GROUNDED, 0.0f, 0.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});
    }

    return seq;
}

FlightSequence createDiveAttackSequence(float diveAngle, float pulloutHeight) {
    FlightSequence seq;

    float diveSpeed = 30.0f;  // Fast dive
    float diveRad = glm::radians(diveAngle);

    seq.addKeyframe({0.0f, FlightAnimState::GLIDING, 5.0f, 0.5f, 0.0f, 0.0f, {0, 0, 10}, 0.0f});
    seq.addKeyframe({0.2f, FlightAnimState::DIVING, 10.0f, 0.0f, 0.0f, -diveAngle, {0, -diveSpeed * 0.3f, diveSpeed * 0.5f}, 0.0f});
    seq.addKeyframe({0.5f, FlightAnimState::DIVING, 5.0f, 0.0f, 0.0f, -diveAngle * 1.2f, {0, -diveSpeed, diveSpeed * 0.3f}, 0.0f});
    // Pullout
    seq.addKeyframe({0.7f, FlightAnimState::FLAPPING, 70.0f, 4.0f, 0.0f, 0.0f, {0, -diveSpeed * 0.5f, diveSpeed * 0.7f}, 0.0f});
    seq.addKeyframe({1.0f, FlightAnimState::FLAPPING, 60.0f, 3.0f, 0.0f, 20.0f, {0, 5, 15}, 0.0f});
    seq.addKeyframe({1.3f, FlightAnimState::GLIDING, 5.0f, 0.5f, 0.0f, 5.0f, {0, 2, 12}, 0.0f});

    return seq;
}

FlightSequence createThermalSoarSequence(float thermalRadius, float climbRate) {
    FlightSequence seq;

    // Circling in thermal
    float bankAngle = 25.0f;
    float circleSpeed = 8.0f;

    seq.addKeyframe({0.0f, FlightAnimState::GLIDING, 5.0f, 0.3f, bankAngle, 5.0f, {circleSpeed * 0.5f, climbRate, circleSpeed * 0.866f}, 0.0f});
    seq.addKeyframe({2.0f, FlightAnimState::GLIDING, 5.0f, 0.2f, bankAngle, 5.0f, {-circleSpeed * 0.5f, climbRate, circleSpeed * 0.866f}, 0.0f});
    seq.addKeyframe({4.0f, FlightAnimState::GLIDING, 5.0f, 0.3f, bankAngle, 5.0f, {-circleSpeed, climbRate, 0}, 0.0f});
    seq.addKeyframe({6.0f, FlightAnimState::GLIDING, 5.0f, 0.2f, bankAngle, 5.0f, {-circleSpeed * 0.5f, climbRate, -circleSpeed * 0.866f}, 0.0f});
    seq.addKeyframe({8.0f, FlightAnimState::GLIDING, 5.0f, 0.3f, bankAngle, 5.0f, {circleSpeed * 0.5f, climbRate, -circleSpeed * 0.866f}, 0.0f});
    seq.addKeyframe({10.0f, FlightAnimState::GLIDING, 5.0f, 0.2f, bankAngle, 5.0f, {circleSpeed, climbRate, 0}, 0.0f});

    return seq;
}

FlightSequence createHoverSearchSequence(float duration, float searchRadius) {
    FlightSequence seq;

    // Hover and look around
    seq.addKeyframe({0.0f, FlightAnimState::HOVERING, 55.0f, 25.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});
    seq.addKeyframe({duration * 0.25f, FlightAnimState::HOVERING, 55.0f, 25.0f, 5.0f, -5.0f, {searchRadius * 0.3f, 0.5f, 0}, 10.0f});
    seq.addKeyframe({duration * 0.5f, FlightAnimState::HOVERING, 55.0f, 25.0f, -5.0f, 5.0f, {-searchRadius * 0.3f, -0.5f, 0}, -10.0f});
    seq.addKeyframe({duration * 0.75f, FlightAnimState::HOVERING, 55.0f, 25.0f, 0.0f, -10.0f, {0, 0.3f, searchRadius * 0.2f}, 5.0f});
    seq.addKeyframe({duration, FlightAnimState::HOVERING, 55.0f, 25.0f, 0.0f, 0.0f, {0, 0, 0}, 0.0f});

    return seq;
}

FlightSequence createBarrelRollSequence(float rollDuration) {
    FlightSequence seq;

    float rollSpeed = TWO_PI / rollDuration;

    seq.addKeyframe({0.0f, FlightAnimState::FLAPPING, 60.0f, 3.0f, 0.0f, 0.0f, {0, 0, 10}, 0.0f});
    seq.addKeyframe({rollDuration * 0.25f, FlightAnimState::FLAPPING, 50.0f, 3.0f, 45.0f, 10.0f, {3, 2, 10}, 90.0f});
    seq.addKeyframe({rollDuration * 0.5f, FlightAnimState::FLAPPING, 40.0f, 3.0f, 0.0f, 0.0f, {0, 0, 10}, 180.0f});
    seq.addKeyframe({rollDuration * 0.75f, FlightAnimState::FLAPPING, 50.0f, 3.0f, -45.0f, 10.0f, {-3, 2, 10}, 270.0f});
    seq.addKeyframe({rollDuration, FlightAnimState::FLAPPING, 60.0f, 3.0f, 0.0f, 0.0f, {0, 0, 10}, 360.0f});

    return seq;
}

}  // namespace FlightSequences

// =============================================================================
// MORPHOLOGY WING CONTROLLER IMPLEMENTATION
// =============================================================================

MorphologyWingController::MorphologyWingController() = default;

void MorphologyWingController::initializeFromMorphology(const MorphologyGenes& genes) {
    // Determine wing type from morphology
    m_wingType = determineWingType(genes);

    // Extract physical properties
    m_wingSpan = genes.wingSpan;
    m_wingArea = genes.wingArea;
    m_bodyMass = genes.mass;
    m_hasFourWings = genes.wingCount >= 4;

    // Determine flight capabilities
    float wingLoading = m_bodyMass / m_wingArea;

    // Higher wing loading = harder to hover
    m_canHover = wingLoading < 5.0f || m_wingType == WingType::INSECT_SINGLE ||
                 m_wingType == WingType::INSECT_DOUBLE || m_wingType == WingType::INSECT_COUPLED;

    // Lower aspect ratio = better gliding
    float aspectRatio = (m_wingSpan * m_wingSpan) / m_wingArea;
    m_canGlide = aspectRatio > 5.0f && m_wingType != WingType::INSECT_SINGLE;

    // Calculate max flap frequency (smaller wings = faster)
    m_maxFlapFrequency = 30.0f / std::sqrt(m_wingSpan);
    if (m_wingType == WingType::INSECT_SINGLE) {
        m_maxFlapFrequency = 200.0f;  // Flies can beat very fast
    } else if (m_wingType == WingType::INSECT_COUPLED) {
        m_maxFlapFrequency = 150.0f;  // Bees
    }

    // Configure primary animator
    WingAnimConfig primaryConfig;
    primaryConfig.type = m_wingType;
    primaryConfig.span = m_wingSpan;
    primaryConfig.flapFrequency = m_maxFlapFrequency * 0.5f;  // Start at half max
    primaryConfig.flapAmplitude = 60.0f;
    primaryConfig.glideFactor = m_canGlide ? 0.6f : 0.1f;

    // Adjust for wing type
    switch (m_wingType) {
        case WingType::FEATHERED:
            primaryConfig.downstrokeDuration = 0.55f;
            primaryConfig.elbowFoldAmount = 0.35f;
            primaryConfig.featherSpreadAmount = 0.7f;
            break;
        case WingType::MEMBRANE:
            primaryConfig.downstrokeDuration = 0.5f;
            primaryConfig.elbowFoldAmount = 0.5f;
            primaryConfig.featherSpreadAmount = 0.0f;  // No feathers
            break;
        case WingType::INSECT_SINGLE:
            primaryConfig.flapAmplitude = 80.0f;
            primaryConfig.figureEightAmplitude = 20.0f;
            break;
        case WingType::INSECT_DOUBLE:
            primaryConfig.flapAmplitude = 70.0f;
            primaryConfig.phaseOffset = 0.0f;
            break;
        case WingType::INSECT_COUPLED:
            primaryConfig.flapAmplitude = 75.0f;
            primaryConfig.phaseOffset = 0.1f;  // Slight coupling delay
            break;
    }

    m_primaryAnimator.initialize(primaryConfig);

    // Configure secondary animator for 4-wing creatures
    if (m_hasFourWings) {
        WingAnimConfig secondaryConfig = primaryConfig;
        secondaryConfig.phaseOffset = 0.5f;  // Hind wings out of phase
        secondaryConfig.flapAmplitude *= 0.8f;  // Slightly smaller amplitude
        m_secondaryAnimator.initialize(secondaryConfig);
    }
}

WingType MorphologyWingController::determineWingType(const MorphologyGenes& genes) const {
    // Feathered if has feathers
    if (genes.hasFeathers) {
        return WingType::FEATHERED;
    }

    // Insects based on body type and wing count
    if (genes.hasExoskeleton) {
        if (genes.wingCount == 2) {
            return WingType::INSECT_SINGLE;
        } else if (genes.wingCount >= 4) {
            // Dragonflies have independent wings, others are coupled
            if (genes.wingIndependence > 0.7f) {
                return WingType::INSECT_DOUBLE;
            } else {
                return WingType::INSECT_COUPLED;
            }
        }
    }

    // Default to membrane (bat-like)
    return WingType::MEMBRANE;
}

void MorphologyWingController::setFlightState(FlightAnimState state) {
    m_currentState = state;
    m_primaryAnimator.setFlightState(state);
    if (m_hasFourWings) {
        m_secondaryAnimator.setFlightState(state);
    }
}

void MorphologyWingController::startManeuver(FlightManeuver maneuver) {
    m_currentManeuver = maneuver;
    m_maneuverProgress = 0.0f;
    m_sequenceTime = 0.0f;

    // Create appropriate sequence
    switch (maneuver) {
        case FlightManeuver::TAKEOFF_RUN:
        case FlightManeuver::VERTICAL_TAKEOFF:
            m_activeSequence = FlightSequences::createTakeoffSequence(m_wingType, m_bodyMass, m_wingSpan);
            break;
        case FlightManeuver::LANDING_APPROACH:
            m_activeSequence = FlightSequences::createLandingSequence(m_wingType, glm::length(m_velocity));
            break;
        case FlightManeuver::DIVE_ATTACK:
            m_activeSequence = FlightSequences::createDiveAttackSequence(60.0f, 5.0f);
            break;
        case FlightManeuver::SOAR_THERMAL:
            m_activeSequence = FlightSequences::createThermalSoarSequence(20.0f, 2.0f);
            break;
        case FlightManeuver::HOVER_SEARCH:
            m_activeSequence = FlightSequences::createHoverSearchSequence(5.0f, 2.0f);
            break;
        case FlightManeuver::BARREL_ROLL:
            m_activeSequence = FlightSequences::createBarrelRollSequence(1.5f);
            break;
        default:
            m_activeSequence.clear();
            break;
    }

    m_maneuverDuration = m_activeSequence.getDuration();
}

void MorphologyWingController::setTargetPosition(const glm::vec3& target) {
    m_targetPosition = target;
}

void MorphologyWingController::setTargetVelocity(const glm::vec3& velocity) {
    m_targetVelocity = velocity;
}

void MorphologyWingController::setVelocity(const glm::vec3& velocity) {
    m_velocity = velocity;
    float speed = glm::length(velocity);
    m_primaryAnimator.setVelocity(speed);
    if (m_hasFourWings) {
        m_secondaryAnimator.setVelocity(speed);
    }
}

void MorphologyWingController::setAngularVelocity(const glm::vec3& angularVel) {
    m_angularVelocity = angularVel;
    m_primaryAnimator.setBankAngle(angularVel.z);
    if (m_hasFourWings) {
        m_secondaryAnimator.setBankAngle(angularVel.z);
    }
}

void MorphologyWingController::setAltitude(float altitude) {
    m_altitude = altitude;
}

void MorphologyWingController::setGroundDistance(float distance) {
    m_groundDistance = distance;
}

void MorphologyWingController::setWindDirection(const glm::vec3& wind) {
    m_windDirection = glm::length(wind) > 0.001f ? glm::normalize(wind) : glm::vec3(0, 0, 1);
}

void MorphologyWingController::setWindSpeed(float speed) {
    m_windSpeed = speed;
}

void MorphologyWingController::setAirDensity(float density) {
    m_airDensity = density;
}

void MorphologyWingController::setThermalStrength(float strength) {
    m_thermalStrength = strength;
}

void MorphologyWingController::update(float deltaTime) {
    // Update maneuver if active
    if (m_currentManeuver != FlightManeuver::NONE) {
        updateManeuver(deltaTime);
    } else {
        updateFlightState(deltaTime);
    }

    // Update animators
    m_primaryAnimator.update(deltaTime);
    if (m_hasFourWings) {
        m_secondaryAnimator.update(deltaTime);
        m_leftHindWing = m_secondaryAnimator.getLeftWingPose();
        m_rightHindWing = m_secondaryAnimator.getRightWingPose();
    }

    // Apply wind effects
    applyWindEffects();

    // Update body motion
    updateBodyMotion(deltaTime);
}

void MorphologyWingController::updateFlightState(float deltaTime) {
    // Auto-transition states based on movement
    float speed = glm::length(m_velocity);
    float verticalSpeed = m_velocity.y;

    if (m_groundDistance < 0.1f && speed < 0.5f) {
        if (m_currentState != FlightAnimState::GROUNDED) {
            setFlightState(FlightAnimState::GROUNDED);
        }
        return;
    }

    // Determine best state
    if (verticalSpeed < -5.0f && m_currentState != FlightAnimState::DIVING) {
        setFlightState(FlightAnimState::DIVING);
    } else if (m_canHover && speed < 1.0f && m_groundDistance > 1.0f) {
        setFlightState(FlightAnimState::HOVERING);
    } else if (m_canGlide && speed > 5.0f && std::abs(verticalSpeed) < 1.0f) {
        setFlightState(FlightAnimState::GLIDING);
    } else if (m_groundDistance < 2.0f && verticalSpeed < 0) {
        setFlightState(FlightAnimState::LANDING);
    } else {
        setFlightState(FlightAnimState::FLAPPING);
    }
}

void MorphologyWingController::updateManeuver(float deltaTime) {
    m_sequenceTime += deltaTime;

    if (m_activeSequence.isComplete(m_sequenceTime)) {
        m_currentManeuver = FlightManeuver::NONE;
        m_maneuverProgress = 1.0f;
        return;
    }

    FlightKeyframe keyframe = m_activeSequence.evaluate(m_sequenceTime);
    m_maneuverProgress = m_sequenceTime / m_maneuverDuration;

    // Apply keyframe to animators
    m_primaryAnimator.setFlightState(keyframe.state);
    m_primaryAnimator.setBankAngle(glm::radians(keyframe.bankAngle));
    m_primaryAnimator.setVerticalVelocity(keyframe.velocity.y);

    // Update config
    WingAnimConfig& config = m_primaryAnimator.getConfig();
    config.flapAmplitude = keyframe.flapAmplitude;
    config.flapFrequency = keyframe.flapFrequency;

    if (m_hasFourWings) {
        m_secondaryAnimator.setFlightState(keyframe.state);
        m_secondaryAnimator.setBankAngle(glm::radians(keyframe.bankAngle));
    }
}

void MorphologyWingController::updateBodyMotion(float deltaTime) {
    // Body bobs with wing beats
    float phase = m_primaryAnimator.getPhase();
    float bobAmount = 0.02f * m_wingSpan;

    // More bob during downstroke
    float bob = std::sin(phase * TWO_PI) * bobAmount;
    if (m_primaryAnimator.isDownstroke()) {
        bob *= 1.5f;
    }

    m_bodyOffset = glm::vec3(0.0f, bob, 0.0f);

    // Body tilts with flight direction
    float speed = glm::length(m_velocity);
    if (speed > 0.1f) {
        glm::vec3 forward = glm::normalize(m_velocity);

        // Pitch based on vertical velocity
        float pitch = std::asin(std::clamp(forward.y, -1.0f, 1.0f));

        // Roll based on turning
        float roll = m_angularVelocity.z * 0.3f;

        m_bodyRotation = glm::angleAxis(roll, glm::vec3(0, 0, 1)) *
                         glm::angleAxis(pitch, glm::vec3(1, 0, 0));
    }
}

void MorphologyWingController::applyWindEffects() {
    if (m_windSpeed < 0.1f) return;

    // Wind affects wing poses through turbulence
    float turbulenceIntensity = m_windSpeed * 0.02f;

    // Apply to primary wings
    WingPose leftPose = m_primaryAnimator.getLeftWingPose();
    WingPose rightPose = m_primaryAnimator.getRightWingPose();

    WingTurbulence::applyTurbulence(leftPose, m_sequenceTime, turbulenceIntensity, 0);
    WingTurbulence::applyTurbulence(rightPose, m_sequenceTime, turbulenceIntensity, 1);

    // Headwind lifts wings slightly
    float headwind = glm::dot(m_windDirection, glm::vec3(0, 0, -1)) * m_windSpeed;
    if (headwind > 0.5f) {
        WingTurbulence::applyGust(leftPose, glm::vec3(0, headwind * 0.1f, 0), 1.0f);
        WingTurbulence::applyGust(rightPose, glm::vec3(0, headwind * 0.1f, 0), 1.0f);
    }
}

const WingPose& MorphologyWingController::getLeftWingPose() const {
    return m_primaryAnimator.getLeftWingPose();
}

const WingPose& MorphologyWingController::getRightWingPose() const {
    return m_primaryAnimator.getRightWingPose();
}

const WingPose& MorphologyWingController::getLeftHindWingPose() const {
    return m_leftHindWing;
}

const WingPose& MorphologyWingController::getRightHindWingPose() const {
    return m_rightHindWing;
}

glm::vec3 MorphologyWingController::getBodyOffset() const {
    return m_bodyOffset;
}

glm::quat MorphologyWingController::getBodyRotation() const {
    return m_bodyRotation;
}

float MorphologyWingController::getFlightEfficiency() const {
    // Calculate how efficiently creature is flying
    float speed = glm::length(m_velocity);
    if (speed < 0.1f) return 0.0f;

    // Gliding is most efficient
    if (m_currentState == FlightAnimState::GLIDING) {
        return 0.95f;
    }

    // Hovering is least efficient (high energy)
    if (m_currentState == FlightAnimState::HOVERING) {
        return 0.3f;
    }

    // Normal flight efficiency based on speed vs optimal
    float optimalSpeed = std::sqrt(m_bodyMass * GRAVITY / (0.5f * m_airDensity * m_wingArea));
    float speedRatio = speed / optimalSpeed;
    float efficiency = 1.0f - std::abs(speedRatio - 1.0f) * 0.3f;

    // Wind assistance
    float tailwind = glm::dot(m_windDirection, glm::normalize(m_velocity)) * m_windSpeed;
    efficiency += tailwind * 0.05f;

    return std::clamp(efficiency, 0.1f, 1.0f);
}

float MorphologyWingController::getStaminaCost() const {
    // Energy expenditure per second
    float baseCost = m_bodyMass * 0.1f;

    switch (m_currentState) {
        case FlightAnimState::GROUNDED:
            return 0.0f;
        case FlightAnimState::GLIDING:
            return baseCost * 0.1f;
        case FlightAnimState::HOVERING:
            return baseCost * 2.0f;  // Very expensive
        case FlightAnimState::DIVING:
            return baseCost * 0.2f;
        case FlightAnimState::TAKING_OFF:
        case FlightAnimState::LANDING:
            return baseCost * 1.5f;
        default:
            return baseCost;
    }
}

float MorphologyWingController::calculateLiftCoefficient() const {
    // Simplified lift coefficient
    float baseCoeff = 1.2f;

    switch (m_wingType) {
        case WingType::FEATHERED:
            baseCoeff = 1.4f;  // Good lift
            break;
        case WingType::MEMBRANE:
            baseCoeff = 1.2f;
            break;
        default:
            baseCoeff = 1.0f;
            break;
    }

    // Angle of attack effect (simplified)
    float pitch = std::asin(std::clamp(m_velocity.y / std::max(glm::length(m_velocity), 0.1f), -1.0f, 1.0f));
    float aoa = std::abs(pitch);

    if (aoa < 0.2f) {
        return baseCoeff * (1.0f + aoa * 2.0f);
    } else {
        return baseCoeff * (1.4f - (aoa - 0.2f) * 3.0f);  // Stall effect
    }
}

float MorphologyWingController::calculateDragCoefficient() const {
    float baseCoeff = 0.1f;

    // Feathered wings have lower drag
    if (m_wingType == WingType::FEATHERED) {
        baseCoeff = 0.08f;
    }

    // Higher when wings are flapping
    if (m_currentState == FlightAnimState::FLAPPING) {
        baseCoeff *= 1.5f;
    }

    return baseCoeff;
}

// =============================================================================
// WING PRESETS
// =============================================================================

namespace WingPresets {

WingAnimConfig smallBird() {
    WingAnimConfig config;
    config.type = WingType::FEATHERED;
    config.span = 0.3f;
    config.flapFrequency = 12.0f;
    config.flapAmplitude = 70.0f;
    config.glideFactor = 0.3f;
    config.downstrokeDuration = 0.5f;
    config.elbowFoldAmount = 0.4f;
    config.featherSpreadAmount = 0.8f;
    return config;
}

WingAnimConfig largeBird() {
    WingAnimConfig config;
    config.type = WingType::FEATHERED;
    config.span = 2.0f;
    config.flapFrequency = 2.5f;
    config.flapAmplitude = 55.0f;
    config.glideFactor = 0.7f;
    config.downstrokeDuration = 0.6f;
    config.elbowFoldAmount = 0.3f;
    config.featherSpreadAmount = 0.6f;
    return config;
}

WingAnimConfig seabird() {
    WingAnimConfig config;
    config.type = WingType::FEATHERED;
    config.span = 3.0f;
    config.flapFrequency = 1.5f;
    config.flapAmplitude = 40.0f;
    config.glideFactor = 0.9f;  // Excellent glider
    config.downstrokeDuration = 0.55f;
    config.elbowFoldAmount = 0.25f;
    config.featherSpreadAmount = 0.5f;
    return config;
}

WingAnimConfig hummingbird() {
    WingAnimConfig config;
    config.type = WingType::FEATHERED;
    config.span = 0.1f;
    config.flapFrequency = 50.0f;  // Very fast
    config.flapAmplitude = 120.0f;
    config.glideFactor = 0.0f;  // Cannot glide
    config.downstrokeDuration = 0.5f;
    config.elbowFoldAmount = 0.1f;
    config.featherSpreadAmount = 0.3f;
    return config;
}

WingAnimConfig bat() {
    WingAnimConfig config;
    config.type = WingType::MEMBRANE;
    config.span = 0.4f;
    config.flapFrequency = 8.0f;
    config.flapAmplitude = 80.0f;
    config.glideFactor = 0.4f;
    config.downstrokeDuration = 0.5f;
    config.elbowFoldAmount = 0.6f;
    config.wristFoldAmount = 0.4f;
    config.featherSpreadAmount = 0.0f;
    return config;
}

WingAnimConfig butterfly() {
    WingAnimConfig config;
    config.type = WingType::INSECT_COUPLED;
    config.span = 0.08f;
    config.flapFrequency = 10.0f;
    config.flapAmplitude = 90.0f;
    config.glideFactor = 0.5f;
    config.phaseOffset = 0.05f;
    config.figureEightAmplitude = 10.0f;
    return config;
}

WingAnimConfig dragonfly() {
    WingAnimConfig config;
    config.type = WingType::INSECT_DOUBLE;
    config.span = 0.1f;
    config.flapFrequency = 30.0f;
    config.flapAmplitude = 70.0f;
    config.glideFactor = 0.6f;
    config.phaseOffset = 0.0f;  // Independent
    config.figureEightAmplitude = 15.0f;
    return config;
}

WingAnimConfig bee() {
    WingAnimConfig config;
    config.type = WingType::INSECT_COUPLED;
    config.span = 0.02f;
    config.flapFrequency = 130.0f;  // Very fast
    config.flapAmplitude = 85.0f;
    config.glideFactor = 0.0f;
    config.phaseOffset = 0.1f;
    config.figureEightAmplitude = 20.0f;
    return config;
}

WingAnimConfig fly() {
    WingAnimConfig config;
    config.type = WingType::INSECT_SINGLE;
    config.span = 0.01f;
    config.flapFrequency = 200.0f;  // Extremely fast
    config.flapAmplitude = 90.0f;
    config.glideFactor = 0.0f;
    config.figureEightAmplitude = 25.0f;
    return config;
}

WingAnimConfig pterosaur() {
    WingAnimConfig config;
    config.type = WingType::MEMBRANE;
    config.span = 5.0f;
    config.flapFrequency = 1.0f;
    config.flapAmplitude = 50.0f;
    config.glideFactor = 0.85f;  // Excellent glider
    config.downstrokeDuration = 0.6f;
    config.elbowFoldAmount = 0.4f;
    config.wristFoldAmount = 0.3f;
    return config;
}

WingAnimConfig dragon() {
    WingAnimConfig config;
    config.type = WingType::MEMBRANE;
    config.span = 8.0f;
    config.flapFrequency = 0.8f;
    config.flapAmplitude = 60.0f;
    config.glideFactor = 0.7f;
    config.downstrokeDuration = 0.55f;
    config.elbowFoldAmount = 0.35f;
    config.wristFoldAmount = 0.25f;
    return config;
}

}  // namespace WingPresets

// =============================================================================
// WING PHYSICS
// =============================================================================

namespace WingPhysics {

float calculateLift(float airDensity, float velocity, float wingArea, float liftCoeff) {
    return 0.5f * airDensity * velocity * velocity * wingArea * liftCoeff;
}

float calculateDrag(float airDensity, float velocity, float wingArea, float dragCoeff) {
    return 0.5f * airDensity * velocity * velocity * wingArea * dragCoeff;
}

float calculateRequiredFrequency(float mass, float wingArea, float liftCoeff, float airDensity) {
    // Simplified: frequency needed to generate enough lift
    float requiredLift = mass * GRAVITY;
    float liftPerFlap = 0.5f * airDensity * wingArea * liftCoeff;
    return requiredLift / std::max(liftPerFlap, 0.001f);
}

float calculateGlideRatio(float liftCoeff, float dragCoeff) {
    return liftCoeff / std::max(dragCoeff, 0.001f);
}

float calculateMinGlideSpeed(float mass, float wingArea, float airDensity, float maxLiftCoeff) {
    // Minimum speed where lift equals weight
    return std::sqrt((2.0f * mass * GRAVITY) / (airDensity * wingArea * maxLiftCoeff));
}

float calculateTurnRadius(float speed, float bankAngle) {
    float g = GRAVITY;
    float tanBank = std::tan(std::abs(bankAngle));
    if (tanBank < 0.001f) return 9999.0f;  // Nearly straight
    return (speed * speed) / (g * tanBank);
}

float calculateMaxClimbRate(float thrust, float drag, float weight, float velocity) {
    float excessPower = (thrust - drag) * velocity;
    return excessPower / weight;
}

}  // namespace WingPhysics

// =============================================================================
// FORMATION CONTROLLER
// =============================================================================

glm::vec3 FormationController::calculateFormationPosition(int index, int totalCount) const {
    if (index == 0) {
        return m_leaderPosition;  // Leader position
    }

    glm::vec3 offset(0.0f);

    switch (m_formationType) {
        case FormationType::V_FORMATION: {
            // Alternating left/right
            bool isRight = (index % 2) == 1;
            int positionIndex = (index + 1) / 2;

            float lateralOffset = positionIndex * m_spacing * std::sin(m_vAngle);
            float backOffset = positionIndex * m_spacing * std::cos(m_vAngle);

            offset.x = isRight ? lateralOffset : -lateralOffset;
            offset.z = -backOffset;
            break;
        }
        case FormationType::ECHELON: {
            offset.x = index * m_spacing * 0.7f;
            offset.z = -index * m_spacing;
            break;
        }
        case FormationType::LINE_ABREAST: {
            int halfCount = totalCount / 2;
            offset.x = (index - halfCount) * m_spacing;
            break;
        }
        case FormationType::COLUMN: {
            offset.z = -index * m_spacing;
            break;
        }
        case FormationType::CLUSTER: {
            // Random-ish positions
            float angle = index * 2.4f;  // Golden angle approximation
            float radius = m_spacing * (0.5f + 0.5f * (index % 3));
            offset.x = std::cos(angle) * radius;
            offset.z = std::sin(angle) * radius - index * 0.5f;
            break;
        }
    }

    return m_leaderPosition + offset;
}

glm::vec3 FormationController::calculateFormationVelocity(
    const glm::vec3& currentPos,
    const glm::vec3& targetPos,
    const glm::vec3& currentVel
) const {
    glm::vec3 toTarget = targetPos - currentPos;
    float distance = glm::length(toTarget);

    if (distance < 0.1f) {
        return m_leaderVelocity;
    }

    // Blend between catching up and matching leader velocity
    float catchUpFactor = std::min(distance * 0.5f, 1.0f);
    glm::vec3 catchUpVel = glm::normalize(toTarget) * glm::length(m_leaderVelocity) * 1.2f;

    return glm::mix(m_leaderVelocity, catchUpVel, catchUpFactor);
}

float FormationController::getVortexBenefit(int positionIndex) const {
    if (positionIndex == 0) return 0.0f;  // Leader gets no benefit

    // Birds in formation save energy from wing tip vortices
    // Benefit is highest in V formation
    switch (m_formationType) {
        case FormationType::V_FORMATION:
            return 0.15f;  // ~15% energy savings
        case FormationType::ECHELON:
            return 0.12f;
        case FormationType::COLUMN:
            return 0.05f;  // Minimal benefit
        default:
            return 0.0f;
    }
}

} // namespace animation
