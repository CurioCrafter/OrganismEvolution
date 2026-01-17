#include "FlyingCreatures.h"
#include "../../environment/Terrain.h"
#include <algorithm>
#include <random>
#include <cmath>

namespace flying {

// ============================================================================
// FlyingGenome Implementation
// ============================================================================

FlyingGenome::FlyingGenome()
    : wingSpan(1.0f)
    , wingChord(0.15f)
    , aspectRatio(7.0f)
    , wingLoading(20.0f)
    , camber(0.08f)
    , wingTaper(0.4f)
    , wingTwist(3.0f)
    , dihedralAngle(5.0f)
    , sweepAngle(0.0f)
    , tailLength(0.3f)
    , tailSpan(0.2f)
    , tailFork(0.0f)
    , hasTailFeathers(true)
    , bodyStreamlining(0.5f)
    , neckLength(0.2f)
    , legLength(0.15f)
    , retractableLegs(false)
    , breastMuscleRatio(0.25f)
    , supracoracoideus(0.1f)
    , fastTwitchRatio(0.5f)
    , visualAcuity(0.7f)
    , motionSensitivity(0.8f)
    , uvVision(0.0f)
    , nightVision(0.0f)
    , echolocationStrength(0.0f)
    , magneticSense(0.3f)
    , pressureSense(0.2f)
    , flockingStrength(0.5f)
    , territorialRadius(10.0f)
    , migratoryUrge(0.0f)
    , nocturnality(0.0f)
    , aggressionLevel(0.3f)
    , curiosity(0.5f)
    , primaryColor(0.5f, 0.4f, 0.3f)
    , secondaryColor(0.7f, 0.6f, 0.5f)
    , accentColor(0.2f, 0.2f, 0.8f)
    , iridescence(0.0f)
    , patternComplexity(0.3f)
{
}

FlyingGenome FlyingGenome::forSubtype(FlyingSubtype subtype) {
    switch (subtype) {
        case FlyingSubtype::SONGBIRD:
            return FlyingCreatureFactory::createSongbird();
        case FlyingSubtype::RAPTOR_LARGE:
            return FlyingCreatureFactory::createRaptor(0.8f, true);
        case FlyingSubtype::RAPTOR_SMALL:
            return FlyingCreatureFactory::createRaptor(0.4f, false);
        case FlyingSubtype::OWL:
            return FlyingCreatureFactory::createOwl();
        case FlyingSubtype::HUMMINGBIRD:
            return FlyingCreatureFactory::createHummingbird();
        case FlyingSubtype::VULTURE:
            return FlyingCreatureFactory::createVulture();
        case FlyingSubtype::SEABIRD:
            return FlyingCreatureFactory::createSeabird();
        case FlyingSubtype::BUTTERFLY:
            return FlyingCreatureFactory::createButterfly();
        case FlyingSubtype::DRAGONFLY:
            return FlyingCreatureFactory::createDragonfly();
        case FlyingSubtype::BEE:
            return FlyingCreatureFactory::createBee();
        case FlyingSubtype::BEETLE:
            return FlyingCreatureFactory::createBeetle();
        case FlyingSubtype::MOSQUITO:
            return FlyingCreatureFactory::createMosquito();
        case FlyingSubtype::LOCUST:
            return FlyingCreatureFactory::createLocust();
        case FlyingSubtype::MICROBAT:
            return FlyingCreatureFactory::createMicrobat();
        case FlyingSubtype::FRUIT_BAT:
            return FlyingCreatureFactory::createFruitBat();
        case FlyingSubtype::PTEROSAUR:
            return FlyingCreatureFactory::createPterosaur();
        case FlyingSubtype::DRAGON_LARGE:
            return FlyingCreatureFactory::createDragon(3.0f, true);
        case FlyingSubtype::DRAGON_SMALL:
            return FlyingCreatureFactory::createDragon(1.0f, false);
        default:
            return FlyingGenome();
    }
}

FlyingGenome FlyingGenome::crossover(const FlyingGenome& a, const FlyingGenome& b) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    FlyingGenome child;

    // Crossover each trait with 50% chance from each parent
    child.wingSpan = dist(gen) < 0.5f ? a.wingSpan : b.wingSpan;
    child.wingChord = dist(gen) < 0.5f ? a.wingChord : b.wingChord;
    child.aspectRatio = dist(gen) < 0.5f ? a.aspectRatio : b.aspectRatio;
    child.wingLoading = dist(gen) < 0.5f ? a.wingLoading : b.wingLoading;
    child.camber = dist(gen) < 0.5f ? a.camber : b.camber;
    child.wingTaper = dist(gen) < 0.5f ? a.wingTaper : b.wingTaper;
    child.wingTwist = dist(gen) < 0.5f ? a.wingTwist : b.wingTwist;
    child.dihedralAngle = dist(gen) < 0.5f ? a.dihedralAngle : b.dihedralAngle;
    child.sweepAngle = dist(gen) < 0.5f ? a.sweepAngle : b.sweepAngle;

    child.tailLength = dist(gen) < 0.5f ? a.tailLength : b.tailLength;
    child.tailSpan = dist(gen) < 0.5f ? a.tailSpan : b.tailSpan;
    child.tailFork = dist(gen) < 0.5f ? a.tailFork : b.tailFork;
    child.hasTailFeathers = dist(gen) < 0.5f ? a.hasTailFeathers : b.hasTailFeathers;

    child.bodyStreamlining = dist(gen) < 0.5f ? a.bodyStreamlining : b.bodyStreamlining;
    child.breastMuscleRatio = dist(gen) < 0.5f ? a.breastMuscleRatio : b.breastMuscleRatio;
    child.supracoracoideus = dist(gen) < 0.5f ? a.supracoracoideus : b.supracoracoideus;

    child.visualAcuity = dist(gen) < 0.5f ? a.visualAcuity : b.visualAcuity;
    child.nightVision = dist(gen) < 0.5f ? a.nightVision : b.nightVision;
    child.echolocationStrength = dist(gen) < 0.5f ? a.echolocationStrength : b.echolocationStrength;

    child.flockingStrength = dist(gen) < 0.5f ? a.flockingStrength : b.flockingStrength;
    child.territorialRadius = dist(gen) < 0.5f ? a.territorialRadius : b.territorialRadius;
    child.aggressionLevel = dist(gen) < 0.5f ? a.aggressionLevel : b.aggressionLevel;

    // Blend colors
    float t = dist(gen);
    child.primaryColor = glm::mix(a.primaryColor, b.primaryColor, t);
    child.secondaryColor = glm::mix(a.secondaryColor, b.secondaryColor, t);
    child.accentColor = glm::mix(a.accentColor, b.accentColor, t);

    return child;
}

void FlyingGenome::mutate(float rate, float strength) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::normal_distribution<float> mutation(0.0f, strength);

    auto mutateValue = [&](float& value, float min, float max) {
        if (dist(gen) < rate) {
            value = std::clamp(value + mutation(gen), min, max);
        }
    };

    mutateValue(wingSpan, 0.3f, 3.0f);
    mutateValue(wingChord, 0.05f, 0.5f);
    mutateValue(aspectRatio, 2.0f, 15.0f);
    mutateValue(wingLoading, 5.0f, 100.0f);
    mutateValue(camber, 0.02f, 0.15f);
    mutateValue(wingTaper, 0.2f, 1.0f);
    mutateValue(wingTwist, 0.0f, 10.0f);
    mutateValue(dihedralAngle, -5.0f, 15.0f);
    mutateValue(sweepAngle, -10.0f, 45.0f);

    mutateValue(breastMuscleRatio, 0.1f, 0.4f);
    mutateValue(supracoracoideus, 0.05f, 0.2f);

    mutateValue(flockingStrength, 0.0f, 1.0f);
    mutateValue(aggressionLevel, 0.0f, 1.0f);
    mutateValue(territorialRadius, 0.0f, 50.0f);

    // Mutate colors
    if (dist(gen) < rate) {
        primaryColor.r = std::clamp(primaryColor.r + mutation(gen) * 0.1f, 0.0f, 1.0f);
        primaryColor.g = std::clamp(primaryColor.g + mutation(gen) * 0.1f, 0.0f, 1.0f);
        primaryColor.b = std::clamp(primaryColor.b + mutation(gen) * 0.1f, 0.0f, 1.0f);
    }
}

float FlyingGenome::calculateLiftCoefficient() const {
    // Cl depends on camber and angle of attack
    // Simplified: higher camber = higher Cl
    return 0.5f + camber * 10.0f;
}

float FlyingGenome::calculateDragCoefficient() const {
    // Cd = Cd0 + Cd_induced
    // Lower aspect ratio = more induced drag
    float Cd0 = 0.02f + (1.0f - bodyStreamlining) * 0.03f;
    float k = 1.0f / (3.14159f * aspectRatio * 0.85f);
    float Cl = calculateLiftCoefficient();
    return Cd0 + k * Cl * Cl;
}

float FlyingGenome::calculateStallSpeed(float mass) const {
    // Vstall = sqrt(2 * W / (rho * S * Cl_max))
    float rho = 1.225f;
    float wingArea = wingSpan * wingChord;
    float Cl_max = calculateLiftCoefficient() * 1.5f;
    return std::sqrt(2.0f * mass * 9.8f / (rho * wingArea * Cl_max));
}

float FlyingGenome::calculateMaxSpeed() const {
    // Based on wing loading and streamlining
    return 10.0f + wingLoading * 0.5f + bodyStreamlining * 15.0f;
}

float FlyingGenome::calculateGlideRatio() const {
    // L/D = Cl / Cd
    return calculateLiftCoefficient() / calculateDragCoefficient();
}

// ============================================================================
// FlyingState Implementation
// ============================================================================

FlyingState::FlyingState()
    : position(0.0f)
    , velocity(0.0f)
    , orientation(1.0f, 0.0f, 0.0f, 0.0f)
    , altitude(0.0f)
    , groundClearance(0.0f)
    , airSpeed(0.0f)
    , groundSpeed(0.0f)
    , verticalSpeed(0.0f)
    , angleOfAttack(0.0f)
    , sideslipAngle(0.0f)
    , bankAngle(0.0f)
    , pitchAngle(0.0f)
    , yawRate(0.0f)
    , throttle(0.0f)
    , pitch(0.0f)
    , roll(0.0f)
    , yaw(0.0f)
    , flightEnergy(100.0f)
    , maxFlightEnergy(100.0f)
    , energyRegenRate(5.0f)
    , currentStamina(1.0f)
    , hasNest(false)
    , timeSinceTakeoff(0.0f)
    , timeSinceLastMeal(0.0f)
    , timeInCurrentState(0.0f)
{
}

// ============================================================================
// ThermalInfo Implementation
// ============================================================================

float ThermalInfo::getStrengthAt(const glm::vec3& pos) const {
    if (!isActive) return 0.0f;
    if (pos.y > maxAltitude) return 0.0f;

    glm::vec2 horizontal(pos.x - center.x, pos.z - center.z);
    float dist = glm::length(horizontal);
    if (dist > radius) return 0.0f;

    // Strength falls off from center
    float falloff = 1.0f - (dist / radius) * (dist / radius);
    return strength * falloff;
}

// ============================================================================
// WindInfo Implementation
// ============================================================================

glm::vec3 WindInfo::getWindAt(const glm::vec3& pos, float time) const {
    glm::vec3 wind = direction * speed;

    // Add gusts
    if (gustiness > 0.0f) {
        float gustPhase = time * 0.5f + pos.x * 0.01f;
        float gust = std::sin(gustPhase) * gustiness;
        wind *= (1.0f + gust);
    }

    // Add turbulence
    if (turbulence > 0.0f) {
        float tx = std::sin(time * 2.0f + pos.y * 0.1f) * turbulence;
        float ty = std::cos(time * 1.5f + pos.x * 0.1f) * turbulence * 0.5f;
        float tz = std::sin(time * 1.8f + pos.z * 0.1f) * turbulence;
        wind += glm::vec3(tx, ty, tz);
    }

    return wind;
}

// ============================================================================
// FlyingCreatureController Implementation
// ============================================================================

FlyingCreatureController::FlyingCreatureController()
    : m_subtype(FlyingSubtype::SONGBIRD)
    , m_wingStructure(WingStructure::FEATHERED_ROUNDED)
    , m_specialization(FlightSpecialization::POWERED)
    , m_currentBehavior(AerialBehavior::PERCHED)
    , m_previousBehavior(AerialBehavior::PERCHED)
    , m_currentLift(0.0f)
    , m_currentDrag(0.0f)
    , m_currentThrust(0.0f)
    , m_isStalling(false)
    , m_stallTimer(0.0f)
    , m_wingPhase(0.0f)
    , m_flapFrequency(3.0f)
    , m_tailSpread(0.0f)
    , m_featherSpread(0.0f)
    , m_wingFoldAmount(1.0f)
    , m_hasTarget(false)
    , m_isLanding(false)
    , m_behaviorTimer(0.0f)
    , m_decisionCooldown(0.0f)
{
}

void FlyingCreatureController::initialize(const FlyingGenome& genome, FlyingSubtype subtype) {
    m_genome = genome;
    m_subtype = subtype;
    m_wingStructure = FlyingCreatureFactory::getWingStructure(subtype);
    m_specialization = FlyingCreatureFactory::getSpecialization(subtype);

    // Set initial state
    m_state.maxFlightEnergy = 100.0f * genome.breastMuscleRatio * 4.0f;
    m_state.flightEnergy = m_state.maxFlightEnergy;
    m_flapFrequency = calculateOptimalFlapFrequency();
}

void FlyingCreatureController::update(float deltaTime, const Terrain& terrain,
                                       const std::vector<ThermalInfo>& thermals,
                                       const WindInfo& wind) {
    // Update timers
    m_state.timeInCurrentState += deltaTime;
    m_behaviorTimer += deltaTime;
    if (m_decisionCooldown > 0.0f) {
        m_decisionCooldown -= deltaTime;
    }

    // Update terrain info
    float terrainHeight = terrain.getHeight(m_state.position.x, m_state.position.z);
    m_state.altitude = m_state.position.y;
    m_state.groundClearance = m_state.position.y - terrainHeight;

    // Update behavior state machine
    updateBehavior(deltaTime, terrain);

    // Only do physics if not perched
    if (m_currentBehavior != AerialBehavior::PERCHED) {
        calculateAerodynamics(deltaTime, wind);
        applyGravity(deltaTime);
        applyLift(deltaTime);
        applyDrag(deltaTime);
        applyThrust(deltaTime);
        applyThermalForce(thermals, deltaTime);
        applyWindForce(wind, deltaTime);
        updateOrientation(deltaTime);
        enforceFlightEnvelope(deltaTime, terrain);

        // Update position
        m_state.position += m_state.velocity * deltaTime;
    }

    // Update animation
    updateAnimation(deltaTime);

    // Update energy
    if (m_currentBehavior == AerialBehavior::PERCHED) {
        m_state.flightEnergy = std::min(m_state.maxFlightEnergy,
                                         m_state.flightEnergy + m_state.energyRegenRate * deltaTime);
    }
}

void FlyingCreatureController::calculateAerodynamics(float deltaTime, const WindInfo& wind) {
    // Air-relative velocity
    glm::vec3 airVel = m_state.velocity - wind.getWindAt(m_state.position, m_behaviorTimer);
    m_state.airSpeed = glm::length(airVel);
    m_state.groundSpeed = glm::length(glm::vec2(m_state.velocity.x, m_state.velocity.z));
    m_state.verticalSpeed = m_state.velocity.y;

    // Check for stall
    float stallSpeed = m_genome.calculateStallSpeed(1.0f);
    m_isStalling = m_state.airSpeed < stallSpeed * 0.9f;

    if (m_isStalling) {
        m_stallTimer += deltaTime;
    } else {
        m_stallTimer = 0.0f;
    }

    // Calculate lift
    float rho = 1.225f;
    float wingArea = m_genome.wingSpan * m_genome.wingChord;
    float Cl = m_isStalling ? 0.3f : m_genome.calculateLiftCoefficient();
    m_currentLift = 0.5f * rho * m_state.airSpeed * m_state.airSpeed * wingArea * Cl;

    // Calculate drag
    float Cd = m_genome.calculateDragCoefficient();
    if (m_isStalling) Cd *= 2.0f;
    m_currentDrag = 0.5f * rho * m_state.airSpeed * m_state.airSpeed * wingArea * Cd;
}

void FlyingCreatureController::applyGravity(float deltaTime) {
    m_state.velocity.y -= 9.8f * deltaTime;
}

void FlyingCreatureController::applyLift(float deltaTime) {
    if (m_state.airSpeed < 0.1f) return;

    // Lift acts perpendicular to velocity, mostly upward
    float liftForce = m_currentLift;

    // Reduce lift during stall
    if (m_isStalling) {
        liftForce *= 0.3f;
    }

    // Apply based on bank angle
    float effectiveLift = liftForce * std::cos(m_state.bankAngle);
    m_state.velocity.y += effectiveLift * deltaTime;
}

void FlyingCreatureController::applyDrag(float deltaTime) {
    if (m_state.airSpeed < 0.1f) return;

    glm::vec3 dragDir = -glm::normalize(m_state.velocity);
    float dragMag = m_currentDrag * deltaTime;
    dragMag = std::min(dragMag, m_state.airSpeed * 0.5f);
    m_state.velocity += dragDir * dragMag;
}

void FlyingCreatureController::applyThrust(float deltaTime) {
    if (m_state.throttle <= 0.0f) return;

    // Thrust from flapping
    glm::vec3 forward = m_state.orientation * glm::vec3(1.0f, 0.0f, 0.0f);
    float thrustPower = m_genome.breastMuscleRatio * 50.0f * m_state.throttle;

    // Energy cost
    float energyCost = thrustPower * deltaTime * 0.1f;
    if (m_state.flightEnergy >= energyCost) {
        m_state.velocity += forward * thrustPower * deltaTime;
        m_state.flightEnergy -= energyCost;
        m_currentThrust = thrustPower;
    } else {
        m_currentThrust = 0.0f;
    }
}

void FlyingCreatureController::applyThermalForce(const std::vector<ThermalInfo>& thermals,
                                                   float deltaTime) {
    for (const auto& thermal : thermals) {
        float strength = thermal.getStrengthAt(m_state.position);
        if (strength > 0.0f) {
            m_state.velocity.y += strength * deltaTime;
        }
    }
}

void FlyingCreatureController::applyWindForce(const WindInfo& wind, float deltaTime) {
    glm::vec3 windForce = wind.getWindAt(m_state.position, m_behaviorTimer);
    m_state.velocity += windForce * 0.1f * deltaTime;
}

void FlyingCreatureController::updateOrientation(float deltaTime) {
    // Update bank angle based on roll input
    float targetBank = m_state.roll * glm::radians(60.0f);
    m_state.bankAngle += (targetBank - m_state.bankAngle) * 3.0f * deltaTime;

    // Update pitch based on velocity
    if (m_state.airSpeed > 1.0f) {
        float targetPitch = std::atan2(m_state.velocity.y, m_state.groundSpeed);
        targetPitch = std::clamp(targetPitch, glm::radians(-45.0f), glm::radians(45.0f));
        m_state.pitchAngle += (targetPitch - m_state.pitchAngle) * 2.0f * deltaTime;
    }

    // Update yaw to match velocity direction
    if (m_state.groundSpeed > 0.5f) {
        float targetYaw = std::atan2(m_state.velocity.z, m_state.velocity.x);
        m_state.orientation = glm::angleAxis(targetYaw, glm::vec3(0, 1, 0));
    }
}

void FlyingCreatureController::enforceFlightEnvelope(float deltaTime, const Terrain& terrain) {
    // Speed limits
    float maxSpeed = m_genome.calculateMaxSpeed();
    if (m_state.airSpeed > maxSpeed) {
        m_state.velocity = glm::normalize(m_state.velocity) * maxSpeed;
    }

    // Altitude limits
    float terrainHeight = terrain.getHeight(m_state.position.x, m_state.position.z);
    float minAlt = terrainHeight + 2.0f;
    float maxAlt = terrainHeight + 200.0f;

    if (m_state.position.y < minAlt) {
        m_state.position.y = minAlt;
        m_state.velocity.y = std::max(m_state.velocity.y, 2.0f);
    }

    if (m_state.position.y > maxAlt) {
        m_state.position.y = maxAlt;
        m_state.velocity.y = std::min(m_state.velocity.y, 0.0f);
    }
}

void FlyingCreatureController::updateBehavior(float deltaTime, const Terrain& terrain) {
    executeBehavior(deltaTime, terrain);

    // Check for behavior transitions
    if (m_decisionCooldown <= 0.0f) {
        selectBehavior();
        m_decisionCooldown = 0.5f;
    }
}

void FlyingCreatureController::selectBehavior() {
    // Priority-based behavior selection

    // Immediate danger - evade
    if (!m_state.predatorPositions.empty()) {
        float nearestDist = FLT_MAX;
        for (const auto& pred : m_state.predatorPositions) {
            float dist = glm::length(pred - m_state.position);
            nearestDist = std::min(nearestDist, dist);
        }
        if (nearestDist < 20.0f) {
            transitionBehavior(AerialBehavior::EVADING);
            return;
        }
    }

    // Low energy - land
    if (m_state.flightEnergy < m_state.maxFlightEnergy * 0.1f) {
        if (m_currentBehavior != AerialBehavior::PERCHED) {
            transitionBehavior(AerialBehavior::LANDING);
            return;
        }
    }

    // Hungry - hunt or forage
    if (m_state.timeSinceLastMeal > 30.0f) {
        if (!m_state.preyPositions.empty() && m_genome.aggressionLevel > 0.5f) {
            transitionBehavior(AerialBehavior::HUNTING_SEARCH);
            return;
        }
        if (!m_state.foodPositions.empty()) {
            transitionBehavior(AerialBehavior::FORAGING_SEARCH);
            return;
        }
    }

    // Social - flock
    if (!m_state.nearbyFlock.empty() && m_genome.flockingStrength > 0.5f) {
        transitionBehavior(AerialBehavior::FLOCKING);
        return;
    }

    // Default - cruise or perch
    if (m_currentBehavior == AerialBehavior::PERCHED) {
        if (m_state.flightEnergy > m_state.maxFlightEnergy * 0.8f) {
            transitionBehavior(AerialBehavior::TAKING_OFF);
        }
    } else {
        transitionBehavior(AerialBehavior::CRUISING);
    }
}

void FlyingCreatureController::executeBehavior(float deltaTime, const Terrain& terrain) {
    switch (m_currentBehavior) {
        case AerialBehavior::PERCHED:
            updatePerched(deltaTime, terrain);
            break;
        case AerialBehavior::TAKING_OFF:
            updateTakingOff(deltaTime);
            break;
        case AerialBehavior::CRUISING:
            updateCruising(deltaTime);
            break;
        case AerialBehavior::GLIDING:
            updateGliding(deltaTime);
            break;
        case AerialBehavior::LANDING:
            updateLanding(deltaTime, terrain);
            break;
        case AerialBehavior::HOVERING:
            updateHovering(deltaTime);
            break;
        case AerialBehavior::FLOCKING:
            updateFlocking(deltaTime);
            break;
        case AerialBehavior::EVADING:
            updateEvading(deltaTime);
            break;
        case AerialBehavior::HUNTING_DIVE:
            updateDiving(deltaTime);
            break;
        default:
            updateCruising(deltaTime);
            break;
    }
}

void FlyingCreatureController::transitionBehavior(AerialBehavior newBehavior) {
    if (newBehavior != m_currentBehavior) {
        m_previousBehavior = m_currentBehavior;
        m_currentBehavior = newBehavior;
        m_state.timeInCurrentState = 0.0f;
        m_behaviorTimer = 0.0f;
    }
}

void FlyingCreatureController::updatePerched(float deltaTime, const Terrain& terrain) {
    m_state.velocity = glm::vec3(0.0f);
    m_state.throttle = 0.0f;
    m_wingFoldAmount = 1.0f;

    // Stay on terrain
    float terrainHeight = terrain.getHeight(m_state.position.x, m_state.position.z);
    m_state.position.y = terrainHeight;
}

void FlyingCreatureController::updateTakingOff(float deltaTime) {
    m_state.throttle = 1.0f;
    m_wingFoldAmount = 0.0f;

    // Build speed
    glm::vec3 forward(std::cos(m_state.yawRate), 0.0f, std::sin(m_state.yawRate));
    m_state.velocity += forward * 10.0f * deltaTime;
    m_state.velocity.y += 5.0f * deltaTime;

    // Transition to flying once airborne
    if (m_state.groundClearance > 5.0f && m_state.airSpeed > 5.0f) {
        transitionBehavior(AerialBehavior::CRUISING);
    }
}

void FlyingCreatureController::updateCruising(float deltaTime) {
    m_state.throttle = 0.6f;
    m_wingFoldAmount = 0.0f;

    // Maintain altitude
    float targetAlt = 25.0f;
    if (m_state.altitude < targetAlt) {
        m_state.pitch = 0.3f;
    } else if (m_state.altitude > targetAlt + 10.0f) {
        m_state.pitch = -0.2f;
    } else {
        m_state.pitch = 0.0f;
    }

    // Head toward target if set
    if (m_hasTarget) {
        glm::vec3 toTarget = m_targetPosition - m_state.position;
        float dist = glm::length(toTarget);
        if (dist > 1.0f) {
            toTarget = glm::normalize(toTarget);
            // Calculate turn direction
            glm::vec3 forward = m_state.orientation * glm::vec3(1, 0, 0);
            float cross = forward.x * toTarget.z - forward.z * toTarget.x;
            m_state.roll = std::clamp(cross * 2.0f, -1.0f, 1.0f);
        }
    }
}

void FlyingCreatureController::updateGliding(float deltaTime) {
    m_state.throttle = 0.0f;
    m_wingFoldAmount = 0.0f;
    m_featherSpread = 0.3f;

    // Gradually lose altitude
    if (m_state.groundClearance < 10.0f) {
        transitionBehavior(AerialBehavior::CRUISING);
    }
}

void FlyingCreatureController::updateLanding(float deltaTime, const Terrain& terrain) {
    m_state.throttle = 0.2f;

    // Slow down
    m_state.velocity *= (1.0f - deltaTime);

    // Descend
    m_state.velocity.y = std::max(m_state.velocity.y - 3.0f * deltaTime, -2.0f);

    // Touch down
    float terrainHeight = terrain.getHeight(m_state.position.x, m_state.position.z);
    if (m_state.position.y <= terrainHeight + 0.5f) {
        m_state.position.y = terrainHeight;
        m_state.velocity = glm::vec3(0.0f);
        transitionBehavior(AerialBehavior::PERCHED);
    }
}

void FlyingCreatureController::updateHovering(float deltaTime) {
    m_state.throttle = 1.0f;
    m_flapFrequency = calculateOptimalFlapFrequency() * 2.0f;

    // Counter gravity
    m_state.velocity.y += 9.8f * deltaTime;

    // Dampen horizontal movement
    m_state.velocity.x *= (1.0f - 2.0f * deltaTime);
    m_state.velocity.z *= (1.0f - 2.0f * deltaTime);

    // High energy cost
    m_state.flightEnergy -= 2.0f * deltaTime;
}

void FlyingCreatureController::updateFlocking(float deltaTime) {
    m_state.throttle = 0.5f;

    if (m_state.nearbyFlock.empty()) {
        transitionBehavior(AerialBehavior::CRUISING);
        return;
    }

    // Flocking forces
    glm::vec3 separation = calculateSeparation(3.0f);
    glm::vec3 alignment = calculateAlignment();
    glm::vec3 cohesion = calculateCohesion();

    glm::vec3 flockingForce = separation * 1.5f + alignment * 1.0f + cohesion * 1.0f;
    m_state.velocity += flockingForce * deltaTime;
}

void FlyingCreatureController::updateEvading(float deltaTime) {
    m_state.throttle = 1.0f;

    if (m_state.predatorPositions.empty()) {
        transitionBehavior(AerialBehavior::CRUISING);
        return;
    }

    // Flee from nearest predator
    glm::vec3 fleeDir(0.0f);
    for (const auto& pred : m_state.predatorPositions) {
        glm::vec3 away = m_state.position - pred;
        float dist = glm::length(away);
        if (dist > 0.1f) {
            fleeDir += glm::normalize(away) / (dist * dist);
        }
    }

    if (glm::length(fleeDir) > 0.1f) {
        fleeDir = glm::normalize(fleeDir);
        m_state.velocity += fleeDir * 20.0f * deltaTime;
    }
}

void FlyingCreatureController::updateDiving(float deltaTime) {
    m_state.throttle = 0.0f;
    m_wingFoldAmount = 0.8f;

    // Steep dive
    m_state.velocity.y -= 15.0f * deltaTime;

    // Pull out near ground
    if (m_state.groundClearance < 5.0f) {
        m_state.velocity.y = std::max(m_state.velocity.y, 5.0f);
        transitionBehavior(AerialBehavior::CRUISING);
    }
}

glm::vec3 FlyingCreatureController::calculateSeparation(float radius) {
    glm::vec3 force(0.0f);
    for (const auto& neighbor : m_state.nearbyFlock) {
        glm::vec3 diff = m_state.position - neighbor;
        float dist = glm::length(diff);
        if (dist > 0.1f && dist < radius) {
            force += glm::normalize(diff) / dist;
        }
    }
    return force;
}

glm::vec3 FlyingCreatureController::calculateAlignment() {
    // Would need neighbor velocities - simplified
    return glm::vec3(0.0f);
}

glm::vec3 FlyingCreatureController::calculateCohesion() {
    if (m_state.nearbyFlock.empty()) return glm::vec3(0.0f);

    glm::vec3 center(0.0f);
    for (const auto& neighbor : m_state.nearbyFlock) {
        center += neighbor;
    }
    center /= static_cast<float>(m_state.nearbyFlock.size());

    return glm::normalize(center - m_state.position);
}

void FlyingCreatureController::updateAnimation(float deltaTime) {
    // Update wing phase
    if (m_currentBehavior != AerialBehavior::PERCHED) {
        m_wingPhase += m_flapFrequency * deltaTime;
        while (m_wingPhase >= 1.0f) m_wingPhase -= 1.0f;
    }

    // Adjust flap frequency based on state
    float targetFreq = calculateOptimalFlapFrequency();
    if (m_currentBehavior == AerialBehavior::HOVERING) {
        targetFreq *= 2.0f;
    } else if (m_currentBehavior == AerialBehavior::GLIDING) {
        targetFreq *= 0.1f;
    } else if (m_currentBehavior == AerialBehavior::HUNTING_DIVE) {
        targetFreq = 0.0f;
    }
    m_flapFrequency += (targetFreq - m_flapFrequency) * 3.0f * deltaTime;

    // Update tail spread (more spread during maneuvering)
    float targetTailSpread = std::abs(m_state.roll) + std::abs(m_state.pitch) * 0.5f;
    m_tailSpread += (targetTailSpread - m_tailSpread) * 5.0f * deltaTime;
}

float FlyingCreatureController::calculateOptimalFlapFrequency() const {
    // Smaller creatures flap faster
    // f = k * sqrt(g / wingspan)
    float k = 0.8f;
    return k * std::sqrt(9.8f / m_genome.wingSpan);
}

// Commands
void FlyingCreatureController::setTargetPosition(const glm::vec3& target) {
    m_targetPosition = target;
    m_hasTarget = true;
}

void FlyingCreatureController::setFlockmates(const std::vector<glm::vec3>& positions) {
    m_state.nearbyFlock = positions;
}

void FlyingCreatureController::setPredators(const std::vector<glm::vec3>& positions) {
    m_state.predatorPositions = positions;
}

void FlyingCreatureController::setPrey(const std::vector<glm::vec3>& positions) {
    m_state.preyPositions = positions;
}

void FlyingCreatureController::setFoodSources(const std::vector<glm::vec3>& positions) {
    m_state.foodPositions = positions;
}

void FlyingCreatureController::commandTakeoff() {
    if (m_currentBehavior == AerialBehavior::PERCHED) {
        transitionBehavior(AerialBehavior::TAKING_OFF);
    }
}

void FlyingCreatureController::commandLand(const glm::vec3& landingSpot) {
    m_landingSpot = landingSpot;
    m_isLanding = true;
    transitionBehavior(AerialBehavior::LANDING);
}

void FlyingCreatureController::commandDive(const glm::vec3& target) {
    m_targetPosition = target;
    m_hasTarget = true;
    transitionBehavior(AerialBehavior::HUNTING_DIVE);
}

void FlyingCreatureController::commandFlock() {
    transitionBehavior(AerialBehavior::FLOCKING);
}

void FlyingCreatureController::commandEvade(const glm::vec3& threat) {
    m_state.predatorPositions.push_back(threat);
    transitionBehavior(AerialBehavior::EVADING);
}

void FlyingCreatureController::commandHunt(const glm::vec3& prey) {
    m_state.preyPositions.push_back(prey);
    transitionBehavior(AerialBehavior::HUNTING_SEARCH);
}

void FlyingCreatureController::commandReturn() {
    if (m_state.hasNest) {
        setTargetPosition(m_state.nestPosition);
    }
}

// ============================================================================
// FlyingCreatureFactory Implementation
// ============================================================================

FlyingGenome FlyingCreatureFactory::createSongbird(float size) {
    FlyingGenome g;
    g.wingSpan = size * 8.0f;
    g.wingChord = size * 1.2f;
    g.aspectRatio = 5.5f;
    g.wingLoading = 15.0f;
    g.camber = 0.08f;
    g.bodyStreamlining = 0.6f;
    g.breastMuscleRatio = 0.22f;
    g.flockingStrength = 0.7f;
    g.aggressionLevel = 0.2f;
    g.primaryColor = glm::vec3(0.4f, 0.35f, 0.3f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createRaptor(float size, bool isLarge) {
    FlyingGenome g;
    g.wingSpan = size * (isLarge ? 2.5f : 1.5f);
    g.wingChord = size * 0.35f;
    g.aspectRatio = isLarge ? 7.0f : 5.5f;
    g.wingLoading = isLarge ? 45.0f : 35.0f;
    g.camber = 0.06f;
    g.bodyStreamlining = 0.75f;
    g.breastMuscleRatio = 0.28f;
    g.visualAcuity = 0.95f;
    g.aggressionLevel = 0.85f;
    g.flockingStrength = 0.1f;
    g.territorialRadius = 100.0f;
    g.primaryColor = glm::vec3(0.3f, 0.25f, 0.2f);
    g.secondaryColor = glm::vec3(0.9f, 0.85f, 0.8f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createOwl(float size) {
    FlyingGenome g;
    g.wingSpan = size * 2.2f;
    g.wingChord = size * 0.4f;
    g.aspectRatio = 5.0f;
    g.wingLoading = 25.0f;
    g.camber = 0.1f;
    g.bodyStreamlining = 0.5f;
    g.breastMuscleRatio = 0.24f;
    g.nightVision = 0.95f;
    g.nocturnality = 0.9f;
    g.aggressionLevel = 0.7f;
    g.flockingStrength = 0.0f;
    g.primaryColor = glm::vec3(0.5f, 0.45f, 0.4f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createSeabird(float size) {
    FlyingGenome g;
    g.wingSpan = size * 3.0f;
    g.wingChord = size * 0.2f;
    g.aspectRatio = 12.0f;
    g.wingLoading = 50.0f;
    g.camber = 0.05f;
    g.bodyStreamlining = 0.8f;
    g.breastMuscleRatio = 0.2f;
    g.flockingStrength = 0.4f;
    g.migratoryUrge = 0.8f;
    g.primaryColor = glm::vec3(0.9f, 0.9f, 0.95f);
    g.secondaryColor = glm::vec3(0.2f, 0.2f, 0.25f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createHummingbird(float size) {
    FlyingGenome g;
    g.wingSpan = size * 4.0f;
    g.wingChord = size * 0.8f;
    g.aspectRatio = 4.0f;
    g.wingLoading = 8.0f;
    g.camber = 0.12f;
    g.bodyStreamlining = 0.7f;
    g.breastMuscleRatio = 0.35f;
    g.supracoracoideus = 0.18f;
    g.flockingStrength = 0.0f;
    g.territorialRadius = 5.0f;
    g.aggressionLevel = 0.6f;
    g.primaryColor = glm::vec3(0.1f, 0.6f, 0.3f);
    g.iridescence = 0.8f;
    return g;
}

FlyingGenome FlyingCreatureFactory::createVulture(float size) {
    FlyingGenome g;
    g.wingSpan = size * 2.8f;
    g.wingChord = size * 0.5f;
    g.aspectRatio = 6.0f;
    g.wingLoading = 55.0f;
    g.camber = 0.07f;
    g.bodyStreamlining = 0.5f;
    g.breastMuscleRatio = 0.18f;
    g.flockingStrength = 0.3f;
    g.visualAcuity = 0.9f;
    g.primaryColor = glm::vec3(0.15f, 0.12f, 0.1f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createButterfly(float size) {
    FlyingGenome g;
    g.wingSpan = size * 30.0f;
    g.wingChord = size * 15.0f;
    g.aspectRatio = 2.5f;
    g.wingLoading = 2.0f;
    g.camber = 0.15f;
    g.bodyStreamlining = 0.3f;
    g.breastMuscleRatio = 0.15f;
    g.flockingStrength = 0.2f;
    g.uvVision = 0.9f;
    g.primaryColor = glm::vec3(0.9f, 0.5f, 0.1f);
    g.secondaryColor = glm::vec3(0.1f, 0.1f, 0.1f);
    g.patternComplexity = 0.9f;
    return g;
}

FlyingGenome FlyingCreatureFactory::createDragonfly(float size) {
    FlyingGenome g;
    g.wingSpan = size * 25.0f;
    g.wingChord = size * 4.0f;
    g.aspectRatio = 8.0f;
    g.wingLoading = 3.0f;
    g.camber = 0.05f;
    g.bodyStreamlining = 0.9f;
    g.breastMuscleRatio = 0.3f;
    g.visualAcuity = 0.85f;
    g.aggressionLevel = 0.8f;
    g.flockingStrength = 0.0f;
    g.primaryColor = glm::vec3(0.2f, 0.4f, 0.6f);
    g.iridescence = 0.5f;
    return g;
}

FlyingGenome FlyingCreatureFactory::createBee(float size) {
    FlyingGenome g;
    g.wingSpan = size * 20.0f;
    g.wingChord = size * 8.0f;
    g.aspectRatio = 3.0f;
    g.wingLoading = 5.0f;
    g.camber = 0.1f;
    g.bodyStreamlining = 0.5f;
    g.breastMuscleRatio = 0.25f;
    g.flockingStrength = 0.8f;
    g.uvVision = 0.8f;
    g.primaryColor = glm::vec3(0.9f, 0.7f, 0.0f);
    g.secondaryColor = glm::vec3(0.1f, 0.1f, 0.1f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createBeetle(float size) {
    FlyingGenome g;
    g.wingSpan = size * 15.0f;
    g.wingChord = size * 10.0f;
    g.aspectRatio = 2.0f;
    g.wingLoading = 8.0f;
    g.camber = 0.08f;
    g.bodyStreamlining = 0.3f;
    g.breastMuscleRatio = 0.2f;
    g.flockingStrength = 0.1f;
    g.primaryColor = glm::vec3(0.1f, 0.3f, 0.15f);
    g.iridescence = 0.6f;
    return g;
}

FlyingGenome FlyingCreatureFactory::createMosquito(float size) {
    FlyingGenome g;
    g.wingSpan = size * 40.0f;
    g.wingChord = size * 10.0f;
    g.aspectRatio = 4.0f;
    g.wingLoading = 1.0f;
    g.bodyStreamlining = 0.7f;
    g.breastMuscleRatio = 0.2f;
    g.flockingStrength = 0.6f;
    g.nocturnality = 0.7f;
    g.primaryColor = glm::vec3(0.3f, 0.3f, 0.3f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createLocust(float size) {
    FlyingGenome g;
    g.wingSpan = size * 20.0f;
    g.wingChord = size * 8.0f;
    g.aspectRatio = 3.5f;
    g.wingLoading = 4.0f;
    g.bodyStreamlining = 0.6f;
    g.breastMuscleRatio = 0.25f;
    g.flockingStrength = 0.95f;
    g.migratoryUrge = 0.9f;
    g.primaryColor = glm::vec3(0.5f, 0.45f, 0.2f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createMicrobat(float size) {
    FlyingGenome g;
    g.wingSpan = size * 6.0f;
    g.wingChord = size * 2.0f;
    g.aspectRatio = 5.0f;
    g.wingLoading = 10.0f;
    g.camber = 0.1f;
    g.bodyStreamlining = 0.6f;
    g.breastMuscleRatio = 0.22f;
    g.echolocationStrength = 0.95f;
    g.nightVision = 0.7f;
    g.nocturnality = 1.0f;
    g.flockingStrength = 0.5f;
    g.hasTailFeathers = false;
    g.primaryColor = glm::vec3(0.2f, 0.18f, 0.15f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createFruitBat(float size) {
    FlyingGenome g;
    g.wingSpan = size * 4.0f;
    g.wingChord = size * 1.0f;
    g.aspectRatio = 6.0f;
    g.wingLoading = 20.0f;
    g.camber = 0.08f;
    g.bodyStreamlining = 0.5f;
    g.breastMuscleRatio = 0.2f;
    g.echolocationStrength = 0.3f;
    g.nightVision = 0.9f;
    g.nocturnality = 0.9f;
    g.flockingStrength = 0.6f;
    g.hasTailFeathers = false;
    g.primaryColor = glm::vec3(0.25f, 0.2f, 0.15f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createPterosaur(float size) {
    FlyingGenome g;
    g.wingSpan = size * 3.0f;
    g.wingChord = size * 0.4f;
    g.aspectRatio = 10.0f;
    g.wingLoading = 60.0f;
    g.camber = 0.06f;
    g.bodyStreamlining = 0.75f;
    g.breastMuscleRatio = 0.25f;
    g.aggressionLevel = 0.6f;
    g.flockingStrength = 0.2f;
    g.hasTailFeathers = false;
    g.primaryColor = glm::vec3(0.5f, 0.4f, 0.35f);
    return g;
}

FlyingGenome FlyingCreatureFactory::createDragon(float size, bool isLarge) {
    FlyingGenome g;
    g.wingSpan = size * (isLarge ? 4.0f : 2.5f);
    g.wingChord = size * (isLarge ? 1.0f : 0.6f);
    g.aspectRatio = 5.0f;
    g.wingLoading = isLarge ? 100.0f : 60.0f;
    g.camber = 0.1f;
    g.bodyStreamlining = 0.6f;
    g.breastMuscleRatio = 0.35f;
    g.aggressionLevel = 0.9f;
    g.flockingStrength = 0.0f;
    g.territorialRadius = 200.0f;
    g.hasTailFeathers = false;
    g.primaryColor = glm::vec3(0.6f, 0.1f, 0.1f);
    g.secondaryColor = glm::vec3(0.2f, 0.15f, 0.1f);
    return g;
}

WingStructure FlyingCreatureFactory::getWingStructure(FlyingSubtype subtype) {
    switch (subtype) {
        case FlyingSubtype::RAPTOR_LARGE:
        case FlyingSubtype::RAPTOR_SMALL:
        case FlyingSubtype::VULTURE:
            return WingStructure::FEATHERED_BROAD;
        case FlyingSubtype::SEABIRD:
            return WingStructure::FEATHERED_LONG;
        case FlyingSubtype::OWL:
            return WingStructure::FEATHERED_ROUNDED;
        case FlyingSubtype::HUMMINGBIRD:
        case FlyingSubtype::SONGBIRD:
        case FlyingSubtype::CORVID:
            return WingStructure::FEATHERED_POINTED;
        case FlyingSubtype::MICROBAT:
        case FlyingSubtype::FRUIT_BAT:
        case FlyingSubtype::VAMPIRE_BAT:
            return WingStructure::MEMBRANE_BAT;
        case FlyingSubtype::PTEROSAUR:
            return WingStructure::MEMBRANE_PTEROSAUR;
        case FlyingSubtype::DRAGON_SMALL:
        case FlyingSubtype::DRAGON_LARGE:
            return WingStructure::MEMBRANE_DRAGON;
        case FlyingSubtype::FLY:
        case FlyingSubtype::MOSQUITO:
            return WingStructure::INSECT_DIPTERA;
        case FlyingSubtype::BUTTERFLY:
        case FlyingSubtype::MOTH:
            return WingStructure::INSECT_LEPIDOPTERA;
        case FlyingSubtype::DRAGONFLY:
        case FlyingSubtype::DAMSELFLY:
            return WingStructure::INSECT_ODONATA;
        case FlyingSubtype::BEE:
        case FlyingSubtype::WASP:
            return WingStructure::INSECT_HYMENOPTERA;
        case FlyingSubtype::BEETLE:
            return WingStructure::INSECT_COLEOPTERA;
        default:
            return WingStructure::FEATHERED_ROUNDED;
    }
}

FlightSpecialization FlyingCreatureFactory::getSpecialization(FlyingSubtype subtype) {
    switch (subtype) {
        case FlyingSubtype::VULTURE:
        case FlyingSubtype::SEABIRD:
            return FlightSpecialization::SOARING;
        case FlyingSubtype::HUMMINGBIRD:
            return FlightSpecialization::HOVERING;
        case FlyingSubtype::RAPTOR_LARGE:
        case FlyingSubtype::RAPTOR_SMALL:
            return FlightSpecialization::DIVING;
        case FlyingSubtype::OWL:
            return FlightSpecialization::SILENT;
        case FlyingSubtype::DRAGONFLY:
            return FlightSpecialization::ACROBATIC;
        case FlyingSubtype::LOCUST:
        case FlyingSubtype::SWARM_LOCUST:
            return FlightSpecialization::MIGRATORY;
        default:
            return FlightSpecialization::POWERED;
    }
}

// ============================================================================
// SwarmSystem Implementation
// ============================================================================

SwarmSystem::SwarmSystem()
    : m_swarmType(FlyingSubtype::LOCUST)
    , m_swarmSize(100)
    , m_swarmRadius(20.0f)
    , m_separationWeight(1.5f)
    , m_alignmentWeight(1.0f)
    , m_cohesionWeight(1.0f)
    , m_goalWeight(0.5f)
    , m_noiseAmount(0.2f)
    , m_hasTarget(false)
    , m_maxSpeed(10.0f)
    , m_neighborRadius(5.0f)
    , m_separationRadius(1.5f)
    , m_wingBeatFrequency(20.0f)
{
}

void SwarmSystem::setSwarmType(FlyingSubtype type) {
    m_swarmType = type;

    // Adjust parameters based on type
    switch (type) {
        case FlyingSubtype::LOCUST:
        case FlyingSubtype::SWARM_LOCUST:
            m_maxSpeed = 12.0f;
            m_neighborRadius = 8.0f;
            m_wingBeatFrequency = 25.0f;
            break;
        case FlyingSubtype::MOSQUITO:
        case FlyingSubtype::SWARM_MOSQUITO:
            m_maxSpeed = 5.0f;
            m_neighborRadius = 3.0f;
            m_wingBeatFrequency = 400.0f;
            break;
        case FlyingSubtype::BEE:
            m_maxSpeed = 8.0f;
            m_neighborRadius = 4.0f;
            m_wingBeatFrequency = 200.0f;
            break;
        default:
            m_maxSpeed = 10.0f;
            m_neighborRadius = 5.0f;
            m_wingBeatFrequency = 30.0f;
            break;
    }
}

void SwarmSystem::setSwarmSize(int count) {
    m_swarmSize = count;
    m_positions.resize(count);
    m_velocities.resize(count);
    m_orientations.resize(count);

    // Initialize positions around center
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (int i = 0; i < count; ++i) {
        m_positions[i] = m_swarmCenter + glm::vec3(
            dist(gen) * m_swarmRadius,
            dist(gen) * m_swarmRadius * 0.5f,
            dist(gen) * m_swarmRadius
        );
        m_velocities[i] = glm::vec3(dist(gen), dist(gen) * 0.2f, dist(gen)) * m_maxSpeed * 0.5f;
        m_orientations[i] = glm::quat(1, 0, 0, 0);
    }
}

void SwarmSystem::setSwarmCenter(const glm::vec3& center) {
    m_swarmCenter = center;
}

void SwarmSystem::setSwarmRadius(float radius) {
    m_swarmRadius = radius;
}

void SwarmSystem::update(float deltaTime, const Terrain& terrain) {
    for (int i = 0; i < static_cast<int>(m_positions.size()); ++i) {
        updateIndividual(i, deltaTime);

        // Keep above terrain
        float terrainHeight = terrain.getHeight(m_positions[i].x, m_positions[i].z);
        if (m_positions[i].y < terrainHeight + 2.0f) {
            m_positions[i].y = terrainHeight + 2.0f;
            m_velocities[i].y = std::abs(m_velocities[i].y);
        }
    }
}

void SwarmSystem::updateIndividual(int index, float deltaTime) {
    glm::vec3 force = calculateFlockingForce(index);

    // Add attractors/repellers
    for (const auto& [pos, strength] : m_attractors) {
        glm::vec3 toAttractor = pos - m_positions[index];
        float dist = glm::length(toAttractor);
        if (dist > 0.1f) {
            force += glm::normalize(toAttractor) * strength / (dist + 1.0f);
        }
    }

    for (const auto& [pos, strength] : m_repellers) {
        glm::vec3 away = m_positions[index] - pos;
        float dist = glm::length(away);
        if (dist > 0.1f && dist < strength * 2.0f) {
            force += glm::normalize(away) * strength / (dist * dist + 0.1f);
        }
    }

    // Target seeking
    if (m_hasTarget) {
        glm::vec3 toTarget = m_targetPosition - m_positions[index];
        if (glm::length(toTarget) > 1.0f) {
            force += glm::normalize(toTarget) * m_goalWeight;
        }
    }

    // Add noise
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    force += glm::vec3(dist(gen), dist(gen) * 0.3f, dist(gen)) * m_noiseAmount;

    // Apply force
    m_velocities[index] += force * deltaTime;

    // Limit speed
    float speed = glm::length(m_velocities[index]);
    if (speed > m_maxSpeed) {
        m_velocities[index] = glm::normalize(m_velocities[index]) * m_maxSpeed;
    }

    // Update position
    m_positions[index] += m_velocities[index] * deltaTime;

    // Update orientation
    if (speed > 0.1f) {
        glm::vec3 forward = glm::normalize(m_velocities[index]);
        float yaw = std::atan2(forward.z, forward.x);
        m_orientations[index] = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
    }
}

glm::vec3 SwarmSystem::calculateFlockingForce(int index) {
    glm::vec3 separation(0.0f);
    glm::vec3 alignment(0.0f);
    glm::vec3 cohesion(0.0f);
    int neighborCount = 0;

    for (int j = 0; j < static_cast<int>(m_positions.size()); ++j) {
        if (j == index) continue;

        glm::vec3 diff = m_positions[index] - m_positions[j];
        float dist = glm::length(diff);

        if (dist < m_neighborRadius) {
            // Separation
            if (dist < m_separationRadius && dist > 0.01f) {
                separation += glm::normalize(diff) / dist;
            }

            // Alignment
            alignment += m_velocities[j];

            // Cohesion
            cohesion += m_positions[j];

            neighborCount++;
        }
    }

    glm::vec3 force(0.0f);

    if (neighborCount > 0) {
        // Average alignment
        alignment /= static_cast<float>(neighborCount);
        if (glm::length(alignment) > 0.1f) {
            force += glm::normalize(alignment) * m_alignmentWeight;
        }

        // Average cohesion (steer toward center of neighbors)
        cohesion /= static_cast<float>(neighborCount);
        glm::vec3 toCenter = cohesion - m_positions[index];
        if (glm::length(toCenter) > 0.1f) {
            force += glm::normalize(toCenter) * m_cohesionWeight;
        }
    }

    // Separation is always applied
    force += separation * m_separationWeight;

    return force;
}

void SwarmSystem::setTargetPosition(const glm::vec3& target) {
    m_targetPosition = target;
    m_hasTarget = true;
}

void SwarmSystem::addAttractor(const glm::vec3& pos, float strength) {
    m_attractors.push_back({pos, strength});
}

void SwarmSystem::addRepeller(const glm::vec3& pos, float strength) {
    m_repellers.push_back({pos, strength});
}

void SwarmSystem::clearForces() {
    m_attractors.clear();
    m_repellers.clear();
    m_hasTarget = false;
}

// ============================================================================
// MurmurationSystem Implementation
// ============================================================================

MurmurationSystem::MurmurationSystem()
    : m_flockRadius(50.0f)
    , m_topologicalNeighbors(7)
{
}

void MurmurationSystem::initialize(int birdCount, const glm::vec3& center) {
    m_flockCenter = center;
    m_positions.resize(birdCount);
    m_velocities.resize(birdCount);
    m_phases.resize(birdCount);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-m_flockRadius, m_flockRadius);
    std::uniform_real_distribution<float> velDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> phaseDist(0.0f, 1.0f);

    for (int i = 0; i < birdCount; ++i) {
        m_positions[i] = center + glm::vec3(posDist(gen), posDist(gen) * 0.5f, posDist(gen));
        m_velocities[i] = glm::vec3(velDist(gen), velDist(gen) * 0.2f, velDist(gen));
        m_phases[i] = phaseDist(gen);
    }

    rebuildNeighborCache();
}

void MurmurationSystem::update(float deltaTime, const Terrain& terrain) {
    // Rebuild neighbor cache periodically
    static float rebuildTimer = 0.0f;
    rebuildTimer += deltaTime;
    if (rebuildTimer > 0.5f) {
        rebuildNeighborCache();
        rebuildTimer = 0.0f;
    }

    // Update each bird
    for (int i = 0; i < static_cast<int>(m_positions.size()); ++i) {
        updateBird(i, deltaTime);

        // Terrain avoidance
        float terrainHeight = terrain.getHeight(m_positions[i].x, m_positions[i].z);
        if (m_positions[i].y < terrainHeight + 5.0f) {
            m_positions[i].y = terrainHeight + 5.0f;
            m_velocities[i].y = std::max(m_velocities[i].y, 2.0f);
        }

        // Update wing phase
        m_phases[i] += 3.0f * deltaTime;
        while (m_phases[i] >= 1.0f) m_phases[i] -= 1.0f;
    }

    // Update flock center
    m_flockCenter = glm::vec3(0.0f);
    for (const auto& pos : m_positions) {
        m_flockCenter += pos;
    }
    m_flockCenter /= static_cast<float>(m_positions.size());
}

void MurmurationSystem::rebuildNeighborCache() {
    int n = static_cast<int>(m_positions.size());
    m_neighborCache.resize(n);

    for (int i = 0; i < n; ++i) {
        // Find N nearest neighbors
        std::vector<std::pair<float, int>> distances;
        for (int j = 0; j < n; ++j) {
            if (j != i) {
                float dist = glm::length(m_positions[j] - m_positions[i]);
                distances.push_back({dist, j});
            }
        }

        std::partial_sort(distances.begin(),
                         distances.begin() + std::min(m_topologicalNeighbors, (int)distances.size()),
                         distances.end());

        m_neighborCache[i].clear();
        for (int k = 0; k < std::min(m_topologicalNeighbors, (int)distances.size()); ++k) {
            m_neighborCache[i].push_back(distances[k].second);
        }
    }
}

void MurmurationSystem::updateBird(int index, float deltaTime) {
    glm::vec3 force = calculateMurmurationForce(index);

    // Predator avoidance
    for (const auto& predator : m_predatorThreats) {
        glm::vec3 away = m_positions[index] - predator;
        float dist = glm::length(away);
        if (dist < 30.0f && dist > 0.1f) {
            force += glm::normalize(away) * 50.0f / (dist * dist);
        }
    }

    // Apply force
    m_velocities[index] += force * deltaTime;

    // Speed limits
    float speed = glm::length(m_velocities[index]);
    float minSpeed = 5.0f;
    float maxSpeed = 20.0f;

    if (speed > maxSpeed) {
        m_velocities[index] = glm::normalize(m_velocities[index]) * maxSpeed;
    } else if (speed < minSpeed && speed > 0.1f) {
        m_velocities[index] = glm::normalize(m_velocities[index]) * minSpeed;
    }

    // Update position
    m_positions[index] += m_velocities[index] * deltaTime;
}

glm::vec3 MurmurationSystem::calculateMurmurationForce(int index) {
    glm::vec3 separation(0.0f);
    glm::vec3 alignment(0.0f);
    glm::vec3 cohesion(0.0f);

    const auto& neighbors = m_neighborCache[index];

    for (int j : neighbors) {
        glm::vec3 diff = m_positions[index] - m_positions[j];
        float dist = glm::length(diff);

        // Separation (stronger at close range)
        if (dist > 0.01f && dist < 3.0f) {
            separation += glm::normalize(diff) / (dist * dist);
        }

        // Alignment (match velocity)
        alignment += m_velocities[j];

        // Cohesion (move toward neighbors)
        cohesion += m_positions[j];
    }

    glm::vec3 force(0.0f);

    if (!neighbors.empty()) {
        float n = static_cast<float>(neighbors.size());

        alignment /= n;
        if (glm::length(alignment) > 0.1f) {
            force += (glm::normalize(alignment) * 15.0f - m_velocities[index]) * 0.5f;
        }

        cohesion /= n;
        glm::vec3 toCenter = cohesion - m_positions[index];
        force += toCenter * 0.3f;
    }

    force += separation * 3.0f;

    // Boundary force - keep flock together
    glm::vec3 toFlockCenter = m_flockCenter - m_positions[index];
    float distToCenter = glm::length(toFlockCenter);
    if (distToCenter > m_flockRadius) {
        force += glm::normalize(toFlockCenter) * (distToCenter - m_flockRadius) * 0.5f;
    }

    return force;
}

void MurmurationSystem::addPredatorThreat(const glm::vec3& position) {
    m_predatorThreats.push_back(position);
}

void MurmurationSystem::clearThreats() {
    m_predatorThreats.clear();
}

} // namespace flying
