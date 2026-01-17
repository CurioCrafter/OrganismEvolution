# Phase 8, Agent 10: Procedural Animation, Rigging, and Creature Activities

## Overview

This document details the implementation of the procedural animation system, morphology-based rig generation, and creature activity system. These systems work together to provide visible, believable creature behaviors including eating, mating, sleeping, excreting, grooming, and various displays.

---

## Files Created/Modified

### New Files

| File | Purpose |
|------|---------|
| `src/animation/ActivitySystem.h` | Activity state machine, triggers, and event system |
| `src/animation/ActivitySystem.cpp` | Activity state machine implementation |
| `src/animation/ProceduralRig.h` | Morphology-based rig generation |
| `src/animation/ProceduralRig.cpp` | Rig generation implementation |
| `src/animation/ActivityAnimations.h` | Motion clips and secondary motion system |
| `src/animation/ActivityAnimations.cpp` | Procedural animation generation |

### Modified Files

| File | Changes |
|------|---------|
| `src/entities/Creature.h` | Added activity system integration, physiological state |
| `src/entities/Creature.cpp` | Activity update loop, physiological simulation |

---

## 1. Rig Generation System

### Rig Categories

The system automatically categorizes creatures based on morphology:

| Category | Criteria |
|----------|----------|
| `BIPED` | 2 legs |
| `QUADRUPED` | 4 legs |
| `HEXAPOD` | 6 legs |
| `OCTOPOD` | 8+ legs or tentacles |
| `SERPENTINE` | No legs, many segments |
| `AQUATIC` | Fins, no legs |
| `FLYING` | Can fly with wings |
| `RADIAL` | Radial symmetry |

### Bone Structure

The rig generator creates bones based on morphology genes:

```cpp
struct RigDefinition {
    SpineDefinition spine;      // Pelvis to head
    TailDefinition tail;        // Tail chain
    HeadDefinition head;        // Head, jaw, eyes, features
    std::vector<LimbDefinition> limbs;  // Legs, arms, wings, fins
    int totalBones;             // Max 64 for GPU skinning
};
```

### Bone Counts by Category

| Category | Spine | Tail | Head | Limbs | Features | Total Range |
|----------|-------|------|------|-------|----------|-------------|
| Biped | 4-8 | 0-6 | 2-4 | 6-10 | 0-4 | 12-32 |
| Quadruped | 5-10 | 4-10 | 2-4 | 12-16 | 0-4 | 23-44 |
| Hexapod | 3-5 | 0-2 | 2-4 | 12-18 | 2-4 | 19-33 |
| Serpentine | 12-30 | 0 | 2-3 | 0 | 0-2 | 14-35 |
| Aquatic | 5-10 | 4-8 | 2-3 | 4-8 | 0-2 | 15-31 |
| Flying | 4-6 | 3-6 | 2-3 | 8-12 | 0-2 | 17-29 |

### LOD System

Three LOD levels reduce bone counts for distant creatures:

- **LOD 0**: Full bone count
- **LOD 1**: ~66% of bones (reduce segments)
- **LOD 2**: ~50% of bones (minimal features)

---

## 2. Activity State Machine

### Activity Types

```cpp
enum class ActivityType {
    IDLE,               // Default resting state
    EATING,             // Consuming food
    DRINKING,           // Consuming water
    MATING,             // Reproduction behavior
    SLEEPING,           // Rest/sleep state
    EXCRETING,          // Peeing/pooping
    GROOMING,           // Self-cleaning
    THREAT_DISPLAY,     // Warning display
    SUBMISSIVE_DISPLAY, // Submissive posture
    MATING_DISPLAY,     // Courtship display
    NEST_BUILDING,      // Building shelter
    PLAYING,            // Playful behavior
    INVESTIGATING,      // Examining something
    CALLING,            // Vocalizing
    PARENTAL_CARE       // Caring for young (feeding, protecting)
};
```

### Activity Sub-Types

#### Excretion Types
- `URINATE` - Bladder emptying
- `DEFECATE` - Bowel movement

#### Grooming Types
- `SCRATCH` - Scratching body parts
- `LICK` - Self-licking
- `SHAKE` - Body shake (like wet dog)
- `PREEN` - Wing preening (flying creatures)
- `STRETCH` - Full body stretch

#### Display Types
- `CREST_RAISE` - Raise crest/frill
- `WING_SPREAD` - Spread wings
- `TAIL_FAN` - Fan out tail
- `BODY_INFLATE` - Puff up body
- `COLOR_FLASH` - Bioluminescence flash
- `VOCALIZE` - Make sounds

### Activity Triggers

The system monitors creature state to trigger activities:

```cpp
struct ActivityTriggers {
    float hungerLevel;          // 0-1, triggers EATING
    float thirstLevel;          // 0-1, triggers DRINKING
    float energyLevel;          // 0-1, affects all activities
    float reproductionUrge;     // 0-1, triggers MATING/MATING_DISPLAY
    bool potentialMateNearby;   // Required for mating
    float bladderFullness;      // 0-1, triggers EXCRETING (urinate)
    float bowelFullness;        // 0-1, triggers EXCRETING (defecate)
    float dirtyLevel;           // 0-1, triggers GROOMING
    float fatigueLevel;         // 0-1, triggers SLEEPING
    float threatLevel;          // 0-1, triggers THREAT/SUBMISSIVE_DISPLAY
    bool territoryIntruder;     // Triggers THREAT_DISPLAY
    bool foodNearby;            // Required for EATING
    bool isJuvenile;            // Enables PLAYING
    float playUrge;             // 0-1, triggers PLAYING
    bool hasOffspringNearby;    // Required for PARENTAL_CARE
    float offspringHungerLevel; // 0-1, triggers feeding behavior
    float parentalUrge;         // 0-1, triggers PARENTAL_CARE
};
```

### Activity Configuration

Each activity has configurable parameters:

| Activity | Duration | Cooldown | Priority | Can Interrupt |
|----------|----------|----------|----------|---------------|
| IDLE | 1-10s | 0s | 0 | Yes |
| EATING | 2-8s | 5s | 7 | Yes |
| DRINKING | 1.5-4s | 10s | 6 | Yes |
| MATING | 3-10s | 30s | 8 | Yes |
| SLEEPING | 10-60s | 120s | 3 | Yes |
| EXCRETING | 1-3s | 20s | 5 | No |
| GROOMING | 2-8s | 15s | 2 | Yes |
| THREAT_DISPLAY | 1.5-5s | 10s | 9 | Yes |
| SUBMISSIVE | 2-6s | 5s | 8 | Yes |
| MATING_DISPLAY | 3-12s | 20s | 6 | Yes |
| NEST_BUILDING | 5-20s | 60s | 4 | Yes |
| PLAYING | 5-20s | 30s | 2 | Yes |
| INVESTIGATING | 2-8s | 5s | 3 | Yes |
| CALLING | 1-4s | 15s | 4 | Yes |
| PARENTAL_CARE | 3-15s | 10s | 7 | Yes |

### Activity Scoring

Activities are selected based on trigger values and thresholds:

```cpp
// Example scoring for EATING
score = foodNearby ? (hungerLevel * 1.2f) : 0.0f;

// Example scoring for SLEEPING
score = fatigueLevel * (1.0f - threatLevel);

// Example scoring for GROOMING
score = dirtyLevel * (1.0f - threatLevel) * (1.0f - hungerLevel * 0.5f);
```

---

## 3. Animation System

### Motion Clip Structure

```cpp
struct ActivityMotionClip {
    std::string name;
    ActivityType activityType;
    float duration;
    bool isLooping;
    std::vector<BoneChannel> boneChannels;  // Per-bone keyframes
    bool hasRootMotion;
    bool isAdditive;
};
```

### Secondary Motion Layer

Provides procedural overlays for organic movement:

| Motion | Description | Parameters |
|--------|-------------|------------|
| Breathing | Chest expansion cycle | Rate: 0.3 Hz, Amplitude: 0.02 |
| Tail Wag | Natural tail movement | Speed: 2 Hz, Amplitude: 0.3 rad |
| Head Bob | Subtle head movement | Speed: 1 Hz, Amplitude: 0.01 |
| Eye Blink | Periodic blinking | Rate: 0.15 Hz, Duration: 0.1s |
| Body Sway | Idle swaying | Speed: 0.3 Hz, Amplitude: 0.005 |
| Ear Twitch | Random ear movement | Rate: 0.2 Hz, Amplitude: 0.15 rad |

### Activity-Specific Animations

#### EATING Animation
- Head bobs downward (pecking/chewing motion)
- Body lowers slightly
- Frequency: 4 Hz bobbing
- Look-at target: food position

#### SLEEPING Animation
- Body lowers to resting position
- Very slow breathing (0.5x normal)
- Minimal secondary motion
- Head tucks slightly

#### EXCRETING Animation
- Squat position (body lowers 10-12%)
- Hold posture for duration
- Optional leg raise for urination
- Strain motion for defecation

#### GROOMING Animations

| Type | Motion |
|------|--------|
| SCRATCH | Body tilts, rapid movement |
| LICK | Head turns to body |
| SHAKE | Rapid lateral oscillation |
| PREEN | Head reaches to wings |
| STRETCH | Body elongates, back arches |

#### THREAT_DISPLAY Animation
- Body rises up (15%)
- Head tilts forward aggressively
- Puffing motion (chest expansion)
- Wings/crest spread if applicable

#### MATING_DISPLAY Animation
- Body bobbing motion
- Tail fanning (if applicable)
- Wing spreading (if applicable)
- Looking at mate position

#### NEST_BUILDING Animation
- Alternating gather/place cycle (3 cycles per activity)
- Gather phase: Head down, picking up materials
- Place phase: Head up, arranging materials with weaving motion
- Occasional pause to inspect work (at 70-80% progress)

#### PARENTAL_CARE Animation
- Gentle protective posture
- Body lowers to interact with young
- Soft bobbing motion during active care phase
- Head movements simulating feeding/grooming young
- Looking toward offspring position

---

## 4. Integration with Agent 8 (Inspection UI)

### Exposed Activity Data

The creature exposes activity state for UI inspection:

```cpp
// Get current activity
animation::ActivityType getCurrentActivity() const;
std::string getCurrentActivityName() const;
float getActivityProgress() const;
bool isPerformingActivity() const;

// Get trigger values
const animation::ActivityTriggers& getActivityTriggers() const;

// Get physiological state
float getHungerLevel() const;
float getFatigueLevel() const;
float getBladderFullness() const;
float getBowelFullness() const;
float getDirtyLevel() const;
float getReproductionUrge() const;

// Parental care state
bool hasNearbyOffspring() const;
float getParentalUrge() const;
float getOffspringHungerLevel() const;
```

### Event Callbacks

UI can register for activity transition events:

```cpp
struct ActivityEvent {
    ActivityType activity;
    ActivityEventType eventType;  // STARTED, COMPLETED, INTERRUPTED, FAILED
    float timestamp;
    uint32_t creatureId;
    glm::vec3 position;
};

m_activitySystem.registerEventCallback([](const ActivityEvent& event) {
    // Log or display activity transitions
});
```

---

## 5. Physiological Simulation

### State Variables

| Variable | Range | Update Rate | Affected By |
|----------|-------|-------------|-------------|
| `fatigueLevel` | 0-1 | +0.005/s active, -0.1/s sleeping | Movement, activities |
| `bladderFullness` | 0-1 | +0.01/s after meals | Eating, excreting |
| `bowelFullness` | 0-1 | +0.005/s after meals | Eating, excreting |
| `dirtyLevel` | 0-1 | +0.002/s always | Time, grooming |
| `hungerLevel` | 0-1 | Derived from energy | Eating |

### Digestion Timeline

1. **0-5s after meal**: No change
2. **5-60s after meal**: Bladder/bowel filling
3. **At threshold**: Triggers EXCRETING activity
4. **After excretion**: Levels reset

---

## 6. Motion Parameter Ranges

### Body Offsets

| Activity | Y Offset | Z Offset | Notes |
|----------|----------|----------|-------|
| EATING | -0.05 | - | Lowered |
| DRINKING | -0.10 | - | Head down |
| SLEEPING | -0.15 | - | Resting |
| EXCRETING | -0.10 to -0.12 | - | Squatting |
| THREAT | +0.15 | - | Rising up |
| SUBMISSIVE | -0.20 | - | Crouching |
| PLAYING | +0.08 (bouncing) | - | Bouncy |

### Rotation Ranges (radians)

| Activity | Pitch | Yaw | Roll |
|----------|-------|-----|------|
| Eating head bob | ±0.15 | - | - |
| Drinking lap | 0.3-0.5 | - | - |
| Threat display | -0.15 | - | - |
| Submissive | +0.30 | - | - |
| Shake grooming | - | - | ±0.20 |

---

## 7. Usage Example

```cpp
// In creature update loop
void Creature::update(float deltaTime, ...) {
    // ... existing updates ...

    // Update physiological state
    updatePhysiologicalState(deltaTime);

    // Update activity system
    updateActivitySystem(deltaTime, foodPositions, otherCreatures);

    // Update animation
    updateAnimation(deltaTime);
}

// Query current activity for UI
std::string activityName = creature->getCurrentActivityName();
float progress = creature->getActivityProgress();
bool isActive = creature->isPerformingActivity();

// Get detailed trigger information
const auto& triggers = creature->getActivityTriggers();
float hunger = triggers.hungerLevel;
float fatigue = triggers.fatigueLevel;
```

---

## 8. Performance Considerations

### Bone Count Limits
- Maximum 64 bones per skeleton (GPU uniform limit)
- LOD system reduces bones for distant creatures
- Feature bones (horns, antennae) removed at LOD 2

### Update Frequency
- Activity state machine: Every frame
- Physiological state: Every frame
- Motion clip sampling: Every frame
- Secondary motion: Every frame

### Memory Usage
- `ActivityStateMachine`: ~200 bytes per creature
- `ActivityAnimationDriver`: ~100 bytes per creature
- `SecondaryMotionLayer`: ~150 bytes per creature
- Motion clips: Shared across creatures of same morphology

---

## 9. Future Extensions

- [ ] Add swimming-specific activities (surface breathing, diving)
- [ ] Add flying-specific activities (preening, dust bathing)
- [ ] Add social activities (grooming others, play fighting)
- [x] Add nest building with actual nest objects (NEST_BUILDING activity implemented)
- [ ] Add territorial marking (visual scent markers)
- [ ] Add death animation and carcass interaction
- [x] Add parental care behavior (PARENTAL_CARE activity implemented)

---

## Version History

- **Phase 8, Agent 10 (Final)** - Complete implementation with all activities
  - Added PARENTAL_CARE activity for caring for young
  - Enhanced NEST_BUILDING with proper animation
  - All 15 activity types fully functional
  - Full UI integration for Agent 8 inspection panel
