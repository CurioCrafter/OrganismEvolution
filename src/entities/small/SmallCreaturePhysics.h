#pragma once

#include "SmallCreatures.h"
#include <DirectXMath.h>

class Terrain;

namespace small {

using namespace DirectX;

// Physics constants for different scales
struct PhysicsConstants {
    // Gravity and air resistance
    static constexpr float GRAVITY = 9.81f;
    static constexpr float AIR_DENSITY = 1.225f;        // kg/m^3

    // Surface tension (for water surface walking)
    static constexpr float SURFACE_TENSION = 0.072f;    // N/m for water

    // Friction coefficients
    static constexpr float FRICTION_GROUND = 0.6f;
    static constexpr float FRICTION_GRASS = 0.4f;
    static constexpr float FRICTION_BARK = 0.7f;
    static constexpr float FRICTION_LEAF = 0.3f;

    // Jump multipliers (relative to body length)
    static constexpr float FROG_JUMP_MULT = 20.0f;      // Frogs jump 20x body length
    static constexpr float GRASSHOPPER_JUMP_MULT = 30.0f;
    static constexpr float SPIDER_JUMP_MULT = 50.0f;    // Jumping spiders
    static constexpr float FLEA_JUMP_MULT = 150.0f;     // (If we had fleas)

    // Climbing parameters
    static constexpr float MIN_CLIMB_ANGLE = 0.7f;      // ~45 degrees
    static constexpr float MAX_CLIMB_SPEED_MULT = 0.5f; // Climbing is slower

    // Burrowing
    static constexpr float BURROW_SPEED_MULT = 0.2f;
    static constexpr float SOIL_RESISTANCE = 50.0f;
};

// Surface type for physics calculations
enum class SurfaceType {
    GROUND,
    GRASS,
    WATER_SURFACE,
    UNDERWATER,
    TREE_BARK,
    LEAF,
    AIR
};

// Movement state
struct MovementState {
    SurfaceType currentSurface;
    bool isGrounded;
    bool isClimbing;
    bool isSwimming;
    bool isBurrowing;
    bool isJumping;
    bool isGliding;
    float climbAngle;           // Angle of surface being climbed
    XMFLOAT3 surfaceNormal;     // Normal of current surface
    float groundHeight;         // Height of ground below
    float waterLevel;           // Height of water surface (if any)
};

// Main physics system for small creatures
class SmallCreaturePhysics {
public:
    // Update a single creature's physics
    static void update(SmallCreature& creature, float deltaTime,
                       Terrain* terrain, MicroSpatialGrid& grid);

    // Get movement state for a creature at position
    static MovementState getMovementState(const XMFLOAT3& position, float size,
                                          Terrain* terrain);

    // Surface tension check - can this creature walk on water?
    static bool canWalkOnWater(const SmallCreature& creature);

    // Jumping calculations
    static XMFLOAT3 calculateJump(const SmallCreature& creature,
                                  const XMFLOAT3& targetPos,
                                  float jumpStrength);

    // Climbing calculations
    static bool canClimbSurface(const SmallCreature& creature,
                                float surfaceAngle, SurfaceType surface);
    static XMFLOAT3 calculateClimbVelocity(const SmallCreature& creature,
                                           const XMFLOAT3& targetPos,
                                           const XMFLOAT3& surfaceNormal);

    // Burrowing
    static bool canBurrow(const SmallCreature& creature, Terrain* terrain);
    static XMFLOAT3 calculateBurrowVelocity(const SmallCreature& creature,
                                            const XMFLOAT3& targetPos);

    // Flying insects
    static XMFLOAT3 calculateFlyingVelocity(const SmallCreature& creature,
                                            const XMFLOAT3& targetPos,
                                            float deltaTime);

    // Swimming
    static XMFLOAT3 calculateSwimmingVelocity(const SmallCreature& creature,
                                              const XMFLOAT3& targetPos,
                                              float waterLevel);

    // Collision with vegetation/objects
    static bool checkVegetationCollision(const XMFLOAT3& position, float size);

private:
    // Apply drag based on size and velocity
    static XMFLOAT3 applyDrag(const XMFLOAT3& velocity, float size,
                               SurfaceType surface);

    // Calculate maximum speed based on creature properties
    static float getMaxSpeed(const SmallCreature& creature, SurfaceType surface);

    // Apply steering towards target
    static XMFLOAT3 steerTowards(const XMFLOAT3& currentVel,
                                 const XMFLOAT3& targetDir,
                                 float maxSpeed, float steerStrength);
};

// Specialized physics for different movement modes
class InsectFlight {
public:
    // Hovering (bees, flies)
    static XMFLOAT3 calculateHover(const SmallCreature& creature,
                                   float targetHeight, float deltaTime);

    // Darting flight (dragonflies)
    static XMFLOAT3 calculateDart(const SmallCreature& creature,
                                  const XMFLOAT3& targetPos);

    // Erratic flight (flies, gnats)
    static XMFLOAT3 calculateErratic(const SmallCreature& creature,
                                     float deltaTime, std::mt19937& rng);

    // Gliding (butterflies)
    static XMFLOAT3 calculateGlide(const SmallCreature& creature,
                                   const XMFLOAT3& windDirection);

    // Swarm cohesion
    static XMFLOAT3 calculateSwarmForce(const SmallCreature& creature,
                                        const std::vector<SmallCreature*>& neighbors);
};

class ArachnidMovement {
public:
    // Web traversal
    static XMFLOAT3 calculateWebMovement(const SmallCreature& creature,
                                         const XMFLOAT3& webCenter,
                                         float webRadius);

    // Rappelling (dropping on silk)
    static XMFLOAT3 calculateRappel(const SmallCreature& creature,
                                    float targetHeight);

    // Pouncing (jumping spiders)
    static XMFLOAT3 calculatePounce(const SmallCreature& creature,
                                    const XMFLOAT3& targetPos);
};

class BurrowingMovement {
public:
    // Tunnel creation (moles, earthworms)
    static XMFLOAT3 calculateTunneling(const SmallCreature& creature,
                                       const XMFLOAT3& targetPos,
                                       float soilHardness);

    // Surface emergence
    static XMFLOAT3 calculateEmergence(const SmallCreature& creature,
                                       float groundLevel);
};

} // namespace small
