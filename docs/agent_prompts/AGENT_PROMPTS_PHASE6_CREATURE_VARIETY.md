# Phase 5 Agent Prompts - Creature Variety & Visual Diversity

These prompts focus on making creatures visually distinct, interesting, and varied - moving beyond simple blobs to creatures with real character.

---

## Agent 1: Procedural Body Morphology System (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Create procedural body generation where creature shape is determined by genome, not just color/size.

============================================================================
PHASE 1: DESIGN BODY PLAN SYSTEM (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research procedural creatures:
- Web search "procedural creature generation Spore"
- Web search "L-system creature body generation"
- Web search "metaball creature mesh generation"
- Document approaches: metaballs, L-systems, modular parts

SUB-AGENT 1B - Analyze current mesh system:
- Read src/graphics/procedural/ files
- Read current creature mesh generation
- Document MetaballSystem if exists
- Identify integration points

CHECKPOINT 1: Create docs/BODY_MORPHOLOGY_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT BODY PLAN GENES (1+ hour)
============================================================================

Add body shape genes to genome:

```cpp
// src/entities/genetics/BodyPlanGenes.h

struct BodyPlanGenes {
    // Core body shape
    float bodyLength;          // 0.5 - 3.0 (short to elongated)
    float bodyWidth;           // 0.3 - 1.5 (thin to wide)
    float bodyHeight;          // 0.3 - 1.2 (flat to tall)
    BodyShape baseShape;       // SPHERICAL, ELLIPSOID, CYLINDRICAL, TAPERED

    // Segmentation
    int segmentCount;          // 1-8 body segments
    float segmentTaper;        // How segments change size
    float segmentSpacing;      // Gap between segments

    // Symmetry
    SymmetryType symmetry;     // BILATERAL, RADIAL_4, RADIAL_6, ASYMMETRIC
    float asymmetryFactor;     // How much asymmetry to apply

    // Surface features
    float bumpiness;           // Surface noise
    float ridgeCount;          // Spine ridges
    float bulgePositions[4];   // Where body bulges out

    // Head distinction
    float headSize;            // Relative to body
    float headOffset;          // Forward extension
    bool hasDistinctHead;      // Some creatures are headless blobs

    // Tail
    float tailLength;          // 0 = no tail, up to 2.0
    float tailThickness;       // Base thickness
    float tailTaper;           // How quickly it thins
    TailShape tailShape;       // POINTED, ROUNDED, FLAT, FORKED
};

enum class BodyShape {
    SPHERICAL,      // Ball-like (basic herbivore)
    ELLIPSOID,      // Egg-shaped
    CYLINDRICAL,    // Worm-like
    TAPERED,        // Teardrop (fast swimmers)
    FLATTENED,      // Pancake (bottom dwellers)
    SEGMENTED,      // Caterpillar-like
    BULBOUS,        // Big body, small head
    SERPENTINE      // Snake-like
};

enum class SymmetryType {
    BILATERAL,      // Left-right mirror (most animals)
    RADIAL_4,       // 4-way symmetry (starfish-like)
    RADIAL_6,       // 6-way symmetry (jellyfish-like)
    RADIAL_8,       // 8-way (octopus-like)
    ASYMMETRIC      // No symmetry (unique creatures)
};
```

CHECKPOINT 2: Body genes defined and inheritable

============================================================================
PHASE 3: IMPLEMENT MESH GENERATOR (1.5+ hours)
============================================================================

Generate mesh from body genes:

```cpp
// src/graphics/procedural/BodyMeshGenerator.h

class BodyMeshGenerator {
public:
    struct GeneratedMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<BoneWeight> boneWeights;  // For animation
        BoundingBox bounds;
    };

    GeneratedMesh generate(const BodyPlanGenes& genes, int lodLevel = 0);

private:
    // Core generation
    void generateBaseShape(const BodyPlanGenes& genes, MetaballSystem& system);
    void applySegmentation(const BodyPlanGenes& genes, MetaballSystem& system);
    void applySymmetry(const BodyPlanGenes& genes, MetaballSystem& system);
    void addSurfaceDetail(const BodyPlanGenes& genes, MetaballSystem& system);

    // Shape primitives
    void addEllipsoid(MetaballSystem& system, glm::vec3 pos, glm::vec3 radii, float strength);
    void addCapsule(MetaballSystem& system, glm::vec3 start, glm::vec3 end, float radius);
    void addTaperedCylinder(MetaballSystem& system, glm::vec3 start, glm::vec3 end,
                            float startRadius, float endRadius);
};

BodyMeshGenerator::GeneratedMesh BodyMeshGenerator::generate(
    const BodyPlanGenes& genes, int lodLevel) {

    MetaballSystem metaballs;

    // Base body
    switch (genes.baseShape) {
        case BodyShape::SPHERICAL:
            addEllipsoid(metaballs, glm::vec3(0),
                        glm::vec3(genes.bodyWidth, genes.bodyHeight, genes.bodyLength),
                        1.0f);
            break;

        case BodyShape::CYLINDRICAL:
            addCapsule(metaballs,
                      glm::vec3(0, 0, -genes.bodyLength * 0.5f),
                      glm::vec3(0, 0, genes.bodyLength * 0.5f),
                      genes.bodyWidth * 0.5f);
            break;

        case BodyShape::TAPERED:
            addTaperedCylinder(metaballs,
                              glm::vec3(0, 0, -genes.bodyLength * 0.5f),
                              glm::vec3(0, 0, genes.bodyLength * 0.5f),
                              genes.bodyWidth * 0.3f,  // Narrow front
                              genes.bodyWidth * 0.6f); // Wide back
            break;

        case BodyShape::SEGMENTED:
            float segmentLength = genes.bodyLength / genes.segmentCount;
            for (int i = 0; i < genes.segmentCount; i++) {
                float z = -genes.bodyLength * 0.5f + segmentLength * (i + 0.5f);
                float scale = 1.0f - (i * genes.segmentTaper / genes.segmentCount);
                addEllipsoid(metaballs, glm::vec3(0, 0, z),
                            glm::vec3(genes.bodyWidth * scale,
                                     genes.bodyHeight * scale,
                                     segmentLength * 0.4f),
                            0.8f);
            }
            break;
        // ... more shapes
    }

    // Add head if distinct
    if (genes.hasDistinctHead) {
        glm::vec3 headPos(0, genes.bodyHeight * 0.2f,
                         genes.bodyLength * 0.5f + genes.headOffset);
        addEllipsoid(metaballs, headPos,
                    glm::vec3(genes.headSize * 0.5f), 0.9f);
    }

    // Add tail
    if (genes.tailLength > 0.1f) {
        generateTail(metaballs, genes);
    }

    // Apply symmetry modifications
    if (genes.symmetry == SymmetryType::RADIAL_4 ||
        genes.symmetry == SymmetryType::RADIAL_6) {
        applyRadialSymmetry(metaballs, genes);
    }

    // March cubes to extract mesh
    int resolution = lodLevel == 0 ? 32 : (lodLevel == 1 ? 16 : 8);
    return marchingCubes(metaballs, resolution);
}
```

CHECKPOINT 3: Body meshes generate from genes

============================================================================
PHASE 4: SIZE VARIATION SYSTEM (45+ minutes)
============================================================================

Implement dramatic size differences:

```cpp
// Size ranges by creature type
struct SizeRange {
    float minScale;
    float maxScale;
    float baseSize;  // In world units
};

std::map<CreatureType, SizeRange> SIZE_RANGES = {
    // Tiny creatures
    {CreatureType::MICROBE,       {0.1f, 0.3f, 0.2f}},   // Barely visible
    {CreatureType::INSECT,        {0.2f, 0.5f, 0.3f}},   // Small bugs

    // Small creatures
    {CreatureType::SMALL_PREY,    {0.5f, 1.0f, 0.8f}},   // Mice-sized
    {CreatureType::FLYING_INSECT, {0.3f, 0.8f, 0.5f}},   // Flies/bees

    // Medium creatures
    {CreatureType::GRAZER,        {1.0f, 2.5f, 1.5f}},   // Dog-sized
    {CreatureType::SMALL_PREDATOR,{1.5f, 3.0f, 2.0f}},   // Wolf-sized

    // Large creatures
    {CreatureType::APEX_PREDATOR, {3.0f, 6.0f, 4.0f}},   // Bear-sized
    {CreatureType::BROWSER,       {2.5f, 5.0f, 3.5f}},   // Deer-sized

    // Giant creatures
    {CreatureType::MEGAFAUNA,     {6.0f, 15.0f, 10.0f}}, // Elephant-sized
    {CreatureType::AQUATIC_APEX,  {5.0f, 20.0f, 12.0f}}, // Whale-sized
};

// Size affects gameplay
void Creature::applySizeEffects() {
    float scale = m_phenotype.size;

    // Movement
    m_maxSpeed *= (1.0f / sqrt(scale));     // Bigger = slower top speed
    m_acceleration *= (1.0f / scale);        // Bigger = slower acceleration
    m_turnRate *= (1.0f / scale);            // Bigger = wider turns

    // Stats
    m_maxHealth *= scale * scale;            // Bigger = much more health
    m_maxEnergy *= scale * 1.5f;             // Bigger = more energy storage
    m_energyConsumption *= pow(scale, 0.75f); // Kleiber's law

    // Combat
    m_attackDamage *= scale;                 // Bigger = more damage
    m_attackRange *= sqrt(scale);            // Bigger = longer reach

    // Reproduction
    m_gestationTime *= scale;                // Bigger = longer gestation
    m_offspringCount /= scale;               // Bigger = fewer offspring
}
```

CHECKPOINT 4: Size variation affects gameplay

============================================================================
PHASE 5: VISUAL DISTINCTION (30+ minutes)
============================================================================

Make creatures instantly recognizable:

```cpp
// Distinct visual features based on genes
void CreatureRenderer::applyVisualTraits(const Phenotype& p) {
    // Body color from genes (not just species)
    m_baseColor = p.color;

    // Pattern overlay
    switch (p.patternType) {
        case Pattern::SOLID:
            m_patternTexture = nullptr;
            break;
        case Pattern::SPOTTED:
            m_patternTexture = generateSpotPattern(p.patternScale, p.patternColor);
            break;
        case Pattern::STRIPED:
            m_patternTexture = generateStripePattern(p.stripeAngle, p.patternColor);
            break;
        case Pattern::MOTTLED:
            m_patternTexture = generateNoisePattern(p.patternScale);
            break;
        case Pattern::GRADIENT:
            m_patternTexture = generateGradient(p.color, p.secondaryColor);
            break;
    }

    // Surface properties
    m_roughness = p.skinRoughness;       // Smooth (fish) to rough (toad)
    m_metallic = p.iridescence * 0.3f;   // Beetle-like sheen
    m_subsurface = p.translucency;       // Jellyfish-like glow
}
```

CHECKPOINT 5: Creatures visually distinct

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/BODY_MORPHOLOGY_DESIGN.md
- [ ] BodyPlanGenes struct in genome
- [ ] 8+ body shape types
- [ ] Segmented body support
- [ ] Radial symmetry option
- [ ] BodyMeshGenerator class
- [ ] Metaball-based mesh generation
- [ ] Size ranges for all creature types
- [ ] Size affects gameplay stats
- [ ] Visual pattern system
- [ ] LOD mesh generation

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 2: Modular Limb & Appendage System (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Create modular limb system where creatures can have varying numbers and types of legs, arms, wings, fins, tentacles, etc.

============================================================================
PHASE 1: DESIGN APPENDAGE SYSTEM (1+ hour)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research limb systems:
- Web search "procedural limb generation games"
- Web search "creature appendage attachment points"
- Web search "IK chain procedural animation"
- Document modular limb approaches

SUB-AGENT 1B - Analyze current limb code:
- Read existing skeleton/animation code
- Read limb-related genome traits
- Document current leg/wing implementation
- Identify extension points

CHECKPOINT 1: Create docs/APPENDAGE_SYSTEM_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT APPENDAGE GENES (1+ hour)
============================================================================

```cpp
// src/entities/genetics/AppendageGenes.h

enum class AppendageType {
    NONE,
    LEG_WALKING,      // Standard walking leg
    LEG_JUMPING,      // Powerful hind leg
    LEG_GRASPING,     // Can grab/hold
    ARM,              // Manipulator arm
    WING_FEATHERED,   // Bird-style wing
    WING_MEMBRANE,    // Bat-style wing
    WING_INSECT,      // Dragonfly-style
    FIN_PECTORAL,     // Side fin
    FIN_DORSAL,       // Back fin
    FIN_CAUDAL,       // Tail fin
    TENTACLE,         // Flexible grasping
    ANTENNA,          // Sensory
    TAIL,             // Balance/weapon
    PINCER,           // Crab claw
    SPINE,            // Defensive spike
    HORN,             // Head protrusion
    FLIPPER           // Seal-style
};

struct AppendageGene {
    AppendageType type;
    int count;                    // How many (per side for bilateral)
    float length;                 // 0.2 - 2.0 relative to body
    float thickness;              // 0.1 - 0.5 relative to length
    int segmentCount;             // 1-5 joints
    float flexibility;            // 0-1 how bendy
    glm::vec3 attachPoint;        // Where on body (normalized)
    float attachAngle;            // Angle from body
    bool hasClaw;                 // End in claw/hand
    float clawSize;
};

struct AppendageLayout {
    // Organized by attachment zone
    std::vector<AppendageGene> frontAppendages;   // Head area
    std::vector<AppendageGene> upperAppendages;   // Upper body (wings, arms)
    std::vector<AppendageGene> lowerAppendages;   // Lower body (legs)
    std::vector<AppendageGene> rearAppendages;    // Tail area

    // Constraints
    int maxTotalAppendages = 12;

    // Helper methods
    int getTotalLegCount() const;
    int getWingCount() const;
    bool canFly() const;
    bool canSwim() const;
    bool canGrasp() const;
};

// Example configurations
AppendageLayout QUADRUPED = {
    .lowerAppendages = {
        {LEG_WALKING, 2, 0.8f, 0.15f, 3, 0.3f, {0.3f, -0.3f, 0.3f}, -45.0f},  // Front legs
        {LEG_WALKING, 2, 1.0f, 0.15f, 3, 0.3f, {0.3f, -0.3f, -0.3f}, -45.0f}  // Rear legs
    },
    .rearAppendages = {
        {TAIL, 1, 0.6f, 0.1f, 4, 0.8f, {0, 0, -0.5f}, 0.0f}
    }
};

AppendageLayout HEXAPOD = {
    .lowerAppendages = {
        {LEG_WALKING, 2, 0.6f, 0.1f, 3, 0.2f, {0.4f, -0.2f, 0.3f}, -60.0f},
        {LEG_WALKING, 2, 0.7f, 0.1f, 3, 0.2f, {0.4f, -0.2f, 0.0f}, -75.0f},
        {LEG_WALKING, 2, 0.6f, 0.1f, 3, 0.2f, {0.4f, -0.2f, -0.3f}, -60.0f}
    },
    .frontAppendages = {
        {ANTENNA, 2, 0.4f, 0.02f, 2, 0.9f, {0.1f, 0.2f, 0.5f}, 30.0f}
    }
};

AppendageLayout OCTOPOD = {
    .lowerAppendages = {
        {TENTACLE, 8, 1.5f, 0.15f, 6, 0.95f, {0.4f, -0.1f, 0.0f}, -90.0f, true, 0.1f}
    }
};

AppendageLayout AVIAN = {
    .upperAppendages = {
        {WING_FEATHERED, 2, 1.5f, 0.3f, 3, 0.5f, {0.3f, 0.2f, 0.0f}, 45.0f}
    },
    .lowerAppendages = {
        {LEG_WALKING, 2, 0.8f, 0.1f, 3, 0.2f, {0.15f, -0.3f, -0.1f}, -80.0f, true, 0.15f}
    },
    .rearAppendages = {
        {TAIL, 1, 0.4f, 0.2f, 2, 0.3f, {0, 0.1f, -0.5f}, -15.0f}  // Fan tail
    }
};
```

CHECKPOINT 2: Appendage genes defined

============================================================================
PHASE 3: IMPLEMENT LIMB MESH GENERATION (1+ hour)
============================================================================

```cpp
// src/graphics/procedural/LimbGenerator.h

class LimbGenerator {
public:
    struct LimbMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<int> boneIndices;      // For skeleton
        std::vector<glm::mat4> bonePoses;  // Rest pose
    };

    LimbMesh generateLimb(const AppendageGene& gene);
    LimbMesh generateWing(const AppendageGene& gene);
    LimbMesh generateTentacle(const AppendageGene& gene);
    LimbMesh generateFin(const AppendageGene& gene);

private:
    void generateSegmentedLimb(LimbMesh& mesh, int segments,
                               float length, float baseThickness,
                               float taperFactor);
    void addJoint(LimbMesh& mesh, glm::vec3 position, float radius);
    void addClaw(LimbMesh& mesh, glm::vec3 position, float size, int fingers);
};

LimbGenerator::LimbMesh LimbGenerator::generateLimb(const AppendageGene& gene) {
    LimbMesh mesh;

    float segmentLength = gene.length / gene.segmentCount;
    float currentThickness = gene.thickness;
    glm::vec3 currentPos(0);
    glm::vec3 direction(0, -1, 0);  // Default downward

    for (int i = 0; i < gene.segmentCount; i++) {
        // Generate segment
        glm::vec3 nextPos = currentPos + direction * segmentLength;

        generateCapsuleSegment(mesh, currentPos, nextPos,
                              currentThickness, currentThickness * 0.85f);

        // Add bone
        mesh.boneIndices.push_back(i);
        mesh.bonePoses.push_back(glm::translate(glm::mat4(1), currentPos));

        // Add joint sphere
        if (i < gene.segmentCount - 1) {
            addJoint(mesh, nextPos, currentThickness * 0.9f);
        }

        currentPos = nextPos;
        currentThickness *= 0.85f;  // Taper

        // Bend direction slightly for natural pose
        direction = glm::normalize(direction + glm::vec3(0, -0.1f, 0.05f));
    }

    // Add claw/foot if specified
    if (gene.hasClaw) {
        addClaw(mesh, currentPos, gene.clawSize,
                gene.type == AppendageType::PINCER ? 2 : 3);
    }

    return mesh;
}

LimbGenerator::LimbMesh LimbGenerator::generateWing(const AppendageGene& gene) {
    LimbMesh mesh;

    // Wing bones: shoulder, elbow, wrist, fingers
    glm::vec3 shoulder(0);
    glm::vec3 elbow = shoulder + glm::vec3(gene.length * 0.3f, 0.1f, 0);
    glm::vec3 wrist = elbow + glm::vec3(gene.length * 0.3f, -0.05f, 0);
    glm::vec3 tip = wrist + glm::vec3(gene.length * 0.4f, 0, 0);

    // Arm bones
    generateCapsuleSegment(mesh, shoulder, elbow, gene.thickness, gene.thickness * 0.8f);
    generateCapsuleSegment(mesh, elbow, wrist, gene.thickness * 0.8f, gene.thickness * 0.6f);

    // Wing membrane (triangle fan or feathers)
    if (gene.type == AppendageType::WING_FEATHERED) {
        generateFeatheredWing(mesh, shoulder, elbow, wrist, tip, gene.length);
    } else if (gene.type == AppendageType::WING_MEMBRANE) {
        generateMembraneWing(mesh, shoulder, elbow, wrist, tip);
    } else {
        generateInsectWing(mesh, shoulder, tip, gene.length, gene.thickness);
    }

    return mesh;
}

LimbGenerator::LimbMesh LimbGenerator::generateTentacle(const AppendageGene& gene) {
    LimbMesh mesh;

    // Many small segments for smooth bending
    int segments = gene.segmentCount * 3;  // More segments for tentacles
    float segmentLength = gene.length / segments;
    float thickness = gene.thickness;

    glm::vec3 pos(0);
    glm::vec3 dir(0, -1, 0);

    for (int i = 0; i < segments; i++) {
        glm::vec3 nextPos = pos + dir * segmentLength;

        // Tentacle is smooth, no visible joints
        generateSmoothSegment(mesh, pos, nextPos, thickness);

        mesh.boneIndices.push_back(i);
        mesh.bonePoses.push_back(glm::translate(glm::mat4(1), pos));

        pos = nextPos;
        thickness *= 0.95f;  // Gradual taper

        // Slight curl
        dir = glm::normalize(dir + glm::vec3(0.02f, 0, 0.02f));
    }

    // Suckers on underside (optional visual detail)
    if (gene.type == AppendageType::TENTACLE) {
        addSuckers(mesh, segments);
    }

    return mesh;
}
```

CHECKPOINT 3: Limb meshes generate correctly

============================================================================
PHASE 4: SKELETON GENERATION (45+ minutes)
============================================================================

```cpp
// Generate skeleton to match appendages

class CreatureSkeletonBuilder {
public:
    Skeleton build(const BodyPlanGenes& body, const AppendageLayout& appendages);

private:
    int addBone(Skeleton& skel, const std::string& name, int parent,
                const glm::mat4& localTransform);
    void addLimbChain(Skeleton& skel, int parentBone,
                      const AppendageGene& limb, const std::string& prefix);
};

Skeleton CreatureSkeletonBuilder::build(const BodyPlanGenes& body,
                                         const AppendageLayout& appendages) {
    Skeleton skeleton;

    // Root bone
    int root = addBone(skeleton, "root", -1, glm::mat4(1));

    // Spine bones based on body segments
    int prevSpine = root;
    for (int i = 0; i < body.segmentCount; i++) {
        float z = -body.bodyLength * 0.5f +
                  (body.bodyLength / body.segmentCount) * (i + 0.5f);
        int spinebone = addBone(skeleton, "spine_" + std::to_string(i),
                                prevSpine, glm::translate(glm::mat4(1), {0, 0, z}));
        prevSpine = spinebone;
    }

    // Head bone
    if (body.hasDistinctHead) {
        addBone(skeleton, "head", prevSpine,
                glm::translate(glm::mat4(1), {0, 0, body.headOffset}));
    }

    // Add limbs to appropriate spine segments
    for (const auto& limb : appendages.lowerAppendages) {
        int attachBone = getSpineBoneAtZ(skeleton, limb.attachPoint.z);

        if (limb.count == 2 && body.symmetry == SymmetryType::BILATERAL) {
            addLimbChain(skeleton, attachBone, limb, "left_");
            addLimbChain(skeleton, attachBone, mirrorLimb(limb), "right_");
        } else {
            for (int i = 0; i < limb.count; i++) {
                float angle = (2.0f * PI * i) / limb.count;
                addLimbChain(skeleton, attachBone, rotateLimb(limb, angle),
                            "limb_" + std::to_string(i) + "_");
            }
        }
    }

    // Wings
    for (const auto& wing : appendages.upperAppendages) {
        if (wing.type == AppendageType::WING_FEATHERED ||
            wing.type == AppendageType::WING_MEMBRANE ||
            wing.type == AppendageType::WING_INSECT) {
            int attachBone = getSpineBoneAtZ(skeleton, wing.attachPoint.z);
            addLimbChain(skeleton, attachBone, wing, "left_wing_");
            addLimbChain(skeleton, attachBone, mirrorLimb(wing), "right_wing_");
        }
    }

    // Tail
    for (const auto& tail : appendages.rearAppendages) {
        if (tail.type == AppendageType::TAIL) {
            addLimbChain(skeleton, prevSpine, tail, "tail_");
        }
    }

    skeleton.calculateInverseBindPose();
    return skeleton;
}
```

CHECKPOINT 4: Skeletons generated from genes

============================================================================
PHASE 5: APPENDAGE ANIMATION (45+ minutes)
============================================================================

```cpp
// Procedural animation for different appendage types

class AppendageAnimator {
public:
    void animateWalking(Skeleton& skeleton, const AppendageLayout& layout,
                        float phase, float speed);
    void animateFlying(Skeleton& skeleton, const AppendageLayout& layout,
                       float phase, float altitude);
    void animateSwimming(Skeleton& skeleton, const AppendageLayout& layout,
                         float phase, float speed);
    void animateTentacles(Skeleton& skeleton, const AppendageLayout& layout,
                          float time, glm::vec3 targetDirection);

private:
    void animateLegCycle(Skeleton& skeleton, int legBoneStart,
                         float phase, float strideLength);
    void animateWingFlap(Skeleton& skeleton, int wingBoneStart,
                         float phase, float amplitude);
    void animateTentacleReach(Skeleton& skeleton, int tentacleBoneStart,
                              glm::vec3 targetDir, float reach);
};

void AppendageAnimator::animateWalking(Skeleton& skeleton,
                                        const AppendageLayout& layout,
                                        float phase, float speed) {
    int legIndex = 0;
    int totalLegs = layout.getTotalLegCount();

    for (const auto& limb : layout.lowerAppendages) {
        if (limb.type == AppendageType::LEG_WALKING ||
            limb.type == AppendageType::LEG_JUMPING) {

            for (int i = 0; i < limb.count * 2; i++) {  // *2 for bilateral
                // Gait pattern based on leg count
                float legPhase;
                if (totalLegs == 2) {
                    // Bipedal - opposite phase
                    legPhase = phase + (i * 0.5f);
                } else if (totalLegs == 4) {
                    // Quadruped - diagonal pairs
                    legPhase = phase + (i % 2 == 0 ? 0 : 0.5f) +
                               (i < 2 ? 0 : 0.25f);
                } else if (totalLegs == 6) {
                    // Hexapod - tripod gait
                    legPhase = phase + ((i % 2 == 0) ? 0 : 0.5f);
                } else {
                    // Many legs - wave pattern
                    legPhase = phase + (float)i / totalLegs;
                }

                int boneStart = skeleton.getBoneIndex("leg_" + std::to_string(legIndex));
                animateLegCycle(skeleton, boneStart, fmod(legPhase, 1.0f),
                               speed * limb.length);
                legIndex++;
            }
        }
    }
}

void AppendageAnimator::animateTentacles(Skeleton& skeleton,
                                          const AppendageLayout& layout,
                                          float time,
                                          glm::vec3 targetDirection) {
    int tentacleIndex = 0;

    for (const auto& limb : layout.lowerAppendages) {
        if (limb.type == AppendageType::TENTACLE) {
            for (int i = 0; i < limb.count; i++) {
                int boneStart = skeleton.getBoneIndex("limb_" + std::to_string(tentacleIndex));

                // Idle wave motion
                float wavePhase = time * 0.5f + (float)i * 0.3f;

                // Reach toward target
                float reachAmount = glm::dot(targetDirection,
                                            getTentacleDirection(i, limb.count));

                for (int seg = 0; seg < limb.segmentCount * 3; seg++) {
                    float segPhase = wavePhase + seg * 0.2f;
                    float bend = sin(segPhase) * 0.1f * limb.flexibility;

                    // Add reach influence
                    bend += reachAmount * 0.05f * (seg / (float)(limb.segmentCount * 3));

                    skeleton.rotateBone(boneStart + seg,
                                       glm::angleAxis(bend, glm::vec3(1, 0, 0)));
                }

                tentacleIndex++;
            }
        }
    }
}
```

CHECKPOINT 5: Appendages animate procedurally

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/APPENDAGE_SYSTEM_DESIGN.md
- [ ] AppendageType enum (15+ types)
- [ ] AppendageGene struct
- [ ] AppendageLayout with zones
- [ ] LimbGenerator class
- [ ] Leg mesh generation
- [ ] Wing mesh generation (3 types)
- [ ] Tentacle mesh generation
- [ ] Fin mesh generation
- [ ] CreatureSkeletonBuilder
- [ ] AppendageAnimator with gaits
- [ ] Tentacle procedural animation
- [ ] Working 2/4/6/8-legged creatures

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 3: Exotic Creature Types & Locomotion (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Add exotic creature types with unique locomotion - worms, jellyfish, crabs, snakes, etc.

============================================================================
PHASE 1: DESIGN EXOTIC TYPES (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research exotic locomotion:
- Web search "snake locomotion simulation game"
- Web search "jellyfish procedural animation"
- Web search "crab walking animation procedural"
- Document unique movement patterns

SUB-AGENT 1B - Design new creature types:
- List 10+ exotic creature types
- Document body requirements for each
- Document locomotion style for each
- Identify animation challenges

CHECKPOINT 1: Create docs/EXOTIC_CREATURES_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT EXOTIC CREATURE TYPES (1+ hour)
============================================================================

```cpp
// New creature types
enum class CreatureType {
    // ... existing types ...

    // Exotic terrestrial
    SNAKE,              // Legless serpentine
    WORM,               // Segmented, underground
    SLUG,               // Slow, mucus trail
    CRAB,               // Sideways walking
    SPIDER,             // 8 legs, web spinning
    CENTIPEDE,          // Many legs, fast

    // Exotic aquatic
    JELLYFISH,          // Pulsing locomotion
    OCTOPUS,            // Tentacle propulsion
    SQUID,              // Jet propulsion
    STARFISH,           // Tube feet, slow
    SEA_URCHIN,         // Spiny, defensive
    EEL,                // Serpentine swimming

    // Exotic flying
    BUTTERFLY,          // Erratic flight
    DRAGONFLY,          // Hovering, fast
    BEETLE,             // Heavy, buzzing

    // Exotic misc
    AMOEBA,             // Shape-shifting blob
    SLIME_MOLD,         // Colony creature
    PLANT_CREATURE      // Photosynthetic, sessile
};

// Locomotion types
enum class LocomotionType {
    WALKING,            // Standard leg walking
    RUNNING,            // Fast leg movement
    HOPPING,            // Kangaroo-like
    CRAWLING,           // Low to ground
    SLITHERING,         // Snake movement
    SIDEWAYS_WALKING,   // Crab style
    INCH_WORMING,       // Caterpillar style
    ROLLING,            // Ball form

    SWIMMING,           // Standard fish
    PULSING,            // Jellyfish
    JET_PROPULSION,     // Squid
    TENTACLE_WALKING,   // Octopus on seafloor
    UNDULATING,         // Eel/sea snake

    FLAPPING,           // Bird/bat
    GLIDING,            // No flapping
    HOVERING,           // Dragonfly
    BUZZING,            // Beetle

    BURROWING,          // Underground
    CLIMBING,           // Vertical surfaces
    SESSILE             // Stationary
};
```

CHECKPOINT 2: Exotic types defined

============================================================================
PHASE 3: IMPLEMENT SNAKE/WORM LOCOMOTION (1+ hour)
============================================================================

```cpp
// Serpentine movement

class SerpentineLocomotion {
public:
    void update(Creature& creature, float deltaTime);
    void applyToSkeleton(Skeleton& skeleton, float phase);

private:
    // Lateral undulation (most snakes)
    void lateralUndulation(Skeleton& skeleton, float phase, float speed);

    // Rectilinear movement (slow, straight)
    void rectilinear(Skeleton& skeleton, float phase, float speed);

    // Sidewinding (desert snakes)
    void sidewinding(Skeleton& skeleton, float phase, float speed);

    // Concertina (tight spaces)
    void concertina(Skeleton& skeleton, float phase, float speed);

    float m_waveAmplitude = 0.3f;
    float m_waveFrequency = 2.0f;
    float m_wavePropagation = 1.5f;  // How fast wave travels down body
};

void SerpentineLocomotion::lateralUndulation(Skeleton& skeleton,
                                              float phase, float speed) {
    int spineCount = skeleton.getSpineBoneCount();

    for (int i = 0; i < spineCount; i++) {
        // Wave travels from head to tail
        float bonePhase = phase - (float)i / spineCount * m_wavePropagation;
        float bend = sin(bonePhase * 2.0f * PI) * m_waveAmplitude;

        // Reduce amplitude at head and tail
        float falloff = 1.0f - abs((float)i / spineCount - 0.5f) * 0.5f;
        bend *= falloff;

        // Apply yaw rotation to spine bone
        skeleton.rotateBoneLocal(i, glm::angleAxis(bend, glm::vec3(0, 1, 0)));
    }

    // Ground contact points push creature forward
    glm::vec3 movement(0);
    for (int i = 0; i < spineCount; i++) {
        float bonePhase = phase - (float)i / spineCount * m_wavePropagation;
        if (sin(bonePhase * 2.0f * PI) > 0.7f) {
            // This segment is pushing against ground
            glm::vec3 pushDir = skeleton.getBoneForward(i);
            movement += pushDir * speed * 0.1f;
        }
    }

    // Apply movement
    creature.addVelocity(movement);
}

// Worm-specific inchworm movement
class InchwormLocomotion {
public:
    void update(Creature& creature, float deltaTime);

private:
    float m_stretchPhase = 0.0f;
    float m_cycleTime = 1.0f;
};

void InchwormLocomotion::update(Creature& creature, float deltaTime) {
    m_stretchPhase += deltaTime / m_cycleTime;
    if (m_stretchPhase > 1.0f) m_stretchPhase -= 1.0f;

    Skeleton& skeleton = creature.getSkeleton();
    int spineCount = skeleton.getSpineBoneCount();

    if (m_stretchPhase < 0.5f) {
        // Stretch phase - front extends, rear anchored
        float stretch = m_stretchPhase * 2.0f;
        for (int i = 0; i < spineCount; i++) {
            float t = (float)i / spineCount;
            float spacing = glm::mix(0.8f, 1.2f, t * stretch);
            skeleton.setSegmentSpacing(i, spacing);
        }
    } else {
        // Contract phase - rear catches up to front
        float contract = (m_stretchPhase - 0.5f) * 2.0f;
        for (int i = 0; i < spineCount; i++) {
            float t = (float)i / spineCount;
            float spacing = glm::mix(1.2f, 0.8f, (1.0f - t) * contract);
            skeleton.setSegmentSpacing(i, spacing);
        }

        // Move forward
        creature.addVelocity(creature.getForward() * contract * 0.5f);
    }
}
```

CHECKPOINT 3: Serpentine locomotion working

============================================================================
PHASE 4: IMPLEMENT JELLYFISH LOCOMOTION (45+ minutes)
============================================================================

```cpp
class JellyfishLocomotion {
public:
    void update(Creature& creature, float deltaTime);
    void applyToMesh(DynamicMesh& mesh, float phase);

private:
    float m_pulsePhase = 0.0f;
    float m_pulseRate = 0.8f;       // Pulses per second
    float m_contractionAmount = 0.3f;
    float m_driftFactor = 0.2f;     // Passive current drift
};

void JellyfishLocomotion::update(Creature& creature, float deltaTime) {
    m_pulsePhase += deltaTime * m_pulseRate;

    // Bell contraction cycle
    float contraction = (sin(m_pulsePhase * 2.0f * PI) + 1.0f) * 0.5f;

    // Apply to mesh (bell shape)
    DynamicMesh& mesh = creature.getDynamicMesh();
    applyBellContraction(mesh, contraction);

    // Propulsion during contraction
    if (contraction > 0.7f) {
        glm::vec3 thrust = glm::vec3(0, 1, 0) * contraction * 2.0f;
        creature.addVelocity(thrust * deltaTime);
    }

    // Passive sinking between pulses
    creature.addVelocity(glm::vec3(0, -0.5f, 0) * deltaTime);

    // Drift with current
    glm::vec3 current = creature.getWorld().getCurrentAt(creature.getPosition());
    creature.addVelocity(current * m_driftFactor * deltaTime);

    // Tentacle trailing
    animateTentacles(creature.getSkeleton(), m_pulsePhase, contraction);
}

void JellyfishLocomotion::applyBellContraction(DynamicMesh& mesh, float contraction) {
    // Vertices on the bell rim contract inward during pulse
    for (Vertex& v : mesh.vertices) {
        if (v.position.y < 0) {  // Below center = rim
            float rimFactor = -v.position.y / mesh.maxY;
            float squeeze = 1.0f - (contraction * m_contractionAmount * rimFactor);

            // Contract horizontally
            v.position.x *= squeeze;
            v.position.z *= squeeze;

            // Curve upward during contraction
            v.position.y -= contraction * 0.2f * rimFactor;
        }
    }

    mesh.recalculateNormals();
}

void JellyfishLocomotion::animateTentacles(Skeleton& skeleton, float phase,
                                            float contraction) {
    // Tentacles trail behind, wave gently
    int tentacleCount = skeleton.getTentacleCount();

    for (int t = 0; t < tentacleCount; t++) {
        int boneStart = skeleton.getTentacleBoneStart(t);
        int boneCount = skeleton.getTentacleBoneCount(t);

        for (int b = 0; b < boneCount; b++) {
            float boneFactor = (float)b / boneCount;

            // Trail behind during contraction
            float trailAngle = contraction * 0.5f * boneFactor;

            // Gentle wave
            float wave = sin(phase * 2.0f + t * 0.5f + b * 0.3f) * 0.1f;

            skeleton.rotateBoneLocal(boneStart + b,
                glm::angleAxis(trailAngle + wave, glm::vec3(1, 0, 0)));
        }
    }
}
```

CHECKPOINT 4: Jellyfish locomotion working

============================================================================
PHASE 5: IMPLEMENT CRAB LOCOMOTION (45+ minutes)
============================================================================

```cpp
class CrabLocomotion {
public:
    void update(Creature& creature, float deltaTime);

private:
    void sidewaysWalk(Creature& creature, float phase, float speed);
    void animateLegs(Skeleton& skeleton, float phase, bool movingRight);
    void animateClaws(Skeleton& skeleton, float threatLevel);

    float m_walkPhase = 0.0f;
    bool m_facingRight = true;
};

void CrabLocomotion::update(Creature& creature, float deltaTime) {
    glm::vec3 desiredMove = creature.getDesiredDirection();

    // Crabs prefer sideways movement
    glm::vec3 right = creature.getRight();
    glm::vec3 forward = creature.getForward();

    float rightComponent = glm::dot(desiredMove, right);
    float forwardComponent = glm::dot(desiredMove, forward);

    // Mostly sideways, some forward/back
    glm::vec3 actualMove = right * rightComponent * 1.0f +
                           forward * forwardComponent * 0.3f;

    if (glm::length(actualMove) > 0.01f) {
        m_walkPhase += deltaTime * glm::length(actualMove) * 2.0f;
        m_facingRight = rightComponent > 0;
    }

    // Animate legs for sideways gait
    animateLegs(creature.getSkeleton(), m_walkPhase, m_facingRight);

    // Claws raise when threatened
    float threatLevel = creature.getThreatLevel();
    animateClaws(creature.getSkeleton(), threatLevel);

    creature.setVelocity(actualMove * creature.getMaxSpeed());
}

void CrabLocomotion::animateLegs(Skeleton& skeleton, float phase, bool movingRight) {
    // 8 legs in 4 pairs, alternating
    // Legs on leading side reach forward, trailing side push

    for (int pair = 0; pair < 4; pair++) {
        float pairPhase = phase + pair * 0.25f;

        // Left leg
        int leftLeg = skeleton.getBoneIndex("leg_left_" + std::to_string(pair));
        float leftAngle = movingRight ?
            sin(pairPhase * 2.0f * PI) * 0.3f :           // Leading
            sin(pairPhase * 2.0f * PI + PI) * 0.3f;       // Trailing

        // Right leg
        int rightLeg = skeleton.getBoneIndex("leg_right_" + std::to_string(pair));
        float rightAngle = !movingRight ?
            sin(pairPhase * 2.0f * PI) * 0.3f :
            sin(pairPhase * 2.0f * PI + PI) * 0.3f;

        // Apply rotation (legs move laterally)
        skeleton.rotateBoneLocal(leftLeg, glm::angleAxis(leftAngle, glm::vec3(0, 0, 1)));
        skeleton.rotateBoneLocal(rightLeg, glm::angleAxis(rightAngle, glm::vec3(0, 0, 1)));
    }
}

void CrabLocomotion::animateClaws(Skeleton& skeleton, float threatLevel) {
    int leftClaw = skeleton.getBoneIndex("claw_left");
    int rightClaw = skeleton.getBoneIndex("claw_right");

    // Raise claws when threatened
    float raiseAngle = threatLevel * 0.8f;  // Up to ~45 degrees

    skeleton.rotateBoneLocal(leftClaw, glm::angleAxis(raiseAngle, glm::vec3(1, 0, 0)));
    skeleton.rotateBoneLocal(rightClaw, glm::angleAxis(raiseAngle, glm::vec3(1, 0, 0)));

    // Open/close claws when very threatened
    if (threatLevel > 0.7f) {
        float clamp = sin(skeleton.getTime() * 5.0f) * 0.5f + 0.5f;
        skeleton.setClawOpenness(leftClaw, clamp);
        skeleton.setClawOpenness(rightClaw, clamp);
    }
}
```

CHECKPOINT 5: Crab locomotion working

============================================================================
PHASE 6: INTEGRATE EXOTIC TYPES (30+ minutes)
============================================================================

Wire into creature spawning and behavior:

```cpp
// Spawning exotic types
void World::spawnExoticCreatures() {
    // Snakes in grasslands
    if (biome == BiomeType::SAVANNA || biome == BiomeType::TEMPERATE_GRASSLAND) {
        spawnCreature(CreatureType::SNAKE, position);
    }

    // Jellyfish in ocean
    if (isDeepWater(position)) {
        spawnCreature(CreatureType::JELLYFISH, position);
    }

    // Crabs on beaches
    if (biome == BiomeType::BEACH) {
        spawnCreature(CreatureType::CRAB, position);
    }

    // Worms underground (spawn when digging?)
    // Spiders in forests
    // etc.
}

// Behavior patterns for exotic types
void Creature::updateExoticBehavior(float dt) {
    switch (m_type) {
        case CreatureType::JELLYFISH:
            // Drift passively, catch tiny prey
            m_locomotion = std::make_unique<JellyfishLocomotion>();
            break;

        case CreatureType::SNAKE:
            // Ambush predator, constrict prey
            m_locomotion = std::make_unique<SerpentineLocomotion>();
            break;

        case CreatureType::CRAB:
            // Scavenger, defensive when threatened
            m_locomotion = std::make_unique<CrabLocomotion>();
            break;
    }
}
```

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/EXOTIC_CREATURES_DESIGN.md
- [ ] 15+ exotic creature types
- [ ] LocomotionType enum
- [ ] SerpentineLocomotion (snake/worm)
- [ ] InchwormLocomotion
- [ ] JellyfishLocomotion with bell pulsing
- [ ] CrabLocomotion (sideways walk)
- [ ] Tentacle trailing animation
- [ ] Biome-appropriate spawning
- [ ] Exotic creature behaviors
- [ ] Visual distinction for each type

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 4: Size Scaling & Physics (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement dramatic size variation from microscopic to massive creatures with proper physics scaling.

============================================================================
PHASE 1: DESIGN SIZE SYSTEM (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research size scaling:
- Web search "creature size scaling physics games"
- Web search "Kleiber's law metabolic scaling"
- Web search "square-cube law game design"
- Document real-world scaling relationships

SUB-AGENT 1B - Analyze current size handling:
- Read creature size code
- Read physics/collision code
- Document current size range
- Identify scaling issues

CHECKPOINT 1: Create docs/SIZE_SCALING_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT SIZE CATEGORIES (1+ hour)
============================================================================

```cpp
// src/entities/CreatureSize.h

enum class SizeCategory {
    MICROSCOPIC,    // 0.05 - 0.2 units (bacteria-like)
    TINY,           // 0.2 - 0.5 units (insects)
    SMALL,          // 0.5 - 1.0 units (mice, birds)
    MEDIUM,         // 1.0 - 2.5 units (dogs, humans)
    LARGE,          // 2.5 - 5.0 units (horses, bears)
    HUGE,           // 5.0 - 15.0 units (elephants)
    COLOSSAL,       // 15.0 - 50.0 units (whales)
    TITANIC         // 50.0+ units (kaiju-scale)
};

struct SizeScaling {
    float baseSize;
    float minScale;
    float maxScale;

    // Derived properties
    float getActualSize(float scaleFactor) const {
        return baseSize * glm::mix(minScale, maxScale, scaleFactor);
    }
};

// Size affects everything
struct SizeEffects {
    // Movement (square-cube law)
    float speedMultiplier;       // Bigger = relatively slower
    float accelerationMult;      // Bigger = slower accel
    float turnRateMult;          // Bigger = wider turns
    float jumpHeightMult;        // Bigger = lower relative jump

    // Stats
    float healthMultiplier;      // Bigger = much more HP
    float energyCapacityMult;    // Bigger = more energy
    float metabolicRate;         // Kleiber's law: mass^0.75
    float hungerRate;            // How fast energy depletes

    // Combat
    float damageMultiplier;      // Bigger = more damage
    float attackRange;           // Bigger = longer reach
    float intimidationFactor;    // Bigger = scarier

    // Reproduction
    float gestationTime;         // Bigger = longer
    float offspringCount;        // Bigger = fewer offspring
    float maturityTime;          // Bigger = slower to grow

    // Perception
    float visionRange;           // Bigger = see further
    float hearingRange;          // Bigger = hear further
    float stealthFactor;         // Bigger = easier to spot
};

SizeEffects calculateSizeEffects(float actualSize) {
    SizeEffects effects;

    // Square-cube law: surface/volume ratio decreases with size
    // Strength scales with cross-section (size^2)
    // Weight scales with volume (size^3)
    // Relative strength decreases as size^(-1)

    float sizeNormalized = actualSize;  // 1.0 = "normal" size

    // Movement - larger creatures are relatively slower
    effects.speedMultiplier = 1.0f / sqrt(sizeNormalized);
    effects.accelerationMult = 1.0f / sizeNormalized;
    effects.turnRateMult = 1.0f / sizeNormalized;
    effects.jumpHeightMult = 1.0f / pow(sizeNormalized, 0.33f);

    // Stats
    effects.healthMultiplier = sizeNormalized * sizeNormalized;  // Size^2
    effects.energyCapacityMult = sizeNormalized * 1.5f;
    effects.metabolicRate = pow(sizeNormalized, 0.75f);  // Kleiber's law
    effects.hungerRate = effects.metabolicRate / effects.energyCapacityMult;

    // Combat
    effects.damageMultiplier = sizeNormalized;
    effects.attackRange = sqrt(sizeNormalized);
    effects.intimidationFactor = sizeNormalized * sizeNormalized;

    // Reproduction
    effects.gestationTime = sqrt(sizeNormalized);
    effects.offspringCount = 1.0f / sqrt(sizeNormalized);
    effects.maturityTime = pow(sizeNormalized, 0.5f);

    // Perception
    effects.visionRange = pow(sizeNormalized, 0.4f);
    effects.hearingRange = pow(sizeNormalized, 0.3f);
    effects.stealthFactor = 1.0f / sizeNormalized;

    return effects;
}
```

CHECKPOINT 2: Size scaling formulas implemented

============================================================================
PHASE 3: IMPLEMENT COLLISION SCALING (45+ minutes)
============================================================================

```cpp
// Size-appropriate collision detection

class ScaledCollisionSystem {
public:
    void updateCollisions(std::vector<Creature>& creatures);

private:
    // Use spatial partitioning appropriate to size
    void insertBySize(Creature& creature);

    // Different collision handling by size ratio
    void handleCollision(Creature& a, Creature& b);
    void handleSizeDisparity(Creature& large, Creature& small);

    // Spatial grids at different scales
    SpatialGrid m_microGrid;      // For microscopic creatures
    SpatialGrid m_smallGrid;      // For small creatures
    SpatialGrid m_mediumGrid;     // For medium creatures
    SpatialGrid m_largeGrid;      // For large creatures
    SpatialGrid m_massiveGrid;    // For huge+ creatures
};

void ScaledCollisionSystem::handleCollision(Creature& a, Creature& b) {
    float sizeA = a.getActualSize();
    float sizeB = b.getActualSize();
    float sizeRatio = sizeA / sizeB;

    if (sizeRatio > 5.0f) {
        // A is much larger than B
        handleSizeDisparity(a, b);
    } else if (sizeRatio < 0.2f) {
        // B is much larger than A
        handleSizeDisparity(b, a);
    } else {
        // Similar sizes - normal collision
        handleNormalCollision(a, b);
    }
}

void ScaledCollisionSystem::handleSizeDisparity(Creature& large, Creature& small) {
    // Large creature barely notices small one
    // Small creature gets pushed/trampled

    float sizeRatio = large.getActualSize() / small.getActualSize();

    if (sizeRatio > 20.0f) {
        // Huge disparity - small creature might be crushed
        if (large.isMoving() && small.isBelowCenterMass(large)) {
            small.takeDamage(large.getWeight() * 0.01f);
            small.applyForce(large.getVelocity() * large.getWeight() * 0.5f);
        }
    } else if (sizeRatio > 10.0f) {
        // Large can eat small in one bite (if predator)
        if (large.isPredator() && large.isHungry()) {
            large.instantKill(small);
        }
    } else {
        // Moderate disparity - push aside
        glm::vec3 pushDir = glm::normalize(small.getPosition() - large.getPosition());
        small.applyForce(pushDir * large.getMomentum() * 0.3f);
    }
}

// Render size appropriately
void ScaledRenderer::renderCreature(const Creature& creature) {
    float size = creature.getActualSize();

    if (size < 0.3f) {
        // Very small - use simpler mesh, potentially instanced
        renderMicroCreature(creature);
    } else if (size > 10.0f) {
        // Very large - ensure proper LOD, maybe special shadow
        renderMegaCreature(creature);
    } else {
        // Normal size
        renderStandardCreature(creature);
    }
}
```

CHECKPOINT 3: Collision scaling implemented

============================================================================
PHASE 4: IMPLEMENT SIZE-BASED INTERACTIONS (45+ minutes)
============================================================================

```cpp
// How different sizes interact

class SizeInteractionSystem {
public:
    // Predation - can A eat B?
    bool canEat(const Creature& predator, const Creature& prey);
    float getEatDifficulty(const Creature& predator, const Creature& prey);

    // Intimidation
    float getIntimidationLevel(const Creature& intimidator, const Creature& target);
    bool shouldFlee(const Creature& creature, const Creature& threat);

    // Combat
    float calculateDamage(const Creature& attacker, const Creature& defender);
    bool canDamage(const Creature& attacker, const Creature& defender);
};

bool SizeInteractionSystem::canEat(const Creature& predator, const Creature& prey) {
    float sizeRatio = predator.getActualSize() / prey.getActualSize();

    // Must be at least 1.5x larger to eat (for swallowing)
    // Some predators (snakes) can eat larger - check traits
    float minRatio = predator.hasTrait(Trait::EXPANDABLE_JAW) ? 0.8f : 1.5f;

    // Can't eat something more than 50x smaller (too tiny to bother)
    float maxRatio = 50.0f;

    return sizeRatio >= minRatio && sizeRatio <= maxRatio;
}

float SizeInteractionSystem::getIntimidationLevel(const Creature& intimidator,
                                                    const Creature& target) {
    float sizeRatio = intimidator.getActualSize() / target.getActualSize();

    // Base intimidation from size
    float intimidation = sizeRatio - 1.0f;  // Same size = 0 intimidation

    // Modify by other factors
    if (intimidator.isPredator()) intimidation *= 1.5f;
    if (intimidator.hasWeapons()) intimidation *= 1.3f;  // Claws, horns
    if (intimidator.isBrightlyColored()) intimidation *= 0.8f;  // Warning colors

    // Target's bravery reduces intimidation
    intimidation *= (1.0f - target.getBravery() * 0.5f);

    return glm::clamp(intimidation, 0.0f, 10.0f);
}

bool SizeInteractionSystem::shouldFlee(const Creature& creature, const Creature& threat) {
    float intimidation = getIntimidationLevel(threat, creature);
    float threshold = creature.getFleeThreshold();

    // Group presence reduces fear
    int nearbyAllies = creature.countNearbyAllies(10.0f);
    threshold += nearbyAllies * 0.2f;

    return intimidation > threshold;
}

float SizeInteractionSystem::calculateDamage(const Creature& attacker,
                                              const Creature& defender) {
    float baseDamage = attacker.getBaseDamage();

    // Size affects damage
    float sizeRatio = attacker.getActualSize() / defender.getActualSize();

    if (sizeRatio > 1.0f) {
        // Larger attacker does more damage
        baseDamage *= sqrt(sizeRatio);
    } else {
        // Smaller attacker does less damage
        baseDamage *= sizeRatio;
    }

    // But defender's size also provides protection
    float armorFromSize = defender.getActualSize() * 0.1f;
    baseDamage -= armorFromSize;

    return std::max(0.0f, baseDamage);
}
```

CHECKPOINT 4: Size interactions working

============================================================================
PHASE 5: IMPLEMENT SIZE VISUALIZATION (30+ minutes)
============================================================================

```cpp
// Visual scale indicators

class SizeVisualization {
public:
    // Camera adjustments for size
    void adjustCameraForCreature(Camera& camera, const Creature& focus);

    // UI size indicators
    void renderSizeComparison(const Creature& creature);
    void renderScaleBar(float worldSize);

    // LOD based on size
    int getLODLevel(const Creature& creature, float distanceToCamera);
};

void SizeVisualization::adjustCameraForCreature(Camera& camera, const Creature& focus) {
    float size = focus.getActualSize();

    // Distance based on creature size
    float baseDistance = 5.0f;
    float distance = baseDistance * sqrt(size);

    // Height based on size
    float height = size * 0.5f + 2.0f;

    camera.setFollowDistance(distance);
    camera.setFollowHeight(height);
}

void SizeVisualization::renderSizeComparison(const Creature& creature) {
    // Show silhouette comparison to "human scale" (size 1.0)
    float size = creature.getActualSize();

    ImGui::Text("Size: %.2f units", size);

    // Draw comparison
    float humanHeight = 50.0f;  // pixels
    float creatureHeight = humanHeight * size;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Human silhouette
    draw->AddRectFilled(pos, {pos.x + 10, pos.y + humanHeight},
                        IM_COL32(128, 128, 128, 128));

    // Creature silhouette (capped for display)
    float displayHeight = std::min(creatureHeight, 200.0f);
    draw->AddRectFilled({pos.x + 20, pos.y + humanHeight - displayHeight},
                        {pos.x + 30, pos.y + humanHeight},
                        IM_COL32(255, 128, 0, 200));

    if (creatureHeight > 200.0f) {
        ImGui::Text("(%.1fx human scale)", size);
    }
}
```

CHECKPOINT 5: Size visualization complete

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SIZE_SCALING_DESIGN.md
- [ ] SizeCategory enum (8 categories)
- [ ] SizeEffects struct with all scaling
- [ ] Kleiber's law metabolic scaling
- [ ] Square-cube law physics
- [ ] Multi-scale spatial grids
- [ ] Size disparity collision handling
- [ ] Predation size requirements
- [ ] Intimidation system
- [ ] Camera size adjustments
- [ ] Size comparison UI
- [ ] LOD by size

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 5: Surface Features & Ornamentation (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Add horns, spines, shells, antlers, frills, and other surface features that make creatures unique.

============================================================================
PHASE 1: DESIGN FEATURE SYSTEM (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research creature ornamentation:
- Web search "procedural horn generation games"
- Web search "animal defensive structures evolution"
- Web search "procedural shell patterns"
- Document natural ornamentation types

SUB-AGENT 1B - Design feature categories:
- List all surface feature types
- Document functional purposes
- Design gene representation
- Plan mesh generation

CHECKPOINT 1: Create docs/SURFACE_FEATURES_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT SURFACE FEATURE GENES (1+ hour)
============================================================================

```cpp
// src/entities/genetics/SurfaceFeatureGenes.h

enum class SurfaceFeatureType {
    // Head features
    HORN_SINGLE,        // Unicorn style
    HORN_PAIR,          // Bull/ram style
    HORN_MULTIPLE,      // Many small horns
    ANTLER,             // Branching antlers
    CREST,              // Head crest/mohawk
    FRILL,              // Triceratops-style frill
    CROWN,              // Spike crown

    // Body armor
    SHELL_DOME,         // Turtle shell
    SHELL_SPIRAL,       // Snail shell
    ARMOR_PLATES,       // Armadillo style
    ARMOR_SCALES,       // Pangolin scales
    CARAPACE,           // Beetle shell

    // Spines/spikes
    SPINE_ROW,          // Spine down the back
    SPINE_ARRAY,        // Porcupine-like
    SPIKE_SHOULDER,     // Shoulder spikes
    SPIKE_TAIL,         // Tail spikes

    // Soft features
    MANE,               // Lion mane
    RUFF,               // Neck ruff
    DEWLAP,             // Throat flap
    WATTLE,             // Chin flap
    FIN_SAIL,           // Dimetrodon sail
    HUMP,               // Camel hump

    // Tails
    TAIL_FAN,           // Peacock-style
    TAIL_CLUB,          // Ankylosaurus
    TAIL_RATTLE,        // Rattlesnake
    TAIL_PREHENSILE,    // Monkey tail

    // Eyes
    EYE_STALKS,         // Snail style
    EYE_MULTIPLE,       // Spider style
    EYE_COMPOUND,       // Insect style

    // Other
    BARBELS,            // Catfish whiskers
    PROBOSCIS,          // Elephant trunk
    BIOLUMINESCENCE,    // Glowing spots
    FEATHER_DISPLAY     // Decorative feathers
};

struct SurfaceFeatureGene {
    SurfaceFeatureType type;
    float size;             // 0.1 - 2.0 relative to body
    float count;            // Number of features (for arrays)
    glm::vec3 position;     // Attachment point (normalized)
    glm::vec3 direction;    // Growth direction
    float curvature;        // How curved (for horns/spines)
    float taper;            // How much it narrows
    int segments;           // For branching features
    bool isSymmetric;       // Mirror on other side
    glm::vec3 color;        // Feature color
    float iridescence;      // Shiny factor
};

struct SurfaceFeatureLayout {
    std::vector<SurfaceFeatureGene> headFeatures;
    std::vector<SurfaceFeatureGene> bodyFeatures;
    std::vector<SurfaceFeatureGene> tailFeatures;

    // Constraints
    int maxHeadFeatures = 3;
    int maxBodyFeatures = 5;
    int maxTailFeatures = 2;

    // Helpers
    bool hasShell() const;
    bool hasSpines() const;
    bool hasHorns() const;
    float getArmorCoverage() const;
    float getIntimidationBonus() const;
};

// Feature templates
SurfaceFeatureGene createBullHorns(float size = 1.0f) {
    return {
        .type = SurfaceFeatureType::HORN_PAIR,
        .size = size,
        .count = 2,
        .position = {0.2f, 0.8f, 0.9f},  // Top sides of head
        .direction = {0.5f, 0.8f, 0.3f}, // Up and out
        .curvature = 0.3f,
        .taper = 0.7f,
        .segments = 1,
        .isSymmetric = true,
        .color = {0.3f, 0.25f, 0.2f},  // Dark bone
        .iridescence = 0.0f
    };
}

SurfaceFeatureGene createDeerAntlers(int branches = 4) {
    return {
        .type = SurfaceFeatureType::ANTLER,
        .size = 1.5f,
        .count = 2,
        .position = {0.3f, 0.9f, 0.85f},
        .direction = {0.2f, 1.0f, 0.1f},
        .curvature = 0.1f,
        .taper = 0.5f,
        .segments = branches,
        .isSymmetric = true,
        .color = {0.4f, 0.35f, 0.25f},
        .iridescence = 0.0f
    };
}

SurfaceFeatureGene createSpineRow(int spines = 12) {
    return {
        .type = SurfaceFeatureType::SPINE_ROW,
        .size = 0.5f,
        .count = spines,
        .position = {0.0f, 1.0f, 0.5f},  // Along top centerline
        .direction = {0.0f, 1.0f, 0.0f},
        .curvature = 0.0f,
        .taper = 0.8f,
        .segments = 1,
        .isSymmetric = false,
        .color = {0.2f, 0.2f, 0.25f},
        .iridescence = 0.1f
    };
}

SurfaceFeatureGene createTurtleShell() {
    return {
        .type = SurfaceFeatureType::SHELL_DOME,
        .size = 1.2f,
        .count = 1,
        .position = {0.0f, 0.6f, 0.0f},  // Covers back
        .direction = {0.0f, 1.0f, 0.0f},
        .curvature = 0.8f,  // Very domed
        .taper = 0.0f,
        .segments = 6,  // Scutes/plates
        .isSymmetric = true,
        .color = {0.3f, 0.25f, 0.15f},  // Brown
        .iridescence = 0.0f
    };
}
```

CHECKPOINT 2: Surface feature genes defined

============================================================================
PHASE 3: IMPLEMENT FEATURE MESH GENERATION (1+ hour)
============================================================================

```cpp
// src/graphics/procedural/FeatureMeshGenerator.h

class FeatureMeshGenerator {
public:
    struct FeatureMesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec3 attachPoint;
        glm::mat4 orientation;
    };

    FeatureMesh generateHorn(const SurfaceFeatureGene& gene);
    FeatureMesh generateAntler(const SurfaceFeatureGene& gene);
    FeatureMesh generateSpine(const SurfaceFeatureGene& gene);
    FeatureMesh generateShell(const SurfaceFeatureGene& gene);
    FeatureMesh generateFrill(const SurfaceFeatureGene& gene);
    FeatureMesh generateMane(const SurfaceFeatureGene& gene);

private:
    void generateCurvedCone(FeatureMesh& mesh, float length, float baseRadius,
                            float tipRadius, float curvature, int segments);
    void generateBranch(FeatureMesh& mesh, glm::vec3 start, glm::vec3 dir,
                        float length, float radius, int branches, int depth);
    void generateDome(FeatureMesh& mesh, float radius, float height, int segments);
    void generatePlates(FeatureMesh& mesh, float coverage, int plateCount);
};

FeatureMeshGenerator::FeatureMesh FeatureMeshGenerator::generateHorn(
    const SurfaceFeatureGene& gene) {
    FeatureMesh mesh;

    float length = gene.size * 0.5f;
    float baseRadius = gene.size * 0.1f;
    float tipRadius = baseRadius * (1.0f - gene.taper);

    // Curved cone
    int rings = 8;
    int segments = 12;
    float angle = 0.0f;

    for (int r = 0; r <= rings; r++) {
        float t = (float)r / rings;
        float radius = glm::mix(baseRadius, tipRadius, t);

        // Apply curvature
        float curveAngle = gene.curvature * PI * t;
        glm::vec3 center(0, t * length * cos(curveAngle),
                         t * length * sin(curveAngle) * 0.3f);

        for (int s = 0; s < segments; s++) {
            float theta = (float)s / segments * 2.0f * PI;
            glm::vec3 pos = center + glm::vec3(cos(theta) * radius, 0, sin(theta) * radius);

            Vertex v;
            v.position = pos;
            v.normal = glm::normalize(glm::vec3(cos(theta), 0, sin(theta)));
            v.uv = glm::vec2((float)s / segments, t);
            v.color = gene.color;
            mesh.vertices.push_back(v);
        }
    }

    // Generate indices
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            int current = r * segments + s;
            int next = r * segments + (s + 1) % segments;
            int above = (r + 1) * segments + s;
            int aboveNext = (r + 1) * segments + (s + 1) % segments;

            mesh.indices.push_back(current);
            mesh.indices.push_back(above);
            mesh.indices.push_back(next);

            mesh.indices.push_back(next);
            mesh.indices.push_back(above);
            mesh.indices.push_back(aboveNext);
        }
    }

    mesh.attachPoint = gene.position;
    mesh.orientation = glm::lookAt(glm::vec3(0), gene.direction, glm::vec3(0, 1, 0));

    return mesh;
}

FeatureMeshGenerator::FeatureMesh FeatureMeshGenerator::generateAntler(
    const SurfaceFeatureGene& gene) {
    FeatureMesh mesh;

    // Main beam
    glm::vec3 start(0);
    glm::vec3 mainDir = glm::normalize(gene.direction);
    float mainLength = gene.size * 0.6f;
    float baseRadius = gene.size * 0.05f;

    // Recursive branching
    generateBranch(mesh, start, mainDir, mainLength, baseRadius,
                   gene.segments, 0);

    mesh.attachPoint = gene.position;
    return mesh;
}

void FeatureMeshGenerator::generateBranch(FeatureMesh& mesh, glm::vec3 start,
                                           glm::vec3 dir, float length,
                                           float radius, int branches, int depth) {
    if (depth > 4) return;  // Max recursion

    // Add this branch segment
    glm::vec3 end = start + dir * length;
    generateCurvedCone(mesh, length, radius, radius * 0.7f, 0.1f, 6);

    // Spawn branches
    if (branches > 0 && depth < 3) {
        float branchPoint = 0.4f + depth * 0.15f;  // Where to branch
        glm::vec3 branchStart = start + dir * (length * branchPoint);

        // Forward tine
        glm::vec3 forwardDir = glm::normalize(dir + glm::vec3(0.3f, 0.2f, 0));
        generateBranch(mesh, branchStart, forwardDir, length * 0.6f,
                      radius * 0.6f, branches - 1, depth + 1);

        // Back tine (if enough branches)
        if (branches > 2) {
            glm::vec3 backDir = glm::normalize(dir + glm::vec3(-0.2f, 0.3f, 0));
            generateBranch(mesh, branchStart, backDir, length * 0.5f,
                          radius * 0.5f, 0, depth + 1);
        }
    }
}

FeatureMeshGenerator::FeatureMesh FeatureMeshGenerator::generateShell(
    const SurfaceFeatureGene& gene) {
    FeatureMesh mesh;

    if (gene.type == SurfaceFeatureType::SHELL_DOME) {
        // Turtle-style dome shell
        generateDome(mesh, gene.size * 0.5f, gene.size * 0.3f * gene.curvature, 16);

        // Add scute pattern
        if (gene.segments > 0) {
            addScutePattern(mesh, gene.segments);
        }
    } else if (gene.type == SurfaceFeatureType::SHELL_SPIRAL) {
        // Snail-style spiral shell
        generateSpiral(mesh, gene.size, gene.curvature, 3.0f);  // 3 revolutions
    }

    mesh.attachPoint = gene.position;
    return mesh;
}
```

CHECKPOINT 3: Feature mesh generation working

============================================================================
PHASE 4: IMPLEMENT FEATURE EFFECTS (45+ minutes)
============================================================================

```cpp
// How features affect gameplay

class FeatureEffects {
public:
    void applyFeatureEffects(Creature& creature, const SurfaceFeatureLayout& features);

private:
    void applyDefensiveEffects(Creature& creature, const SurfaceFeatureGene& feature);
    void applyOffensiveEffects(Creature& creature, const SurfaceFeatureGene& feature);
    void applySocialEffects(Creature& creature, const SurfaceFeatureGene& feature);
};

void FeatureEffects::applyFeatureEffects(Creature& creature,
                                          const SurfaceFeatureLayout& features) {
    for (const auto& feature : features.headFeatures) {
        switch (feature.type) {
            case SurfaceFeatureType::HORN_PAIR:
            case SurfaceFeatureType::HORN_SINGLE:
                // Horns are weapons
                creature.addMeleeWeapon("horn", feature.size * 10.0f);  // Damage
                creature.addIntimidation(feature.size * 0.3f);
                break;

            case SurfaceFeatureType::ANTLER:
                // Antlers for display and combat
                creature.addMeleeWeapon("antler", feature.size * 8.0f);
                creature.addMateAttraction(feature.size * 0.5f);  // Larger = more attractive
                break;

            case SurfaceFeatureType::FRILL:
                // Display and intimidation
                creature.addIntimidation(feature.size * 0.5f);
                creature.addThermoregulation(feature.size * 0.2f);
                break;

            case SurfaceFeatureType::CREST:
                // Social signaling
                creature.addMateAttraction(feature.size * 0.3f);
                break;
        }
    }

    for (const auto& feature : features.bodyFeatures) {
        switch (feature.type) {
            case SurfaceFeatureType::SHELL_DOME:
            case SurfaceFeatureType::CARAPACE:
                // Shells provide armor but slow movement
                creature.addArmor(feature.size * 30.0f);
                creature.multiplySpeed(0.7f);
                break;

            case SurfaceFeatureType::ARMOR_PLATES:
                creature.addArmor(feature.size * 20.0f);
                creature.multiplySpeed(0.85f);
                break;

            case SurfaceFeatureType::SPINE_ROW:
            case SurfaceFeatureType::SPINE_ARRAY:
                // Spines deter attackers
                creature.addContactDamage(feature.count * 2.0f);
                creature.addDefenseRating(feature.count * 0.1f);
                break;

            case SurfaceFeatureType::MANE:
                // Manes intimidate and protect neck
                creature.addIntimidation(feature.size * 0.2f);
                creature.addNeckProtection(feature.size * 10.0f);
                break;

            case SurfaceFeatureType::FIN_SAIL:
                // Sail for thermoregulation
                creature.addThermoregulation(feature.size * 0.5f);
                creature.addMateAttraction(feature.size * 0.2f);
                break;

            case SurfaceFeatureType::HUMP:
                // Energy storage
                creature.addEnergyCapacity(feature.size * 50.0f);
                break;
        }
    }

    for (const auto& feature : features.tailFeatures) {
        switch (feature.type) {
            case SurfaceFeatureType::TAIL_CLUB:
                creature.addTailWeapon(feature.size * 15.0f);
                break;

            case SurfaceFeatureType::TAIL_FAN:
                creature.addMateAttraction(feature.size * 0.6f);
                break;

            case SurfaceFeatureType::TAIL_RATTLE:
                creature.addWarningSignal(0.5f);  // Warns predators
                break;
        }
    }
}

// Feature-based combat
float FeatureEffects::calculateFeatureAttackDamage(const Creature& attacker,
                                                    const std::string& weaponType) {
    float damage = 0.0f;

    if (weaponType == "horn_charge") {
        // Horns do more damage when charging
        float speed = glm::length(attacker.getVelocity());
        damage = attacker.getWeaponDamage("horn") * (1.0f + speed * 0.5f);
    } else if (weaponType == "antler_lock") {
        // Antler combat for dominance
        damage = attacker.getWeaponDamage("antler") * 0.5f;  // Non-lethal
    } else if (weaponType == "tail_swing") {
        damage = attacker.getWeaponDamage("tail") * 1.2f;
    }

    return damage;
}
```

CHECKPOINT 4: Feature effects working

============================================================================
PHASE 5: PROCEDURAL PATTERN GENERATION (30+ minutes)
============================================================================

```cpp
// Generate patterns on features

class FeaturePatternGenerator {
public:
    Texture2D generateHornRings(float ringCount, glm::vec3 baseColor, glm::vec3 ringColor);
    Texture2D generateShellSpiral(float coils, glm::vec3 color1, glm::vec3 color2);
    Texture2D generateScutePattern(int rows, int cols, glm::vec3 color);
    Texture2D generateSpineGradient(glm::vec3 baseColor, glm::vec3 tipColor);
};

Texture2D FeaturePatternGenerator::generateHornRings(float ringCount,
                                                      glm::vec3 baseColor,
                                                      glm::vec3 ringColor) {
    int width = 128, height = 64;
    std::vector<glm::vec4> pixels(width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float u = (float)x / width;
            float v = (float)y / height;

            // Rings along length (v axis)
            float ring = sin(v * ringCount * 2.0f * PI) * 0.5f + 0.5f;
            ring = pow(ring, 3.0f);  // Sharpen rings

            glm::vec3 color = glm::mix(baseColor, ringColor, ring * 0.3f);

            // Add wear at tip
            color = glm::mix(color, baseColor * 0.7f, v * 0.3f);

            pixels[y * width + x] = glm::vec4(color, 1.0f);
        }
    }

    return createTexture(pixels, width, height);
}

Texture2D FeaturePatternGenerator::generateScutePattern(int rows, int cols,
                                                         glm::vec3 color) {
    int size = 256;
    std::vector<glm::vec4> pixels(size * size);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float u = (float)x / size;
            float v = (float)y / size;

            // Hexagonal pattern for scutes
            float cellU = u * cols;
            float cellV = v * rows;

            // Offset every other row
            if ((int)cellV % 2 == 1) {
                cellU += 0.5f;
            }

            float cellX = fmod(cellU, 1.0f);
            float cellY = fmod(cellV, 1.0f);

            // Distance from cell center
            float dist = sqrt((cellX - 0.5f) * (cellX - 0.5f) +
                             (cellY - 0.5f) * (cellY - 0.5f));

            // Edge darkening
            float edge = smoothstep(0.3f, 0.5f, dist);
            glm::vec3 finalColor = color * (1.0f - edge * 0.4f);

            // Subtle variation per cell
            float cellId = floor(cellU) + floor(cellV) * cols;
            float variation = sin(cellId * 12.9898f) * 0.1f;
            finalColor *= (1.0f + variation);

            pixels[y * size + x] = glm::vec4(finalColor, 1.0f);
        }
    }

    return createTexture(pixels, size, size);
}
```

CHECKPOINT 5: Feature patterns generating

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SURFACE_FEATURES_DESIGN.md
- [ ] SurfaceFeatureType enum (30+ types)
- [ ] SurfaceFeatureGene struct
- [ ] Feature templates (horns, antlers, shells, etc.)
- [ ] FeatureMeshGenerator class
- [ ] Horn mesh generation
- [ ] Antler branching generation
- [ ] Shell/dome mesh generation
- [ ] Spine array generation
- [ ] FeatureEffects class
- [ ] Armor/weapon effects from features
- [ ] Pattern textures for features
- [ ] Integration with creature rendering

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 6: Skin Patterns & Coloration (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Implement rich procedural skin patterns - spots, stripes, camouflage, warning colors, iridescence.

============================================================================
PHASE 1: DESIGN PATTERN SYSTEM (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research pattern generation:
- Web search "reaction-diffusion patterns procedural"
- Web search "Turing patterns animal skin"
- Web search "procedural camouflage generation"
- Document pattern algorithms

SUB-AGENT 1B - Design coloration genetics:
- List all pattern types
- Document color inheritance
- Design gene representation
- Plan shader implementation

CHECKPOINT 1: Create docs/SKIN_PATTERNS_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT COLORATION GENES (1+ hour)
============================================================================

```cpp
// src/entities/genetics/ColorationGenes.h

enum class PatternType {
    SOLID,              // Single color
    SPOTTED,            // Random spots (leopard)
    ROSETTES,           // Ring spots (jaguar)
    STRIPED,            // Parallel stripes (zebra, tiger)
    BANDED,             // Horizontal bands
    MOTTLED,            // Random blotches
    GRADIENT,           // Color fade
    COUNTERSHADED,      // Dark top, light bottom
    DISRUPTIVE,         // Irregular patches
    DAPPLED,            // Light spots (fawn)

    // Complex patterns
    RETICULATED,        // Net-like (giraffe)
    MARBLED,            // Swirled colors
    TORTOISESHELL,      // Patches of 3 colors
    TABBY,              // Striped with agouti
    PIEBALD,            // Large irregular patches

    // Special
    IRIDESCENT,         // Color-shifting
    BIOLUMINESCENT,     // Glowing spots
    TRANSPARENT,        // See-through (jellyfish)
    METALLIC            // Shiny (beetles)
};

struct ColorationGenes {
    // Base colors
    glm::vec3 primaryColor;     // Main body color
    glm::vec3 secondaryColor;   // Pattern color
    glm::vec3 tertiaryColor;    // Accent color (optional)
    glm::vec3 underbellyColor;  // Underside color

    // Pattern
    PatternType pattern;
    float patternScale;         // Size of pattern elements
    float patternContrast;      // How distinct pattern is
    float patternDensity;       // How many pattern elements
    float patternIrregularity;  // How random/organic

    // Pattern modifiers
    bool hasCountershading;     // Darker on top
    float countershadeStrength;
    bool hasEyeSpots;           // Fake eyes for defense
    int eyeSpotCount;
    glm::vec3 eyeSpotColor;

    // Surface properties
    float roughness;            // Matte to shiny
    float metallic;             // Metallic sheen
    float iridescence;          // Color shift with angle
    float translucency;         // Light pass-through
    float subsurfaceScattering; // Skin-like light scatter

    // Special features
    bool hasBioluminescence;
    glm::vec3 glowColor;
    float glowIntensity;
    std::vector<glm::vec2> glowSpots;  // Where glow occurs
};

// Inheritance helpers
ColorationGenes crossover(const ColorationGenes& parent1,
                          const ColorationGenes& parent2,
                          float mutationRate) {
    ColorationGenes child;

    // Colors blend with some randomness
    child.primaryColor = glm::mix(parent1.primaryColor, parent2.primaryColor,
                                   randomFloat(0.3f, 0.7f));
    child.secondaryColor = glm::mix(parent1.secondaryColor, parent2.secondaryColor,
                                     randomFloat(0.3f, 0.7f));

    // Pattern inherited from one parent (discrete trait)
    child.pattern = (randomFloat() > 0.5f) ? parent1.pattern : parent2.pattern;

    // Pattern properties average with variation
    child.patternScale = (parent1.patternScale + parent2.patternScale) * 0.5f;
    child.patternContrast = (parent1.patternContrast + parent2.patternContrast) * 0.5f;

    // Mutations
    if (randomFloat() < mutationRate) {
        // Random color mutation
        int channel = randomInt(0, 2);
        child.primaryColor[channel] += randomFloat(-0.1f, 0.1f);
        child.primaryColor = glm::clamp(child.primaryColor, glm::vec3(0), glm::vec3(1));
    }

    if (randomFloat() < mutationRate * 0.5f) {
        // Rare pattern mutation
        child.pattern = static_cast<PatternType>(randomInt(0, (int)PatternType::METALLIC));
    }

    return child;
}
```

CHECKPOINT 2: Coloration genes defined

============================================================================
PHASE 3: IMPLEMENT PATTERN GENERATION (1+ hour)
============================================================================

```cpp
// src/graphics/procedural/PatternGenerator.h

class PatternGenerator {
public:
    Texture2D generate(const ColorationGenes& genes, int resolution = 512);

private:
    // Pattern algorithms
    void generateSpots(std::vector<glm::vec4>& pixels, int w, int h,
                       const ColorationGenes& genes);
    void generateStripes(std::vector<glm::vec4>& pixels, int w, int h,
                         const ColorationGenes& genes);
    void generateReticulated(std::vector<glm::vec4>& pixels, int w, int h,
                             const ColorationGenes& genes);
    void generateMarbled(std::vector<glm::vec4>& pixels, int w, int h,
                         const ColorationGenes& genes);

    // Turing patterns (reaction-diffusion)
    void generateTuringPattern(std::vector<glm::vec4>& pixels, int w, int h,
                               float activatorRate, float inhibitorRate);

    // Post-processing
    void applyCountershading(std::vector<glm::vec4>& pixels, int w, int h,
                             float strength);
    void addEyeSpots(std::vector<glm::vec4>& pixels, int w, int h,
                     const std::vector<glm::vec2>& positions, glm::vec3 color);
};

void PatternGenerator::generateSpots(std::vector<glm::vec4>& pixels, int w, int h,
                                       const ColorationGenes& genes) {
    // Fill with primary color
    for (auto& p : pixels) {
        p = glm::vec4(genes.primaryColor, 1.0f);
    }

    // Generate spot positions using Poisson disk sampling
    std::vector<glm::vec2> spotCenters;
    float minDist = 1.0f / (genes.patternDensity * 10.0f);
    poissonDiskSample(spotCenters, w, h, minDist * w);

    // Draw spots
    for (const auto& center : spotCenters) {
        float spotSize = genes.patternScale * 0.05f * w;
        spotSize *= (1.0f + randomFloat(-0.3f, 0.3f) * genes.patternIrregularity);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                float dist = glm::distance(glm::vec2(x, y), center * glm::vec2(w, h));

                if (dist < spotSize) {
                    float blend = smoothstep(spotSize, spotSize * 0.7f, dist);
                    blend *= genes.patternContrast;

                    pixels[y * w + x] = glm::vec4(
                        glm::mix(genes.primaryColor, genes.secondaryColor, blend),
                        1.0f
                    );
                }
            }
        }
    }
}

void PatternGenerator::generateStripes(std::vector<glm::vec4>& pixels, int w, int h,
                                         const ColorationGenes& genes) {
    float stripeFreq = genes.patternDensity * 20.0f;
    float stripeWidth = 1.0f / stripeFreq;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float u = (float)x / w;
            float v = (float)y / h;

            // Vertical stripes with wobble
            float wobble = sin(v * 10.0f + u * 5.0f) * genes.patternIrregularity * 0.1f;
            float stripe = sin((u + wobble) * stripeFreq * 2.0f * PI);

            // Sharpen stripe edges
            stripe = smoothstep(-0.3f, 0.3f, stripe);
            stripe *= genes.patternContrast;

            glm::vec3 color = glm::mix(genes.primaryColor, genes.secondaryColor, stripe);
            pixels[y * w + x] = glm::vec4(color, 1.0f);
        }
    }
}

void PatternGenerator::generateReticulated(std::vector<glm::vec4>& pixels, int w, int h,
                                            const ColorationGenes& genes) {
    // Voronoi cells for giraffe-like pattern
    std::vector<glm::vec2> cellCenters;
    int numCells = (int)(genes.patternDensity * 50.0f);
    for (int i = 0; i < numCells; i++) {
        cellCenters.push_back(glm::vec2(randomFloat(), randomFloat()));
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            glm::vec2 p((float)x / w, (float)y / h);

            // Find nearest cell center
            float minDist = 999.0f;
            float secondDist = 999.0f;
            for (const auto& c : cellCenters) {
                float d = glm::distance(p, c);
                if (d < minDist) {
                    secondDist = minDist;
                    minDist = d;
                } else if (d < secondDist) {
                    secondDist = d;
                }
            }

            // Edge detection (difference between nearest and second nearest)
            float edge = secondDist - minDist;
            float borderWidth = genes.patternScale * 0.01f;
            float border = smoothstep(0.0f, borderWidth, edge);

            // Primary color in cells, secondary on borders
            glm::vec3 color = glm::mix(genes.secondaryColor, genes.primaryColor,
                                        border * genes.patternContrast);

            pixels[y * w + x] = glm::vec4(color, 1.0f);
        }
    }
}

// Turing pattern (creates spots/stripes organically)
void PatternGenerator::generateTuringPattern(std::vector<glm::vec4>& pixels, int w, int h,
                                              float activatorRate, float inhibitorRate) {
    // Activator-inhibitor model
    std::vector<float> activator(w * h, 0.0f);
    std::vector<float> inhibitor(w * h, 0.0f);

    // Random initial conditions
    for (int i = 0; i < w * h; i++) {
        activator[i] = randomFloat(-0.1f, 0.1f);
        inhibitor[i] = randomFloat(-0.1f, 0.1f);
    }

    // Simulate reaction-diffusion
    for (int iter = 0; iter < 5000; iter++) {
        std::vector<float> newA(w * h), newI(w * h);

        for (int y = 1; y < h - 1; y++) {
            for (int x = 1; x < w - 1; x++) {
                int idx = y * w + x;

                // Laplacian (diffusion)
                float lapA = activator[idx - 1] + activator[idx + 1] +
                             activator[idx - w] + activator[idx + w] -
                             4.0f * activator[idx];
                float lapI = inhibitor[idx - 1] + inhibitor[idx + 1] +
                             inhibitor[idx - w] + inhibitor[idx + w] -
                             4.0f * inhibitor[idx];

                // Reaction
                float a = activator[idx];
                float i = inhibitor[idx];
                float reaction = a * a / (1.0f + i);

                newA[idx] = a + (activatorRate * lapA + reaction - a) * 0.1f;
                newI[idx] = i + (inhibitorRate * lapI + reaction - i) * 0.1f;
            }
        }

        activator = newA;
        inhibitor = newI;
    }

    // Convert to color
    for (int i = 0; i < w * h; i++) {
        float value = glm::clamp(activator[i], 0.0f, 1.0f);
        pixels[i] = glm::vec4(value, value, value, 1.0f);
    }
}
```

CHECKPOINT 3: Pattern generation algorithms working

============================================================================
PHASE 4: IMPLEMENT PATTERN SHADER (45+ minutes)
============================================================================

```hlsl
// shaders/hlsl/creature_pattern.hlsl

// Pattern parameters from genes
cbuffer PatternParams : register(b2) {
    float4 primaryColor;
    float4 secondaryColor;
    float4 tertiaryColor;
    float patternScale;
    float patternContrast;
    float iridescence;
    float metallic;
    float roughness;
    float subsurface;
    int patternType;
    float time;
};

Texture2D patternTexture : register(t2);
SamplerState patternSampler : register(s2);

// Procedural pattern functions
float spots(float2 uv, float scale, float irregularity) {
    float2 cell = floor(uv * scale);
    float2 local = frac(uv * scale);

    // Random offset per cell
    float2 offset = hash22(cell) * irregularity;
    float dist = length(local - 0.5 + offset * 0.3);

    return smoothstep(0.3, 0.2, dist);
}

float stripes(float2 uv, float scale, float wobble) {
    float wave = sin(uv.y * 10.0 + uv.x * 5.0) * wobble;
    return sin((uv.x + wave) * scale * 6.28318) * 0.5 + 0.5;
}

float3 applyIridescence(float3 baseColor, float3 normal, float3 viewDir, float strength) {
    float fresnel = pow(1.0 - saturate(dot(normal, viewDir)), 3.0);

    // Hue shift based on viewing angle
    float hueShift = fresnel * strength * 2.0;
    float3 hsl = rgbToHsl(baseColor);
    hsl.x = frac(hsl.x + hueShift);
    hsl.y = saturate(hsl.y + fresnel * 0.3);  // More saturated at edges

    return hslToRgb(hsl);
}

float4 PSCreaturePattern(PSInput input) : SV_Target {
    float2 uv = input.texCoord;
    float3 normal = normalize(input.normal);
    float3 viewDir = normalize(cameraPos - input.worldPos);

    // Sample pattern texture
    float4 pattern = patternTexture.Sample(patternSampler, uv);

    // Blend colors based on pattern
    float3 baseColor = lerp(primaryColor.rgb, secondaryColor.rgb,
                            pattern.r * patternContrast);

    // Apply countershading (darker on top based on world normal)
    float countershade = saturate(dot(normal, float3(0, 1, 0)) * 0.5 + 0.5);
    baseColor *= lerp(0.7, 1.0, countershade);

    // Apply iridescence
    if (iridescence > 0.01) {
        baseColor = applyIridescence(baseColor, normal, viewDir, iridescence);
    }

    // Subsurface scattering for translucent creatures
    float3 subsurfaceColor = float3(0, 0, 0);
    if (subsurface > 0.01) {
        float3 lightDir = normalize(lightPos - input.worldPos);
        float backlight = saturate(dot(-lightDir, viewDir));
        subsurfaceColor = primaryColor.rgb * backlight * subsurface * 0.5;
    }

    // Final color with lighting
    float3 finalColor = baseColor + subsurfaceColor;

    return float4(finalColor, 1.0);
}
```

CHECKPOINT 4: Pattern shader working

============================================================================
PHASE 5: ENVIRONMENTAL ADAPTATION (30+ minutes)
============================================================================

```cpp
// Colors adapt to environment over generations

class ColorAdaptation {
public:
    // Camouflage fitness
    float calculateCamouflageFitness(const Creature& creature, const Environment& env);

    // Warning color effectiveness
    float calculateWarningEffectiveness(const Creature& creature);

    // Mate attraction (sexual selection)
    float calculateMateAttraction(const Creature& creature);
};

float ColorAdaptation::calculateCamouflageFitness(const Creature& creature,
                                                   const Environment& env) {
    // Sample environment colors where creature lives
    std::vector<glm::vec3> envColors = env.sampleColorsAt(creature.getHomeRange());

    // Calculate how well creature matches
    float matchScore = 0.0f;
    glm::vec3 creatureColor = creature.getAverageColor();

    for (const auto& envColor : envColors) {
        float dist = glm::distance(creatureColor, envColor);
        matchScore += 1.0f / (1.0f + dist * 5.0f);
    }

    return matchScore / envColors.size();
}

float ColorAdaptation::calculateWarningEffectiveness(const Creature& creature) {
    // Bright, high-contrast colors are more effective warnings
    glm::vec3 color = creature.getPrimaryColor();

    float brightness = (color.r + color.g + color.b) / 3.0f;
    float saturation = glm::length(color - glm::vec3(brightness));

    // Red and yellow are best warning colors
    float warningHue = 0.0f;
    if (color.r > color.g && color.r > color.b) {
        warningHue = 1.0f;  // Red
    } else if (color.r > 0.5f && color.g > 0.5f && color.b < 0.3f) {
        warningHue = 0.8f;  // Yellow
    }

    return saturation * warningHue * creature.getPatternContrast();
}

// Apply selection pressure
void applyColorSelection(Population& population, const Environment& env) {
    for (auto& creature : population) {
        float fitness = 1.0f;

        if (creature.isPrey()) {
            // Prey benefits from camouflage
            fitness *= calculateCamouflageFitness(creature, env);
        }

        if (creature.isToxic()) {
            // Toxic creatures benefit from warning colors
            fitness *= calculateWarningEffectiveness(creature);
        }

        if (creature.isMale()) {
            // Males may benefit from bright colors (sexual selection)
            fitness *= 0.7f + calculateMateAttraction(creature) * 0.6f;
        }

        creature.setColorFitness(fitness);
    }
}
```

CHECKPOINT 5: Color adaptation working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SKIN_PATTERNS_DESIGN.md
- [ ] PatternType enum (20+ types)
- [ ] ColorationGenes struct
- [ ] Pattern inheritance/mutation
- [ ] PatternGenerator class
- [ ] Spot pattern generation
- [ ] Stripe pattern generation
- [ ] Reticulated pattern (Voronoi)
- [ ] Turing reaction-diffusion
- [ ] Pattern shader (HLSL)
- [ ] Iridescence effect
- [ ] Subsurface scattering
- [ ] Countershading
- [ ] Camouflage fitness calculation
- [ ] Warning color system
- [ ] Sexual selection for colors

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 7: Aquatic Creature Variety (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Create diverse aquatic creatures - fish, sharks, whales, rays, eels, bizarre deep-sea life.

============================================================================
PHASE 1: DESIGN AQUATIC BODY PLANS (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research aquatic body forms:
- Web search "fish body shape hydrodynamics"
- Web search "deep sea creature adaptations"
- Web search "procedural fish generation"
- Document body plan variations

SUB-AGENT 1B - Analyze current aquatic code:
- Read existing aquatic creature code
- Read swimming locomotion
- Document fin systems
- Identify extension points

CHECKPOINT 1: Create docs/AQUATIC_VARIETY_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT AQUATIC BODY TYPES (1+ hour)
============================================================================

```cpp
// src/entities/genetics/AquaticBodyGenes.h

enum class AquaticBodyType {
    // Standard fish shapes
    FUSIFORM,           // Torpedo-shaped (tuna, shark)
    LATERALLY_COMPRESSED, // Flat side-to-side (angelfish, sunfish)
    DEPRESSED,          // Flat top-to-bottom (ray, flounder)
    ELONGATED,          // Long and thin (eel, pipefish)
    GLOBIFORM,          // Round (pufferfish)

    // Specialized shapes
    ANGUILLIFORM,       // Snake-like (eel, moray)
    SAGITTIFORM,        // Arrow-shaped (pike)
    TAENIFORM,          // Ribbon-like (oarfish)
    FILIFORM,           // Thread-like (hagfish)

    // Invertebrate shapes
    BELL,               // Jellyfish
    RADIAL,             // Starfish, urchin
    CEPHALOPOD,         // Octopus, squid
    CRUSTACEAN,         // Crab, lobster
    VERMIFORM           // Worm-like
};

struct AquaticBodyGenes {
    AquaticBodyType bodyType;

    // Body proportions
    float bodyLength;           // 0.5 - 5.0
    float bodyDepth;            // Height relative to length
    float bodyWidth;            // Thickness
    float headProportion;       // Head size relative to body
    float tailProportion;       // Tail length relative to body

    // Hydrodynamics
    float streamlining;         // 0-1, affects speed vs maneuverability
    float crossSection;         // Drag factor

    // Fin layout
    FinLayout fins;
    bool hasDorsalFin;
    bool hasAnalFin;
    bool hasPelvicFins;
    bool hasPectoralFins;
    bool hasCaudalFin;

    // Special features
    bool hasBarbels;            // Whisker-like sensors
    bool hasElectroreception;   // Shark/ray sense
    bool hasBioluminescence;
    float jawSize;              // Relative jaw size
    bool hasLure;               // Anglerfish-style
};

struct FinLayout {
    // Dorsal fin(s)
    int dorsalFinCount;         // 1-3
    float dorsalFinHeight;
    float dorsalFinLength;
    float dorsalFinPosition;    // Along body (0-1)

    // Pectoral fins
    float pectoralFinSize;
    float pectoralFinAngle;
    bool pectoralAsWings;       // Ray-style

    // Caudal (tail) fin
    CaudalFinType caudalType;
    float caudalFinSize;
    float caudalFinFork;        // How forked (0-1)

    // Pelvic/Anal
    float pelvicFinSize;
    float analFinSize;
};

enum class CaudalFinType {
    HOMOCERCAL,         // Symmetric (most fish)
    HETEROCERCAL,       // Asymmetric upper lobe (sharks)
    PROTOCERCAL,        // Simple rounded (primitive)
    DIPHYCERCAL,        // Double lobe
    LUNATE,             // Crescent moon (fast swimmers)
    FORKED,             // Deep fork (tuna)
    TRUNCATE,           // Squared off
    ROUNDED,            // Round (slow movers)
    POINTED,            // Comes to point
    FILAMENTOUS         // Long trailing threads
};

// Body templates
AquaticBodyGenes createSharkBody() {
    return {
        .bodyType = AquaticBodyType::FUSIFORM,
        .bodyLength = 3.0f,
        .bodyDepth = 0.3f,
        .bodyWidth = 0.25f,
        .headProportion = 0.25f,
        .tailProportion = 0.3f,
        .streamlining = 0.9f,
        .crossSection = 0.7f,
        .hasDorsalFin = true,
        .hasAnalFin = true,
        .hasPelvicFins = true,
        .hasPectoralFins = true,
        .hasCaudalFin = true,
        .hasElectroreception = true,
        .jawSize = 1.5f,
        .fins = {
            .dorsalFinCount = 2,
            .dorsalFinHeight = 0.5f,
            .caudalType = CaudalFinType::HETEROCERCAL,
            .caudalFinFork = 0.3f
        }
    };
}

AquaticBodyGenes createRayBody() {
    return {
        .bodyType = AquaticBodyType::DEPRESSED,
        .bodyLength = 1.5f,
        .bodyDepth = 0.1f,
        .bodyWidth = 2.0f,
        .headProportion = 0.15f,
        .tailProportion = 0.5f,
        .streamlining = 0.5f,
        .hasElectroreception = true,
        .fins = {
            .pectoralAsWings = true,
            .pectoralFinSize = 2.0f,
            .caudalType = CaudalFinType::FILAMENTOUS
        }
    };
}

AquaticBodyGenes createAnglerfish() {
    return {
        .bodyType = AquaticBodyType::GLOBIFORM,
        .bodyLength = 0.8f,
        .bodyDepth = 1.2f,
        .headProportion = 0.6f,  // Huge head
        .hasBioluminescence = true,
        .jawSize = 2.5f,  // Massive jaw
        .hasLure = true
    };
}
```

CHECKPOINT 2: Aquatic body types defined

============================================================================
PHASE 3: IMPLEMENT AQUATIC MESH GENERATION (1+ hour)
============================================================================

```cpp
// src/graphics/procedural/AquaticMeshGenerator.h

class AquaticMeshGenerator {
public:
    GeneratedMesh generateFish(const AquaticBodyGenes& genes);
    GeneratedMesh generateRay(const AquaticBodyGenes& genes);
    GeneratedMesh generateJellyfish(const AquaticBodyGenes& genes);
    GeneratedMesh generateEel(const AquaticBodyGenes& genes);

private:
    void generateFusiformBody(GeneratedMesh& mesh, const AquaticBodyGenes& genes);
    void generateCompressedBody(GeneratedMesh& mesh, const AquaticBodyGenes& genes);
    void generateFinMesh(GeneratedMesh& mesh, FinType type, const FinLayout& layout);
    void generateTailFin(GeneratedMesh& mesh, CaudalFinType type, float size, float fork);
};

void AquaticMeshGenerator::generateFusiformBody(GeneratedMesh& mesh,
                                                  const AquaticBodyGenes& genes) {
    // Torpedo-shaped body using revolution surface
    int rings = 24;
    int segments = 16;

    for (int r = 0; r <= rings; r++) {
        float t = (float)r / rings;  // 0 to 1 along body

        // Body profile - widest in middle, tapered at ends
        float profile = sin(t * PI);  // Basic oval
        profile *= (1.0f + genes.streamlining * (cos(t * 2 * PI) * 0.2f));

        // Cross-section shape
        float radiusX = genes.bodyWidth * 0.5f * profile;
        float radiusY = genes.bodyDepth * 0.5f * profile;

        float z = (t - 0.5f) * genes.bodyLength;

        for (int s = 0; s < segments; s++) {
            float theta = (float)s / segments * 2.0f * PI;

            // Elliptical cross-section
            float x = cos(theta) * radiusX;
            float y = sin(theta) * radiusY;

            Vertex v;
            v.position = glm::vec3(x, y, z);
            v.normal = glm::normalize(glm::vec3(x / (radiusX * radiusX),
                                                 y / (radiusY * radiusY), 0));
            v.uv = glm::vec2((float)s / segments, t);
            mesh.vertices.push_back(v);
        }
    }

    // Generate indices for quad strips
    generateQuadStripIndices(mesh, rings, segments);
}

void AquaticMeshGenerator::generateTailFin(GeneratedMesh& mesh, CaudalFinType type,
                                            float size, float fork) {
    switch (type) {
        case CaudalFinType::HOMOCERCAL:
            // Symmetric tail
            generateSymmetricTail(mesh, size, fork);
            break;

        case CaudalFinType::HETEROCERCAL:
            // Shark-style - upper lobe larger
            generateAsymmetricTail(mesh, size, fork, 1.5f);  // Upper 50% larger
            break;

        case CaudalFinType::LUNATE:
            // Crescent moon - highly efficient
            generateLunateTail(mesh, size);
            break;

        case CaudalFinType::FORKED:
            // Deep V fork
            generateForkedTail(mesh, size, fork);
            break;

        case CaudalFinType::FILAMENTOUS:
            // Long trailing fin rays
            generateFilamentousTail(mesh, size);
            break;
    }
}

void AquaticMeshGenerator::generateForkedTail(GeneratedMesh& mesh, float size, float fork) {
    // Two lobes with gap between
    float lobeAngle = fork * 45.0f;  // How spread apart

    // Upper lobe
    std::vector<glm::vec3> upperProfile = {
        {0, 0, 0},
        {size * 0.3f, size * 0.2f, 0},
        {size * 0.6f, size * 0.5f, 0},
        {size, size * (0.3f + fork * 0.5f), 0}
    };

    // Lower lobe (mirror)
    std::vector<glm::vec3> lowerProfile = {
        {0, 0, 0},
        {size * 0.3f, -size * 0.2f, 0},
        {size * 0.6f, -size * 0.5f, 0},
        {size, -size * (0.3f + fork * 0.5f), 0}
    };

    // Extrude profiles with thickness
    extrudeFinProfile(mesh, upperProfile, 0.02f);
    extrudeFinProfile(mesh, lowerProfile, 0.02f);
}

GeneratedMesh AquaticMeshGenerator::generateRay(const AquaticBodyGenes& genes) {
    GeneratedMesh mesh;

    // Flat diamond-shaped body with wing-like pectoral fins
    int segments = 32;

    // Body is nearly flat disc
    for (int s = 0; s < segments; s++) {
        float angle = (float)s / segments * 2.0f * PI;

        // Diamond shape - wider than long
        float r = genes.bodyWidth * 0.5f *
                 (1.0f + 0.3f * cos(2.0f * angle));  // Elongated on sides

        float x = cos(angle) * r;
        float z = sin(angle) * r * (genes.bodyLength / genes.bodyWidth);
        float y = genes.bodyDepth * 0.1f * sin(angle * 2.0f);  // Slight curve

        Vertex v;
        v.position = glm::vec3(x, y, z);
        v.normal = glm::vec3(0, 1, 0);
        mesh.vertices.push_back(v);
    }

    // Whip-like tail
    int tailSegments = 16;
    for (int t = 0; t < tailSegments; t++) {
        float tailT = (float)t / tailSegments;
        float tailRadius = 0.05f * (1.0f - tailT);
        float z = genes.bodyLength * 0.5f + tailT * genes.tailProportion * genes.bodyLength;

        Vertex v;
        v.position = glm::vec3(0, sin(tailT * 3.0f) * 0.1f, z);
        v.normal = glm::vec3(0, 1, 0);
        mesh.vertices.push_back(v);
    }

    return mesh;
}

GeneratedMesh AquaticMeshGenerator::generateJellyfish(const AquaticBodyGenes& genes) {
    GeneratedMesh mesh;

    // Bell-shaped body
    int rings = 16;
    int segments = 24;

    for (int r = 0; r <= rings; r++) {
        float t = (float)r / rings;

        // Bell profile - dome on top, hollow underneath
        float radius = genes.bodyWidth * 0.5f * sin(t * PI * 0.9f);
        float height = genes.bodyDepth * cos(t * PI * 0.7f);

        for (int s = 0; s < segments; s++) {
            float theta = (float)s / segments * 2.0f * PI;

            Vertex v;
            v.position = glm::vec3(cos(theta) * radius, height, sin(theta) * radius);
            v.normal = glm::normalize(v.position);
            v.uv = glm::vec2((float)s / segments, t);
            mesh.vertices.push_back(v);
        }
    }

    // Tentacles added as separate meshes (see LimbGenerator)

    return mesh;
}
```

CHECKPOINT 3: Aquatic mesh generation working

============================================================================
PHASE 4: IMPLEMENT SWIMMING ANIMATION VARIETY (45+ minutes)
============================================================================

```cpp
// Different swimming styles

class SwimmingAnimator {
public:
    void animateCarangiform(Skeleton& skeleton, float phase, float speed);  // Body wave
    void animateThunniform(Skeleton& skeleton, float phase, float speed);   // Tail only
    void animateAnguilliform(Skeleton& skeleton, float phase, float speed); // Full body wave
    void animateRajiform(Skeleton& skeleton, float phase, float speed);     // Wing flapping
    void animateLavriform(Skeleton& skeleton, float phase, float speed);    // Pectoral rowing
    void animateJellyfishPulse(DynamicMesh& mesh, float phase);
};

void SwimmingAnimator::animateCarangiform(Skeleton& skeleton, float phase, float speed) {
    // Most common - wave starts at middle, increases toward tail
    int spineCount = skeleton.getSpineBoneCount();

    for (int i = 0; i < spineCount; i++) {
        float t = (float)i / spineCount;

        // Wave amplitude increases toward tail
        float amplitude = t * t * 0.3f;

        // Wave travels backward
        float bonePhase = phase - t * 1.5f;
        float bend = sin(bonePhase * 2.0f * PI) * amplitude * speed;

        skeleton.rotateBoneLocal(i, glm::angleAxis(bend, glm::vec3(0, 1, 0)));
    }
}

void SwimmingAnimator::animateThunniform(Skeleton& skeleton, float phase, float speed) {
    // Tuna/shark style - only tail region moves
    int spineCount = skeleton.getSpineBoneCount();
    int tailStart = spineCount * 2 / 3;  // Last third is tail

    // Rigid front body
    for (int i = 0; i < tailStart; i++) {
        skeleton.rotateBoneLocal(i, glm::quat(1, 0, 0, 0));  // No rotation
    }

    // Flexible tail
    for (int i = tailStart; i < spineCount; i++) {
        float t = (float)(i - tailStart) / (spineCount - tailStart);
        float amplitude = t * 0.4f;
        float bend = sin((phase - t) * 2.0f * PI) * amplitude * speed;

        skeleton.rotateBoneLocal(i, glm::angleAxis(bend, glm::vec3(0, 1, 0)));
    }
}

void SwimmingAnimator::animateAnguilliform(Skeleton& skeleton, float phase, float speed) {
    // Eel style - entire body is one continuous wave
    int spineCount = skeleton.getSpineBoneCount();

    float waveLength = 1.5f;  // Waves visible on body
    float amplitude = 0.2f;

    for (int i = 0; i < spineCount; i++) {
        float t = (float)i / spineCount;
        float bonePhase = phase - t * waveLength;

        // Consistent amplitude throughout
        float bend = sin(bonePhase * 2.0f * PI) * amplitude * speed;

        skeleton.rotateBoneLocal(i, glm::angleAxis(bend, glm::vec3(0, 1, 0)));
    }
}

void SwimmingAnimator::animateRajiform(Skeleton& skeleton, float phase, float speed) {
    // Ray/skate style - undulating pectoral fins
    int leftWing = skeleton.getBoneIndex("pectoral_left_0");
    int rightWing = skeleton.getBoneIndex("pectoral_right_0");
    int wingBones = 8;  // Bones per wing

    for (int i = 0; i < wingBones; i++) {
        float t = (float)i / wingBones;
        float wavePhase = phase - t * 0.8f;

        // Wave travels from center to wing tips
        float amplitude = 0.5f * (0.3f + t * 0.7f);
        float bend = sin(wavePhase * 2.0f * PI) * amplitude * speed;

        skeleton.rotateBoneLocal(leftWing + i, glm::angleAxis(bend, glm::vec3(0, 0, 1)));
        skeleton.rotateBoneLocal(rightWing + i, glm::angleAxis(-bend, glm::vec3(0, 0, 1)));
    }
}

void SwimmingAnimator::animateJellyfishPulse(DynamicMesh& mesh, float phase) {
    // Bell contracts and expands
    float contraction = (sin(phase * 2.0f * PI) + 1.0f) * 0.5f;

    for (Vertex& v : mesh.vertices) {
        // Only affect bell rim (bottom half)
        if (v.position.y < 0) {
            float rimFactor = -v.position.y / mesh.getMaxY();

            // Contract horizontally
            float squeeze = 1.0f - contraction * 0.3f * rimFactor;
            v.position.x = v.restPosition.x * squeeze;
            v.position.z = v.restPosition.z * squeeze;

            // Curve upward during contraction
            v.position.y = v.restPosition.y - contraction * 0.2f * rimFactor;
        }
    }

    mesh.recalculateNormals();
}
```

CHECKPOINT 4: Swimming animation variety working

============================================================================
PHASE 5: DEEP SEA ADAPTATIONS (30+ minutes)
============================================================================

```cpp
// Deep sea creature features

struct DeepSeaAdaptations {
    bool hasLargeEyes;          // For low light
    bool hasTubeEyes;           // Barreleye fish
    bool hasNoEyes;             // Blind cave fish
    bool hasBioluminescence;
    BiolumPattern biolumPattern;
    bool hasTransparentHead;    // Barreleye
    bool hasExpandableStomach;  // Can eat larger prey
    bool hasHingedJaw;          // Extreme gape
    float jawGape;              // How wide jaw opens
    bool hasLure;               // Anglerfish
    float lureBrightness;
    bool hasPhotophores;        // Light-producing organs
    std::vector<glm::vec3> photophorePositions;
};

enum class BiolumPattern {
    NONE,
    SCATTERED_SPOTS,    // Random glowing spots
    VENTRAL_COUNTERSHADE, // Underside to match light from above
    LURE,               // Single glowing lure
    RUNNING_LIGHTS,     // Rows of lights
    FLASHING,           // Blink patterns
    FULL_BODY           // Entire body glows
};

class DeepSeaCreatureGenerator {
public:
    void applyDeepSeaAdaptations(Creature& creature, float depth);

private:
    void generateAnglerfish(Creature& creature);
    void generateViperfish(Creature& creature);
    void generateBarreleye(Creature& creature);
    void generateGulperEel(Creature& creature);
};

void DeepSeaCreatureGenerator::applyDeepSeaAdaptations(Creature& creature, float depth) {
    if (depth < 200.0f) return;  // Not deep enough

    auto& genes = creature.getGenome();

    // Pressure adaptations
    genes.bodyGenes.bodyWidth *= 1.2f;  // Wider to resist pressure

    // Light adaptations
    if (depth > 1000.0f) {
        // Midnight zone - no light
        if (randomFloat() > 0.3f) {
            genes.deepSea.hasLargeEyes = true;
            genes.eyeSize *= 2.0f;
        } else {
            genes.deepSea.hasNoEyes = true;
            genes.eyeSize = 0.0f;
        }

        // Bioluminescence common
        genes.deepSea.hasBioluminescence = true;
        genes.deepSea.biolumPattern = randomChoice({
            BiolumPattern::SCATTERED_SPOTS,
            BiolumPattern::VENTRAL_COUNTERSHADE,
            BiolumPattern::RUNNING_LIGHTS
        });
    }

    // Food scarcity adaptations
    if (depth > 2000.0f) {
        genes.deepSea.hasExpandableStomach = true;
        genes.deepSea.hasHingedJaw = true;
        genes.deepSea.jawGape = 1.5f + randomFloat() * 1.0f;
    }

    // Regenerate mesh with adaptations
    creature.regenerateMesh();
}

void DeepSeaCreatureGenerator::generateAnglerfish(Creature& creature) {
    auto& genes = creature.getGenome();

    genes.aquatic.bodyType = AquaticBodyType::GLOBIFORM;
    genes.aquatic.headProportion = 0.6f;  // Huge head
    genes.aquatic.jawSize = 2.0f;

    genes.deepSea.hasLure = true;
    genes.deepSea.lureBrightness = 0.8f;
    genes.deepSea.hasBioluminescence = true;
    genes.deepSea.biolumPattern = BiolumPattern::LURE;

    // Huge mouth with teeth
    genes.features.add({
        .type = SurfaceFeatureType::TEETH_ARRAY,
        .size = 0.3f,
        .count = 20,
        .position = {0, 0, 0.9f}  // Front of head
    });
}

void DeepSeaCreatureGenerator::generateGulperEel(Creature& creature) {
    auto& genes = creature.getGenome();

    genes.aquatic.bodyType = AquaticBodyType::ELONGATED;
    genes.aquatic.bodyLength = 4.0f;
    genes.aquatic.headProportion = 0.3f;
    genes.aquatic.jawSize = 5.0f;  // Enormous

    genes.deepSea.hasExpandableStomach = true;
    genes.deepSea.hasHingedJaw = true;
    genes.deepSea.jawGape = 3.0f;  // Can gape 180 degrees

    // Whip-like tail with light
    genes.deepSea.hasPhotophores = true;
    genes.deepSea.photophorePositions = {{0, 0, -0.9f}};  // Tail tip
}
```

CHECKPOINT 5: Deep sea creatures working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/AQUATIC_VARIETY_DESIGN.md
- [ ] AquaticBodyType enum (15+ types)
- [ ] AquaticBodyGenes struct
- [ ] FinLayout with all fin types
- [ ] CaudalFinType enum (10 types)
- [ ] Body templates (shark, ray, eel, anglerfish)
- [ ] AquaticMeshGenerator class
- [ ] Fusiform body generation
- [ ] Ray body generation
- [ ] Jellyfish bell generation
- [ ] Tail fin generation (5+ types)
- [ ] SwimmingAnimator class
- [ ] Carangiform swimming
- [ ] Anguilliform swimming
- [ ] Rajiform swimming
- [ ] DeepSeaAdaptations struct
- [ ] Bioluminescence patterns
- [ ] Deep sea creature types

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 8: Flying Creature Variety (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Create diverse flying creatures - birds, bats, insects, pterosaurs, and fantasy flyers.

============================================================================
PHASE 1: DESIGN FLYING BODY PLANS (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research flight adaptations:
- Web search "bird flight body adaptations"
- Web search "insect wing mechanics procedural"
- Web search "bat wing membrane generation"
- Document flight-related anatomy

SUB-AGENT 1B - Analyze current flight code:
- Read existing flight behavior
- Read wing animation code
- Document flight physics
- Identify extension points

CHECKPOINT 1: Create docs/FLYING_VARIETY_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT FLYING BODY TYPES (1+ hour)
============================================================================

```cpp
// src/entities/genetics/FlyingBodyGenes.h

enum class FlyingBodyType {
    // Birds
    PASSERINE,          // Songbird - small, agile
    RAPTOR,             // Hawk/eagle - soaring predator
    WATERFOWL,          // Duck/goose - heavy body
    WADER,              // Heron/stork - long legs
    HUMMINGBIRD,        // Tiny, hovering
    SWIFT,              // Extreme aerial - rarely lands
    PENGUIN_LIKE,       // Flightless, uses wings for swimming

    // Bats
    MICROBAT,           // Small, echolocation
    MEGABAT,            // Large, fruit eater
    VAMPIRE_BAT,        // Specialized blood feeder

    // Insects
    DRAGONFLY,          // 4-wing, hovering
    BUTTERFLY,          // Large wings, slow
    BEETLE,             // Hardened forewings
    FLY,                // 2-wing, fast
    MOTH,               // Nocturnal, fuzzy

    // Prehistoric/Fantasy
    PTEROSAUR,          // Membrane wings, large head
    DRAGON,             // 4 legs + 2 wings
    WINGED_SERPENT,     // Snake with wings
    FLYING_FISH,        // Gliding fins
    FLOATING_JELLYFISH  // Lighter-than-air
};

enum class WingType {
    FEATHERED_POINTED,  // Fast, efficient (falcon)
    FEATHERED_ROUNDED,  // Maneuverable (owl)
    FEATHERED_SLOTTED,  // Soaring (eagle)
    FEATHERED_HIGH_LIFT,// Slow, high lift (pheasant)

    MEMBRANE_FINGER,    // Bat-style, fingers extend
    MEMBRANE_ARM,       // Pterosaur-style, single finger
    MEMBRANE_BODY,      // Flying squirrel, body-attached

    INSECT_MEMBRANOUS,  // Dragonfly, transparent
    INSECT_SCALED,      // Butterfly, covered in scales
    INSECT_HARDENED     // Beetle elytra
};

struct FlyingBodyGenes {
    FlyingBodyType bodyType;
    WingType wingType;

    // Body proportions
    float bodyLength;
    float bodyMass;             // Affects flight physics
    float breastSize;           // Flight muscle mass
    float tailLength;
    float tailSpread;           // For steering

    // Wing dimensions
    float wingspan;             // Total span
    float wingChord;            // Wing width
    float aspectRatio;          // Span/chord (high = gliding, low = maneuvering)
    float wingLoading;          // Mass/wing area
    float sweepAngle;           // Wing sweep back

    // Wing details
    int primaryFeathers;        // Outer flight feathers
    int secondaryFeathers;      // Inner flight feathers
    bool hasSlots;              // Slotted tips for soaring
    bool hasCrest;              // Head crest
    float alulaeSize;           // Small leading edge feathers

    // Leg configuration
    float legLength;
    bool legsTucked;            // In flight
    bool hasPerchingFeet;
    bool hasTalons;

    // Special features
    bool hasLongNeck;
    bool hasLongBeak;
    float beakLength;
    BeakShape beakShape;
};

enum class BeakShape {
    SEED_CRACKER,       // Finch - thick, conical
    PROBE,              // Hummingbird - long, thin
    FILTER,             // Flamingo - curved, filtering
    HOOK,               // Raptor - curved, tearing
    SPEAR,              // Heron - long, straight
    FLAT,               // Duck - wide, filtering
    GENERAL             // Typical bird beak
};

// Flight style affects animation
enum class FlightStyle {
    FLAPPING_CONTINUOUS,  // Constant wing beats
    FLAPPING_BOUNDING,    // Flap-glide-flap pattern
    SOARING,              // Minimal flapping, uses thermals
    HOVERING,             // Stationary in air
    GLIDING,              // No flapping, descending
    POWERED_GLIDE         // Occasional flaps
};

struct FlightCapabilities {
    float maxSpeed;
    float cruiseSpeed;
    float maneuverability;      // Turn rate
    float climbRate;
    float diveSpeed;
    float hoverEfficiency;      // 0 = can't hover, 1 = perfect hover
    float glidingEfficiency;    // Lift/drag ratio
    FlightStyle preferredStyle;
};
```

CHECKPOINT 2: Flying body types defined

============================================================================
PHASE 3: IMPLEMENT WING MESH GENERATION (1+ hour)
============================================================================

```cpp
// src/graphics/procedural/WingMeshGenerator.h

class WingMeshGenerator {
public:
    GeneratedMesh generateFeatheredWing(const FlyingBodyGenes& genes);
    GeneratedMesh generateMembraneWing(const FlyingBodyGenes& genes);
    GeneratedMesh generateInsectWing(const FlyingBodyGenes& genes);

private:
    void generateFeather(GeneratedMesh& mesh, glm::vec3 base, glm::vec3 tip,
                         float width, FeatherType type);
    void generateMembrane(GeneratedMesh& mesh, const std::vector<glm::vec3>& bones,
                          float tension);
    void generateWingVeins(GeneratedMesh& mesh, float scale);
};

void WingMeshGenerator::generateFeatheredWing(const FlyingBodyGenes& genes) {
    GeneratedMesh mesh;

    // Wing bone structure
    glm::vec3 shoulder(0);
    glm::vec3 elbow = shoulder + glm::vec3(genes.wingspan * 0.25f, 0, 0);
    glm::vec3 wrist = elbow + glm::vec3(genes.wingspan * 0.25f, -0.05f, 0);
    glm::vec3 tip = wrist + glm::vec3(genes.wingspan * 0.25f, 0, 0);

    // Arm bones (humerus, radius/ulna, hand)
    generateCapsuleSegment(mesh, shoulder, elbow, 0.03f, 0.025f);
    generateCapsuleSegment(mesh, elbow, wrist, 0.02f, 0.015f);
    generateCapsuleSegment(mesh, wrist, tip, 0.01f, 0.005f);

    // Primary feathers (outer, longest)
    float primaryLength = genes.wingspan * 0.3f;
    for (int i = 0; i < genes.primaryFeathers; i++) {
        float t = (float)i / genes.primaryFeathers;
        glm::vec3 base = glm::mix(wrist, tip, t);

        // Feathers angle back
        glm::vec3 featherTip = base + glm::vec3(
            primaryLength * (1.0f - t * 0.3f),  // Shorter toward tip
            -0.1f,
            -primaryLength * 0.2f * t           // Angle back
        );

        float width = 0.03f * (1.0f - t * 0.5f);
        generateFeather(mesh, base, featherTip, width, FeatherType::PRIMARY);
    }

    // Secondary feathers (inner, attached to arm)
    float secondaryLength = genes.wingspan * 0.2f;
    for (int i = 0; i < genes.secondaryFeathers; i++) {
        float t = (float)i / genes.secondaryFeathers;
        glm::vec3 base = glm::mix(elbow, wrist, t);

        glm::vec3 featherTip = base + glm::vec3(
            secondaryLength,
            -0.05f,
            -secondaryLength * 0.1f
        );

        float width = 0.025f;
        generateFeather(mesh, base, featherTip, width, FeatherType::SECONDARY);
    }

    // Covert feathers (small, covering base of flight feathers)
    generateCovertFeathers(mesh, genes);

    // Alula (small feathers on "thumb")
    if (genes.alulaeSize > 0.0f) {
        generateAlula(mesh, wrist, genes.alulaeSize);
    }

    return mesh;
}

void WingMeshGenerator::generateMembraneWing(const FlyingBodyGenes& genes) {
    GeneratedMesh mesh;

    if (genes.wingType == WingType::MEMBRANE_FINGER) {
        // Bat-style: membrane stretched between elongated fingers
        std::vector<glm::vec3> fingerTips;

        glm::vec3 wrist(genes.wingspan * 0.15f, 0, 0);

        // 4-5 elongated fingers
        for (int f = 0; f < 5; f++) {
            float angle = -30.0f + f * 30.0f;  // Spread of fingers
            float length = genes.wingspan * 0.35f * (1.0f - abs(f - 2) * 0.1f);

            glm::vec3 tip = wrist + glm::vec3(
                cos(glm::radians(angle)) * length,
                sin(glm::radians(angle)) * length * 0.3f,
                0
            );
            fingerTips.push_back(tip);

            // Finger bone
            generateCapsuleSegment(mesh, wrist, tip, 0.01f, 0.003f);
        }

        // Membrane between fingers
        for (int f = 0; f < 4; f++) {
            generateMembranePatch(mesh, wrist, fingerTips[f], fingerTips[f + 1]);
        }

        // Membrane to body
        glm::vec3 hip(0, -genes.bodyLength * 0.4f, 0);
        generateMembranePatch(mesh, wrist, fingerTips[4], hip);

    } else if (genes.wingType == WingType::MEMBRANE_ARM) {
        // Pterosaur-style: single elongated finger
        glm::vec3 shoulder(0);
        glm::vec3 elbow = shoulder + glm::vec3(genes.wingspan * 0.2f, 0, 0);
        glm::vec3 wrist = elbow + glm::vec3(genes.wingspan * 0.15f, -0.05f, 0);
        glm::vec3 tip = wrist + glm::vec3(genes.wingspan * 0.4f, 0.1f, 0);

        // Bones
        generateCapsuleSegment(mesh, shoulder, elbow, 0.025f, 0.02f);
        generateCapsuleSegment(mesh, elbow, wrist, 0.02f, 0.015f);
        generateCapsuleSegment(mesh, wrist, tip, 0.015f, 0.005f);

        // Single large membrane
        glm::vec3 hip(0, -genes.bodyLength * 0.3f, 0);
        std::vector<glm::vec3> outline = {shoulder, elbow, wrist, tip, hip};
        generateMembrane(mesh, outline, 0.8f);
    }

    return mesh;
}

void WingMeshGenerator::generateInsectWing(const FlyingBodyGenes& genes) {
    GeneratedMesh mesh;

    // Thin, transparent membrane with veins
    float length = genes.wingspan * 0.4f;
    float width = genes.wingChord;

    // Wing outline (elliptical or complex based on type)
    std::vector<glm::vec2> outline;
    int points = 32;

    if (genes.wingType == WingType::INSECT_MEMBRANOUS) {
        // Dragonfly - narrow, elongated
        for (int i = 0; i < points; i++) {
            float t = (float)i / points;
            float x = t * length;
            float y = sin(t * PI) * width * 0.3f * (1.0f - t * 0.5f);
            outline.push_back({x, y});
        }
    } else if (genes.wingType == WingType::INSECT_SCALED) {
        // Butterfly - wider, more rounded
        for (int i = 0; i < points; i++) {
            float t = (float)i / points * 2.0f * PI;
            float r = width * (1.0f + 0.3f * cos(2.0f * t));
            outline.push_back({cos(t) * r + length * 0.5f, sin(t) * r});
        }
    }

    // Generate thin wing mesh
    for (const auto& p : outline) {
        Vertex v;
        v.position = glm::vec3(p.x, 0, p.y);
        v.normal = glm::vec3(0, 1, 0);
        v.uv = glm::vec2(p.x / length, p.y / width + 0.5f);
        mesh.vertices.push_back(v);
    }

    // Add vein pattern
    generateWingVeins(mesh, length);

    return mesh;
}
```

CHECKPOINT 3: Wing mesh generation working

============================================================================
PHASE 4: IMPLEMENT FLIGHT ANIMATION VARIETY (45+ minutes)
============================================================================

```cpp
// Different flight animation styles

class FlightAnimator {
public:
    void animateFlapContinuous(Skeleton& skeleton, float phase, float speed);
    void animateFlapBounding(Skeleton& skeleton, float phase, float speed);
    void animateSoaring(Skeleton& skeleton, float phase, float bankAngle);
    void animateHovering(Skeleton& skeleton, float phase);
    void animateInsectWings(Skeleton& skeleton, float phase, float frequency);
    void animateDive(Skeleton& skeleton, float phase, float diveAngle);

private:
    void setWingPose(Skeleton& skeleton, float downstroke, float sweep);
};

void FlightAnimator::animateFlapContinuous(Skeleton& skeleton, float phase, float speed) {
    // Standard bird flapping
    float wingAngle = sin(phase * 2.0f * PI) * 45.0f;  // +/- 45 degrees

    int leftWing = skeleton.getBoneIndex("wing_left_shoulder");
    int rightWing = skeleton.getBoneIndex("wing_right_shoulder");

    // Downstroke: wings pull down and forward
    // Upstroke: wings lift and sweep back
    float sweep = cos(phase * 2.0f * PI) * 10.0f;

    skeleton.rotateBone(leftWing, glm::angleAxis(
        glm::radians(wingAngle), glm::vec3(0, 0, 1)));
    skeleton.rotateBone(rightWing, glm::angleAxis(
        glm::radians(-wingAngle), glm::vec3(0, 0, 1)));

    // Elbow bends during upstroke
    int leftElbow = skeleton.getBoneIndex("wing_left_elbow");
    int rightElbow = skeleton.getBoneIndex("wing_right_elbow");

    float elbowBend = (wingAngle > 0) ? 0.0f : -20.0f;  // Bend on upstroke
    skeleton.rotateBone(leftElbow, glm::angleAxis(
        glm::radians(elbowBend), glm::vec3(0, 0, 1)));
    skeleton.rotateBone(rightElbow, glm::angleAxis(
        glm::radians(-elbowBend), glm::vec3(0, 0, 1)));

    // Wrist twists for feather angle
    int leftWrist = skeleton.getBoneIndex("wing_left_wrist");
    int rightWrist = skeleton.getBoneIndex("wing_right_wrist");

    float wristTwist = wingAngle * 0.5f;
    skeleton.rotateBone(leftWrist, glm::angleAxis(
        glm::radians(wristTwist), glm::vec3(1, 0, 0)));
    skeleton.rotateBone(rightWrist, glm::angleAxis(
        glm::radians(-wristTwist), glm::vec3(1, 0, 0)));
}

void FlightAnimator::animateHovering(Skeleton& skeleton, float phase) {
    // Hummingbird-style figure-8 wing motion
    float angle = phase * 2.0f * PI;

    // Wings trace figure-8 pattern
    float horizontal = sin(angle) * 80.0f;        // Large horizontal sweep
    float vertical = sin(angle * 2.0f) * 20.0f;   // Small vertical motion

    // Wing rotates at end of each stroke
    float twist = cos(angle) * 45.0f;

    int leftWing = skeleton.getBoneIndex("wing_left_shoulder");
    int rightWing = skeleton.getBoneIndex("wing_right_shoulder");

    // Apply combined rotation
    glm::quat leftRot = glm::angleAxis(glm::radians(horizontal), glm::vec3(0, 1, 0)) *
                        glm::angleAxis(glm::radians(vertical), glm::vec3(0, 0, 1)) *
                        glm::angleAxis(glm::radians(twist), glm::vec3(1, 0, 0));

    glm::quat rightRot = glm::angleAxis(glm::radians(-horizontal), glm::vec3(0, 1, 0)) *
                         glm::angleAxis(glm::radians(-vertical), glm::vec3(0, 0, 1)) *
                         glm::angleAxis(glm::radians(-twist), glm::vec3(1, 0, 0));

    skeleton.setBoneRotation(leftWing, leftRot);
    skeleton.setBoneRotation(rightWing, rightRot);
}

void FlightAnimator::animateInsectWings(Skeleton& skeleton, float phase, float frequency) {
    // Very fast wing beats (invisible individual strokes)
    // Show as blur effect instead

    float blur = 0.8f;  // Wing motion blur amount

    if (frequency > 100.0f) {
        // Wings shown as transparent blur disk
        skeleton.setWingBlurMode(true, blur);
    } else {
        // Slower insects show actual wing motion
        float wingAngle = sin(phase * frequency * 2.0f * PI) * 60.0f;

        // Dragonfly has 4 independent wings
        int wings[] = {
            skeleton.getBoneIndex("wing_front_left"),
            skeleton.getBoneIndex("wing_front_right"),
            skeleton.getBoneIndex("wing_rear_left"),
            skeleton.getBoneIndex("wing_rear_right")
        };

        // Front and rear pairs out of phase
        for (int i = 0; i < 4; i++) {
            float offset = (i < 2) ? 0.0f : 0.5f;
            float angle = sin((phase + offset) * frequency * 2.0f * PI) * 60.0f;
            int sign = (i % 2 == 0) ? 1 : -1;

            skeleton.rotateBone(wings[i], glm::angleAxis(
                glm::radians(angle * sign), glm::vec3(0, 0, 1)));
        }
    }
}

void FlightAnimator::animateSoaring(Skeleton& skeleton, float phase, float bankAngle) {
    // Minimal wing motion, mostly pose adjustments

    // Wings held at slight dihedral (upward angle)
    float dihedral = 5.0f;

    // Slight adjustments for balance
    float adjust = sin(phase * 0.5f) * 2.0f;

    int leftWing = skeleton.getBoneIndex("wing_left_shoulder");
    int rightWing = skeleton.getBoneIndex("wing_right_shoulder");

    skeleton.rotateBone(leftWing, glm::angleAxis(
        glm::radians(dihedral + adjust - bankAngle * 0.2f), glm::vec3(0, 0, 1)));
    skeleton.rotateBone(rightWing, glm::angleAxis(
        glm::radians(-dihedral - adjust + bankAngle * 0.2f), glm::vec3(0, 0, 1)));

    // Tail spreads for control
    int tail = skeleton.getBoneIndex("tail");
    skeleton.setTailSpread(tail, 0.3f + abs(bankAngle) * 0.01f);

    // Primary feathers slot open
    skeleton.setPrimarySlotting(true);
}
```

CHECKPOINT 4: Flight animation variety working

============================================================================
PHASE 5: IMPLEMENT FANTASY FLYERS (30+ minutes)
============================================================================

```cpp
// Fantasy and exotic flying creatures

class FantasyFlyerGenerator {
public:
    void generateDragon(Creature& creature, DragonType type);
    void generateGryphon(Creature& creature);
    void generatePegasus(Creature& creature);
    void generateFlyingWhale(Creature& creature);
    void generateFloatingJellyfish(Creature& creature);
};

enum class DragonType {
    WESTERN,        // 4 legs + 2 wings
    EASTERN,        // Serpentine, no wings (magical flight)
    WYVERN,         // 2 legs + 2 wings (legs are wings)
    DRAKE,          // 4 legs, no wings (flightless)
    AMPHIPTERE      // Winged serpent
};

void FantasyFlyerGenerator::generateDragon(Creature& creature, DragonType type) {
    auto& genes = creature.getGenome();

    switch (type) {
        case DragonType::WESTERN:
            genes.flying.bodyType = FlyingBodyType::DRAGON;
            genes.bodyPlan.segmentCount = 1;  // Solid body
            genes.bodyPlan.bodyLength = 5.0f;
            genes.bodyPlan.hasDistinctHead = true;
            genes.bodyPlan.headSize = 1.5f;

            // 4 legs
            genes.appendages.lowerAppendages = {
                {AppendageType::LEG_WALKING, 4, 1.0f, 0.15f, 3, 0.2f}
            };

            // Large bat-style wings
            genes.flying.wingType = WingType::MEMBRANE_FINGER;
            genes.flying.wingspan = genes.bodyPlan.bodyLength * 2.5f;

            // Long tail
            genes.bodyPlan.tailLength = genes.bodyPlan.bodyLength * 0.8f;

            // Features
            genes.features.headFeatures = {
                {SurfaceFeatureType::HORN_PAIR, 0.5f},
                {SurfaceFeatureType::SPINE_ROW, 0.3f}
            };
            genes.features.bodyFeatures = {
                {SurfaceFeatureType::ARMOR_SCALES, 1.0f},
                {SurfaceFeatureType::SPINE_ROW, 0.4f}
            };
            break;

        case DragonType::EASTERN:
            genes.bodyPlan.baseShape = BodyShape::SERPENTINE;
            genes.bodyPlan.segmentCount = 12;
            genes.bodyPlan.bodyLength = 10.0f;

            // Small legs
            genes.appendages.lowerAppendages = {
                {AppendageType::LEG_WALKING, 4, 0.3f, 0.1f, 3, 0.3f}
            };

            // No wings - magical flight
            genes.flying.wingType = WingType::NONE;
            genes.flying.magicalFlight = true;

            // Whiskers and antlers
            genes.features.headFeatures = {
                {SurfaceFeatureType::ANTLER, 0.8f},
                {SurfaceFeatureType::BARBELS, 0.5f}
            };
            break;

        case DragonType::WYVERN:
            genes.flying.bodyType = FlyingBodyType::DRAGON;
            genes.bodyPlan.bodyLength = 3.0f;

            // Wings ARE the front legs
            genes.appendages.upperAppendages = {
                {AppendageType::WING_MEMBRANE, 2, 2.0f, 0.2f, 4, 0.5f,
                 {0.3f, 0.5f, 0.3f}, 45.0f, true, 0.15f}  // Has claws
            };
            genes.appendages.lowerAppendages = {
                {AppendageType::LEG_WALKING, 2, 0.8f, 0.12f, 3, 0.2f}
            };
            break;
    }

    creature.regenerateMesh();
}

void FantasyFlyerGenerator::generateFloatingJellyfish(Creature& creature) {
    // Lighter-than-air creature
    auto& genes = creature.getGenome();

    genes.flying.bodyType = FlyingBodyType::FLOATING_JELLYFISH;
    genes.aquatic.bodyType = AquaticBodyType::BELL;  // Reuse bell shape

    // Gas-filled bell
    genes.bodyPlan.bodyWidth = 2.0f;
    genes.bodyPlan.bodyHeight = 1.5f;

    // Very long trailing tentacles
    genes.appendages.rearAppendages = {
        {AppendageType::TENTACLE, 8, 5.0f, 0.05f, 12, 0.95f}
    };

    // Bioluminescence
    genes.coloration.hasBioluminescence = true;
    genes.coloration.glowColor = glm::vec3(0.3f, 0.6f, 1.0f);
    genes.coloration.translucency = 0.7f;

    // Flight is passive floating
    genes.flying.flightCapabilities.preferredStyle = FlightStyle::GLIDING;
    genes.flying.flightCapabilities.hoverEfficiency = 1.0f;  // Perfect hover
    genes.flying.buoyancy = 1.0f;  // Lighter than air

    creature.regenerateMesh();
}
```

CHECKPOINT 5: Fantasy flyers working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/FLYING_VARIETY_DESIGN.md
- [ ] FlyingBodyType enum (20+ types)
- [ ] WingType enum (10 types)
- [ ] FlyingBodyGenes struct
- [ ] FlightCapabilities struct
- [ ] BeakShape enum
- [ ] WingMeshGenerator class
- [ ] Feathered wing generation
- [ ] Membrane wing generation
- [ ] Insect wing generation
- [ ] FlightAnimator class
- [ ] Continuous flapping animation
- [ ] Hovering animation
- [ ] Soaring animation
- [ ] Insect wing blur
- [ ] FantasyFlyerGenerator
- [ ] Dragon types (Western, Eastern, Wyvern)
- [ ] Floating creatures

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 9: Eyes, Mouths & Sensory Organs (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Create varied eyes, mouths, noses, ears, and other sensory organs that give creatures personality.

============================================================================
PHASE 1: DESIGN FACIAL FEATURE SYSTEM (45+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Research eye/face variety:
- Web search "procedural eye generation games"
- Web search "animal eye types evolution"
- Web search "creature face generation"
- Document facial feature variety

SUB-AGENT 1B - Design sensory organ genes:
- List all eye types
- List all mouth types
- Document sensory organs
- Plan mesh/texture generation

CHECKPOINT 1: Create docs/SENSORY_ORGANS_DESIGN.md

============================================================================
PHASE 2: IMPLEMENT EYE GENETICS (1+ hour)
============================================================================

```cpp
// src/entities/genetics/EyeGenes.h

enum class EyeType {
    // Standard eyes
    SIMPLE,             // Basic dot eyes
    CAMERA,             // Vertebrate eye with lens
    COMPOUND,           // Insect eye (many facets)
    PINHOLE,            // Nautilus-style (no lens)

    // Specialized
    TUBE,               // Deep sea (barreleye)
    STALKED,            // Snail/crab on stalks
    REFLECTIVE,         // Cat-eye (tapetum lucidum)
    MULTIFOCAL,         // Different focus zones

    // Exotic
    FACETED_JEWEL,      // Fantasy crystalline
    GLOWING,            // Bioluminescent eyes
    VOID,               // Pure black, no detail
    SPIRAL_PUPIL,       // Fantasy spiral pattern
    HORIZONTAL_PUPIL,   // Goat-style
    VERTICAL_PUPIL,     // Cat-style
    W_PUPIL,            // Cuttlefish
    STAR_PUPIL          // Fantasy star-shaped
};

enum class EyePlacement {
    FORWARD,            // Predator - binocular vision
    LATERAL,            // Prey - wide field of view
    TOP,                // Frog/hippo - peek above water
    MULTIPLE_ROWS,      // Spider - 8 eyes
    SINGLE_CYCLOPEAN,   // One central eye
    ASYMMETRIC          // Flounder - both on one side
};

struct EyeGenes {
    EyeType type;
    EyePlacement placement;

    int eyeCount;               // 1-8 typical, more for compound
    float eyeSize;              // Relative to head
    float eyeSpacing;           // Distance between eyes
    glm::vec3 eyePosition;      // On head (normalized)

    // Eye appearance
    glm::vec3 irisColor;
    glm::vec3 scleraColor;      // White of eye
    glm::vec3 pupilColor;       // Usually black
    float pupilSize;            // Relative to iris
    PupilShape pupilShape;

    // Special features
    bool hasEyelids;
    float eyelidPosition;       // 0=open, 1=closed
    bool hasNictitating;        // Third eyelid
    bool hasEyeshine;           // Reflective (nocturnal)
    glm::vec3 eyeshineColor;

    // Compound eye specific
    int facetCount;             // For compound eyes
    float facetSize;

    // Stalk specific
    float stalkLength;
    float stalkFlexibility;
};

enum class PupilShape {
    ROUND,
    VERTICAL_SLIT,
    HORIZONTAL_SLIT,
    W_SHAPED,
    STAR,
    CRESCENT,
    HEART,              // Fantasy
    KEYHOLE,
    RECTANGULAR         // Goat/octopus
};

// Eye templates
EyeGenes createPredatorEyes() {
    return {
        .type = EyeType::CAMERA,
        .placement = EyePlacement::FORWARD,
        .eyeCount = 2,
        .eyeSize = 0.15f,
        .eyeSpacing = 0.3f,
        .irisColor = glm::vec3(0.8f, 0.6f, 0.2f),  // Amber
        .pupilShape = PupilShape::VERTICAL_SLIT,
        .hasEyeshine = true,
        .eyeshineColor = glm::vec3(0.0f, 1.0f, 0.5f)  // Green shine
    };
}

EyeGenes createPreyEyes() {
    return {
        .type = EyeType::CAMERA,
        .placement = EyePlacement::LATERAL,
        .eyeCount = 2,
        .eyeSize = 0.2f,  // Larger for better vision
        .eyeSpacing = 0.8f,  // Wide apart
        .irisColor = glm::vec3(0.3f, 0.2f, 0.1f),  // Brown
        .pupilShape = PupilShape::HORIZONTAL_SLIT,
        .hasEyelids = true
    };
}

EyeGenes createSpiderEyes() {
    return {
        .type = EyeType::CAMERA,
        .placement = EyePlacement::MULTIPLE_ROWS,
        .eyeCount = 8,
        .eyeSize = 0.08f,
        .irisColor = glm::vec3(0.1f, 0.1f, 0.1f),  // Dark
        .pupilShape = PupilShape::ROUND,
        .hasEyeshine = true
    };
}

EyeGenes createInsectEyes() {
    return {
        .type = EyeType::COMPOUND,
        .placement = EyePlacement::LATERAL,
        .eyeCount = 2,
        .eyeSize = 0.3f,  // Large compound eyes
        .facetCount = 3000,
        .facetSize = 0.01f,
        .irisColor = glm::vec3(0.2f, 0.5f, 0.3f)  // Iridescent green
    };
}
```

CHECKPOINT 2: Eye genetics defined

============================================================================
PHASE 3: IMPLEMENT MOUTH GENETICS (45+ minutes)
============================================================================

```cpp
// src/entities/genetics/MouthGenes.h

enum class MouthType {
    // Standard
    SIMPLE_OPENING,     // Basic circular mouth
    LIPPED,             // Soft lips (mammal)
    BEAKED,             // Hard beak (bird)
    SNOUTED,            // Extended snout

    // Specialized
    FANGED,             // Large canines
    TUSKED,             // Protruding tusks
    BALEEN,             // Filter feeding
    PROBOSCIS,          // Long tube (mosquito, butterfly)

    // Exotic
    MANDIBLES,          // Insect/crustacean
    SUCKER,             // Lamprey, leech
    RADULA,             // Snail rasping tongue
    TENTACLE_MOUTH,     // Surrounded by tentacles
    VERTICAL_JAW,       // Opens sideways
    MULTIPLE            // Multiple mouths
};

enum class TeethType {
    NONE,
    CONICAL,            // Fish, reptile - simple points
    SERRATED,           // Shark - cutting edges
    CRUSHING,           // Molars - flat grinding
    NEEDLE,             // Anglerfish - thin, long
    VENOMOUS,           // Snake fangs
    TUSKS,              // Elephant, walrus
    RODENT,             // Ever-growing incisors
    HERBIVORE_RIDGE,    // Horse-like grinding surface
    CARNASSIAL          // Carnivore shearing teeth
};

struct MouthGenes {
    MouthType type;
    float mouthSize;            // Relative to head
    glm::vec3 mouthPosition;    // On head

    // Jaw
    float jawLength;
    float jawWidth;
    float gapeAngle;            // How wide can open
    bool hasHingedJaw;          // Snake-style

    // Teeth
    TeethType teethType;
    int teethCount;
    float teethSize;
    bool hasVisibleTeeth;       // When mouth closed

    // Lips/Beak
    bool hasLips;
    glm::vec3 lipColor;
    float lipThickness;
    BeakShape beakShape;        // If beaked

    // Tongue
    bool hasTongue;
    float tongueLength;
    TongueType tongueType;

    // Special
    bool hasTusks;
    float tuskLength;
    bool hasFangs;
    float fangLength;
};

enum class TongueType {
    NONE,
    STANDARD,           // Normal tongue
    FORKED,             // Snake - sensory
    LONG_STICKY,        // Frog/anteater - catching prey
    PREHENSILE,         // Giraffe - grasping
    RASPING,            // Cat - rough surface
    TUBE                // Butterfly - drinking tube
};

// Mouth templates
MouthGenes createPredatorMouth() {
    return {
        .type = MouthType::FANGED,
        .mouthSize = 0.4f,
        .jawLength = 0.3f,
        .gapeAngle = 90.0f,
        .teethType = TeethType::CARNASSIAL,
        .teethCount = 28,
        .hasVisibleTeeth = true,
        .hasFangs = true,
        .fangLength = 0.1f
    };
}

MouthGenes createHerbivoreMouth() {
    return {
        .type = MouthType::LIPPED,
        .mouthSize = 0.2f,
        .hasLips = true,
        .lipThickness = 0.03f,
        .teethType = TeethType::HERBIVORE_RIDGE,
        .hasTongue = true,
        .tongueType = TongueType::PREHENSILE
    };
}

MouthGenes createInsectMouth() {
    return {
        .type = MouthType::MANDIBLES,
        .mouthSize = 0.15f,
        .jawWidth = 0.2f,
        .gapeAngle = 120.0f,
        .teethType = TeethType::NONE
    };
}
```

CHECKPOINT 3: Mouth genetics defined

============================================================================
PHASE 4: IMPLEMENT OTHER SENSORY ORGANS (45+ minutes)
============================================================================

```cpp
// src/entities/genetics/SensoryGenes.h

enum class NoseType {
    NONE,
    NOSTRILS,           // Simple holes
    SNOUT,              // Protruding (dog)
    TRUNK,              // Elephant
    STAR_NOSE,          // Star-nosed mole
    PIG_SNOUT,          // Flat, round
    BEAK_NOSTRILS,      // Bird - on beak
    SPIRACLE            // Top of head (whale)
};

enum class EarType {
    NONE,
    INTERNAL,           // No visible ear (fish, reptile)
    ROUND,              // Bear, human
    POINTED,            // Cat, fox
    LARGE_ROUND,        // Elephant, mouse
    LONG,               // Rabbit, donkey
    BAT,                // Large, complex shape
    HORN_SHAPED,        // Goat
    TUFTED,             // Lynx
    FLOPPY              // Dog, elephant
};

struct SensoryGenes {
    // Nose/Olfactory
    NoseType noseType;
    float noseSize;
    glm::vec3 nosePosition;
    bool hasSensitiveNose;      // Enhanced smell
    bool hasWetNose;            // Better scent detection

    // Ears
    EarType earType;
    float earSize;
    glm::vec3 earPosition;
    bool canRotateEars;
    float earRotationRange;

    // Whiskers/Vibrissae
    bool hasWhiskers;
    int whiskerCount;
    float whiskerLength;
    glm::vec3 whiskerPositions[4];  // Muzzle, eyebrow, chin, cheek

    // Antennae (insects)
    bool hasAntennae;
    int antennaeSegments;
    float antennaeLength;
    AntennaShape antennaeShape;

    // Special sensors
    bool hasElectroreceptors;   // Shark, platypus
    bool hasPitOrgans;          // Heat sensing (snake)
    bool hasLateralLine;        // Water vibration (fish)
    bool hasEcholocation;       // Bat, dolphin
    bool hasMagnetoreception;   // Bird migration
};

enum class AntennaShape {
    FILIFORM,           // Thread-like
    SETACEOUS,          // Bristle-like
    CLAVATE,            // Club-shaped
    PECTINATE,          // Comb-like
    PLUMOSE,            // Feathery
    LAMELLATE           // Plate-like (beetle)
};

// Generate mesh for sensory organs
class SensoryMeshGenerator {
public:
    GeneratedMesh generateEyes(const EyeGenes& genes);
    GeneratedMesh generateMouth(const MouthGenes& genes);
    GeneratedMesh generateNose(const SensoryGenes& genes);
    GeneratedMesh generateEars(const SensoryGenes& genes);
    GeneratedMesh generateWhiskers(const SensoryGenes& genes);
    GeneratedMesh generateAntennae(const SensoryGenes& genes);

private:
    void generateEyeball(GeneratedMesh& mesh, glm::vec3 position, float size,
                         const EyeGenes& genes);
    void generateCompoundEye(GeneratedMesh& mesh, glm::vec3 position, float size,
                             int facets);
    void generateTeeth(GeneratedMesh& mesh, const MouthGenes& genes);
};

void SensoryMeshGenerator::generateEyeball(GeneratedMesh& mesh, glm::vec3 position,
                                            float size, const EyeGenes& genes) {
    // Sphere for eyeball
    int rings = 16, segments = 24;

    for (int r = 0; r <= rings; r++) {
        float phi = (float)r / rings * PI;
        for (int s = 0; s < segments; s++) {
            float theta = (float)s / segments * 2.0f * PI;

            float x = sin(phi) * cos(theta) * size;
            float y = sin(phi) * sin(theta) * size;
            float z = cos(phi) * size;

            Vertex v;
            v.position = position + glm::vec3(x, y, z);
            v.normal = glm::normalize(glm::vec3(x, y, z));

            // UV for eye texture
            v.uv = glm::vec2((float)s / segments, (float)r / rings);

            // Color based on position
            float distFromCenter = sqrt(x * x + y * y) / size;
            if (distFromCenter < genes.pupilSize * 0.5f) {
                v.color = genes.pupilColor;  // Pupil
            } else if (distFromCenter < 0.5f) {
                v.color = genes.irisColor;   // Iris
            } else {
                v.color = genes.scleraColor; // Sclera
            }

            mesh.vertices.push_back(v);
        }
    }

    generateSphereIndices(mesh, rings, segments);
}

void SensoryMeshGenerator::generateCompoundEye(GeneratedMesh& mesh, glm::vec3 position,
                                                 float size, int facets) {
    // Hemisphere covered in hexagonal facets
    float radius = size;
    int rings = (int)sqrt(facets / 2);
    int facetsPerRing = facets / rings;

    for (int r = 0; r < rings; r++) {
        float phi = (float)r / rings * PI * 0.5f;  // Hemisphere
        int ringFacets = (int)(facetsPerRing * sin(phi) + 1);

        for (int f = 0; f < ringFacets; f++) {
            float theta = (float)f / ringFacets * 2.0f * PI;

            glm::vec3 center(
                sin(phi) * cos(theta) * radius,
                sin(phi) * sin(theta) * radius,
                cos(phi) * radius
            );

            // Each facet is a small hexagon
            float facetRadius = 2.0f * PI * radius * sin(phi) / ringFacets * 0.4f;
            generateHexagon(mesh, position + center, glm::normalize(center),
                           facetRadius);
        }
    }
}
```

CHECKPOINT 4: Sensory organs implemented

============================================================================
PHASE 5: IMPLEMENT FACIAL ANIMATION (30+ minutes)
============================================================================

```cpp
// Animate eyes, mouth, ears

class FacialAnimator {
public:
    void animateBlinking(Skeleton& skeleton, float time, float blinkRate);
    void animateEyeTracking(Skeleton& skeleton, glm::vec3 targetPos);
    void animateMouthOpen(Skeleton& skeleton, float openAmount);
    void animateEarRotation(Skeleton& skeleton, glm::vec3 soundDirection);
    void animateEmotions(Skeleton& skeleton, Emotion emotion, float intensity);
};

enum class Emotion {
    NEUTRAL,
    ALERT,
    ANGRY,
    SCARED,
    HAPPY,
    CURIOUS,
    TIRED,
    HUNTING
};

void FacialAnimator::animateBlinking(Skeleton& skeleton, float time, float blinkRate) {
    // Periodic blinks
    float blinkCycle = fmod(time * blinkRate, 1.0f);

    float eyelidClose = 0.0f;
    if (blinkCycle < 0.1f) {
        // Quick close
        eyelidClose = blinkCycle / 0.1f;
    } else if (blinkCycle < 0.15f) {
        // Closed
        eyelidClose = 1.0f;
    } else if (blinkCycle < 0.25f) {
        // Open
        eyelidClose = 1.0f - (blinkCycle - 0.15f) / 0.1f;
    }

    skeleton.setBlendShape("eyelid_close", eyelidClose);
}

void FacialAnimator::animateEyeTracking(Skeleton& skeleton, glm::vec3 targetPos) {
    // Eyes follow a target

    glm::vec3 headPos = skeleton.getBoneWorldPos("head");
    glm::vec3 toTarget = glm::normalize(targetPos - headPos);

    // Limit eye rotation
    float maxYaw = 30.0f;
    float maxPitch = 20.0f;

    float yaw = glm::clamp(atan2(toTarget.x, toTarget.z), -maxYaw, maxYaw);
    float pitch = glm::clamp(asin(toTarget.y), -maxPitch, maxPitch);

    int leftEye = skeleton.getBoneIndex("eye_left");
    int rightEye = skeleton.getBoneIndex("eye_right");

    glm::quat eyeRot = glm::angleAxis(yaw, glm::vec3(0, 1, 0)) *
                       glm::angleAxis(pitch, glm::vec3(1, 0, 0));

    skeleton.setBoneRotation(leftEye, eyeRot);
    skeleton.setBoneRotation(rightEye, eyeRot);
}

void FacialAnimator::animateEmotions(Skeleton& skeleton, Emotion emotion, float intensity) {
    switch (emotion) {
        case Emotion::ANGRY:
            skeleton.setBlendShape("brow_furrow", intensity);
            skeleton.setBlendShape("nostril_flare", intensity * 0.5f);
            skeleton.setBlendShape("lip_curl", intensity * 0.3f);
            break;

        case Emotion::SCARED:
            skeleton.setBlendShape("eyes_wide", intensity);
            skeleton.setBlendShape("ears_back", intensity);
            skeleton.setBlendShape("mouth_tense", intensity * 0.5f);
            break;

        case Emotion::ALERT:
            skeleton.setBlendShape("ears_forward", intensity);
            skeleton.setBlendShape("eyes_wide", intensity * 0.5f);
            skeleton.setBlendShape("nostrils_flare", intensity * 0.3f);
            break;

        case Emotion::HUNTING:
            skeleton.setBlendShape("eyes_narrow", intensity * 0.5f);
            skeleton.setBlendShape("ears_forward", intensity);
            skeleton.setBlendShape("mouth_open", intensity * 0.2f);
            break;

        case Emotion::CURIOUS:
            skeleton.setBlendShape("head_tilt", intensity * 0.3f);
            skeleton.setBlendShape("ears_perked", intensity);
            skeleton.setBlendShape("eyes_wide", intensity * 0.3f);
            break;
    }
}

void FacialAnimator::animateEarRotation(Skeleton& skeleton, glm::vec3 soundDirection) {
    // Ears rotate toward sound

    int leftEar = skeleton.getBoneIndex("ear_left");
    int rightEar = skeleton.getBoneIndex("ear_right");

    // Calculate rotation for each ear
    float leftAngle = atan2(soundDirection.x + 0.5f, soundDirection.z);
    float rightAngle = atan2(soundDirection.x - 0.5f, soundDirection.z);

    // Limit rotation
    leftAngle = glm::clamp(leftAngle, -0.5f, 0.5f);
    rightAngle = glm::clamp(rightAngle, -0.5f, 0.5f);

    skeleton.rotateBone(leftEar, glm::angleAxis(leftAngle, glm::vec3(0, 1, 0)));
    skeleton.rotateBone(rightEar, glm::angleAxis(rightAngle, glm::vec3(0, 1, 0)));
}
```

CHECKPOINT 5: Facial animation working

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/SENSORY_ORGANS_DESIGN.md
- [ ] EyeType enum (15+ types)
- [ ] EyePlacement enum
- [ ] EyeGenes struct
- [ ] PupilShape enum
- [ ] Eye templates (predator, prey, spider, insect)
- [ ] MouthType enum (15+ types)
- [ ] TeethType enum
- [ ] MouthGenes struct
- [ ] Mouth templates
- [ ] NoseType enum
- [ ] EarType enum
- [ ] SensoryGenes struct
- [ ] SensoryMeshGenerator class
- [ ] Eyeball mesh generation
- [ ] Compound eye generation
- [ ] Teeth generation
- [ ] FacialAnimator class
- [ ] Blinking animation
- [ ] Eye tracking
- [ ] Emotion expressions
- [ ] Ear rotation toward sound

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

## Agent 10: Visual Polish & Integration (4-5 hours)

```
PROJECT: C:\Users\andre\Desktop\OrganismEvolution
WORKING DIRECTORY: C:\Users\andre\Desktop\OrganismEvolution

CRITICAL INSTRUCTION: You MUST work for MULTIPLE HOURS. SPAWN SUB-AGENTS for parallel work.

MISSION: Integrate all creature variety systems, polish visuals, ensure creatures look cohesive and impressive.

============================================================================
PHASE 1: AUDIT ALL CREATURE SYSTEMS (45+ minutes)
============================================================================

SPAWN 3 SUB-AGENTS IN PARALLEL:

SUB-AGENT 1A - Audit body systems:
- Read BodyPlanGenes implementation
- Read AppendageGenes implementation
- Read SurfaceFeatureGenes implementation
- Document integration issues

SUB-AGENT 1B - Audit visual systems:
- Read ColorationGenes implementation
- Read EyeGenes implementation
- Read MouthGenes implementation
- Document rendering issues

SUB-AGENT 1C - Audit locomotion systems:
- Read all locomotion classes
- Read animation systems
- Document animation conflicts
- Identify missing transitions

CHECKPOINT 1: Create docs/CREATURE_VARIETY_AUDIT.md

============================================================================
PHASE 2: IMPLEMENT UNIFIED CREATURE GENERATOR (1+ hour)
============================================================================

```cpp
// src/entities/CreatureGenerator.h

class CreatureGenerator {
public:
    // Generate complete creature from high-level description
    Creature generate(const CreatureArchetype& archetype, int seed);

    // Generate random creature appropriate for biome
    Creature generateForBiome(BiomeType biome, int seed);

    // Creature archetypes
    Creature generatePredator(PredatorStyle style, float size, int seed);
    Creature generateHerbivore(HerbivoreStyle style, float size, int seed);
    Creature generateAquatic(AquaticNiche niche, float size, int seed);
    Creature generateFlying(FlyingStyle style, float size, int seed);
    Creature generateExotic(ExoticType type, float size, int seed);

private:
    // Coherence checking
    void ensureBodyCoherence(CreatureGenome& genome);
    void ensureAppendageCoherence(CreatureGenome& genome);
    void ensureLocomotionCoherence(CreatureGenome& genome);
    void ensureVisualCoherence(CreatureGenome& genome);

    // Style application
    void applyPredatorTraits(CreatureGenome& genome);
    void applyPreyTraits(CreatureGenome& genome);
    void applyAquaticTraits(CreatureGenome& genome);
    void applyFlyingTraits(CreatureGenome& genome);

    // Random generation
    BodyPlanGenes randomBodyPlan(BodyCategory category, int seed);
    AppendageLayout randomAppendages(BodyPlanGenes& body, int seed);
    SurfaceFeatureLayout randomFeatures(BodyPlanGenes& body, int seed);
    ColorationGenes randomColoration(BiomeType biome, int seed);
    EyeGenes randomEyes(bool predator, int seed);
    MouthGenes randomMouth(DietType diet, int seed);
};

enum class CreatureArchetype {
    // Terrestrial
    SMALL_PREY,
    LARGE_HERBIVORE,
    SMALL_PREDATOR,
    APEX_PREDATOR,
    ARMORED_HERBIVORE,
    PACK_HUNTER,

    // Aquatic
    SCHOOLING_FISH,
    REEF_DWELLER,
    OPEN_OCEAN_PREDATOR,
    DEEP_SEA_CREATURE,
    COASTAL_FORAGER,

    // Flying
    SOARING_RAPTOR,
    HOVERING_POLLINATOR,
    SWIFT_AERIAL,
    NOCTURNAL_FLYER,

    // Exotic
    BURROWING,
    PARASITIC,
    COLONIAL,
    AMBUSH_PREDATOR
};

Creature CreatureGenerator::generate(const CreatureArchetype& archetype, int seed) {
    Random rng(seed);
    CreatureGenome genome;

    switch (archetype) {
        case CreatureArchetype::APEX_PREDATOR:
            genome.bodyPlan = randomBodyPlan(BodyCategory::LARGE_QUADRUPED, seed);
            genome.bodyPlan.bodyLength *= rng.range(1.5f, 2.5f);  // Make larger

            genome.appendages = randomAppendages(genome.bodyPlan, seed);
            // Ensure powerful legs
            for (auto& limb : genome.appendages.lowerAppendages) {
                limb.thickness *= 1.3f;
                limb.type = AppendageType::LEG_WALKING;
            }

            // Predator features
            genome.features = randomFeatures(genome.bodyPlan, seed);
            genome.features.headFeatures.push_back(createFangsMouth(1.2f));

            // Forward-facing eyes
            genome.eyes = createPredatorEyes();
            genome.eyes.eyeSize *= 1.2f;

            // Large jaw
            genome.mouth = createPredatorMouth();
            genome.mouth.gapeAngle = 100.0f;

            // Dark, intimidating colors
            genome.coloration.primaryColor = rng.colorRange(
                glm::vec3(0.2f, 0.15f, 0.1f),
                glm::vec3(0.4f, 0.35f, 0.25f)
            );
            genome.coloration.pattern = PatternType::STRIPED;
            applyPredatorTraits(genome);
            break;

        case CreatureArchetype::SMALL_PREY:
            genome.bodyPlan = randomBodyPlan(BodyCategory::SMALL_QUADRUPED, seed);
            genome.bodyPlan.bodyLength *= rng.range(0.3f, 0.7f);

            // Quick, agile legs
            genome.appendages = randomAppendages(genome.bodyPlan, seed);
            for (auto& limb : genome.appendages.lowerAppendages) {
                limb.length *= 1.2f;  // Long legs for running
            }

            // Wide-set prey eyes
            genome.eyes = createPreyEyes();

            // Small herbivore mouth
            genome.mouth = createHerbivoreMouth();

            // Camouflage coloring
            genome.coloration.pattern = PatternType::MOTTLED;
            applyPreyTraits(genome);
            break;

        case CreatureArchetype::DEEP_SEA_CREATURE:
            genome.bodyPlan = randomBodyPlan(BodyCategory::AQUATIC_ELONGATED, seed);
            genome.aquatic.bodyType = AquaticBodyType::ELONGATED;

            // Minimal appendages
            genome.appendages.lowerAppendages.clear();

            // Large eyes or none
            if (rng.chance(0.7f)) {
                genome.eyes.eyeSize *= 3.0f;
                genome.eyes.type = EyeType::TUBE;
            } else {
                genome.eyes.eyeCount = 0;
            }

            // Huge mouth for rare prey
            genome.mouth.type = MouthType::FANGED;
            genome.mouth.gapeAngle = 150.0f;
            genome.mouth.hasHingedJaw = true;

            // Bioluminescence
            genome.coloration.hasBioluminescence = true;
            genome.coloration.translucency = 0.3f;
            break;

        // ... more archetypes
    }

    // Ensure everything is coherent
    ensureBodyCoherence(genome);
    ensureAppendageCoherence(genome);
    ensureLocomotionCoherence(genome);
    ensureVisualCoherence(genome);

    return Creature(genome);
}

void CreatureGenerator::ensureVisualCoherence(CreatureGenome& genome) {
    // Eyes should match head size
    float headSize = genome.bodyPlan.headSize;
    float maxEyeSize = headSize * 0.4f;
    if (genome.eyes.eyeSize > maxEyeSize) {
        genome.eyes.eyeSize = maxEyeSize;
    }

    // Mouth should fit on head
    float maxMouthSize = headSize * 0.6f;
    if (genome.mouth.mouthSize > maxMouthSize) {
        genome.mouth.mouthSize = maxMouthSize;
    }

    // Surface features shouldn't clip through each other
    resolveFeatureClipping(genome.features);

    // Colors should be coherent
    ensureColorHarmony(genome.coloration);
}
```

CHECKPOINT 2: Unified generator implemented

============================================================================
PHASE 3: IMPLEMENT MESH ASSEMBLY (1+ hour)
============================================================================

```cpp
// Combine all mesh components into final creature

class CreatureMeshAssembler {
public:
    CompleteMesh assemble(const CreatureGenome& genome);

private:
    void assembleBody(CompleteMesh& mesh, const BodyPlanGenes& body);
    void attachAppendages(CompleteMesh& mesh, const AppendageLayout& appendages,
                          const BodyPlanGenes& body);
    void attachFeatures(CompleteMesh& mesh, const SurfaceFeatureLayout& features);
    void attachFace(CompleteMesh& mesh, const EyeGenes& eyes, const MouthGenes& mouth,
                    const SensoryGenes& sensory);
    void applyColors(CompleteMesh& mesh, const ColorationGenes& colors);
    void generateSkeleton(CompleteMesh& mesh, const CreatureGenome& genome);
    void calculateBoneWeights(CompleteMesh& mesh);
};

CompleteMesh CreatureMeshAssembler::assemble(const CreatureGenome& genome) {
    CompleteMesh mesh;

    // Generate body
    BodyMeshGenerator bodyGen;
    mesh.bodyMesh = bodyGen.generate(genome.bodyPlan);

    // Attach appendages at correct positions
    LimbGenerator limbGen;
    for (const auto& limb : genome.appendages.lowerAppendages) {
        auto limbMesh = limbGen.generateLimb(limb);
        attachToBody(mesh, limbMesh, limb.attachPoint, limb.attachAngle,
                     genome.bodyPlan.symmetry);
    }

    for (const auto& wing : genome.appendages.upperAppendages) {
        if (isWingType(wing.type)) {
            auto wingMesh = wingGen.generateWing(wing);
            attachToBody(mesh, wingMesh, wing.attachPoint, wing.attachAngle,
                        genome.bodyPlan.symmetry);
        }
    }

    // Surface features
    FeatureMeshGenerator featureGen;
    for (const auto& feature : genome.features.headFeatures) {
        auto featureMesh = featureGen.generate(feature);
        attachToHead(mesh, featureMesh, feature);
    }
    for (const auto& feature : genome.features.bodyFeatures) {
        auto featureMesh = featureGen.generate(feature);
        attachToBody(mesh, featureMesh, feature.position);
    }

    // Face
    SensoryMeshGenerator sensoryGen;
    auto eyeMesh = sensoryGen.generateEyes(genome.eyes);
    auto mouthMesh = sensoryGen.generateMouth(genome.mouth);
    attachToHead(mesh, eyeMesh, genome.eyes.eyePosition);
    attachToHead(mesh, mouthMesh, genome.mouth.mouthPosition);

    // Generate skeleton matching the mesh
    generateSkeleton(mesh, genome);

    // Calculate bone weights for skinning
    calculateBoneWeights(mesh);

    // Apply materials and colors
    applyColors(mesh, genome.coloration);

    // Generate LOD levels
    mesh.lodLevels[0] = mesh;  // Full detail
    mesh.lodLevels[1] = simplifyMesh(mesh, 0.5f);
    mesh.lodLevels[2] = simplifyMesh(mesh, 0.25f);
    mesh.lodLevels[3] = simplifyMesh(mesh, 0.1f);

    return mesh;
}

void CreatureMeshAssembler::attachToBody(CompleteMesh& mesh, const GeneratedMesh& part,
                                          glm::vec3 attachPoint, float attachAngle,
                                          SymmetryType symmetry) {
    // Transform part to world space
    glm::mat4 transform = calculateAttachTransform(mesh.bodyMesh, attachPoint, attachAngle);

    // Add transformed vertices
    int baseIndex = mesh.vertices.size();
    for (const auto& v : part.vertices) {
        Vertex transformed = v;
        transformed.position = glm::vec3(transform * glm::vec4(v.position, 1.0f));
        transformed.normal = glm::mat3(transform) * v.normal;
        mesh.vertices.push_back(transformed);
    }

    // Add offset indices
    for (uint32_t idx : part.indices) {
        mesh.indices.push_back(baseIndex + idx);
    }

    // Mirror for bilateral symmetry
    if (symmetry == SymmetryType::BILATERAL && attachPoint.x > 0.01f) {
        glm::mat4 mirrorTransform = transform;
        mirrorTransform[0][0] *= -1.0f;  // Mirror X

        baseIndex = mesh.vertices.size();
        for (const auto& v : part.vertices) {
            Vertex transformed = v;
            transformed.position = glm::vec3(mirrorTransform * glm::vec4(v.position, 1.0f));
            transformed.normal = glm::mat3(mirrorTransform) * v.normal;
            transformed.normal.x *= -1.0f;
            mesh.vertices.push_back(transformed);
        }

        // Reverse winding for mirrored geometry
        for (int i = part.indices.size() - 1; i >= 0; i--) {
            mesh.indices.push_back(baseIndex + part.indices[i]);
        }
    }
}
```

CHECKPOINT 3: Mesh assembly working

============================================================================
PHASE 4: IMPLEMENT VISUAL POLISH (45+ minutes)
============================================================================

```cpp
// Final visual touches

class CreatureVisualPolish {
public:
    void applyShaderEffects(Creature& creature);
    void generateDetailTextures(Creature& creature);
    void setupDynamicEffects(Creature& creature);
    void calibrateAnimation(Creature& creature);

private:
    void generateNormalMap(Creature& creature);
    void generateRoughnessMap(Creature& creature);
    void addSkinSubsurface(Creature& creature);
    void addEyeReflections(Creature& creature);
    void addWetness(Creature& creature, float amount);
    void addDirt(Creature& creature, float amount);
    void addScars(Creature& creature, int count);
};

void CreatureVisualPolish::applyShaderEffects(Creature& creature) {
    auto& material = creature.getMaterial();

    // Base PBR setup
    material.setAlbedoTexture(creature.getPatternTexture());
    material.setNormalMap(generateNormalMap(creature));
    material.setRoughnessMap(generateRoughnessMap(creature));

    // Subsurface scattering for skin
    if (creature.hasSoftSkin()) {
        material.setSubsurfaceScattering(true);
        material.setSubsurfaceColor(creature.getColoration().primaryColor * 0.8f);
        material.setSubsurfaceRadius(0.5f);
    }

    // Iridescence for special creatures
    if (creature.getColoration().iridescence > 0.0f) {
        material.setIridescence(creature.getColoration().iridescence);
        material.setIridescenceIOR(1.5f);
    }

    // Eye special effects
    if (creature.hasEyes()) {
        auto& eyeMaterial = creature.getEyeMaterial();
        eyeMaterial.setReflectivity(0.3f);
        eyeMaterial.setClearcoat(0.8f);

        if (creature.getEyes().hasEyeshine) {
            eyeMaterial.setEmissive(creature.getEyes().eyeshineColor * 0.2f);
        }
    }

    // Wet/slimy creatures
    if (creature.isAquatic() || creature.isAmphibian()) {
        addWetness(creature, 0.5f);
    }
}

void CreatureVisualPolish::generateDetailTextures(Creature& creature) {
    int resolution = 1024;

    // Detail normal map for skin texture
    Texture2D detailNormal(resolution, resolution);
    switch (creature.getSkinType()) {
        case SkinType::SCALES:
            generateScalePattern(detailNormal, creature.getScaleSize());
            break;
        case SkinType::FUR:
            generateFurDirection(detailNormal);
            break;
        case SkinType::FEATHERS:
            generateFeatherPattern(detailNormal);
            break;
        case SkinType::SMOOTH:
            generateSubtleNoise(detailNormal, 0.1f);
            break;
        case SkinType::BUMPY:
            generateVoronoiBumps(detailNormal, creature.getBumpSize());
            break;
    }

    creature.getMaterial().setDetailNormalMap(detailNormal);
}

void CreatureVisualPolish::calibrateAnimation(Creature& creature) {
    // Ensure animation speeds match creature size
    float sizeScale = creature.getActualSize();

    // Larger creatures move slower but with bigger strides
    creature.setAnimationSpeed(1.0f / sqrt(sizeScale));
    creature.setStrideLength(sqrt(sizeScale));

    // Blinking rate varies
    creature.setBlinkRate(2.0f / sizeScale);  // Smaller blink faster

    // Breathing rate
    creature.setBreathRate(0.5f / sqrt(sizeScale));

    // Idle fidgeting
    creature.setIdleIntensity(1.0f / sizeScale);
}

void CreatureVisualPolish::addScars(Creature& creature, int count) {
    // Add battle damage for older creatures
    auto& mesh = creature.getMesh();
    auto& texture = creature.getPatternTexture();

    for (int i = 0; i < count; i++) {
        // Random position on body
        glm::vec2 uv = randomUVOnBody(creature);

        // Scar parameters
        float length = creature.getRandom().range(0.02f, 0.08f);
        float width = length * 0.1f;
        float angle = creature.getRandom().range(0.0f, 2.0f * PI);

        // Draw scar on texture
        glm::vec3 scarColor = creature.getColoration().primaryColor * 0.7f;
        drawScarLine(texture, uv, length, width, angle, scarColor);

        // Add slight mesh deformation
        addMeshScarIndent(mesh, uv, length, width * 0.5f);
    }
}
```

CHECKPOINT 4: Visual polish applied

============================================================================
PHASE 5: FINAL TESTING & DOCUMENTATION (30+ minutes)
============================================================================

SPAWN 2 SUB-AGENTS IN PARALLEL:

SUB-AGENT 5A - Test creature generation:
- Generate 100 random creatures
- Check for mesh errors
- Check for animation issues
- Document any problems

SUB-AGENT 5B - Create showcase:
- Generate example creatures of each type
- Screenshot each for documentation
- Create creature gallery
- Document creature variety

```cpp
// Test creature variety
void testCreatureVariety() {
    CreatureGenerator gen;
    std::vector<std::string> issues;

    // Test each archetype
    for (auto archetype : getAllArchetypes()) {
        for (int seed = 0; seed < 10; seed++) {
            try {
                Creature c = gen.generate(archetype, seed);

                // Validate mesh
                if (!c.getMesh().isValid()) {
                    issues.push_back("Invalid mesh: " + archetypeName(archetype) +
                                    " seed " + std::to_string(seed));
                }

                // Validate skeleton
                if (!c.getSkeleton().isValid()) {
                    issues.push_back("Invalid skeleton: " + archetypeName(archetype));
                }

                // Validate animations
                for (auto anim : c.getAnimations()) {
                    if (!anim.isValid()) {
                        issues.push_back("Invalid animation: " + anim.getName());
                    }
                }

                // Check visual coherence
                if (c.getEyes().eyeSize > c.getHead().size * 0.5f) {
                    issues.push_back("Eyes too large for head");
                }

            } catch (const std::exception& e) {
                issues.push_back("Exception: " + std::string(e.what()));
            }
        }
    }

    // Write report
    std::ofstream report("docs/CREATURE_VARIETY_TEST_REPORT.md");
    report << "# Creature Variety Test Report\\n\\n";
    report << "Tested " << getAllArchetypes().size() * 10 << " creatures\\n\\n";

    if (issues.empty()) {
        report << "## Result: PASS\\n\\n";
        report << "All creatures generated successfully.\\n";
    } else {
        report << "## Result: ISSUES FOUND\\n\\n";
        for (const auto& issue : issues) {
            report << "- " << issue << "\\n";
        }
    }
}
```

CHECKPOINT 5: Testing complete

============================================================================
DELIVERABLES CHECKLIST
============================================================================

- [ ] docs/CREATURE_VARIETY_AUDIT.md
- [ ] CreatureGenerator class
- [ ] All creature archetypes
- [ ] Coherence checking functions
- [ ] CreatureMeshAssembler class
- [ ] Multi-part mesh assembly
- [ ] Skeleton generation
- [ ] Bone weight calculation
- [ ] LOD generation
- [ ] CreatureVisualPolish class
- [ ] Shader effect setup
- [ ] Detail texture generation
- [ ] Animation calibration
- [ ] Scar/damage system
- [ ] Creature variety test suite
- [ ] Test report generated
- [ ] Example creature gallery

ESTIMATED TIME: 4-5 hours with sub-agents
```

---

# Phase 5 Summary

These 10 agents focus entirely on creating visually diverse, interesting creatures:

1. **Agent 1: Body Morphology** - Procedural body shapes from genes (spherical, elongated, segmented, etc.)
2. **Agent 2: Limb & Appendages** - Modular limbs (legs, wings, tentacles, fins) with any count
3. **Agent 3: Exotic Locomotion** - Snake, jellyfish, crab, and other unique movement
4. **Agent 4: Size Scaling** - Microscopic to massive creatures with proper physics
5. **Agent 5: Surface Features** - Horns, shells, spines, manes, and ornamentation
6. **Agent 6: Skin Patterns** - Spots, stripes, camouflage, iridescence, bioluminescence
7. **Agent 7: Aquatic Variety** - Fish, rays, eels, jellyfish, deep-sea creatures
8. **Agent 8: Flying Variety** - Birds, bats, insects, dragons, fantasy flyers
9. **Agent 9: Sensory Organs** - Eyes, mouths, ears, whiskers with expressions
10. **Agent 10: Integration** - Unite all systems, polish visuals, ensure coherence

Run Agents 1-9 in parallel, then Agent 10 after all complete.

**Expected Result**: Creatures that are genuinely visually distinct - from tiny iridescent insects to massive armored predators, with unique body plans, appendages, patterns, and expressive faces.
