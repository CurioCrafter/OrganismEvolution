#include "Terrain.h"
#include "../utils/PerlinNoise.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

// Helper function
float smoothstep(float edge0, float edge1, float x);

Terrain::Terrain(int width, int depth, float scale)
    : width(width), depth(depth), scale(scale), waterLevel(0.35f), VAO(0), VBO(0), EBO(0), indexCount(0) {
}

Terrain::~Terrain() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Terrain::generate(unsigned int seed) {
    PerlinNoise noise(seed);
    heightMap.resize(width * depth);

    // Generate height map using Perlin noise
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            float nx = (float)x / width;
            float nz = (float)z / depth;

            // Multiple octaves for more interesting terrain
            float height = noise.octaveNoise(nx * 4.0f, nz * 4.0f, 6, 0.5f);

            // Create island effect (distance from center)
            float dx = 2.0f * nx - 1.0f;
            float dz = 2.0f * nz - 1.0f;
            float distance = sqrt(dx * dx + dz * dz);
            float islandFactor = 1.0f - smoothstep(0.3f, 1.0f, distance);

            height = height * islandFactor;

            heightMap[z * width + x] = height;
        }
    }

    setupMesh();
}

void Terrain::setupMesh() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z < depth; z++) {
        for (int x = 0; x < width; x++) {
            float height = heightMap[z * width + x];
            glm::vec3 position(
                (x - width / 2.0f) * scale,
                height * 30.0f, // Height scale
                (z - depth / 2.0f) * scale
            );

            glm::vec3 color = getTerrainColor(height);

            // Calculate normal (simplified - average of adjacent heights)
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            if (x > 0 && x < width - 1 && z > 0 && z < depth - 1) {
                float hL = heightMap[z * width + (x - 1)];
                float hR = heightMap[z * width + (x + 1)];
                float hD = heightMap[(z - 1) * width + x];
                float hU = heightMap[(z + 1) * width + x];

                normal = glm::normalize(glm::vec3(hL - hR, 2.0f, hD - hU));
            }

            // Position
            vertices.push_back(position.x);
            vertices.push_back(position.y);
            vertices.push_back(position.z);

            // Normal
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // Color
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }

    // Generate indices
    for (int z = 0; z < depth - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            unsigned int topLeft = z * width + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * width + x;
            unsigned int bottomRight = bottomLeft + 1;

            // Triangle 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Triangle 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    indexCount = indices.size();

    // Create buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Terrain::render() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

float Terrain::getHeight(float x, float z) const {
    // Convert world coordinates to grid coordinates
    int gridX = (int)((x / scale) + width / 2.0f);
    int gridZ = (int)((z / scale) + depth / 2.0f);

    if (gridX < 0 || gridX >= width || gridZ < 0 || gridZ >= depth) {
        return 0.0f;
    }

    return heightMap[gridZ * width + gridX] * 30.0f;
}

bool Terrain::isWater(float x, float z) const {
    int gridX = (int)((x / scale) + width / 2.0f);
    int gridZ = (int)((z / scale) + depth / 2.0f);

    if (gridX < 0 || gridX >= width || gridZ < 0 || gridZ >= depth) {
        return true;
    }

    return heightMap[gridZ * width + gridX] < waterLevel;
}

glm::vec3 Terrain::getTerrainColor(float height) {
    // Water
    if (height < waterLevel) {
        return glm::vec3(0.2f, 0.4f, 0.8f);
    }
    // Beach/Sand
    else if (height < 0.42f) {
        return glm::vec3(0.9f, 0.85f, 0.6f);
    }
    // Grass
    else if (height < 0.65f) {
        return glm::vec3(0.3f, 0.7f, 0.3f);
    }
    // Forest
    else if (height < 0.8f) {
        return glm::vec3(0.2f, 0.5f, 0.2f);
    }
    // Mountain
    else {
        return glm::vec3(0.6f, 0.6f, 0.6f);
    }
}

float smoothstep(float edge0, float edge1, float x) {
    float t = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
