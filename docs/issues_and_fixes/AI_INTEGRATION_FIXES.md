# AI Neural Network Integration Fixes

This document lists critical issues identified through code review and their fixes.

## Issue 1: Eligibility Trace Explosion (CRITICAL)

**File:** `src/ai/NeuralNetwork.cpp`
**Function:** `accumulateHebbian()`
**Problem:** Eligibility traces grow unbounded, causing learning instability.

**Current code (problematic):**
```cpp
conn.eligibility = 0.95f * conn.eligibility + correlation;
```

**Fixed code:**
```cpp
// Clamp activities to prevent explosion
float preActivity = conn.recurrent ? fromNode.prevValue : fromNode.value;
preActivity = std::max(-1.0f, std::min(1.0f, preActivity));
float postActivity = std::max(-1.0f, std::min(1.0f, toNode.value));
float correlation = preActivity * postActivity;

// Use replacing trace with bounded accumulation
conn.eligibility = 0.9f * conn.eligibility + 0.1f * correlation;
conn.eligibility = std::max(-1.0f, std::min(1.0f, conn.eligibility));
```

---

## Issue 2: Missing Eligibility Decay for Modular Brain (CRITICAL)

**File:** `src/ai/BrainModules.cpp`
**Function:** `CreatureBrain::learn()`
**Problem:** NEAT path decays eligibility, modular path does not.

**Current code:**
```cpp
if (m_useNEATGenome && m_neatNetwork) {
    m_neatNetwork->updatePlasticity(effectiveReward, learningRate);
    m_neatNetwork->decayEligibility(0.95f);  // Decay present
} else {
    m_decision.updatePlasticity(effectiveReward, learningRate);
    // NO DECAY - BUG!
}
```

**Fixed code:**
```cpp
if (m_useNEATGenome && m_neatNetwork) {
    m_neatNetwork->updatePlasticity(effectiveReward, learningRate);
    m_neatNetwork->decayEligibility(0.95f);
} else {
    m_decision.updatePlasticity(effectiveReward, learningRate);
    // Add decay for decision maker network
    for (auto& conn : m_decision.getNetwork().getConnections()) {
        conn.eligibility *= 0.95f;
    }
}
```

---

## Issue 3: NEAT Crossover Inherits Wrong Nodes (IMPORTANT)

**File:** `src/ai/NEATGenome.cpp`
**Function:** `NEATGenome::crossover()`
**Problem:** Unique hidden nodes from less fit parent are incorrectly inherited.

**Current code (violates NEAT spec):**
```cpp
// Add any unique hidden nodes from other parent
for (const auto& node : other.m_nodes) {
    if (node.type == NodeType::HIDDEN && childNodeIds.find(node.id) == childNodeIds.end()) {
        child.m_nodes.push_back(node);  // WRONG - should not inherit
    }
}
```

**Fixed code:**
```cpp
// Per NEAT specification: Only inherit disjoint/excess genes from the FITTER parent
// Do NOT add unique nodes from the less fit parent
// The fitter parent's nodes are already copied at line 299
// Simply remove this entire for loop (lines 311-316)
```

---

## Issue 4: Weight Mutation Probability Bug (IMPORTANT)

**File:** `src/ai/NEATGenome.cpp`
**Function:** `mutateWeights()`
**Problem:** Using `else if` with new random draw causes incorrect probability.

**Current code:**
```cpp
if (prob(rng) < perturbChance) {
    // perturb
} else if (prob(rng) < replaceChance) {  // NEW random draw - WRONG
    // replace
}
```

**Fixed code:**
```cpp
float r = prob(rng);  // Single random draw
if (r < perturbChance) {
    // perturb
    conn.weight += perturbDist(rng);
    conn.weight = std::max(-5.0f, std::min(5.0f, conn.weight));
} else if (r < perturbChance + replaceChance) {  // Same random value
    // replace
    conn.weight = weightDist(rng);
}
// else: no mutation (probability = 1 - perturbChance - replaceChance)
```

---

## Issue 5: Name Collision - Two NeuralNetwork Classes (INTEGRATION)

**Problem:** Two unrelated `NeuralNetwork` classes exist:
- `src/entities/NeuralNetwork.h` (legacy, 4-input simple network)
- `src/ai/NeuralNetwork.h` (new NEAT-compatible network)

**Solution options:**

1. **Use namespaces explicitly:**
   ```cpp
   // In Creature.h
   std::unique_ptr<ai::NeuralNetwork> neatBrain;  // New AI
   std::unique_ptr<::NeuralNetwork> legacyBrain;  // Old (global namespace)
   ```

2. **Rename legacy class:**
   ```cpp
   // Rename src/entities/NeuralNetwork.h to LegacyNeuralNetwork.h
   class LegacyNeuralNetwork { ... };
   ```

3. **Migrate fully to new system** (recommended for new features)

---

## Issue 6: SensorySystem vs ai::SensoryInput Type Mismatch (INTEGRATION)

**Problem:** `Creature` uses `SensorySystem` but `CreatureBrainInterface` expects `ai::SensoryInput`.

**Solution:** Create conversion function:
```cpp
// In Creature.cpp or CreatureBrainInterface.cpp
ai::SensoryInput convertSensoryData(
    const SensorySystem& sensory,
    const glm::vec3& position,
    float visionRange,
    float energy,
    float maxEnergy)
{
    ai::SensoryInput input;

    // Get nearest percepts by type
    if (sensory.hasThreatNearby()) {
        auto threat = sensory.getNearestThreat();
        input.nearestPredatorDistance = threat.distance / visionRange;
        input.nearestPredatorAngle = threat.angle / M_PI;  // Normalize to [-1, 1]
    } else {
        input.nearestPredatorDistance = 1.0f;
        input.nearestPredatorAngle = 0.0f;
    }

    // Food
    auto foodPercepts = sensory.getPerceptsByType(DetectionType::FOOD);
    if (!foodPercepts.empty()) {
        const auto& nearest = foodPercepts[0];
        input.nearestFoodDistance = nearest.distance / visionRange;
        input.nearestFoodAngle = nearest.angle / M_PI;
    } else {
        input.nearestFoodDistance = 1.0f;
        input.nearestFoodAngle = 0.0f;
    }

    // Internal state
    input.energy = energy / maxEnergy;
    input.health = 1.0f;  // Or from creature health if tracked

    return input;
}
```

---

## Issue 7: Test False Positives (TEST QUALITY)

**File:** `src/ai/BrainTests.cpp`

### testEvolutionImprovesFitness
**Problem:** Passes even if fitness doesn't improve.
```cpp
// Current
assert(finalFitness >= initialFitness);

// Fixed - require meaningful improvement
assert(finalFitness > initialFitness + 0.5f);
```

### testLearningImprovesBehavior
**Problem:** Test explicitly states it doesn't verify learning.
```cpp
// Current - just checks it doesn't crash
TestReporter::pass();

// Fixed - verify learning occurred
assert(finalFleeResponse > initialFleeResponse + 0.05f);
```

### testHebbianLearning
**Problem:** Only checks weight changed, not direction.
```cpp
// Current
assert(finalWeight != initialWeight);

// Fixed - verify weight increased for positive correlation with positive reward
assert(finalWeight > initialWeight);
```

---

## Integration Steps

1. **Apply fixes in order:**
   - Issue 1 (eligibility explosion) - critical for learning
   - Issue 2 (missing decay) - critical for modular brain
   - Issue 3 (crossover) - important for evolution
   - Issue 4 (mutation probability) - important for evolution

2. **Add `ai::CreatureBrainInterface` to Creature class:**
   ```cpp
   // Creature.h
   #include "../ai/CreatureBrainInterface.h"

   class Creature {
   private:
       std::unique_ptr<ai::CreatureBrainInterface> m_aiBrain;
       bool m_useAIBrain = false;  // Toggle between legacy and AI
   };
   ```

3. **Modify Creature::update() to use AI brain when enabled:**
   ```cpp
   if (m_useAIBrain && m_aiBrain) {
       auto sensoryInput = convertSensoryData(sensory, position, genome.visionRange, energy, maxEnergy);
       auto output = m_aiBrain->process(/* sensory data */);

       // Apply motor output
       rotation += output.turnAngle * M_PI;
       float desiredSpeed = output.speed * genome.speed;
       velocity = glm::vec3(cos(rotation), 0, sin(rotation)) * desiredSpeed;
   } else {
       // Existing steering behavior code
   }
   ```

4. **Run tests to verify fixes work**

---

## Summary

| Issue | Severity | Type | Status |
|-------|----------|------|--------|
| 1. Eligibility explosion | Critical | Bug | Needs fix |
| 2. Missing modular decay | Critical | Bug | Needs fix |
| 3. Crossover wrong nodes | Important | Spec violation | Needs fix |
| 4. Mutation probability | Important | Bug | Needs fix |
| 5. Name collision | Integration | Architecture | Design choice |
| 6. Type mismatch | Integration | Architecture | Needs bridge |
| 7. Test false positives | Quality | Testing | Needs fix |
