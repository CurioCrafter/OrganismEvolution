#include "FishMeshGenerator.h"
#include "MarchingCubes.h"
#include <cmath>
#include <algorithm>

FishMeshGenerator::FishMeshGenerator() {}

// =============================================================================
// MAIN GENERATION ENTRY POINTS
// =============================================================================

MeshData FishMeshGenerator::generateFromGenome(const Genome& genome, AquaticSpecies species) {
    switch (species) {
        // Fish types
        case AquaticSpecies::GENERIC_FISH:
        case AquaticSpecies::TROPICAL_FISH:
        case AquaticSpecies::CORAL_REEF_FISH:
            return generateFish(genome, FishBodyShape::LATERALLY_COMPRESSED);

        case AquaticSpecies::TUNA:
        case AquaticSpecies::MACKEREL:
            return generateFish(genome, FishBodyShape::FUSIFORM);

        case AquaticSpecies::DEEP_SEA_FISH:
        case AquaticSpecies::ANGLERFISH:
            return generateFish(genome, FishBodyShape::GLOBIFORM);

        case AquaticSpecies::BARRACUDA:
            return generateFish(genome, FishBodyShape::TORPEDO);

        // Sharks
        case AquaticSpecies::SHARK:
        case AquaticSpecies::GREAT_WHITE:
        case AquaticSpecies::HAMMERHEAD:
            return generateShark(genome);

        // Rays
        case AquaticSpecies::MANTA_RAY:
        case AquaticSpecies::STINGRAY:
            return generateRay(genome);

        // Eels
        case AquaticSpecies::EEL:
        case AquaticSpecies::MORAY_EEL:
        case AquaticSpecies::ELECTRIC_EEL:
            return generateEel(genome);

        // Jellyfish
        case AquaticSpecies::JELLYFISH:
        case AquaticSpecies::BOX_JELLYFISH: {
            JellyfishConfig config;
            config.bellRadius = genome.size * 0.5f;
            config.bellHeight = config.bellRadius * 0.8f;
            config.tentacleCount = 16 + static_cast<int>(genome.size * 8);
            config.tentacleLength = genome.size * 2.0f;
            return generateJellyfish(genome, config);
        }

        // Cephalopods
        case AquaticSpecies::OCTOPUS: {
            CephalopodConfig config;
            config.mantleLength = genome.size * 0.8f;
            config.armCount = 8;
            config.armLength = genome.size * 1.5f;
            return generateOctopus(genome, config);
        }

        case AquaticSpecies::SQUID:
        case AquaticSpecies::GIANT_SQUID: {
            CephalopodConfig config;
            config.mantleLength = genome.size;
            config.armCount = 10;
            config.armLength = genome.size * 1.2f;
            config.hasTentacles = true;
            config.tentacleLength = genome.size * 2.5f;
            config.finSize = 0.4f;
            return generateSquid(genome, config);
        }

        // Crustaceans
        case AquaticSpecies::CRAB: {
            CrustaceanConfig config;
            config.carapaceWidth = genome.size * 0.8f;
            config.carapaceLength = genome.size * 0.6f;
            config.clawSize = genome.size * 0.5f;
            config.tailLength = 0.1f;
            config.hasTailFan = false;
            return generateCrab(genome, config);
        }

        case AquaticSpecies::LOBSTER:
        case AquaticSpecies::SHRIMP: {
            CrustaceanConfig config;
            config.carapaceLength = genome.size * 0.6f;
            config.tailLength = genome.size * 0.5f;
            return generateLobster(genome, config);
        }

        // Marine mammals
        case AquaticSpecies::DOLPHIN:
        case AquaticSpecies::ORCA:
            return generateDolphin(genome);

        case AquaticSpecies::WHALE:
        case AquaticSpecies::BLUE_WHALE:
        case AquaticSpecies::HUMPBACK:
            return generateWhale(genome);

        case AquaticSpecies::SEAHORSE:
            return generateSeaHorse(genome);

        default:
            return generateFish(genome, FishBodyShape::FUSIFORM);
    }
}

// =============================================================================
// FISH GENERATION
// =============================================================================

MeshData FishMeshGenerator::generateFish(const Genome& genome, FishBodyShape bodyShape) {
    MetaballSystem metaballs;

    float length = genome.size * 2.0f;
    float width = genome.size * 0.5f;
    float height = genome.size * 0.6f;

    // Adjust dimensions based on body shape
    switch (bodyShape) {
        case FishBodyShape::LATERALLY_COMPRESSED:
            width *= 0.5f;
            height *= 1.3f;
            break;
        case FishBodyShape::DEPRESSED:
            height *= 0.4f;
            width *= 1.5f;
            break;
        case FishBodyShape::ELONGATED:
            length *= 2.0f;
            width *= 0.4f;
            height *= 0.4f;
            break;
        case FishBodyShape::GLOBIFORM:
            length *= 0.7f;
            width *= 1.2f;
            height *= 1.2f;
            break;
        case FishBodyShape::TORPEDO:
            width *= 0.7f;
            height *= 0.8f;
            break;
        default:
            break;
    }

    // Build main body
    buildFishBody(metaballs, bodyShape, length, width, height);

    // Build head
    buildFishHead(metaballs, glm::vec3(length * 0.4f, 0.0f, 0.0f),
                  genome.size * 0.4f, genome.visionRange > 30.0f);

    // Build tail section
    buildFishTail(metaballs, glm::vec3(-length * 0.4f, 0.0f, 0.0f),
                  genome.size * 0.3f, genome.caudalFinType);

    // Convert metaballs to mesh
    MeshData mesh = marchingCubes(metaballs);

    // Add fins
    FinConfiguration fins;
    fins.dorsalHeight = genome.dorsalFinHeight;
    fins.pectoralSize = genome.pectoralFinWidth;
    fins.caudalSize = genome.tailSize;
    fins.caudalForkDepth = genome.caudalFinType;

    // Dorsal fin
    addDorsalFin(mesh,
                 glm::vec3(length * 0.1f, height * 0.5f, 0.0f),
                 glm::vec3(-length * 0.2f, height * 0.5f, 0.0f),
                 fins.dorsalHeight * genome.size,
                 false);

    // Pectoral fins (left and right)
    addPectoralFin(mesh,
                   glm::vec3(length * 0.2f, 0.0f, width * 0.5f),
                   fins.pectoralSize * genome.size,
                   0.3f, false);
    addPectoralFin(mesh,
                   glm::vec3(length * 0.2f, 0.0f, -width * 0.5f),
                   fins.pectoralSize * genome.size,
                   0.3f, true);

    // Caudal fin
    addCaudalFin(mesh,
                 glm::vec3(-length * 0.5f, 0.0f, 0.0f),
                 fins.caudalSize * genome.size,
                 fins.caudalForkDepth,
                 0.0f);

    // Pelvic fins
    addPelvicFin(mesh,
                 glm::vec3(length * 0.0f, -height * 0.4f, width * 0.3f),
                 genome.pelvicFinSize * genome.size, false);
    addPelvicFin(mesh,
                 glm::vec3(length * 0.0f, -height * 0.4f, -width * 0.3f),
                 genome.pelvicFinSize * genome.size, true);

    // Anal fin
    addAnalFin(mesh,
               glm::vec3(-length * 0.2f, -height * 0.4f, 0.0f),
               genome.analFinSize * genome.size);

    // Smooth and finalize
    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateShark(const Genome& genome) {
    MetaballSystem metaballs;

    float length = genome.size * 3.0f;
    float width = genome.size * 0.6f;
    float height = genome.size * 0.7f;

    // Sharks have torpedo-shaped bodies
    buildFishBody(metaballs, FishBodyShape::TORPEDO, length, width, height);

    // Shark head with pointed snout
    float headSize = genome.size * 0.5f;
    metaballs.addMetaball(glm::vec3(length * 0.4f, 0.0f, 0.0f), headSize, 1.0f);
    metaballs.addMetaball(glm::vec3(length * 0.5f, -headSize * 0.2f, 0.0f), headSize * 0.6f, 0.8f);

    // Shark tail (asymmetric caudal fin)
    buildFishTail(metaballs, glm::vec3(-length * 0.4f, 0.0f, 0.0f),
                  genome.size * 0.25f, 0.8f);

    MeshData mesh = marchingCubes(metaballs);

    // Prominent dorsal fin
    addDorsalFin(mesh,
                 glm::vec3(length * 0.1f, height * 0.5f, 0.0f),
                 glm::vec3(-length * 0.05f, height * 0.5f, 0.0f),
                 genome.dorsalFinHeight * genome.size * 1.5f,
                 false);

    // Smaller second dorsal
    addDorsalFin(mesh,
                 glm::vec3(-length * 0.25f, height * 0.4f, 0.0f),
                 glm::vec3(-length * 0.3f, height * 0.4f, 0.0f),
                 genome.dorsalFinHeight * genome.size * 0.4f,
                 false);

    // Large pectoral fins
    addPectoralFin(mesh,
                   glm::vec3(length * 0.15f, -height * 0.1f, width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 1.3f,
                   0.5f, false);
    addPectoralFin(mesh,
                   glm::vec3(length * 0.15f, -height * 0.1f, -width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 1.3f,
                   0.5f, true);

    // Asymmetric caudal fin (longer upper lobe)
    addCaudalFin(mesh,
                 glm::vec3(-length * 0.5f, 0.0f, 0.0f),
                 genome.tailSize * genome.size,
                 0.6f,
                 0.4f);  // Asymmetry for shark tail

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

// =============================================================================
// JELLYFISH GENERATION
// =============================================================================

MeshData FishMeshGenerator::generateJellyfish(const Genome& genome, const JellyfishConfig& config) {
    MeshData mesh;

    // Build bell
    buildJellyfishBell(mesh, config.bellRadius, config.bellHeight,
                       config.bellThickness, config.frillAmount);

    // Add tentacles
    addJellyfishTentacles(mesh, config.tentacleCount, config.tentacleLength,
                          config.tentacleThickness, config.bellRadius);

    // Add oral arms
    addOralArms(mesh, config.oralArmCount, config.oralArmLength, config.bellRadius);

    calculateNormals(mesh);
    generateUVs(mesh, glm::vec3(0, -1, 0));  // UV from above
    mesh.calculateBounds();

    return mesh;
}

void FishMeshGenerator::buildJellyfishBell(
    MeshData& mesh,
    float radius,
    float height,
    float thickness,
    float frilling
) {
    const int radialSegments = 32;
    const int heightSegments = 16;

    // Generate bell as a dome shape
    for (int h = 0; h <= heightSegments; h++) {
        float v = static_cast<float>(h) / heightSegments;
        float bellY = -height * (1.0f - std::cos(v * 3.14159f * 0.5f));

        // Bell curves inward at the bottom
        float bellRadius = radius * std::sin(v * 3.14159f * 0.5f);

        // Add frilling at the edge
        float frillMod = 1.0f;
        if (h > heightSegments * 0.8f) {
            float frillPhase = v * 20.0f;
            frillMod = 1.0f + frilling * std::sin(frillPhase * 3.14159f);
        }
        bellRadius *= frillMod;

        for (int r = 0; r < radialSegments; r++) {
            float angle = static_cast<float>(r) / radialSegments * 2.0f * 3.14159f;

            // Outer surface
            glm::vec3 outerPos(
                bellRadius * std::cos(angle),
                bellY,
                bellRadius * std::sin(angle)
            );

            // Inner surface (offset by thickness)
            float innerRadius = bellRadius - thickness;
            if (innerRadius < 0) innerRadius = 0;
            glm::vec3 innerPos(
                innerRadius * std::cos(angle),
                bellY + thickness * 0.5f,
                innerRadius * std::sin(angle)
            );

            glm::vec3 normal = glm::normalize(outerPos - glm::vec3(0, bellY, 0));
            mesh.vertices.push_back(Vertex(outerPos, normal,
                glm::vec2(static_cast<float>(r) / radialSegments, v)));

            // Add inner surface vertices for thickness
            if (h < heightSegments) {
                mesh.vertices.push_back(Vertex(innerPos, -normal,
                    glm::vec2(static_cast<float>(r) / radialSegments, v)));
            }
        }
    }

    // Generate indices for bell surface
    for (int h = 0; h < heightSegments; h++) {
        for (int r = 0; r < radialSegments; r++) {
            int current = h * radialSegments * 2 + r * 2;
            int next = h * radialSegments * 2 + ((r + 1) % radialSegments) * 2;
            int nextRow = (h + 1) * radialSegments * 2 + r * 2;
            int nextRowNext = (h + 1) * radialSegments * 2 + ((r + 1) % radialSegments) * 2;

            // Outer surface
            mesh.indices.push_back(current);
            mesh.indices.push_back(nextRow);
            mesh.indices.push_back(next);

            mesh.indices.push_back(next);
            mesh.indices.push_back(nextRow);
            mesh.indices.push_back(nextRowNext);

            // Inner surface (reversed winding)
            if (h < heightSegments - 1) {
                mesh.indices.push_back(current + 1);
                mesh.indices.push_back(next + 1);
                mesh.indices.push_back(nextRow + 1);

                mesh.indices.push_back(next + 1);
                mesh.indices.push_back(nextRowNext + 1);
                mesh.indices.push_back(nextRow + 1);
            }
        }
    }
}

void FishMeshGenerator::addJellyfishTentacles(
    MeshData& mesh,
    int count,
    float length,
    float thickness,
    float bellRadius
) {
    const int lengthSegments = 16;
    const int radialSegments = 4;

    for (int t = 0; t < count; t++) {
        float angle = static_cast<float>(t) / count * 2.0f * 3.14159f;
        float attachRadius = bellRadius * 0.8f;

        glm::vec3 attachPoint(
            attachRadius * std::cos(angle),
            0.0f,
            attachRadius * std::sin(angle)
        );

        // Generate wavy tentacle centerline
        std::vector<glm::vec3> centerline;
        std::vector<float> radii;

        for (int s = 0; s <= lengthSegments; s++) {
            float v = static_cast<float>(s) / lengthSegments;

            // Wavy motion offset
            float waveX = std::sin(v * 4.0f * 3.14159f + angle) * 0.1f * length;
            float waveZ = std::cos(v * 3.0f * 3.14159f + angle * 1.5f) * 0.08f * length;

            glm::vec3 pos = attachPoint + glm::vec3(
                waveX,
                -v * length,
                waveZ
            );
            centerline.push_back(pos);

            // Taper tentacle
            radii.push_back(thickness * (1.0f - v * 0.8f));
        }

        // Generate tube mesh for tentacle
        MeshData tentacle = generateTubeMesh(centerline, radii, radialSegments);
        mergeMeshes(mesh, tentacle);
    }
}

void FishMeshGenerator::addOralArms(
    MeshData& mesh,
    int count,
    float length,
    float bellRadius
) {
    const int lengthSegments = 12;

    for (int a = 0; a < count; a++) {
        float angle = static_cast<float>(a) / count * 2.0f * 3.14159f + 3.14159f / count;

        glm::vec3 attachPoint(
            bellRadius * 0.3f * std::cos(angle),
            0.0f,
            bellRadius * 0.3f * std::sin(angle)
        );

        std::vector<glm::vec3> centerline;
        std::vector<float> radii;

        for (int s = 0; s <= lengthSegments; s++) {
            float v = static_cast<float>(s) / lengthSegments;

            // Oral arms are more rigid, gentle curves
            glm::vec3 pos = attachPoint + glm::vec3(
                std::sin(angle) * v * length * 0.3f,
                -v * length,
                std::cos(angle) * v * length * 0.3f
            );
            centerline.push_back(pos);

            // Wider at base, tapering
            radii.push_back(bellRadius * 0.15f * (1.0f - v * 0.7f));
        }

        MeshData arm = generateTubeMesh(centerline, radii, 6);
        mergeMeshes(mesh, arm);
    }
}

// =============================================================================
// CEPHALOPOD GENERATION
// =============================================================================

MeshData FishMeshGenerator::generateOctopus(const Genome& genome, const CephalopodConfig& config) {
    MetaballSystem metaballs;

    // Build mantle (head/body)
    buildCephalopodMantle(metaballs, config.mantleLength, config.mantleWidth, false);

    // Add eyes
    float eyeOffset = config.mantleWidth * 0.4f;
    addCephalopodEye(metaballs, glm::vec3(config.mantleLength * 0.3f, 0.0f, eyeOffset), config.eyeSize);
    addCephalopodEye(metaballs, glm::vec3(config.mantleLength * 0.3f, 0.0f, -eyeOffset), config.eyeSize);

    MeshData mesh = marchingCubes(metaballs);

    // Add 8 arms
    for (int a = 0; a < config.armCount; a++) {
        float angle = static_cast<float>(a) / config.armCount * 2.0f * 3.14159f;

        glm::vec3 attachPoint(
            config.mantleLength * 0.4f,
            -config.mantleWidth * 0.3f,
            0.0f
        );

        glm::vec3 direction(
            std::cos(angle) * 0.3f + 0.7f,
            -0.5f,
            std::sin(angle)
        );
        direction = glm::normalize(direction);

        addCephalopodArm(mesh, attachPoint, direction,
                         config.armLength, config.armThickness, config.suckerRows);
    }

    smoothMesh(mesh, 1);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateSquid(const Genome& genome, const CephalopodConfig& config) {
    MetaballSystem metaballs;

    // Squid has elongated mantle with fins
    buildCephalopodMantle(metaballs, config.mantleLength * 1.5f, config.mantleWidth * 0.7f, true);

    // Add eyes
    float eyeOffset = config.mantleWidth * 0.35f;
    addCephalopodEye(metaballs, glm::vec3(config.mantleLength * 0.5f, 0.0f, eyeOffset), config.eyeSize);
    addCephalopodEye(metaballs, glm::vec3(config.mantleLength * 0.5f, 0.0f, -eyeOffset), config.eyeSize);

    MeshData mesh = marchingCubes(metaballs);

    // Add 8 regular arms
    for (int a = 0; a < 8; a++) {
        float angle = static_cast<float>(a) / 8 * 2.0f * 3.14159f;

        glm::vec3 attachPoint(config.mantleLength * 0.6f, 0.0f, 0.0f);
        glm::vec3 direction(0.8f, -0.2f, std::sin(angle) * 0.5f);
        direction = glm::normalize(direction);

        addCephalopodArm(mesh, attachPoint, direction,
                         config.armLength, config.armThickness, config.suckerRows);
    }

    // Add 2 long tentacles
    for (int t = 0; t < 2; t++) {
        float side = t == 0 ? 1.0f : -1.0f;

        glm::vec3 attachPoint(config.mantleLength * 0.6f, 0.0f, side * config.mantleWidth * 0.2f);
        glm::vec3 direction(1.0f, 0.0f, side * 0.2f);
        direction = glm::normalize(direction);

        addCephalopodArm(mesh, attachPoint, direction,
                         config.tentacleLength, config.armThickness * 0.6f, 1);
    }

    smoothMesh(mesh, 1);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

void FishMeshGenerator::buildCephalopodMantle(
    MetaballSystem& metaballs,
    float length,
    float width,
    bool hasFins
) {
    // Main mantle body
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), length * 0.4f, 1.0f);
    metaballs.addMetaball(glm::vec3(-length * 0.2f, 0.0f, 0.0f), length * 0.35f, 0.9f);
    metaballs.addMetaball(glm::vec3(-length * 0.4f, 0.0f, 0.0f), length * 0.25f, 0.8f);

    // Head region
    metaballs.addMetaball(glm::vec3(length * 0.3f, 0.0f, 0.0f), length * 0.3f, 1.0f);

    // If squid, add fin bulges
    if (hasFins) {
        metaballs.addMetaball(glm::vec3(-length * 0.3f, 0.0f, width * 0.5f), length * 0.15f, 0.6f);
        metaballs.addMetaball(glm::vec3(-length * 0.3f, 0.0f, -width * 0.5f), length * 0.15f, 0.6f);
    }
}

void FishMeshGenerator::addCephalopodArm(
    MeshData& mesh,
    const glm::vec3& attachPoint,
    const glm::vec3& direction,
    float length,
    float thickness,
    int suckerRows
) {
    const int segments = 16;
    const int radialSegments = 6;

    std::vector<glm::vec3> centerline;
    std::vector<float> radii;

    // Generate curved arm centerline
    glm::vec3 currentPos = attachPoint;
    glm::vec3 currentDir = direction;

    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;

        // Add some curl to the arm
        float curl = v * v * 0.5f;
        glm::vec3 curlOffset(0.0f, -curl * length * 0.3f, 0.0f);

        centerline.push_back(currentPos + curlOffset);

        // Taper from base to tip
        float taper = 1.0f - v * 0.85f;
        radii.push_back(thickness * taper);

        // Move along arm
        currentPos += currentDir * (length / segments);

        // Gradually curve downward
        currentDir = glm::normalize(currentDir + glm::vec3(0.0f, -0.05f, 0.0f));
    }

    MeshData arm = generateTubeMesh(centerline, radii, radialSegments);
    mergeMeshes(mesh, arm);
}

void FishMeshGenerator::addCephalopodEye(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size
) {
    metaballs.addMetaball(position, size, 1.2f);
}

// =============================================================================
// CRUSTACEAN GENERATION
// =============================================================================

MeshData FishMeshGenerator::generateCrab(const Genome& genome, const CrustaceanConfig& config) {
    MetaballSystem metaballs;

    // Build carapace (wide and flat)
    buildCrustaceanCarapace(metaballs, config.carapaceLength, config.carapaceWidth, config.carapaceHeight);

    // Add claws
    if (config.hasClaws) {
        addCrustaceanClaw(metaballs,
            glm::vec3(config.carapaceLength * 0.3f, 0.0f, config.carapaceWidth * 0.6f),
            config.clawSize, false);
        addCrustaceanClaw(metaballs,
            glm::vec3(config.carapaceLength * 0.3f, 0.0f, -config.carapaceWidth * 0.6f),
            config.clawSize, true);
    }

    MeshData mesh = marchingCubes(metaballs);

    // Add walking legs (4 pairs)
    for (int side = -1; side <= 1; side += 2) {
        for (int leg = 0; leg < config.legPairs - 1; leg++) {
            float xOffset = config.carapaceLength * (0.1f - leg * 0.15f);
            glm::vec3 attachPoint(xOffset, -config.carapaceHeight * 0.3f,
                                  side * config.carapaceWidth * 0.5f);

            addCrustaceanLeg(mesh, attachPoint, config.legLength, 3, false);
        }
    }

    // Add eye stalks
    metaballs.addMetaball(
        glm::vec3(config.carapaceLength * 0.4f, config.carapaceHeight * 0.3f, config.carapaceWidth * 0.2f),
        config.carapaceHeight * 0.15f, 0.8f);
    metaballs.addMetaball(
        glm::vec3(config.carapaceLength * 0.4f, config.carapaceHeight * 0.3f, -config.carapaceWidth * 0.2f),
        config.carapaceHeight * 0.15f, 0.8f);

    // Add antennae
    addCrustaceanAntenna(mesh,
        glm::vec3(config.carapaceLength * 0.5f, config.carapaceHeight * 0.2f, config.carapaceWidth * 0.1f),
        config.antennaLength, false);
    addCrustaceanAntenna(mesh,
        glm::vec3(config.carapaceLength * 0.5f, config.carapaceHeight * 0.2f, -config.carapaceWidth * 0.1f),
        config.antennaLength, false);

    smoothMesh(mesh, 1);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateLobster(const Genome& genome, const CrustaceanConfig& config) {
    MetaballSystem metaballs;

    // Build carapace (elongated)
    buildCrustaceanCarapace(metaballs,
        config.carapaceLength * 1.5f, config.carapaceWidth * 0.6f, config.carapaceHeight);

    // Build segmented tail
    float tailX = -config.carapaceLength * 0.5f;
    for (int seg = 0; seg < static_cast<int>(config.tailSegments); seg++) {
        float segScale = 1.0f - seg * 0.1f;
        metaballs.addMetaball(
            glm::vec3(tailX, 0.0f, 0.0f),
            config.carapaceHeight * 0.4f * segScale, 0.9f);
        tailX -= config.tailLength / config.tailSegments;
    }

    // Add large claws
    if (config.hasClaws) {
        addCrustaceanClaw(metaballs,
            glm::vec3(config.carapaceLength * 0.6f, 0.0f, config.carapaceWidth * 0.5f),
            config.clawSize * 1.5f, false);
        addCrustaceanClaw(metaballs,
            glm::vec3(config.carapaceLength * 0.6f, 0.0f, -config.carapaceWidth * 0.5f),
            config.clawSize * 1.5f, true);
    }

    MeshData mesh = marchingCubes(metaballs);

    // Add walking legs
    for (int side = -1; side <= 1; side += 2) {
        for (int leg = 0; leg < config.legPairs - 1; leg++) {
            float xOffset = config.carapaceLength * (0.2f - leg * 0.2f);
            glm::vec3 attachPoint(xOffset, -config.carapaceHeight * 0.3f,
                                  side * config.carapaceWidth * 0.4f);
            addCrustaceanLeg(mesh, attachPoint, config.legLength, 3, false);
        }
    }

    // Add long antennae
    addCrustaceanAntenna(mesh,
        glm::vec3(config.carapaceLength * 0.7f, config.carapaceHeight * 0.2f, config.carapaceWidth * 0.15f),
        config.antennaLength * 2.0f, true);
    addCrustaceanAntenna(mesh,
        glm::vec3(config.carapaceLength * 0.7f, config.carapaceHeight * 0.2f, -config.carapaceWidth * 0.15f),
        config.antennaLength * 2.0f, true);

    smoothMesh(mesh, 1);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

void FishMeshGenerator::buildCrustaceanCarapace(
    MetaballSystem& metaballs,
    float length,
    float width,
    float height
) {
    // Main carapace body
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), length * 0.4f, 1.0f);
    metaballs.addMetaball(glm::vec3(length * 0.2f, 0.0f, 0.0f), length * 0.35f, 0.9f);
    metaballs.addMetaball(glm::vec3(-length * 0.15f, 0.0f, 0.0f), length * 0.3f, 0.85f);

    // Flatten vertically, widen horizontally
    // (This is approximated by additional metaballs)
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, width * 0.3f), length * 0.25f, 0.7f);
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, -width * 0.3f), length * 0.25f, 0.7f);
}

void FishMeshGenerator::addCrustaceanLeg(
    MeshData& mesh,
    const glm::vec3& attachPoint,
    float length,
    int segments,
    bool hasClaw
) {
    std::vector<glm::vec3> centerline;
    std::vector<float> radii;

    float segmentLength = length / segments;
    glm::vec3 pos = attachPoint;
    float angle = -0.5f;  // Start angled down

    for (int s = 0; s <= segments; s++) {
        centerline.push_back(pos);
        radii.push_back(length * 0.05f * (1.0f - s * 0.2f / segments));

        // Move leg segment
        pos.x += segmentLength * 0.3f * std::cos(angle);
        pos.y += segmentLength * std::sin(angle);
        pos.z += (attachPoint.z > 0 ? 1 : -1) * segmentLength * 0.5f;

        angle -= 0.3f;  // Bend at each joint
    }

    MeshData leg = generateTubeMesh(centerline, radii, 4);
    mergeMeshes(mesh, leg);
}

void FishMeshGenerator::addCrustaceanClaw(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    bool mirrored
) {
    float zMod = mirrored ? -1.0f : 1.0f;

    // Claw arm
    metaballs.addMetaball(position, size * 0.3f, 0.9f);
    metaballs.addMetaball(position + glm::vec3(size * 0.4f, 0.0f, zMod * size * 0.1f),
                          size * 0.25f, 0.85f);

    // Claw pincer base
    metaballs.addMetaball(position + glm::vec3(size * 0.7f, 0.0f, zMod * size * 0.15f),
                          size * 0.35f, 1.0f);

    // Upper pincer
    metaballs.addMetaball(position + glm::vec3(size * 1.0f, size * 0.1f, zMod * size * 0.1f),
                          size * 0.15f, 0.8f);

    // Lower pincer
    metaballs.addMetaball(position + glm::vec3(size * 1.0f, -size * 0.1f, zMod * size * 0.1f),
                          size * 0.12f, 0.7f);
}

void FishMeshGenerator::addCrustaceanAntenna(
    MeshData& mesh,
    const glm::vec3& position,
    float length,
    bool thick
) {
    const int segments = 12;
    std::vector<glm::vec3> centerline;
    std::vector<float> radii;

    glm::vec3 pos = position;
    float baseThickness = thick ? length * 0.03f : length * 0.015f;

    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;

        // Gentle curve forward and up
        pos = position + glm::vec3(
            v * length,
            v * length * 0.2f,
            (position.z > 0 ? 1 : -1) * v * length * 0.1f
        );

        centerline.push_back(pos);
        radii.push_back(baseThickness * (1.0f - v * 0.9f));
    }

    MeshData antenna = generateTubeMesh(centerline, radii, 4);
    mergeMeshes(mesh, antenna);
}

// =============================================================================
// OTHER AQUATIC CREATURES
// =============================================================================

MeshData FishMeshGenerator::generateEel(const Genome& genome) {
    MetaballSystem metaballs;

    float length = genome.size * 5.0f;
    float thickness = genome.size * 0.2f;

    // Build elongated serpentine body
    const int segments = 20;
    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;
        float x = (0.5f - v) * length;

        // Taper at both ends
        float taper = 1.0f;
        if (v < 0.1f) taper = v * 10.0f;
        if (v > 0.85f) taper = (1.0f - v) * 6.67f;

        metaballs.addMetaball(glm::vec3(x, 0.0f, 0.0f), thickness * taper, 1.0f);
    }

    MeshData mesh = marchingCubes(metaballs);

    // Add continuous dorsal fin along body
    addDorsalFin(mesh,
                 glm::vec3(length * 0.3f, thickness, 0.0f),
                 glm::vec3(-length * 0.4f, thickness * 0.5f, 0.0f),
                 genome.dorsalFinHeight * genome.size * 0.5f,
                 false);

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateRay(const Genome& genome) {
    MetaballSystem metaballs;

    float bodyWidth = genome.size * 2.0f;
    float bodyLength = genome.size * 1.5f;
    float bodyHeight = genome.size * 0.3f;

    // Flat, disc-shaped body
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), bodyLength * 0.5f, 1.0f);

    // Wing-like pectoral fins (integrated into body)
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, bodyWidth * 0.4f), bodyLength * 0.4f, 0.7f);
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, -bodyWidth * 0.4f), bodyLength * 0.4f, 0.7f);
    metaballs.addMetaball(glm::vec3(-bodyLength * 0.1f, 0.0f, bodyWidth * 0.5f), bodyLength * 0.3f, 0.5f);
    metaballs.addMetaball(glm::vec3(-bodyLength * 0.1f, 0.0f, -bodyWidth * 0.5f), bodyLength * 0.3f, 0.5f);

    // Tail
    float tailLength = bodyLength * 1.5f;
    for (int t = 0; t < 8; t++) {
        float v = static_cast<float>(t) / 8;
        metaballs.addMetaball(
            glm::vec3(-bodyLength * 0.5f - v * tailLength, 0.0f, 0.0f),
            bodyHeight * (1.0f - v * 0.8f), 0.8f);
    }

    MeshData mesh = marchingCubes(metaballs);

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh, glm::vec3(0, 1, 0));  // UV from above
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateWhale(const Genome& genome) {
    MetaballSystem metaballs;

    float length = genome.size * 8.0f;
    float width = genome.size * 1.5f;
    float height = genome.size * 2.0f;

    // Massive body
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), length * 0.3f, 1.0f);
    metaballs.addMetaball(glm::vec3(length * 0.15f, 0.0f, 0.0f), length * 0.28f, 0.95f);
    metaballs.addMetaball(glm::vec3(-length * 0.15f, 0.0f, 0.0f), length * 0.25f, 0.9f);

    // Head
    metaballs.addMetaball(glm::vec3(length * 0.35f, 0.0f, 0.0f), length * 0.2f, 1.0f);

    // Tail section
    metaballs.addMetaball(glm::vec3(-length * 0.3f, 0.0f, 0.0f), length * 0.15f, 0.8f);
    metaballs.addMetaball(glm::vec3(-length * 0.4f, 0.0f, 0.0f), length * 0.1f, 0.7f);

    MeshData mesh = marchingCubes(metaballs);

    // Add flukes (horizontal tail fin)
    addCaudalFin(mesh,
                 glm::vec3(-length * 0.5f, 0.0f, 0.0f),
                 genome.tailSize * genome.size * 2.0f,
                 0.3f, 0.0f);

    // Add pectoral fins
    addPectoralFin(mesh,
                   glm::vec3(length * 0.1f, -height * 0.3f, width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 2.0f,
                   0.4f, false);
    addPectoralFin(mesh,
                   glm::vec3(length * 0.1f, -height * 0.3f, -width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 2.0f,
                   0.4f, true);

    // Dorsal fin (small for most whales, larger for orcas)
    addDorsalFin(mesh,
                 glm::vec3(-length * 0.1f, height * 0.4f, 0.0f),
                 glm::vec3(-length * 0.15f, height * 0.4f, 0.0f),
                 genome.dorsalFinHeight * genome.size,
                 false);

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateDolphin(const Genome& genome) {
    MetaballSystem metaballs;

    float length = genome.size * 4.0f;
    float width = genome.size * 0.6f;
    float height = genome.size * 0.8f;

    // Streamlined body
    metaballs.addMetaball(glm::vec3(0.0f, 0.0f, 0.0f), length * 0.25f, 1.0f);
    metaballs.addMetaball(glm::vec3(length * 0.15f, 0.0f, 0.0f), length * 0.22f, 0.95f);
    metaballs.addMetaball(glm::vec3(-length * 0.15f, 0.0f, 0.0f), length * 0.2f, 0.9f);

    // Distinctive beak/rostrum
    metaballs.addMetaball(glm::vec3(length * 0.35f, 0.0f, 0.0f), length * 0.12f, 0.9f);
    metaballs.addMetaball(glm::vec3(length * 0.45f, -height * 0.05f, 0.0f), length * 0.08f, 0.8f);

    // Melon (forehead)
    metaballs.addMetaball(glm::vec3(length * 0.28f, height * 0.1f, 0.0f), length * 0.1f, 0.85f);

    // Tail section
    metaballs.addMetaball(glm::vec3(-length * 0.3f, 0.0f, 0.0f), length * 0.12f, 0.8f);
    metaballs.addMetaball(glm::vec3(-length * 0.4f, 0.0f, 0.0f), length * 0.08f, 0.7f);

    MeshData mesh = marchingCubes(metaballs);

    // Horizontal flukes
    addCaudalFin(mesh,
                 glm::vec3(-length * 0.5f, 0.0f, 0.0f),
                 genome.tailSize * genome.size * 1.5f,
                 0.4f, 0.0f);

    // Curved dorsal fin
    addDorsalFin(mesh,
                 glm::vec3(-length * 0.05f, height * 0.4f, 0.0f),
                 glm::vec3(-length * 0.15f, height * 0.35f, 0.0f),
                 genome.dorsalFinHeight * genome.size * 1.2f,
                 false);

    // Pectoral flippers
    addPectoralFin(mesh,
                   glm::vec3(length * 0.1f, -height * 0.2f, width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 1.2f,
                   0.3f, false);
    addPectoralFin(mesh,
                   glm::vec3(length * 0.1f, -height * 0.2f, -width * 0.5f),
                   genome.pectoralFinWidth * genome.size * 1.2f,
                   0.3f, true);

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

MeshData FishMeshGenerator::generateSeaHorse(const Genome& genome) {
    MetaballSystem metaballs;

    float bodyHeight = genome.size * 2.0f;
    float bodyWidth = genome.size * 0.3f;

    // Vertical body with segments
    const int segments = 12;
    for (int s = 0; s < segments; s++) {
        float v = static_cast<float>(s) / segments;
        float y = (0.5f - v) * bodyHeight;

        // S-curve body shape
        float xOffset = std::sin(v * 3.14159f * 2.0f) * bodyWidth * 0.3f;

        // Taper at tail
        float taper = 1.0f;
        if (v > 0.7f) taper = (1.0f - v) * 3.33f;

        metaballs.addMetaball(glm::vec3(xOffset, y, 0.0f), bodyWidth * taper, 0.9f);
    }

    // Horse-like head
    metaballs.addMetaball(glm::vec3(bodyWidth * 0.5f, bodyHeight * 0.4f, 0.0f),
                          bodyWidth * 0.8f, 1.0f);
    metaballs.addMetaball(glm::vec3(bodyWidth * 0.9f, bodyHeight * 0.35f, 0.0f),
                          bodyWidth * 0.5f, 0.9f);

    // Snout
    metaballs.addMetaball(glm::vec3(bodyWidth * 1.3f, bodyHeight * 0.3f, 0.0f),
                          bodyWidth * 0.3f, 0.8f);

    // Curled tail
    float tailCurl = 1.5f;
    for (int t = 0; t < 8; t++) {
        float v = static_cast<float>(t) / 8;
        float angle = v * tailCurl * 3.14159f;
        float radius = bodyWidth * (1.0f - v) * 0.5f;

        metaballs.addMetaball(
            glm::vec3(
                std::sin(angle) * bodyWidth * 0.5f,
                -bodyHeight * 0.5f - v * bodyWidth,
                std::cos(angle) * bodyWidth * 0.3f
            ),
            bodyWidth * 0.15f * (1.0f - v * 0.5f), 0.7f);
    }

    MeshData mesh = marchingCubes(metaballs);

    // Small dorsal fin on back
    addDorsalFin(mesh,
                 glm::vec3(-bodyWidth * 0.2f, bodyHeight * 0.1f, 0.0f),
                 glm::vec3(-bodyWidth * 0.2f, -bodyHeight * 0.1f, 0.0f),
                 genome.dorsalFinHeight * genome.size * 0.5f,
                 false);

    smoothMesh(mesh, 2);
    calculateNormals(mesh);
    generateUVs(mesh);
    mesh.calculateBounds();

    return mesh;
}

// =============================================================================
// BODY BUILDING HELPERS
// =============================================================================

void FishMeshGenerator::buildFishBody(
    MetaballSystem& metaballs,
    FishBodyShape shape,
    float length,
    float width,
    float height
) {
    // Create body using overlapping metaballs
    const int segments = 8;

    for (int s = 0; s < segments; s++) {
        float v = static_cast<float>(s) / (segments - 1);
        float x = (0.5f - v) * length;

        // Body profile varies by shape
        float profileRadius;
        float yOffset = 0.0f;

        switch (shape) {
            case FishBodyShape::FUSIFORM:
                // Torpedo - widest in middle
                profileRadius = std::sin(v * 3.14159f) * width;
                break;

            case FishBodyShape::LATERALLY_COMPRESSED:
                // Tall and thin
                profileRadius = std::sin(v * 3.14159f) * height * 0.7f;
                break;

            case FishBodyShape::DEPRESSED:
                // Wide and flat
                profileRadius = std::sin(v * 3.14159f) * width * 1.2f;
                break;

            case FishBodyShape::ELONGATED:
                // Consistent width along body
                profileRadius = width * 0.8f;
                if (v < 0.1f || v > 0.9f) profileRadius *= (1.0f - std::abs(v - 0.5f) * 2.0f);
                break;

            case FishBodyShape::GLOBIFORM:
                // Round
                profileRadius = std::sin(v * 3.14159f) * (width + height) * 0.5f;
                break;

            case FishBodyShape::TORPEDO:
                // Sharp nose, wide back
                profileRadius = std::pow(std::sin(v * 3.14159f * 0.8f), 0.7f) * width;
                break;

            default:
                profileRadius = std::sin(v * 3.14159f) * width;
        }

        if (profileRadius > 0.01f) {
            metaballs.addMetaball(glm::vec3(x, yOffset, 0.0f), profileRadius, 1.0f);
        }
    }
}

void FishMeshGenerator::buildFishHead(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    bool predatory
) {
    metaballs.addMetaball(position, size, 1.0f);

    if (predatory) {
        // Elongated snout
        metaballs.addMetaball(position + glm::vec3(size * 0.8f, -size * 0.1f, 0.0f),
                              size * 0.5f, 0.8f);
        metaballs.addMetaball(position + glm::vec3(size * 1.2f, -size * 0.15f, 0.0f),
                              size * 0.3f, 0.7f);
    } else {
        // Rounded head
        metaballs.addMetaball(position + glm::vec3(size * 0.5f, 0.0f, 0.0f),
                              size * 0.6f, 0.9f);
    }

    // Eyes
    metaballs.addMetaball(position + glm::vec3(size * 0.3f, size * 0.2f, size * 0.4f),
                          size * 0.2f, 0.5f);
    metaballs.addMetaball(position + glm::vec3(size * 0.3f, size * 0.2f, -size * 0.4f),
                          size * 0.2f, 0.5f);
}

void FishMeshGenerator::buildFishTail(
    MetaballSystem& metaballs,
    const glm::vec3& position,
    float size,
    float forkDepth
) {
    // Peduncle (tail base)
    metaballs.addMetaball(position, size * 0.6f, 0.9f);
    metaballs.addMetaball(position + glm::vec3(-size * 0.5f, 0.0f, 0.0f),
                          size * 0.4f, 0.8f);
}

// =============================================================================
// FIN GENERATION
// =============================================================================

void FishMeshGenerator::addDorsalFin(
    MeshData& mesh,
    const glm::vec3& baseStart,
    const glm::vec3& baseEnd,
    float height,
    bool spiked
) {
    // Create fin outline
    std::vector<glm::vec3> outline;

    glm::vec3 baseVec = baseEnd - baseStart;
    float baseLength = glm::length(baseVec);
    glm::vec3 baseDir = baseVec / baseLength;
    glm::vec3 upDir(0.0f, 1.0f, 0.0f);

    const int segments = 8;
    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;
        glm::vec3 basePoint = baseStart + baseDir * baseLength * v;

        // Fin profile - triangular with rounded peak
        float finHeight = std::sin(v * 3.14159f) * height;
        if (spiked) {
            // Multiple peaks for spines
            finHeight *= (0.7f + 0.3f * std::abs(std::sin(v * 3.14159f * 4.0f)));
        }

        outline.push_back(basePoint + upDir * finHeight);
    }

    // Generate fin mesh
    MeshData fin = generateFinMesh(outline, height * 0.02f, true);
    mergeMeshes(mesh, fin);
}

void FishMeshGenerator::addPectoralFin(
    MeshData& mesh,
    const glm::vec3& attachPoint,
    float size,
    float angle,
    bool mirrored
) {
    std::vector<glm::vec3> outline;

    float zMod = mirrored ? -1.0f : 1.0f;

    // Fan-shaped fin
    const int segments = 6;
    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;
        float finAngle = (v - 0.5f) * 1.5f + angle;

        glm::vec3 point = attachPoint + glm::vec3(
            std::cos(finAngle) * size * (0.5f + v * 0.5f),
            std::sin(finAngle) * size * 0.3f,
            zMod * std::sin(finAngle) * size * 0.8f
        );
        outline.push_back(point);
    }

    MeshData fin = generateFinMesh(outline, size * 0.02f, true);
    mergeMeshes(mesh, fin);
}

void FishMeshGenerator::addCaudalFin(
    MeshData& mesh,
    const glm::vec3& attachPoint,
    float size,
    float forkDepth,
    float asymmetry
) {
    std::vector<glm::vec3> outline;

    // Create forked tail shape
    const int segments = 12;
    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;

        // V or forked shape with optional asymmetry
        float y;
        if (v < 0.5f) {
            // Upper lobe
            float t = v * 2.0f;
            y = size * (1.0f + asymmetry * 0.5f) * std::sqrt(t);
        } else {
            // Lower lobe
            float t = (v - 0.5f) * 2.0f;
            y = -size * (1.0f - asymmetry * 0.5f) * std::sqrt(t);
        }

        // Fork depth affects how deeply the tail is notched
        float x = -size * (1.0f - forkDepth * std::pow(std::abs(v - 0.5f) * 2.0f, 2.0f));

        outline.push_back(attachPoint + glm::vec3(x, y, 0.0f));
    }

    MeshData fin = generateFinMesh(outline, size * 0.015f, true);
    mergeMeshes(mesh, fin);
}

void FishMeshGenerator::addPelvicFin(
    MeshData& mesh,
    const glm::vec3& attachPoint,
    float size,
    bool mirrored
) {
    std::vector<glm::vec3> outline;

    float zMod = mirrored ? -1.0f : 1.0f;

    // Small triangular fin
    outline.push_back(attachPoint);
    outline.push_back(attachPoint + glm::vec3(-size * 0.8f, -size * 0.3f, zMod * size * 0.4f));
    outline.push_back(attachPoint + glm::vec3(-size * 0.5f, -size * 0.5f, zMod * size * 0.2f));
    outline.push_back(attachPoint + glm::vec3(0.0f, -size * 0.2f, 0.0f));

    MeshData fin = generateFinMesh(outline, size * 0.01f, true);
    mergeMeshes(mesh, fin);
}

void FishMeshGenerator::addAnalFin(
    MeshData& mesh,
    const glm::vec3& position,
    float size
) {
    std::vector<glm::vec3> outline;

    // Small fin below tail section
    const int segments = 5;
    for (int s = 0; s <= segments; s++) {
        float v = static_cast<float>(s) / segments;
        float finHeight = std::sin(v * 3.14159f) * size;

        outline.push_back(position + glm::vec3(-v * size * 0.8f, -finHeight, 0.0f));
    }

    MeshData fin = generateFinMesh(outline, size * 0.01f, true);
    mergeMeshes(mesh, fin);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

MeshData FishMeshGenerator::marchingCubes(const MetaballSystem& metaballs) {
    MeshData mesh;

    // Get bounds
    glm::vec3 minBounds, maxBounds;
    metaballs.getBounds(minBounds, maxBounds);

    // Add padding
    glm::vec3 padding = (maxBounds - minBounds) * 0.1f;
    minBounds -= padding;
    maxBounds += padding;

    glm::vec3 size = maxBounds - minBounds;
    glm::vec3 cellSize = size / static_cast<float>(m_resolution);

    // Simple marching cubes implementation
    for (int z = 0; z < m_resolution; z++) {
        for (int y = 0; y < m_resolution; y++) {
            for (int x = 0; x < m_resolution; x++) {
                glm::vec3 pos = minBounds + glm::vec3(x, y, z) * cellSize;

                // Sample corners
                float corners[8];
                corners[0] = metaballs.evaluatePotential(pos);
                corners[1] = metaballs.evaluatePotential(pos + glm::vec3(cellSize.x, 0, 0));
                corners[2] = metaballs.evaluatePotential(pos + glm::vec3(cellSize.x, cellSize.y, 0));
                corners[3] = metaballs.evaluatePotential(pos + glm::vec3(0, cellSize.y, 0));
                corners[4] = metaballs.evaluatePotential(pos + glm::vec3(0, 0, cellSize.z));
                corners[5] = metaballs.evaluatePotential(pos + glm::vec3(cellSize.x, 0, cellSize.z));
                corners[6] = metaballs.evaluatePotential(pos + glm::vec3(cellSize.x, cellSize.y, cellSize.z));
                corners[7] = metaballs.evaluatePotential(pos + glm::vec3(0, cellSize.y, cellSize.z));

                // Calculate cube index
                float threshold = metaballs.getThreshold();
                int cubeIndex = 0;
                if (corners[0] >= threshold) cubeIndex |= 1;
                if (corners[1] >= threshold) cubeIndex |= 2;
                if (corners[2] >= threshold) cubeIndex |= 4;
                if (corners[3] >= threshold) cubeIndex |= 8;
                if (corners[4] >= threshold) cubeIndex |= 16;
                if (corners[5] >= threshold) cubeIndex |= 32;
                if (corners[6] >= threshold) cubeIndex |= 64;
                if (corners[7] >= threshold) cubeIndex |= 128;

                // Skip empty or full cells
                if (cubeIndex == 0 || cubeIndex == 255) continue;

                // Generate triangles using marching cubes tables
                // (Simplified - in production, use full lookup tables)
                // For now, just add vertices at edge midpoints where surface crosses

                glm::vec3 edgePositions[12];
                glm::vec3 cornerPositions[8] = {
                    pos,
                    pos + glm::vec3(cellSize.x, 0, 0),
                    pos + glm::vec3(cellSize.x, cellSize.y, 0),
                    pos + glm::vec3(0, cellSize.y, 0),
                    pos + glm::vec3(0, 0, cellSize.z),
                    pos + glm::vec3(cellSize.x, 0, cellSize.z),
                    pos + glm::vec3(cellSize.x, cellSize.y, cellSize.z),
                    pos + glm::vec3(0, cellSize.y, cellSize.z)
                };

                // Interpolate edge positions
                auto interpolate = [&](int e1, int e2) -> glm::vec3 {
                    float t = (threshold - corners[e1]) / (corners[e2] - corners[e1]);
                    t = std::clamp(t, 0.0f, 1.0f);
                    return cornerPositions[e1] + t * (cornerPositions[e2] - cornerPositions[e1]);
                };

                edgePositions[0] = interpolate(0, 1);
                edgePositions[1] = interpolate(1, 2);
                edgePositions[2] = interpolate(2, 3);
                edgePositions[3] = interpolate(3, 0);
                edgePositions[4] = interpolate(4, 5);
                edgePositions[5] = interpolate(5, 6);
                edgePositions[6] = interpolate(6, 7);
                edgePositions[7] = interpolate(7, 4);
                edgePositions[8] = interpolate(0, 4);
                edgePositions[9] = interpolate(1, 5);
                edgePositions[10] = interpolate(2, 6);
                edgePositions[11] = interpolate(3, 7);

                // Use MarchingCubes class for triangle generation
                // This is a simplified inline version
                // In a full implementation, we'd use the triTable lookup
            }
        }
    }

    return mesh;
}

void FishMeshGenerator::smoothMesh(MeshData& mesh, int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        std::vector<glm::vec3> newPositions(mesh.vertices.size());

        // Simple Laplacian smoothing
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            newPositions[i] = mesh.vertices[i].position;
        }

        // Average with neighbors (simplified - assumes sequential vertices are neighbors)
        for (size_t i = 1; i < mesh.vertices.size() - 1; i++) {
            newPositions[i] = (mesh.vertices[i-1].position +
                              mesh.vertices[i].position * 2.0f +
                              mesh.vertices[i+1].position) * 0.25f;
        }

        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            mesh.vertices[i].position = glm::mix(mesh.vertices[i].position,
                                                  newPositions[i], m_smoothing);
        }
    }
}

void FishMeshGenerator::calculateNormals(MeshData& mesh) {
    // Reset normals
    for (auto& vertex : mesh.vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Calculate face normals and accumulate
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        unsigned int i0 = mesh.indices[i];
        unsigned int i1 = mesh.indices[i + 1];
        unsigned int i2 = mesh.indices[i + 2];

        glm::vec3 v0 = mesh.vertices[i0].position;
        glm::vec3 v1 = mesh.vertices[i1].position;
        glm::vec3 v2 = mesh.vertices[i2].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        mesh.vertices[i0].normal += normal;
        mesh.vertices[i1].normal += normal;
        mesh.vertices[i2].normal += normal;
    }

    // Normalize
    for (auto& vertex : mesh.vertices) {
        if (glm::length(vertex.normal) > 0.0001f) {
            vertex.normal = glm::normalize(vertex.normal);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void FishMeshGenerator::generateUVs(MeshData& mesh, const glm::vec3& axis) {
    if (mesh.vertices.empty()) return;

    // Calculate bounds for UV mapping
    glm::vec3 minPos = mesh.vertices[0].position;
    glm::vec3 maxPos = mesh.vertices[0].position;

    for (const auto& vertex : mesh.vertices) {
        minPos = glm::min(minPos, vertex.position);
        maxPos = glm::max(maxPos, vertex.position);
    }

    glm::vec3 size = maxPos - minPos;
    if (size.x < 0.001f) size.x = 1.0f;
    if (size.y < 0.001f) size.y = 1.0f;
    if (size.z < 0.001f) size.z = 1.0f;

    // Generate UVs based on projection axis
    for (auto& vertex : mesh.vertices) {
        glm::vec3 normalized = (vertex.position - minPos) / size;

        if (std::abs(axis.y) > 0.5f) {
            // Project from above/below
            vertex.texCoord = glm::vec2(normalized.x, normalized.z);
        } else if (std::abs(axis.x) > 0.5f) {
            // Project from side
            vertex.texCoord = glm::vec2(normalized.z, normalized.y);
        } else {
            // Project from front/back
            vertex.texCoord = glm::vec2(normalized.x, normalized.y);
        }
    }
}

void FishMeshGenerator::mergeMeshes(MeshData& target, const MeshData& source) {
    unsigned int indexOffset = static_cast<unsigned int>(target.vertices.size());

    // Add vertices
    for (const auto& vertex : source.vertices) {
        target.vertices.push_back(vertex);
    }

    // Add indices with offset
    for (unsigned int index : source.indices) {
        target.indices.push_back(index + indexOffset);
    }
}

MeshData FishMeshGenerator::generateFinMesh(
    const std::vector<glm::vec3>& outline,
    float thickness,
    bool doubleSided
) {
    MeshData mesh;

    if (outline.size() < 3) return mesh;

    // Calculate center point
    glm::vec3 center(0.0f);
    for (const auto& point : outline) {
        center += point;
    }
    center /= static_cast<float>(outline.size());

    // Calculate normal for the fin plane
    glm::vec3 edge1 = outline[1] - outline[0];
    glm::vec3 edge2 = outline[outline.size() - 1] - outline[0];
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    // Create vertices for front face
    for (const auto& point : outline) {
        mesh.vertices.push_back(Vertex(point + normal * thickness * 0.5f, normal));
    }
    // Center vertex
    mesh.vertices.push_back(Vertex(center + normal * thickness * 0.5f, normal));
    unsigned int centerIdx = static_cast<unsigned int>(outline.size());

    // Create triangles (fan from center)
    for (size_t i = 0; i < outline.size(); i++) {
        unsigned int i0 = static_cast<unsigned int>(i);
        unsigned int i1 = static_cast<unsigned int>((i + 1) % outline.size());

        mesh.indices.push_back(i0);
        mesh.indices.push_back(i1);
        mesh.indices.push_back(centerIdx);
    }

    if (doubleSided) {
        // Create back face
        unsigned int offset = static_cast<unsigned int>(mesh.vertices.size());
        for (const auto& point : outline) {
            mesh.vertices.push_back(Vertex(point - normal * thickness * 0.5f, -normal));
        }
        mesh.vertices.push_back(Vertex(center - normal * thickness * 0.5f, -normal));
        unsigned int backCenterIdx = offset + static_cast<unsigned int>(outline.size());

        // Back face triangles (reversed winding)
        for (size_t i = 0; i < outline.size(); i++) {
            unsigned int i0 = offset + static_cast<unsigned int>(i);
            unsigned int i1 = offset + static_cast<unsigned int>((i + 1) % outline.size());

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i0);
            mesh.indices.push_back(backCenterIdx);
        }
    }

    return mesh;
}

MeshData FishMeshGenerator::generateTubeMesh(
    const std::vector<glm::vec3>& centerline,
    const std::vector<float>& radii,
    int radialSegments
) {
    MeshData mesh;

    if (centerline.size() < 2 || centerline.size() != radii.size()) return mesh;

    // Generate vertices along the tube
    for (size_t s = 0; s < centerline.size(); s++) {
        // Calculate tangent direction
        glm::vec3 tangent;
        if (s == 0) {
            tangent = glm::normalize(centerline[1] - centerline[0]);
        } else if (s == centerline.size() - 1) {
            tangent = glm::normalize(centerline[s] - centerline[s - 1]);
        } else {
            tangent = glm::normalize(centerline[s + 1] - centerline[s - 1]);
        }

        // Create perpendicular vectors
        glm::vec3 up = glm::abs(tangent.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(tangent, up));
        up = glm::normalize(glm::cross(right, tangent));

        // Generate ring of vertices
        float radius = radii[s];
        for (int r = 0; r < radialSegments; r++) {
            float angle = static_cast<float>(r) / radialSegments * 2.0f * 3.14159f;

            glm::vec3 offset = right * std::cos(angle) * radius + up * std::sin(angle) * radius;
            glm::vec3 position = centerline[s] + offset;
            glm::vec3 normal = glm::normalize(offset);

            float u = static_cast<float>(r) / radialSegments;
            float v = static_cast<float>(s) / (centerline.size() - 1);

            mesh.vertices.push_back(Vertex(position, normal, glm::vec2(u, v)));
        }
    }

    // Generate indices
    for (size_t s = 0; s < centerline.size() - 1; s++) {
        for (int r = 0; r < radialSegments; r++) {
            unsigned int current = static_cast<unsigned int>(s * radialSegments + r);
            unsigned int next = static_cast<unsigned int>(s * radialSegments + (r + 1) % radialSegments);
            unsigned int nextRow = static_cast<unsigned int>((s + 1) * radialSegments + r);
            unsigned int nextRowNext = static_cast<unsigned int>((s + 1) * radialSegments + (r + 1) % radialSegments);

            mesh.indices.push_back(current);
            mesh.indices.push_back(nextRow);
            mesh.indices.push_back(next);

            mesh.indices.push_back(next);
            mesh.indices.push_back(nextRow);
            mesh.indices.push_back(nextRowNext);
        }
    }

    return mesh;
}

// =============================================================================
// SKELETON GENERATION
// =============================================================================

FishMeshGenerator::SkeletonData FishMeshGenerator::generateFishSkeleton(
    const Genome& genome,
    int spineSegments
) {
    SkeletonData skeleton;

    float bodyLength = genome.size * 2.0f;

    // Root bone
    skeleton.jointPositions.push_back(glm::vec3(0.0f));
    skeleton.parentIndices.push_back(-1);

    // Spine segments
    for (int s = 0; s < spineSegments; s++) {
        float t = static_cast<float>(s) / (spineSegments - 1);
        float x = (0.5f - t) * bodyLength;

        skeleton.jointPositions.push_back(glm::vec3(x, 0.0f, 0.0f));
        skeleton.parentIndices.push_back(s);  // Parent is previous bone
    }

    // Tail bone
    skeleton.jointPositions.push_back(glm::vec3(-bodyLength * 0.5f, 0.0f, 0.0f));
    skeleton.parentIndices.push_back(spineSegments);

    // Calculate bind pose (identity for now)
    for (size_t i = 0; i < skeleton.jointPositions.size(); i++) {
        skeleton.bindPose.push_back(glm::mat4(1.0f));
    }

    return skeleton;
}

FishMeshGenerator::SkeletonData FishMeshGenerator::generateJellyfishSkeleton(int tentacleCount) {
    SkeletonData skeleton;

    // Bell root
    skeleton.jointPositions.push_back(glm::vec3(0.0f));
    skeleton.parentIndices.push_back(-1);

    // Bell rim segments
    const int rimSegments = 8;
    for (int r = 0; r < rimSegments; r++) {
        float angle = static_cast<float>(r) / rimSegments * 2.0f * 3.14159f;
        skeleton.jointPositions.push_back(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        skeleton.parentIndices.push_back(0);
    }

    // Tentacle roots (one per tentacle)
    for (int t = 0; t < tentacleCount; t++) {
        float angle = static_cast<float>(t) / tentacleCount * 2.0f * 3.14159f;
        int rimParent = 1 + (t * rimSegments / tentacleCount);

        skeleton.jointPositions.push_back(glm::vec3(std::cos(angle) * 0.8f, 0.0f, std::sin(angle) * 0.8f));
        skeleton.parentIndices.push_back(rimParent);

        // Tentacle segments
        for (int s = 0; s < 4; s++) {
            skeleton.jointPositions.push_back(
                glm::vec3(std::cos(angle) * 0.8f, -static_cast<float>(s + 1) * 0.5f, std::sin(angle) * 0.8f));
            skeleton.parentIndices.push_back(static_cast<int>(skeleton.jointPositions.size()) - 2);
        }
    }

    for (size_t i = 0; i < skeleton.jointPositions.size(); i++) {
        skeleton.bindPose.push_back(glm::mat4(1.0f));
    }

    return skeleton;
}

FishMeshGenerator::SkeletonData FishMeshGenerator::generateCephalopodSkeleton(int armCount) {
    SkeletonData skeleton;

    // Mantle root
    skeleton.jointPositions.push_back(glm::vec3(0.0f));
    skeleton.parentIndices.push_back(-1);

    // Mantle tip
    skeleton.jointPositions.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    skeleton.parentIndices.push_back(0);

    // Head
    skeleton.jointPositions.push_back(glm::vec3(0.5f, 0.0f, 0.0f));
    skeleton.parentIndices.push_back(0);

    // Arms
    for (int a = 0; a < armCount; a++) {
        float angle = static_cast<float>(a) / armCount * 2.0f * 3.14159f;

        glm::vec3 armBase(0.6f, -0.2f, 0.0f);

        skeleton.jointPositions.push_back(armBase);
        skeleton.parentIndices.push_back(2);  // Attached to head

        // Arm segments
        for (int s = 0; s < 4; s++) {
            glm::vec3 segPos = armBase + glm::vec3(
                std::cos(angle) * (s + 1) * 0.3f,
                -0.3f * (s + 1),
                std::sin(angle) * (s + 1) * 0.3f
            );
            skeleton.jointPositions.push_back(segPos);
            skeleton.parentIndices.push_back(static_cast<int>(skeleton.jointPositions.size()) - 2);
        }
    }

    for (size_t i = 0; i < skeleton.jointPositions.size(); i++) {
        skeleton.bindPose.push_back(glm::mat4(1.0f));
    }

    return skeleton;
}

// =============================================================================
// NOISE FUNCTIONS
// =============================================================================

float FishMeshGenerator::perlinNoise(float x, float y, float z) const {
    // Simple noise function
    auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    auto lerp = [](float a, float b, float t) { return a + t * (b - a); };
    auto grad = [](int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    };

    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    float u = fade(x);
    float v = fade(y);
    float w = fade(z);

    // Simplified permutation (would use full table in production)
    auto perm = [](int i) { return (i * 1103515245 + 12345) & 255; };

    int A = perm(X) + Y;
    int AA = perm(A) + Z;
    int AB = perm(A + 1) + Z;
    int B = perm(X + 1) + Y;
    int BA = perm(B) + Z;
    int BB = perm(B + 1) + Z;

    return lerp(
        lerp(lerp(grad(perm(AA), x, y, z), grad(perm(BA), x - 1, y, z), u),
             lerp(grad(perm(AB), x, y - 1, z), grad(perm(BB), x - 1, y - 1, z), u), v),
        lerp(lerp(grad(perm(AA + 1), x, y, z - 1), grad(perm(BA + 1), x - 1, y, z - 1), u),
             lerp(grad(perm(AB + 1), x, y - 1, z - 1), grad(perm(BB + 1), x - 1, y - 1, z - 1), u), v),
        w);
}

float FishMeshGenerator::fbmNoise(const glm::vec3& pos, int octaves) const {
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        result += perlinNoise(pos.x * frequency, pos.y * frequency, pos.z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return result / maxValue;
}

// =============================================================================
// PATTERN GENERATION
// =============================================================================

namespace AquaticPatterns {

glm::vec4 generateScalePattern(
    const glm::vec2& uv,
    float scaleSize,
    const glm::vec3& baseColor,
    const glm::vec3& highlightColor
) {
    // Hexagonal scale pattern
    float x = uv.x / scaleSize;
    float y = uv.y / scaleSize;

    // Offset every other row
    if (static_cast<int>(y) % 2 == 1) {
        x += 0.5f;
    }

    // Distance to nearest scale center
    float fx = x - std::floor(x) - 0.5f;
    float fy = y - std::floor(y) - 0.5f;
    float dist = std::sqrt(fx * fx + fy * fy);

    // Scale edge highlight
    float edgeHighlight = 1.0f - std::clamp(dist * 3.0f, 0.0f, 1.0f);
    edgeHighlight = std::pow(edgeHighlight, 2.0f);

    glm::vec3 color = glm::mix(baseColor, highlightColor, edgeHighlight * 0.5f);

    return glm::vec4(color, 1.0f);
}

glm::vec3 applyCounterShading(
    const glm::vec3& normal,
    const glm::vec3& baseColor,
    float intensity
) {
    // Dark on top (dorsal), light on bottom (ventral)
    float topness = std::max(0.0f, normal.y);
    float bottomness = std::max(0.0f, -normal.y);

    glm::vec3 darkColor = baseColor * 0.6f;
    glm::vec3 lightColor = baseColor * 1.4f;

    glm::vec3 result = baseColor;
    result = glm::mix(result, darkColor, topness * intensity);
    result = glm::mix(result, lightColor, bottomness * intensity);

    return result;
}

glm::vec3 generateStripes(
    const glm::vec2& uv,
    const glm::vec3& color1,
    const glm::vec3& color2,
    float frequency,
    float angle
) {
    // Rotate UV for angled stripes
    float s = std::sin(angle);
    float c = std::cos(angle);
    float rotatedU = uv.x * c - uv.y * s;

    float stripe = std::sin(rotatedU * frequency * 3.14159f * 2.0f) * 0.5f + 0.5f;
    return glm::mix(color1, color2, stripe);
}

glm::vec3 generateSpots(
    const glm::vec2& uv,
    const glm::vec3& baseColor,
    const glm::vec3& spotColor,
    float spotSize,
    float density
) {
    // Grid-based spots with jitter
    float gridSize = spotSize * 2.0f;
    float gx = std::floor(uv.x / gridSize);
    float gy = std::floor(uv.y / gridSize);

    // Pseudo-random jitter
    float jitterX = std::sin(gx * 127.1f + gy * 311.7f) * 0.3f;
    float jitterY = std::sin(gx * 269.5f + gy * 183.3f) * 0.3f;

    // Center of this cell's spot
    glm::vec2 spotCenter((gx + 0.5f + jitterX) * gridSize, (gy + 0.5f + jitterY) * gridSize);

    // Distance to spot center
    float dist = glm::length(uv - spotCenter);

    // Should this cell have a spot?
    float spotPresence = std::sin(gx * 23.3f + gy * 41.7f) * 0.5f + 0.5f;

    if (spotPresence > (1.0f - density) && dist < spotSize) {
        float blend = 1.0f - dist / spotSize;
        return glm::mix(baseColor, spotColor, blend);
    }

    return baseColor;
}

glm::vec3 generateBiolumGlow(
    const glm::vec3& position,
    const glm::vec3& glowCenter,
    const glm::vec3& glowColor,
    float intensity,
    float radius
) {
    float dist = glm::length(position - glowCenter);
    float falloff = 1.0f - std::clamp(dist / radius, 0.0f, 1.0f);
    falloff = std::pow(falloff, 2.0f);  // Quadratic falloff

    return glowColor * intensity * falloff;
}

glm::vec3 calculateIridescence(
    const glm::vec3& normal,
    const glm::vec3& viewDir,
    float intensity
) {
    float fresnel = 1.0f - std::abs(glm::dot(normal, viewDir));
    fresnel = std::pow(fresnel, 3.0f);

    // Rainbow shift based on view angle
    float hue = fresnel * 360.0f;

    // Simple HSV to RGB (hue only, full saturation/value)
    float h = hue / 60.0f;
    float x = 1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f);

    glm::vec3 rgb;
    if (h < 1) rgb = glm::vec3(1, x, 0);
    else if (h < 2) rgb = glm::vec3(x, 1, 0);
    else if (h < 3) rgb = glm::vec3(0, 1, x);
    else if (h < 4) rgb = glm::vec3(0, x, 1);
    else if (h < 5) rgb = glm::vec3(x, 0, 1);
    else rgb = glm::vec3(1, 0, x);

    return rgb * intensity * fresnel;
}

glm::vec4 generateJellyfishColor(
    const glm::vec3& position,
    float bellRadius,
    const glm::vec3& baseColor,
    float translucency
) {
    float distFromCenter = glm::length(glm::vec2(position.x, position.z));
    float normalizedDist = distFromCenter / bellRadius;

    // More translucent at edges
    float alpha = (1.0f - translucency) + translucency * (1.0f - normalizedDist);

    // Subtle color variation
    glm::vec3 color = baseColor * (0.8f + normalizedDist * 0.4f);

    return glm::vec4(color, alpha);
}

} // namespace AquaticPatterns
