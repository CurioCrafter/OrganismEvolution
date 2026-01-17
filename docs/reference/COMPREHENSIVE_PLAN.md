# Comprehensive Simulation Enhancement Plan

## Phase 1: Critical Fixes (Immediate) ⚡

### 1.1 Fix Scroll Wheel Bug ✅ DONE
- **Issue:** Creatures offset when scrolling
- **Cause:** Simulation::render() uses hardcoded FOV (45.0f) instead of camera.Zoom
- **Fix:** Changed line 87 to use `camera.Zoom`
- **Status:** Fixed, needs rebuild

### 1.2 Fix Nametags Not Displaying
- **Issue:** Nametags invisible
- **Investigation Needed:**
  - Check if nametag shader is being used
  - Verify nametag VAO/VBO setup
  - Check if nametag text rendering is working
- **Files:** `Simulation.cpp` (nametag rendering section)

### 1.3 Fix Disconnected Body Parts
- **Issue:** Metaball body parts don't connect smoothly
- **Cause:** Metaball strength/radius not tuned correctly
- **Fix:** Adjust metaball influence in CreatureBuilder
- **Files:** `src/graphics/procedural/CreatureBuilder.cpp`

## Phase 2: Creature Type System 🦁🐰

### 2.1 Assign Creature Types
**Goal:** Each creature is either herbivore or carnivore

**Implementation:**
```cpp
// In Creature.h - add member:
CreatureType type;

// In Creature constructor - determine type from genome:
if (genome.speed > 12.0f && genome.size > 1.2f) {
    type = CreatureType::CARNIVORE;
} else {
    type = CreatureType::HERBIVORE;
}
```

### 2.2 Visual Differentiation
**Carnivores (Predators):**
- Red/orange color palette
- Larger jaw/teeth
- Sleeker, longer body
- Forward-facing eyes
- Sharp features (ears, claws)

**Herbivores (Prey):**
- Green/brown/tan colors
- Rounder body
- Side-facing eyes
- Defensive features (spikes for large ones)
- Shorter, sturdier legs

**Files to Modify:**
- `src/graphics/procedural/CreatureBuilder.cpp` - Already has type-based building
- `src/graphics/rendering/CreatureRenderer.cpp` - Pass type to shader
- `shaders/creature_fragment.glsl` - Type-based color palette

### 2.3 Unique Procedural Generation
**Current Problem:** All creatures look similar
**Solution:** More genome-driven variation

**Genome → Appearance Mapping:**
- `genome.size` → Overall scale (0.7 - 2.0x)
- `genome.speed` → Leg length, body aspect ratio
- `genome.efficiency` → Body bulk
- `genome.visionRange` → Eye size
- `genome.neuralWeights[0-7]` → Features (tail, ears, spikes, patterns)

**Random Seed per Creature:**
- Use `creature.id` as seed for procedural generation
- Ensures each creature ID always looks the same
- Creates visual diversity

## Phase 3: Animation System 🎬

### 3.1 Skeletal Animation Approach
**Problem:** Body parts need to move together

**Solution: Simple Bone System**
```cpp
struct Bone {
    glm::vec3 position;
    glm::quat rotation;
    std::vector<Bone*> children;
};

// Creature hierarchy:
// Root (body center)
//   ├─ Head
//   ├─ Tail (chain of 3-5 bones)
//   ├─ Leg FL (2 bones: upper, lower)
//   ├─ Leg FR
//   ├─ Leg BL
//   └─ Leg BR
```

### 3.2 Procedural Walk Animation
**Technique:** Inverse Kinematics (IK) for legs

**Walk Cycle:**
1. Determine step frequency from speed
2. Calculate foot target positions
3. IK solver positions legs to reach targets
4. Bob body up/down
5. Sway tail side-to-side

**Implementation:**
- New file: `src/animation/ProceduralAnimator.h/cpp`
- Update creature vertices each frame based on bone transforms
- Use shader instancing with per-bone matrices

### 3.3 Idle/Eating Animations
- **Idle:** Subtle breathing, head turning
- **Eating:** Head bob down, mouth open
- **Running:** Faster leg cycle, body lean forward

## Phase 4: Predator-Prey Interactions 🎯

### 4.1 Enhanced AI Behaviors

**Herbivore Behaviors:**
1. **Foraging** - Seek food (existing)
2. **Fleeing** - Run from nearby predators
3. **Herding** - Stay near other herbivores
4. **Hiding** - Move to dense vegetation/terrain features

**Carnivore Behaviors:**
1. **Hunting** - Chase nearest herbivore
2. **Stalking** - Slow approach when far
3. **Ambush** - Hide and wait near food/water
4. **Eating** - Consume killed prey

### 4.2 Combat System
**Attack Mechanics:**
```cpp
// In Creature::update()
if (type == CreatureType::CARNIVORE) {
    for (auto* prey : nearbyCreatures) {
        if (prey->type == HERBIVORE && distance < attackRange) {
            prey->takeDamage(attackDamage * deltaTime);
            energy += attackDamage * deltaTime * 0.5f; // Gain energy
        }
    }
}
```

**Defense Mechanics:**
- Large herbivores can fight back
- Small herbivores rely on speed/hiding
- Stamina system for extended chases

### 4.3 Population Dynamics
**Lotka-Volterra Model:**
```
dH/dt = αH - βHP  (Herbivores: grow, eaten by predators)
dP/dt = δβHP - γP (Predators: grow from food, die)
```

**Balance Tuning:**
- Start with 80% herbivores, 20% carnivores
- Monitor populations, adjust spawn rates
- Carnivores need more energy, reproduce slower
- Herbivores reproduce faster, die to predation

## Phase 5: Aquatic Life 🐟

### 5.1 Fish AI
**New Creature Archetype: Fish**
```cpp
enum class CreatureHabitat {
    LAND,
    WATER,
    AMPHIBIOUS
};
```

**Fish Behaviors:**
- **Schooling:** Boids algorithm (alignment, cohesion, separation)
- **Swimming:** Smooth undulating movement
- **Feeding:** Nibble aquatic plants (new food type)
- **Predation:** Larger fish eat smaller fish

### 5.2 Fish Appearance
**Procedural Fish Generation:**
- Streamlined body (elongated ellipsoid)
- Tail fin (procedural triangular mesh)
- Dorsal/pectoral fins
- Scale texture (Voronoi patterns)
- Vibrant colors (blues, greens, oranges)

### 5.3 Water Zones
**Behavior by Depth:**
- **Shallow (<2m):** Small fish, minnows
- **Medium (2-5m):** Most fish, plants
- **Deep (>5m):** Large predator fish

### 5.4 Amphibious Creatures
**Frogs/Salamanders:**
- Can exist in water or land
- Prefer staying near water edge
- Eat insects (new tiny food type)
- Unique hopping animation

## Phase 6: Expanded Ecosystem 🌍

### 6.1 More Food Types
1. **Plants (existing)** - Green spheres on land
2. **Aquatic Plants** - Seaweed/algae in water
3. **Insects** - Tiny flying dots, fast moving
4. **Carcasses** - Dead creatures, scavenger food
5. **Fruit** - Grows on trees (future)

### 6.2 Environmental Features
**Dens/Nests:**
- Creatures can claim territory
- Breeding grounds
- Safe zones from predators

**Migration Paths:**
- Seasonal movement patterns
- Follow food availability
- Water sources attract creatures

### 6.3 Time of Day
**Day/Night Cycle:**
- **Day:** Herbivores active, feed openly
- **Night:** Nocturnal predators hunt, herbivores hide
- **Dawn/Dusk:** Transition periods, high activity

**Lighting Changes:**
- Warm morning/evening light
- Cool midday light
- Blue moonlight at night

## Phase 7: Advanced Features 🚀

### 7.1 Creature Abilities
**Special Traits from Evolution:**
- **Camouflage:** Blend with terrain color
- **Speed Burst:** Sprint ability with stamina cost
- **Pack Hunting:** Coordinated carnivore attacks
- **Hibernation:** Survive lean seasons

### 7.2 Environmental Hazards
- **Weather:** Rain reduces visibility, slows movement
- **Drought:** Water areas shrink, food scarce
- **Fire:** Spreads across terrain, creatures flee
- **Disease:** Spreads through population

### 7.3 Evolution Tracking
**Phylogenetic Tree:**
- Track lineages from generation 1
- Visualize species branching
- Show evolutionary adaptations over time

**Statistics Dashboard:**
- Population graphs over time
- Average traits per generation
- Predator/prey ratio
- Extinction events

## Implementation Priority 📋

### Sprint 1 (Next 100 tasks)
1. ✅ Fix scroll wheel bug
2. Fix nametags
3. Fix disconnected body parts
4. Add creature type assignment
5. Implement type-based colors
6. Add more visual variety to creatures
7. Create simple walk animation
8. Implement predator-prey chase
9. Add hunting/fleeing behaviors
10. Test population balance

### Sprint 2
11. Add fish creatures
12. Implement schooling behavior
13. Add aquatic plants
14. Create swimming animation
15. Add amphibious creatures
16. Implement different food types
17. Add day/night cycle
18. Create time-based behaviors
19. Add carcass food source
20. Implement scavenger behavior

### Sprint 3
21. Advanced animations (eating, idle)
22. Skeletal animation system
23. IK for legs
24. Territory/nesting system
25. Migration patterns
26. Weather effects
27. Seasonal changes
28. Evolution visualization
29. Statistics tracking
30. Performance optimization

## Technical Architecture 🔧

### New Systems Needed:

**1. BehaviorTree System**
```cpp
class BehaviorNode {
    virtual Status execute(Creature* creature, float dt) = 0;
};

// Behaviors: Sequence, Selector, Flee, Hunt, Eat, Wander
```

**2. Animation System**
```cpp
class AnimationController {
    void updateBones(float deltaTime);
    void applyAnimation(Mesh& mesh);
};
```

**3. Combat System**
```cpp
class CombatManager {
    void processAttacks(std::vector<Creature*>& creatures);
    void handleDamage(Creature* attacker, Creature* target);
};
```

**4. Population Manager**
```cpp
class PopulationManager {
    void balancePopulations();
    void spawnByType(CreatureType type);
    void enforceCarryingCapacity();
};
```

## Success Metrics 🎯

**Biological Realism:**
- Stable predator-prey oscillation
- Evolutionary adaptation visible
- Ecosystem self-regulation

**Visual Quality:**
- Distinct creature appearances
- Smooth animations
- Believable behaviors

**Performance:**
- 60 FPS with 500+ creatures
- Smooth sailing with fish schools
- No stuttering during complex interactions

## Testing Plan ✅

1. **Unit Tests:** Individual behavior nodes
2. **Integration Tests:** Predator-prey balance
3. **Visual Tests:** Animation smoothness
4. **Performance Tests:** Stress test with 1000 creatures
5. **Balance Tests:** Long-term ecosystem stability

---

**Total Estimated Tasks:** 200+
**Development Time:** Ongoing iterative development
**Current Status:** Phase 1 in progress (fixes)
**Next Immediate Goal:** Get basic predator-prey working
