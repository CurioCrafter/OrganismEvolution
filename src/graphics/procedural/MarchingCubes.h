#pragma once

#include "../mesh/MeshData.h"
#include "MetaballSystem.h"
#include <glm/glm.hpp>

class MarchingCubes {
public:
    // Generate mesh from metaball system
    static MeshData generateMesh(
        const MetaballSystem& metaballs,
        int resolution = 24,
        float isovalue = 1.0f
    );

    // Generate a fallback sphere mesh when marching cubes fails
    // This ensures creatures are always visible for debugging
    static MeshData generateFallbackSphere(
        float radius = 1.0f,
        int segments = 16,
        int rings = 12
    );

private:
    // Marching cubes lookup tables
    static const int edgeTable[256];
    static const int triTable[256][16];

    // Interpolate vertex position on an edge
    static glm::vec3 interpolateVertex(
        const glm::vec3& p1, const glm::vec3& p2,
        float val1, float val2, float isovalue
    );

    // Get edge intersection point
    static int getEdgeId(int x, int y, int z, int edge);
};
