#include "WingMeshGenerator.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>

// =============================================================================
// WING MESH GENERATOR IMPLEMENTATION
// =============================================================================

WingMeshGenerator::WingMeshGenerator() {}

// Generate wing mesh from genome and type
MeshData WingMeshGenerator::generateFromGenome(const Genome& genome, WingMeshType type) {
    switch (type) {
        case WingMeshType::FEATHERED_ELLIPTICAL:
            return generateSongbirdWing(genome);
        case WingMeshType::FEATHERED_HIGH_SPEED:
            return generateRaptorWing(genome);
        case WingMeshType::FEATHERED_HIGH_ASPECT:
            return generateSeabirdWing(genome);
        case WingMeshType::FEATHERED_SLOTTED:
            return generateRaptorWing(genome);  // Uses same base with slots
        case WingMeshType::MEMBRANE_BAT:
            return generateBatWing(genome);
        case WingMeshType::MEMBRANE_DRAGON:
            return generateDragonWing(genome, membraneConfigFromGenome(genome));
        case WingMeshType::INSECT_DIPTERA:
            return generateBeeWing(genome);  // Diptera-style
        case WingMeshType::INSECT_ODONATA:
            return generateDragonflyWing(genome);
        case WingMeshType::INSECT_LEPIDOPTERA:
            return generateButterflyWing(genome);
        case WingMeshType::INSECT_HYMENOPTERA:
            return generateBeeWing(genome);
        default:
            return generateFeatheredWing(genome, configFromGenome(genome));
    }
}

// Generate a pair of wings (mirrored)
std::pair<MeshData, MeshData> WingMeshGenerator::generateWingPair(const Genome& genome,
                                                                   WingMeshType type) {
    MeshData leftWing = generateFromGenome(genome, type);
    MeshData rightWing;

    // Mirror the left wing for the right wing
    rightWing.vertices.reserve(leftWing.vertices.size());
    for (const auto& v : leftWing.vertices) {
        Vertex mirrored = v;
        mirrored.position.x = -mirrored.position.x;
        mirrored.normal.x = -mirrored.normal.x;
        rightWing.vertices.push_back(mirrored);
    }

    // Reverse winding order for mirrored mesh
    rightWing.indices = leftWing.indices;
    for (size_t i = 0; i < rightWing.indices.size(); i += 3) {
        std::swap(rightWing.indices[i], rightWing.indices[i + 2]);
    }

    leftWing.calculateBounds();
    rightWing.calculateBounds();

    return {leftWing, rightWing};
}

// =============================================================================
// FEATHERED WING GENERATION
// =============================================================================

MeshData WingMeshGenerator::generateFeatheredWing(const Genome& genome,
                                                   const FeatherConfig& config) {
    MeshData mesh;

    float wingspan = genome.wingSpan * genome.size;
    float chord = genome.wingChord * wingspan;

    // Generate wing arm bones (humerus, radius/ulna, hand)
    glm::vec3 shoulder(0.0f, 0.0f, 0.0f);
    glm::vec3 elbow(wingspan * 0.3f, 0.0f, 0.0f);
    glm::vec3 wrist(wingspan * 0.55f, 0.0f, 0.0f);
    glm::vec3 tip(wingspan, 0.0f, 0.0f);

    // Apply dihedral angle
    float dihedral = glm::radians(genome.dihedralAngle);
    elbow.y = elbow.x * std::sin(dihedral);
    wrist.y = wrist.x * std::sin(dihedral);
    tip.y = tip.x * std::sin(dihedral);

    // Apply sweep angle
    float sweep = glm::radians(genome.sweepAngle);
    elbow.z = -elbow.x * std::sin(sweep) * 0.2f;
    wrist.z = -wrist.x * std::sin(sweep) * 0.5f;
    tip.z = -tip.x * std::sin(sweep);

    // Generate arm bone geometry
    float armRadius = chord * 0.08f;
    generateWingBone(mesh, shoulder, elbow, armRadius, 8);
    generateWingBone(mesh, elbow, wrist, armRadius * 0.8f, 8);
    generateWingBone(mesh, wrist, tip, armRadius * 0.5f, 6);

    // Generate primary feathers along hand (wrist to tip)
    std::vector<glm::vec3> primaryAttachPoints;
    for (int i = 0; i < config.primaryCount; ++i) {
        float t = static_cast<float>(i) / (config.primaryCount - 1);
        glm::vec3 point = glm::mix(wrist, tip, t);
        // Offset slightly for trailing edge
        point.z -= chord * 0.3f * (1.0f - t * 0.5f);
        primaryAttachPoints.push_back(point);
    }
    generateFeatherRow(mesh, primaryAttachPoints, glm::vec3(0, 0, -1),
                       config, config.primaryLength);

    // Generate secondary feathers along forearm (elbow to wrist)
    std::vector<glm::vec3> secondaryAttachPoints;
    for (int i = 0; i < config.secondaryCount; ++i) {
        float t = static_cast<float>(i) / (config.secondaryCount - 1);
        glm::vec3 point = glm::mix(elbow, wrist, t);
        point.z -= chord * 0.35f;
        secondaryAttachPoints.push_back(point);
    }
    generateFeatherRow(mesh, secondaryAttachPoints, glm::vec3(0, 0, -1),
                       config, config.secondaryLength);

    // Generate tertial feathers near body
    std::vector<glm::vec3> tertialAttachPoints;
    for (int i = 0; i < config.tertialCount; ++i) {
        float t = static_cast<float>(i) / std::max(1, config.tertialCount - 1);
        glm::vec3 point = glm::mix(shoulder, elbow, t * 0.5f);
        point.z -= chord * 0.4f;
        tertialAttachPoints.push_back(point);
    }
    generateFeatherRow(mesh, tertialAttachPoints, glm::vec3(0, 0, -1),
                       config, config.secondaryLength * 0.8f);

    // Generate covert feathers (covering the base of flight feathers)
    for (int row = 0; row < config.covertRows; ++row) {
        float rowOffset = row * chord * 0.1f;
        std::vector<glm::vec3> covertPoints;
        int covertCount = config.primaryCount + config.secondaryCount - row * 2;

        for (int i = 0; i < covertCount; ++i) {
            float t = static_cast<float>(i) / (covertCount - 1);
            glm::vec3 point = glm::mix(shoulder, tip, t);
            point.z -= chord * 0.15f - rowOffset;
            point.y += rowOffset * 0.5f;
            covertPoints.push_back(point);
        }

        float covertLength = config.featherWidth * 3.0f * (1.0f - row * 0.2f);
        generateFeatherRow(mesh, covertPoints, glm::vec3(0, 0, -1),
                           config, covertLength);
    }

    // Generate alula (thumb feathers) at wrist
    generateAlulaFeathers(mesh, wrist + glm::vec3(0, chord * 0.1f, chord * 0.2f), config);

    calculateNormals(mesh);
    generateUVCoordinates(mesh, 1.0f, 1.0f);
    mesh.calculateBounds();

    return mesh;
}

void WingMeshGenerator::generateWingBone(MeshData& mesh, const glm::vec3& start,
                                          const glm::vec3& end, float radius, int segments) {
    glm::vec3 dir = glm::normalize(end - start);
    float length = glm::length(end - start);

    // Create perpendicular vectors for the bone cross-section
    glm::vec3 perp1 = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    if (glm::length(perp1) < 0.01f) {
        perp1 = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
    }
    glm::vec3 perp2 = glm::normalize(glm::cross(dir, perp1));

    unsigned int baseIndex = mesh.vertices.size();

    // Generate cylinder vertices
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        glm::vec3 pos = start + dir * (length * t);
        float currentRadius = radius * (1.0f - t * 0.3f);  // Taper toward end

        for (int j = 0; j < 8; ++j) {
            float angle = static_cast<float>(j) / 8.0f * 2.0f * 3.14159f;
            glm::vec3 offset = perp1 * std::cos(angle) + perp2 * std::sin(angle);
            glm::vec3 vertexPos = pos + offset * currentRadius;
            glm::vec3 normal = offset;

            mesh.vertices.emplace_back(vertexPos, normal, glm::vec2(t, angle / (2.0f * 3.14159f)));
        }
    }

    // Generate indices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < 8; ++j) {
            int next = (j + 1) % 8;
            unsigned int a = baseIndex + i * 8 + j;
            unsigned int b = baseIndex + i * 8 + next;
            unsigned int c = baseIndex + (i + 1) * 8 + j;
            unsigned int d = baseIndex + (i + 1) * 8 + next;

            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
    }
}

void WingMeshGenerator::generateFeatherRow(MeshData& mesh,
                                            const std::vector<glm::vec3>& attachPoints,
                                            const glm::vec3& direction,
                                            const FeatherConfig& config,
                                            float lengthMultiplier) {
    for (size_t i = 0; i < attachPoints.size(); ++i) {
        float positionFactor = static_cast<float>(i) / (attachPoints.size() - 1);

        // Feathers get longer toward the tip
        float length = lengthMultiplier * (0.7f + positionFactor * 0.3f);
        float width = config.featherWidth * (1.0f - positionFactor * 0.3f);

        // Slight fan-out angle
        float angle = positionFactor * 0.2f;
        glm::vec3 featherDir = glm::rotateY(direction, angle);

        generateFeatherGeometry(mesh, attachPoints[i], featherDir, length, width, m_featherDetail);
    }
}

void WingMeshGenerator::generateFeatherGeometry(MeshData& mesh, const glm::vec3& base,
                                                 const glm::vec3& direction,
                                                 float length, float width, int barbCount) {
    unsigned int baseIndex = mesh.vertices.size();

    // Create a feather shape with rachis (shaft) and barbs
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    if (glm::length(right) < 0.01f) {
        right = glm::normalize(glm::cross(dir, glm::vec3(0, 0, 1)));
    }
    up = glm::normalize(glm::cross(right, dir));

    // Generate rachis (central shaft) vertices
    int shaftSegments = std::max(3, barbCount / 2);
    for (int i = 0; i <= shaftSegments; ++i) {
        float t = static_cast<float>(i) / shaftSegments;
        glm::vec3 pos = base + dir * (length * t);

        // Shaft curves slightly
        float curve = std::sin(t * 3.14159f) * length * 0.05f;
        pos += up * curve;

        // Width tapers
        float currentWidth = width * (1.0f - t * 0.7f);

        // Add shaft vertices (simple quad strip)
        mesh.vertices.emplace_back(pos - right * 0.01f, -up, glm::vec2(t, 0.0f));
        mesh.vertices.emplace_back(pos + right * 0.01f, -up, glm::vec2(t, 1.0f));

        // Add barb vertices on each side
        if (i > 0 && i < shaftSegments) {
            glm::vec3 barbL = pos - right * currentWidth;
            glm::vec3 barbR = pos + right * currentWidth;

            mesh.vertices.emplace_back(barbL, -up, glm::vec2(t, 0.0f));
            mesh.vertices.emplace_back(barbR, -up, glm::vec2(t, 1.0f));
        }
    }

    // Generate indices for feather surface
    for (int i = 0; i < shaftSegments; ++i) {
        unsigned int idx = baseIndex + i * 4;
        if (i > 0 && i < shaftSegments - 1) {
            // Full quad with barbs
            mesh.indices.push_back(idx);
            mesh.indices.push_back(idx + 4);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 4);
            mesh.indices.push_back(idx + 5);

            // Left barb triangle
            mesh.indices.push_back(idx);
            mesh.indices.push_back(idx + 2);
            mesh.indices.push_back(idx + 4);

            // Right barb triangle
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 5);
            mesh.indices.push_back(idx + 3);
        } else {
            // Just shaft
            unsigned int next = idx + 2;
            mesh.indices.push_back(idx);
            mesh.indices.push_back(next);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(next);
            mesh.indices.push_back(next + 1);
        }
    }
}

void WingMeshGenerator::generateAlulaFeathers(MeshData& mesh, const glm::vec3& wristPos,
                                               const FeatherConfig& config) {
    // Alula is the "thumb" of the wing with 3-4 small feathers
    int alulaCount = 4;
    glm::vec3 alulaDir(0.3f, 0.1f, 0.2f);  // Points forward and up

    for (int i = 0; i < alulaCount; ++i) {
        float t = static_cast<float>(i) / (alulaCount - 1);
        glm::vec3 attachPoint = wristPos + glm::vec3(0.02f * i, 0.01f * i, 0);
        float length = config.primaryLength * 0.3f * (1.0f - t * 0.3f);

        generateFeatherGeometry(mesh, attachPoint, alulaDir, length,
                                config.featherWidth * 0.6f, 4);
    }
}

void WingMeshGenerator::generateWinglet(MeshData& mesh, const glm::vec3& tipPos,
                                         const FeatherConfig& config) {
    // Winglet feathers that curve upward at wing tip (like eagles)
    int wingletCount = 5;
    for (int i = 0; i < wingletCount; ++i) {
        float t = static_cast<float>(i) / (wingletCount - 1);
        float angle = t * 0.8f;  // Spread angle

        glm::vec3 dir = glm::rotateX(glm::vec3(1, 0, 0), angle);
        dir = glm::rotateZ(dir, -0.3f);  // Curve upward

        float length = config.primaryLength * (0.5f + t * 0.3f);
        generateFeatherGeometry(mesh, tipPos, dir, length,
                                config.featherWidth * 0.8f, m_featherDetail);
    }
}

// =============================================================================
// MEMBRANE WING GENERATION
// =============================================================================

MeshData WingMeshGenerator::generateMembraneWing(const Genome& genome,
                                                  const MembraneConfig& config) {
    MeshData mesh;

    float wingspan = genome.wingSpan * genome.size;

    // Bat/dragon wing has finger bones with membrane between them
    glm::vec3 shoulder(0.0f, 0.0f, 0.0f);
    glm::vec3 elbow(wingspan * 0.25f, wingspan * 0.02f, 0.0f);
    glm::vec3 wrist(wingspan * 0.4f, wingspan * 0.03f, 0.0f);

    // Generate arm bones
    float armRadius = wingspan * 0.02f;
    generateWingBone(mesh, shoulder, elbow, armRadius, 8);
    generateWingBone(mesh, elbow, wrist, armRadius * 0.7f, 8);

    // Generate finger bones from wrist
    std::vector<std::vector<glm::vec3>> fingerPaths;
    for (int i = 0; i < config.fingerCount; ++i) {
        float angle = (static_cast<float>(i) / (config.fingerCount - 1) - 0.5f) *
                      config.fingerSpread * 3.14159f;

        // Fingers curve outward and backward
        std::vector<glm::vec3> fingerPath;
        fingerPath.push_back(wrist);

        float fingerLen = config.fingerLength * wingspan * 0.6f *
                          (1.0f - std::abs(static_cast<float>(i) / config.fingerCount - 0.5f) * 0.3f);

        int fingerSegments = 3;
        for (int j = 1; j <= fingerSegments; ++j) {
            float t = static_cast<float>(j) / fingerSegments;
            glm::vec3 pos = wrist;
            pos.x += fingerLen * t * std::cos(angle);
            pos.z -= fingerLen * t * std::sin(angle);
            pos.y += wingspan * 0.02f * std::sin(t * 3.14159f);  // Slight curve

            fingerPath.push_back(pos);
        }

        fingerPaths.push_back(fingerPath);

        // Generate finger bone geometry
        for (size_t j = 0; j < fingerPath.size() - 1; ++j) {
            float segmentRadius = armRadius * 0.3f * (1.0f - static_cast<float>(j) / fingerPath.size());
            generateFingerBone(mesh, fingerPath[j], fingerPath[j + 1],
                              segmentRadius, segmentRadius * 0.7f, 6);
        }
    }

    // Generate membrane between fingers
    for (int i = 0; i < static_cast<int>(fingerPaths.size()) - 1; ++i) {
        generateMembraneBetween(mesh, fingerPaths[i], fingerPaths[i + 1], config.thickness);
    }

    // Generate membrane from body to first finger (plagiopatagium)
    std::vector<glm::vec3> bodyEdge = {shoulder, elbow, wrist};
    generateMembraneBetween(mesh, bodyEdge, fingerPaths[0], config.thickness);

    // Generate thumb claw
    if (config.thumbSize > 0) {
        glm::vec3 thumbPos = wrist + glm::vec3(0.05f * wingspan, 0.02f * wingspan, 0.05f * wingspan);
        generateThumbClaw(mesh, thumbPos, config.thumbSize * wingspan * 0.1f);
    }

    // Add membrane veins
    if (config.veinDensity > 0) {
        // Generate vein paths across membrane
        std::vector<glm::vec3> veinPath;
        for (int i = 0; i < static_cast<int>(fingerPaths.size()); ++i) {
            if (fingerPaths[i].size() > 1) {
                veinPath.push_back(fingerPaths[i][1]);  // Mid-finger
            }
        }
        generateMembraneVeins(mesh, veinPath, wingspan * 0.005f * config.veinDensity);
    }

    calculateNormals(mesh);
    generateUVCoordinates(mesh, 1.0f, 1.0f);
    mesh.calculateBounds();

    return mesh;
}

void WingMeshGenerator::generateFingerBone(MeshData& mesh, const glm::vec3& start,
                                            const glm::vec3& end, float startRadius,
                                            float endRadius, int segments) {
    glm::vec3 dir = glm::normalize(end - start);
    float length = glm::length(end - start);

    glm::vec3 perp1 = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    if (glm::length(perp1) < 0.01f) {
        perp1 = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
    }
    glm::vec3 perp2 = glm::normalize(glm::cross(dir, perp1));

    unsigned int baseIndex = mesh.vertices.size();

    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        glm::vec3 pos = start + dir * (length * t);
        float radius = glm::mix(startRadius, endRadius, t);

        for (int j = 0; j < 6; ++j) {
            float angle = static_cast<float>(j) / 6.0f * 2.0f * 3.14159f;
            glm::vec3 offset = perp1 * std::cos(angle) + perp2 * std::sin(angle);
            mesh.vertices.emplace_back(pos + offset * radius, offset,
                                       glm::vec2(t, angle / (2.0f * 3.14159f)));
        }
    }

    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < 6; ++j) {
            int next = (j + 1) % 6;
            unsigned int a = baseIndex + i * 6 + j;
            unsigned int b = baseIndex + i * 6 + next;
            unsigned int c = baseIndex + (i + 1) * 6 + j;
            unsigned int d = baseIndex + (i + 1) * 6 + next;

            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(b);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(d);
        }
    }
}

void WingMeshGenerator::generateMembraneBetween(MeshData& mesh,
                                                 const std::vector<glm::vec3>& edge1,
                                                 const std::vector<glm::vec3>& edge2,
                                                 float thickness) {
    if (edge1.size() < 2 || edge2.size() < 2) return;

    unsigned int baseIndex = mesh.vertices.size();

    // Resample edges to same number of points
    int samples = std::max(edge1.size(), edge2.size()) * 2;

    auto sampleEdge = [](const std::vector<glm::vec3>& edge, int idx, int total) -> glm::vec3 {
        float t = static_cast<float>(idx) / (total - 1);
        float edgeT = t * (edge.size() - 1);
        int segIdx = static_cast<int>(edgeT);
        float localT = edgeT - segIdx;

        if (segIdx >= static_cast<int>(edge.size()) - 1) {
            return edge.back();
        }
        return glm::mix(edge[segIdx], edge[segIdx + 1], localT);
    };

    // Generate membrane surface
    for (int i = 0; i < samples; ++i) {
        glm::vec3 p1 = sampleEdge(edge1, i, samples);
        glm::vec3 p2 = sampleEdge(edge2, i, samples);

        // Create quad strip between edges
        float t = static_cast<float>(i) / (samples - 1);

        // Calculate normal (approximate)
        glm::vec3 edgeDir = p2 - p1;
        glm::vec3 spanDir = (i > 0) ?
            sampleEdge(edge1, i, samples) - sampleEdge(edge1, i - 1, samples) :
            sampleEdge(edge1, 1, samples) - sampleEdge(edge1, 0, samples);
        glm::vec3 normal = glm::normalize(glm::cross(edgeDir, spanDir));

        mesh.vertices.emplace_back(p1, normal, glm::vec2(0.0f, t));
        mesh.vertices.emplace_back(p2, normal, glm::vec2(1.0f, t));
    }

    // Generate indices
    for (int i = 0; i < samples - 1; ++i) {
        unsigned int idx = baseIndex + i * 2;
        mesh.indices.push_back(idx);
        mesh.indices.push_back(idx + 2);
        mesh.indices.push_back(idx + 1);
        mesh.indices.push_back(idx + 1);
        mesh.indices.push_back(idx + 2);
        mesh.indices.push_back(idx + 3);
    }
}

void WingMeshGenerator::generateMembraneVeins(MeshData& mesh,
                                               const std::vector<glm::vec3>& veinPaths,
                                               float thickness) {
    // Generate thin tubes along vein paths
    for (size_t i = 0; i < veinPaths.size() - 1; ++i) {
        generateFingerBone(mesh, veinPaths[i], veinPaths[i + 1],
                          thickness, thickness * 0.8f, 4);
    }
}

void WingMeshGenerator::generateThumbClaw(MeshData& mesh, const glm::vec3& position,
                                           float size) {
    // Simple cone for claw
    unsigned int baseIndex = mesh.vertices.size();
    int segments = 6;

    glm::vec3 tip = position + glm::vec3(0, 0, -size);
    mesh.vertices.emplace_back(tip, glm::vec3(0, 0, -1), glm::vec2(0.5f, 0.0f));

    for (int i = 0; i < segments; ++i) {
        float angle = static_cast<float>(i) / segments * 2.0f * 3.14159f;
        glm::vec3 offset(std::cos(angle) * size * 0.3f, std::sin(angle) * size * 0.3f, 0);
        glm::vec3 pos = position + offset;
        glm::vec3 normal = glm::normalize(offset + glm::vec3(0, 0, 0.5f));
        mesh.vertices.emplace_back(pos, normal,
                                   glm::vec2(static_cast<float>(i) / segments, 1.0f));
    }

    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 1 + next);
        mesh.indices.push_back(baseIndex + 1 + i);
    }
}

// =============================================================================
// INSECT WING GENERATION
// =============================================================================

MeshData WingMeshGenerator::generateInsectWing(const Genome& genome,
                                                const InsectWingConfig& config) {
    MeshData mesh;

    float length = config.length * genome.size;
    float width = config.width * genome.size;

    // Generate wing outline
    std::vector<glm::vec3> outline;
    int outlinePoints = 24;

    for (int i = 0; i <= outlinePoints; ++i) {
        float t = static_cast<float>(i) / outlinePoints;
        glm::vec3 point = getInsectWingProfile(t, length, width);
        outline.push_back(point);
    }

    // Generate wing membrane
    generateInsectMembrane(mesh, outline, config.thickness);

    // Generate wing veins
    if (config.veinComplexity > 0) {
        // Main longitudinal veins
        for (int v = 0; v < 5; ++v) {
            float vOffset = (static_cast<float>(v) / 4.0f - 0.5f) * width * 0.8f;
            std::vector<glm::vec3> veinPath;

            for (int i = 0; i < 8; ++i) {
                float t = static_cast<float>(i) / 7.0f;
                glm::vec3 pos(t * length, vOffset * (1.0f - t * 0.5f), config.thickness * 1.5f);
                veinPath.push_back(pos);
            }

            generateInsectVeins(mesh, veinPath, config.thickness * 2.0f);
        }

        // Cross veins
        int crossVeinCount = static_cast<int>(config.veinComplexity * 10);
        for (int i = 0; i < crossVeinCount; ++i) {
            float x = length * (0.2f + 0.6f * static_cast<float>(i) / crossVeinCount);
            std::vector<glm::vec3> crossVein = {
                glm::vec3(x, -width * 0.4f, config.thickness * 1.5f),
                glm::vec3(x, width * 0.4f, config.thickness * 1.5f)
            };
            generateInsectVeins(mesh, crossVein, config.thickness * 1.5f);
        }
    }

    // Generate scales for butterfly wings
    if (config.hasScales) {
        // Generate surface points for scales
        std::vector<glm::vec3> surfacePoints;
        for (int x = 0; x < 10; ++x) {
            for (int y = 0; y < 6; ++y) {
                float tx = static_cast<float>(x) / 9.0f;
                float ty = (static_cast<float>(y) / 5.0f - 0.5f) * 0.8f;
                glm::vec3 pos = getInsectWingProfile(tx, length, width);
                pos.y = ty * width * (1.0f - tx * 0.5f);
                surfacePoints.push_back(pos);
            }
        }
        generateButterflyScales(mesh, surfacePoints, length * 0.02f, config.color);
    }

    calculateNormals(mesh);
    generateUVCoordinates(mesh, 1.0f, 1.0f);
    mesh.calculateBounds();

    return mesh;
}

void WingMeshGenerator::generateInsectVeins(MeshData& mesh,
                                             const std::vector<glm::vec3>& veinPath,
                                             float thickness) {
    if (veinPath.size() < 2) return;

    for (size_t i = 0; i < veinPath.size() - 1; ++i) {
        generateFingerBone(mesh, veinPath[i], veinPath[i + 1],
                          thickness, thickness * 0.8f, 4);
    }
}

void WingMeshGenerator::generateInsectMembrane(MeshData& mesh,
                                                const std::vector<glm::vec3>& outline,
                                                float thickness) {
    if (outline.size() < 3) return;

    unsigned int baseIndex = mesh.vertices.size();

    // Calculate centroid
    glm::vec3 centroid(0.0f);
    for (const auto& p : outline) {
        centroid += p;
    }
    centroid /= static_cast<float>(outline.size());

    // Add center vertex
    mesh.vertices.emplace_back(centroid, glm::vec3(0, 0, 1), glm::vec2(0.5f, 0.5f));

    // Add outline vertices
    for (size_t i = 0; i < outline.size(); ++i) {
        float t = static_cast<float>(i) / outline.size();
        mesh.vertices.emplace_back(outline[i], glm::vec3(0, 0, 1),
                                   glm::vec2(0.5f + 0.5f * std::cos(t * 6.28318f),
                                            0.5f + 0.5f * std::sin(t * 6.28318f)));
    }

    // Generate fan triangles
    for (size_t i = 0; i < outline.size() - 1; ++i) {
        mesh.indices.push_back(baseIndex);  // Center
        mesh.indices.push_back(baseIndex + 1 + i);
        mesh.indices.push_back(baseIndex + 1 + i + 1);
    }
    // Close the fan
    mesh.indices.push_back(baseIndex);
    mesh.indices.push_back(baseIndex + outline.size());
    mesh.indices.push_back(baseIndex + 1);
}

void WingMeshGenerator::generateButterflyScales(MeshData& mesh,
                                                 const std::vector<glm::vec3>& surface,
                                                 float scaleSize,
                                                 const glm::vec3& color) {
    // Generate tiny scale quads across the wing surface
    for (const auto& pos : surface) {
        unsigned int baseIndex = mesh.vertices.size();

        // Each scale is a tiny tilted quad
        glm::vec3 right(scaleSize, 0, 0);
        glm::vec3 up(0, scaleSize * 0.7f, scaleSize * 0.3f);
        glm::vec3 normal(0, 0.3f, 0.95f);

        mesh.vertices.emplace_back(pos, normal, glm::vec2(0, 0));
        mesh.vertices.emplace_back(pos + right, normal, glm::vec2(1, 0));
        mesh.vertices.emplace_back(pos + up, normal, glm::vec2(0, 1));
        mesh.vertices.emplace_back(pos + right + up, normal, glm::vec2(1, 1));

        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 1);
        mesh.indices.push_back(baseIndex + 2);
        mesh.indices.push_back(baseIndex + 1);
        mesh.indices.push_back(baseIndex + 3);
        mesh.indices.push_back(baseIndex + 2);
    }
}

void WingMeshGenerator::generateHaltere(MeshData& mesh, const glm::vec3& position,
                                         float size) {
    // Halteres are vestigial hindwings in flies - small club-shaped organs
    unsigned int baseIndex = mesh.vertices.size();

    // Stalk
    glm::vec3 end = position + glm::vec3(size * 0.7f, 0, 0);
    generateFingerBone(mesh, position, end, size * 0.05f, size * 0.03f, 4);

    // Club (knob at end)
    int segments = 6;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = t * 3.14159f;
        float radius = size * 0.1f * std::sin(angle);
        glm::vec3 pos = end + glm::vec3(size * 0.3f * t, 0, 0);

        for (int j = 0; j < 6; ++j) {
            float phi = static_cast<float>(j) / 6.0f * 2.0f * 3.14159f;
            glm::vec3 offset(0, std::cos(phi) * radius, std::sin(phi) * radius);
            mesh.vertices.emplace_back(pos + offset, glm::normalize(offset),
                                       glm::vec2(t, static_cast<float>(j) / 6.0f));
        }
    }
}

// =============================================================================
// WING PROFILE SHAPES
// =============================================================================

glm::vec3 WingMeshGenerator::getEllipticalWingProfile(float t, float span, float chord) {
    // Elliptical wing planform (good all-around performance)
    float x = t * span;
    float y = chord * std::sqrt(1.0f - std::pow(2.0f * t - 1.0f, 2.0f));
    return glm::vec3(x, 0, -y * 0.5f);
}

glm::vec3 WingMeshGenerator::getPointedWingProfile(float t, float span, float chord) {
    // Pointed wing (high speed, like falcons)
    float x = t * span;
    float y = chord * (1.0f - t);  // Linear taper
    // Add sweep
    float sweepOffset = t * span * 0.3f;
    return glm::vec3(x, 0, -y * 0.5f - sweepOffset);
}

glm::vec3 WingMeshGenerator::getHighAspectWingProfile(float t, float span, float chord) {
    // Long, narrow wing (albatross-style)
    float x = t * span;
    float y = chord * (1.0f - t * 0.3f);  // Gentle taper
    return glm::vec3(x, 0, -y * 0.3f);
}

glm::vec3 WingMeshGenerator::getSlottedWingTip(float t, int slotCount, float slotDepth) {
    // Wing tip with slots between primary feathers (eagles, vultures)
    float angle = t * 3.14159f * 0.5f;
    float slot = 0;
    if (slotCount > 0) {
        slot = std::sin(t * slotCount * 3.14159f) * slotDepth;
    }
    return glm::vec3(std::cos(angle) * (1.0f - slot),
                     std::sin(angle) * 0.1f,
                     -std::sin(angle) * 0.3f);
}

glm::vec3 WingMeshGenerator::getInsectWingProfile(float t, float length, float width) {
    // Typical insect wing shape (pointed at tip)
    float x = t * length;
    float y = width * 4.0f * t * (1.0f - t);  // Parabolic width
    return glm::vec3(x, 0, 0);
}

glm::vec3 WingMeshGenerator::getButterflyWingProfile(float t, float length, float width) {
    // Butterfly wing with broader tip
    float x = t * length;
    // More complex shape with multiple lobes
    float y = width * (0.5f + 0.5f * std::sin(t * 3.14159f * 2.0f)) *
              (1.0f - std::pow(t - 0.5f, 2.0f) * 2.0f);
    return glm::vec3(x, y, 0);
}

// =============================================================================
// SPECIFIC CREATURE WING GENERATORS
// =============================================================================

MeshData WingMeshGenerator::generateSongbirdWing(const Genome& genome) {
    FeatherConfig config = configFromGenome(genome);
    config.primaryCount = 9;
    config.secondaryCount = 9;
    config.primaryLength = genome.wingSpan * 0.4f;
    config.secondaryLength = genome.wingSpan * 0.35f;
    return generateFeatheredWing(genome, config);
}

MeshData WingMeshGenerator::generateRaptorWing(const Genome& genome) {
    FeatherConfig config = configFromGenome(genome);
    config.primaryCount = 10;
    config.secondaryCount = 14;
    config.covertRows = 4;
    config.primaryLength = genome.wingSpan * 0.5f;
    config.secondaryLength = genome.wingSpan * 0.4f;

    MeshData mesh = generateFeatheredWing(genome, config);

    // Add slotted wing tip for raptors
    glm::vec3 tipPos(genome.wingSpan * genome.size, 0, 0);
    generateWinglet(mesh, tipPos, config);

    return mesh;
}

MeshData WingMeshGenerator::generateHummingbirdWing(const Genome& genome) {
    FeatherConfig config = configFromGenome(genome);
    config.primaryCount = 10;
    config.secondaryCount = 6;
    config.covertRows = 2;
    config.primaryLength = genome.wingSpan * 0.6f;  // Long primaries
    config.secondaryLength = genome.wingSpan * 0.25f;  // Short secondaries
    config.featherWidth = 0.08f;
    return generateFeatheredWing(genome, config);
}

MeshData WingMeshGenerator::generateOwlWing(const Genome& genome) {
    FeatherConfig config = configFromGenome(genome);
    config.primaryCount = 10;
    config.secondaryCount = 12;
    config.covertRows = 4;
    config.primaryLength = genome.wingSpan * 0.45f;
    config.secondaryLength = genome.wingSpan * 0.4f;
    config.featherWidth = 0.15f;  // Broader feathers
    // Owls have soft feather edges for silent flight
    config.barbDensity = 0.5f;
    return generateFeatheredWing(genome, config);
}

MeshData WingMeshGenerator::generateSeabirdWing(const Genome& genome) {
    FeatherConfig config = configFromGenome(genome);
    config.primaryCount = 11;
    config.secondaryCount = 20;  // More secondaries for long wing
    config.covertRows = 3;
    config.primaryLength = genome.wingSpan * 0.35f;
    config.secondaryLength = genome.wingSpan * 0.3f;
    config.featherWidth = 0.07f;  // Narrow feathers
    return generateFeatheredWing(genome, config);
}

MeshData WingMeshGenerator::generateBatWing(const Genome& genome) {
    MembraneConfig config = membraneConfigFromGenome(genome);
    config.fingerCount = 4;
    config.fingerLength = 1.0f;
    config.fingerSpread = 0.7f;
    config.thumbSize = 0.12f;
    config.webbing = 1.0f;
    config.veinDensity = 0.6f;
    config.translucency = 0.25f;
    return generateMembraneWing(genome, config);
}

MeshData WingMeshGenerator::generateDragonflyWing(const Genome& genome) {
    InsectWingConfig config = insectConfigFromGenome(genome);
    config.length = genome.wingSpan * 0.5f;
    config.width = genome.wingSpan * 0.1f;
    config.veinComplexity = 0.9f;  // Complex venation
    config.hasScales = false;
    config.hasHindwings = true;
    config.hindwingRatio = 0.95f;  // Nearly equal fore/hindwings
    config.couplingStrength = 0.0f;  // Independent wing movement
    return generateInsectWing(genome, config);
}

MeshData WingMeshGenerator::generateButterflyWing(const Genome& genome) {
    InsectWingConfig config = insectConfigFromGenome(genome);
    config.length = genome.wingSpan * 0.45f;
    config.width = genome.wingSpan * 0.35f;  // Broader wings
    config.veinComplexity = 0.5f;
    config.hasScales = true;
    config.scaleIridescence = 0.3f;
    config.hasHindwings = true;
    config.hindwingRatio = 0.8f;
    config.couplingStrength = 0.5f;
    return generateInsectWing(genome, config);
}

MeshData WingMeshGenerator::generateBeeWing(const Genome& genome) {
    InsectWingConfig config = insectConfigFromGenome(genome);
    config.length = genome.wingSpan * 0.4f;
    config.width = genome.wingSpan * 0.12f;
    config.veinComplexity = 0.4f;
    config.hasScales = false;
    config.hasHindwings = true;
    config.hindwingRatio = 0.65f;  // Smaller hindwings
    config.couplingStrength = 0.9f;  // Coupled for synchronized beating
    return generateInsectWing(genome, config);
}

MeshData WingMeshGenerator::generateDragonWing(const Genome& genome,
                                                const MembraneConfig& config) {
    MembraneConfig dragonConfig = config;
    dragonConfig.fingerCount = 5;  // More fingers for larger wing
    dragonConfig.fingerLength = 1.5f;
    dragonConfig.fingerSpread = 0.85f;
    dragonConfig.thumbSize = 0.2f;
    dragonConfig.webbing = 1.0f;
    dragonConfig.veinDensity = 0.8f;
    dragonConfig.thickness = 0.04f;  // Thicker membrane
    return generateMembraneWing(genome, dragonConfig);
}

// =============================================================================
// SKELETON GENERATION
// =============================================================================

WingSkeleton WingMeshGenerator::generateFeatheredSkeleton(const Genome& genome,
                                                           int segmentCount) {
    WingSkeleton skeleton;

    float wingspan = genome.wingSpan * genome.size;

    // Shoulder
    WingBone shoulder;
    shoulder.name = "shoulder";
    shoulder.position = glm::vec3(0, 0, 0);
    shoulder.rotation = glm::quat(1, 0, 0, 0);
    shoulder.length = wingspan * 0.3f;
    shoulder.parentIndex = -1;
    skeleton.bones.push_back(shoulder);
    skeleton.shoulderIdx = 0;

    // Elbow
    WingBone elbow;
    elbow.name = "elbow";
    elbow.position = glm::vec3(wingspan * 0.3f, 0, 0);
    elbow.rotation = glm::quat(1, 0, 0, 0);
    elbow.length = wingspan * 0.25f;
    elbow.parentIndex = 0;
    skeleton.bones.push_back(elbow);
    skeleton.elbowIdx = 1;

    // Wrist
    WingBone wrist;
    wrist.name = "wrist";
    wrist.position = glm::vec3(wingspan * 0.55f, 0, 0);
    wrist.rotation = glm::quat(1, 0, 0, 0);
    wrist.length = wingspan * 0.45f;
    wrist.parentIndex = 1;
    skeleton.bones.push_back(wrist);
    skeleton.wristIdx = 2;

    // Primary feather bones
    int primaryBones = 5;
    for (int i = 0; i < primaryBones; ++i) {
        float t = static_cast<float>(i) / (primaryBones - 1);
        WingBone primary;
        primary.name = "primary_" + std::to_string(i);
        primary.position = glm::vec3(wingspan * (0.55f + 0.45f * t), 0, 0);
        primary.rotation = glm::quat(1, 0, 0, 0);
        primary.length = wingspan * 0.1f;
        primary.parentIndex = 2;
        skeleton.bones.push_back(primary);

        if (i == 0) skeleton.primaryIdx = skeleton.bones.size() - 1;
    }

    // Calculate bind pose matrices
    skeleton.bindPose.resize(skeleton.bones.size());
    skeleton.inverseBindPose.resize(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), skeleton.bones[i].position);
        local = local * glm::toMat4(skeleton.bones[i].rotation);

        if (skeleton.bones[i].parentIndex >= 0) {
            skeleton.bindPose[i] = skeleton.bindPose[skeleton.bones[i].parentIndex] * local;
        } else {
            skeleton.bindPose[i] = local;
        }

        skeleton.inverseBindPose[i] = glm::inverse(skeleton.bindPose[i]);
    }

    return skeleton;
}

WingSkeleton WingMeshGenerator::generateMembraneSkeleton(const Genome& genome,
                                                          int fingerCount) {
    WingSkeleton skeleton;

    float wingspan = genome.wingSpan * genome.size;

    // Shoulder
    WingBone shoulder;
    shoulder.name = "shoulder";
    shoulder.position = glm::vec3(0, 0, 0);
    shoulder.rotation = glm::quat(1, 0, 0, 0);
    shoulder.length = wingspan * 0.25f;
    shoulder.parentIndex = -1;
    skeleton.bones.push_back(shoulder);
    skeleton.shoulderIdx = 0;

    // Elbow
    WingBone elbow;
    elbow.name = "elbow";
    elbow.position = glm::vec3(wingspan * 0.25f, 0, 0);
    elbow.rotation = glm::quat(1, 0, 0, 0);
    elbow.length = wingspan * 0.15f;
    elbow.parentIndex = 0;
    skeleton.bones.push_back(elbow);
    skeleton.elbowIdx = 1;

    // Wrist
    WingBone wrist;
    wrist.name = "wrist";
    wrist.position = glm::vec3(wingspan * 0.4f, 0, 0);
    wrist.rotation = glm::quat(1, 0, 0, 0);
    wrist.length = wingspan * 0.1f;
    wrist.parentIndex = 1;
    skeleton.bones.push_back(wrist);
    skeleton.wristIdx = 2;

    // Finger bones
    for (int f = 0; f < fingerCount; ++f) {
        float angle = (static_cast<float>(f) / (fingerCount - 1) - 0.5f) * 0.8f * 3.14159f;

        // Each finger has 2-3 segments
        int segments = 3;
        int prevIdx = 2;  // Parent is wrist

        for (int s = 0; s < segments; ++s) {
            float t = static_cast<float>(s + 1) / segments;
            WingBone finger;
            finger.name = "finger_" + std::to_string(f) + "_" + std::to_string(s);

            float fingerLen = wingspan * 0.6f * (1.0f - std::abs(static_cast<float>(f) /
                                                                  fingerCount - 0.5f) * 0.3f);
            finger.position = glm::vec3(
                wingspan * 0.4f + fingerLen * t * std::cos(angle),
                0,
                -fingerLen * t * std::sin(angle)
            );
            finger.rotation = glm::quat(1, 0, 0, 0);
            finger.length = fingerLen / segments;
            finger.parentIndex = prevIdx;

            skeleton.bones.push_back(finger);
            prevIdx = skeleton.bones.size() - 1;
        }
    }

    // Calculate bind pose
    skeleton.bindPose.resize(skeleton.bones.size());
    skeleton.inverseBindPose.resize(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), skeleton.bones[i].position);
        local = local * glm::toMat4(skeleton.bones[i].rotation);

        if (skeleton.bones[i].parentIndex >= 0) {
            skeleton.bindPose[i] = skeleton.bindPose[skeleton.bones[i].parentIndex] * local;
        } else {
            skeleton.bindPose[i] = local;
        }

        skeleton.inverseBindPose[i] = glm::inverse(skeleton.bindPose[i]);
    }

    return skeleton;
}

WingSkeleton WingMeshGenerator::generateInsectSkeleton() {
    WingSkeleton skeleton;

    // Insect wings are simple - just a root and maybe 1-2 segments
    WingBone root;
    root.name = "wing_root";
    root.position = glm::vec3(0, 0, 0);
    root.rotation = glm::quat(1, 0, 0, 0);
    root.length = 0.1f;
    root.parentIndex = -1;
    skeleton.bones.push_back(root);
    skeleton.shoulderIdx = 0;

    WingBone mid;
    mid.name = "wing_mid";
    mid.position = glm::vec3(0.3f, 0, 0);
    mid.rotation = glm::quat(1, 0, 0, 0);
    mid.length = 0.2f;
    mid.parentIndex = 0;
    skeleton.bones.push_back(mid);

    WingBone tip;
    tip.name = "wing_tip";
    tip.position = glm::vec3(0.6f, 0, 0);
    tip.rotation = glm::quat(1, 0, 0, 0);
    tip.length = 0.2f;
    tip.parentIndex = 1;
    skeleton.bones.push_back(tip);

    // Calculate bind pose
    skeleton.bindPose.resize(skeleton.bones.size());
    skeleton.inverseBindPose.resize(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), skeleton.bones[i].position);
        skeleton.bindPose[i] = (skeleton.bones[i].parentIndex >= 0) ?
            skeleton.bindPose[skeleton.bones[i].parentIndex] * local : local;
        skeleton.inverseBindPose[i] = glm::inverse(skeleton.bindPose[i]);
    }

    return skeleton;
}

// =============================================================================
// CONFIG GENERATION FROM GENOME
// =============================================================================

FeatherConfig WingMeshGenerator::configFromGenome(const Genome& genome) {
    FeatherConfig config;

    config.primaryCount = static_cast<int>(9 + genome.wingAspectRatio * 0.3f);
    config.secondaryCount = static_cast<int>(10 + genome.wingSpan * 3);
    config.tertialCount = 4;
    config.covertRows = 3;

    config.primaryLength = genome.wingSpan * 0.4f * genome.size;
    config.secondaryLength = genome.wingSpan * 0.35f * genome.size;
    config.featherWidth = genome.wingChord * 0.3f;

    config.rachisThickness = 0.01f * genome.size;
    config.barbDensity = 1.0f;

    // Color based on genome
    config.baseColor = genome.color;
    config.tipColor = genome.color * 0.6f;

    return config;
}

MembraneConfig WingMeshGenerator::membraneConfigFromGenome(const Genome& genome) {
    MembraneConfig config;

    config.thickness = 0.02f * genome.size;
    config.elasticity = 0.3f;
    config.fingerCount = 4;
    config.fingerLength = genome.wingSpan * 0.6f;
    config.fingerSpread = 0.7f;
    config.thumbSize = 0.1f * genome.size;
    config.webbing = 1.0f;
    config.veinDensity = 0.5f;
    config.translucency = 0.3f;

    config.membraneColor = genome.color * 0.8f;
    config.boneColor = genome.color * 0.6f;

    return config;
}

InsectWingConfig WingMeshGenerator::insectConfigFromGenome(const Genome& genome) {
    InsectWingConfig config;

    config.length = genome.wingSpan * genome.size * 0.5f;
    config.width = genome.wingChord * genome.size;
    config.thickness = 0.002f * genome.size;
    config.veinComplexity = 0.5f;
    config.hasScales = false;
    config.hasHindwings = true;
    config.hindwingRatio = 0.8f;
    config.couplingStrength = 0.0f;
    config.isHardened = false;

    config.color = genome.color;
    config.veinColor = genome.color * 0.5f;

    return config;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void WingMeshGenerator::calculateNormals(MeshData& mesh) {
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
        if (glm::length(vertex.normal) > 0.001f) {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }
}

void WingMeshGenerator::generateUVCoordinates(MeshData& mesh, float uScale, float vScale) {
    if (mesh.vertices.empty()) return;

    // Calculate bounds
    glm::vec3 minPos = mesh.vertices[0].position;
    glm::vec3 maxPos = mesh.vertices[0].position;

    for (const auto& v : mesh.vertices) {
        minPos = glm::min(minPos, v.position);
        maxPos = glm::max(maxPos, v.position);
    }

    glm::vec3 size = maxPos - minPos;
    if (size.x < 0.001f) size.x = 1.0f;
    if (size.z < 0.001f) size.z = 1.0f;

    // Map XZ coordinates to UV
    for (auto& v : mesh.vertices) {
        v.texCoord.x = ((v.position.x - minPos.x) / size.x) * uScale;
        v.texCoord.y = ((v.position.z - minPos.z) / size.z) * vScale;
    }
}

MeshData WingMeshGenerator::generateLOD(const MeshData& highDetail, int targetTriangles) {
    MeshData lod = highDetail;

    // Simple decimation - remove every Nth triangle
    if (highDetail.indices.size() / 3 <= static_cast<size_t>(targetTriangles)) {
        return lod;
    }

    int skip = highDetail.indices.size() / 3 / targetTriangles;
    if (skip < 2) skip = 2;

    lod.indices.clear();
    for (size_t i = 0; i < highDetail.indices.size(); i += 3 * skip) {
        if (i + 2 < highDetail.indices.size()) {
            lod.indices.push_back(highDetail.indices[i]);
            lod.indices.push_back(highDetail.indices[i + 1]);
            lod.indices.push_back(highDetail.indices[i + 2]);
        }
    }

    return lod;
}

// =============================================================================
// WING ANIMATION HELPERS
// =============================================================================

namespace WingMeshAnimation {

void WingMeshPose::applyToSkeleton(WingSkeleton& skeleton) const {
    if (skeleton.shoulderIdx >= 0 && skeleton.shoulderIdx < static_cast<int>(skeleton.bones.size())) {
        skeleton.bones[skeleton.shoulderIdx].rotation = shoulderRotation;
    }
    if (skeleton.elbowIdx >= 0 && skeleton.elbowIdx < static_cast<int>(skeleton.bones.size())) {
        skeleton.bones[skeleton.elbowIdx].rotation = elbowRotation;
    }
    if (skeleton.wristIdx >= 0 && skeleton.wristIdx < static_cast<int>(skeleton.bones.size())) {
        skeleton.bones[skeleton.wristIdx].rotation = wristRotation;
    }
}

MeshData deformMesh(const MeshData& restPose, const WingSkeleton& skeleton,
                    const std::vector<glm::mat4>& boneTransforms) {
    MeshData deformed = restPose;

    // Simple linear blend skinning
    for (size_t i = 0; i < deformed.vertices.size(); ++i) {
        if (i >= skeleton.boneIndices.size() || i >= skeleton.boneWeights.size()) {
            continue;
        }

        glm::vec4 pos(restPose.vertices[i].position, 1.0f);
        glm::vec4 norm(restPose.vertices[i].normal, 0.0f);

        glm::vec4 skinnedPos(0.0f);
        glm::vec4 skinnedNorm(0.0f);

        glm::ivec4 indices = skeleton.boneIndices[i];
        glm::vec4 weights = skeleton.boneWeights[i];

        for (int j = 0; j < 4; ++j) {
            if (weights[j] > 0.0f && indices[j] >= 0 &&
                indices[j] < static_cast<int>(boneTransforms.size())) {
                glm::mat4 transform = boneTransforms[indices[j]] *
                                      skeleton.inverseBindPose[indices[j]];
                skinnedPos += (transform * pos) * weights[j];
                skinnedNorm += (transform * norm) * weights[j];
            }
        }

        deformed.vertices[i].position = glm::vec3(skinnedPos);
        deformed.vertices[i].normal = glm::normalize(glm::vec3(skinnedNorm));
    }

    return deformed;
}

std::vector<glm::mat4> calculateBoneTransforms(const WingSkeleton& skeleton,
                                                const WingMeshPose& pose) {
    std::vector<glm::mat4> transforms(skeleton.bones.size());

    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        const WingBone& bone = skeleton.bones[i];

        glm::mat4 local = glm::translate(glm::mat4(1.0f), bone.position);
        local = local * glm::toMat4(bone.rotation);

        if (bone.parentIndex >= 0) {
            transforms[i] = transforms[bone.parentIndex] * local;
        } else {
            transforms[i] = local;
        }
    }

    return transforms;
}

WingMeshPose interpolatePoses(const WingMeshPose& a, const WingMeshPose& b, float t) {
    WingMeshPose result;

    result.shoulderRotation = glm::slerp(a.shoulderRotation, b.shoulderRotation, t);
    result.elbowRotation = glm::slerp(a.elbowRotation, b.elbowRotation, t);
    result.wristRotation = glm::slerp(a.wristRotation, b.wristRotation, t);
    result.primaryRotation = glm::slerp(a.primaryRotation, b.primaryRotation, t);

    result.featherSpread = glm::mix(a.featherSpread, b.featherSpread, t);
    result.wingTipBend = glm::mix(a.wingTipBend, b.wingTipBend, t);

    // Interpolate finger rotations
    size_t fingerCount = std::min(a.fingerRotations.size(), b.fingerRotations.size());
    result.fingerRotations.resize(fingerCount);
    for (size_t i = 0; i < fingerCount; ++i) {
        result.fingerRotations[i] = glm::slerp(a.fingerRotations[i], b.fingerRotations[i], t);
    }

    return result;
}

std::vector<WingMeshPose> generateFlapCycle(float amplitude, float frequency,
                                             int keyframeCount) {
    std::vector<WingMeshPose> poses;
    poses.reserve(keyframeCount);

    for (int i = 0; i < keyframeCount; ++i) {
        float t = static_cast<float>(i) / keyframeCount;
        float phase = t * 2.0f * 3.14159f;

        WingMeshPose pose;

        // Shoulder provides main flapping motion
        float shoulderAngle = amplitude * std::sin(phase);
        pose.shoulderRotation = glm::angleAxis(shoulderAngle, glm::vec3(0, 0, 1));

        // Elbow flexes more on upstroke
        float elbowAngle = amplitude * 0.3f * (1.0f + std::sin(phase));
        pose.elbowRotation = glm::angleAxis(elbowAngle, glm::vec3(0, 1, 0));

        // Wrist provides twist/pronation
        float wristAngle = amplitude * 0.2f * std::cos(phase);
        pose.wristRotation = glm::angleAxis(wristAngle, glm::vec3(1, 0, 0));

        // Feathers spread on upstroke
        bool upstroke = std::cos(phase) > 0;
        pose.featherSpread = upstroke ? 0.5f : 0.0f;

        // Wing tip bends under air pressure
        pose.wingTipBend = upstroke ? 5.0f : -10.0f;

        poses.push_back(pose);
    }

    return poses;
}

} // namespace WingMeshAnimation
