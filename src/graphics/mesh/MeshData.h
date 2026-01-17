#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;

    Vertex() : position(0.0f), normal(0.0f, 1.0f, 0.0f), texCoord(0.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm)
        : position(pos), normal(norm), texCoord(0.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& uv)
        : position(pos), normal(norm), texCoord(uv) {}
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 boundsMin;
    glm::vec3 boundsMax;

    MeshData() : boundsMin(0.0f), boundsMax(0.0f) {}

    // GPU upload/cleanup are handled by the DX12 path.
    void upload() {}
    void cleanup() {}

    // Calculate bounding box
    void calculateBounds() {
        if (vertices.empty()) return;

        boundsMin = vertices[0].position;
        boundsMax = vertices[0].position;

        for (const auto& vertex : vertices) {
            boundsMin = glm::min(boundsMin, vertex.position);
            boundsMax = glm::max(boundsMax, vertex.position);
        }
    }

    unsigned int getIndexCount() const { return static_cast<unsigned int>(indices.size()); }
    unsigned int getVertexCount() const { return static_cast<unsigned int>(vertices.size()); }

    ~MeshData() { cleanup(); }
};
