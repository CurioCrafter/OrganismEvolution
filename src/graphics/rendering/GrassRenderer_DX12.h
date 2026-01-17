#pragma once

#include "../../environment/GrassSystem.h"
#include "../LODSystem.h"
#include "RHI/RHI.h"
#include <vector>

using namespace Forge;
using namespace Forge::RHI;

// GPU instance data (must match HLSL layout) - 48 bytes with LOD data
#pragma pack(push, 4)
struct GrassInstanceGPU {
    float position[3];        // 12 bytes - World position
    float rotation;           // 4 bytes  - Y rotation (radians)
    float height;             // 4 bytes  - Blade height
    float width;              // 4 bytes  - Blade width
    float bendFactor;         // 4 bytes  - Bend amount (0-1)
    float colorVariation;     // 4 bytes  - Color variation (0-1)
    float distance;           // 4 bytes  - Distance to camera
    float fadeFactor;         // 4 bytes  - LOD fade (0-1)
    uint32_t lodLevel;        // 4 bytes  - LOD level
    float padding;            // 4 bytes  - Align to 48
};
#pragma pack(pop)
static_assert(sizeof(GrassInstanceGPU) == 48, "GrassInstanceGPU must be 48 bytes");

// Grass cluster data for medium LOD (batched grass patches)
#pragma pack(push, 4)
struct GrassClusterGPU {
    float center[3];          // 12 bytes - Cluster center position
    float radius;             // 4 bytes  - Cluster radius
    float density;            // 4 bytes  - Blade density factor
    float avgHeight;          // 4 bytes  - Average blade height
    float colorVariation;     // 4 bytes  - Color variation
    float fadeFactor;         // 4 bytes  - LOD fade
};
#pragma pack(pop)
static_assert(sizeof(GrassClusterGPU) == 32, "GrassClusterGPU must be 32 bytes");

// Grass constant buffer
struct alignas(256) GrassConstants {
    float viewProj[16];       // 64 bytes
    float cameraPos[4];       // 16 bytes - xyz = pos, w = unused
    float windDir[4];         // 16 bytes - xy = direction, z = strength, w = time
    float grassColors[8];     // 32 bytes - base color (rgb + pad) + tip color (rgb + pad)
    float lightDir[4];        // 16 bytes
    float lightColor[4];      // 16 bytes
    float lodParams[4];       // 16 bytes - x=lodDist1, y=lodDist2, z=maxDist, w=fadeRange
    float fogParams[4];       // 16 bytes - x=fogStart, y=fogEnd, z=density, w=unused
    float fogColor[4];        // 16 bytes - rgb=color, w=unused
    float padding[8];         // 32 bytes - pad to 256
};
static_assert(sizeof(GrassConstants) == 256, "GrassConstants must be 256 bytes");

class GrassRendererDX12 {
public:
    GrassRendererDX12();
    ~GrassRendererDX12();

    bool init(IDevice* device, GrassSystem* grassSystem);

    void updateInstances(const glm::vec3& cameraPos);

    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const glm::mat4& viewProj,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    void setWindParams(const glm::vec2& direction, float strength);

    int getVisibleBladeCount() const { return m_visibleCount; }
    int getClusterCount() const { return m_clusterCount; }

    // LOD configuration
    void setLODConfig(const LOD::LODConfig& config) { m_lodConfig = config; }
    const LOD::LODConfig& getLODConfig() const { return m_lodConfig; }

    // LOD statistics
    int getIndividualBladeCount() const { return m_individualCount; }
    int getClusteredBladeCount() const { return m_clusteredCount; }
    int getTexturedPatchCount() const { return m_texturedCount; }

private:
    IDevice* m_device = nullptr;
    GrassSystem* m_grassSystem = nullptr;

    UniquePtr<IBuffer> m_instanceBuffer;
    UniquePtr<IBuffer> m_clusterBuffer;       // For clustered LOD
    UniquePtr<IBuffer> m_bladeVertexBuffer;   // Billboard quad vertices
    UniquePtr<IBuffer> m_bladeIndexBuffer;
    UniquePtr<IBuffer> m_constantBuffer;

    std::vector<GrassInstanceGPU> m_visibleInstances;
    std::vector<GrassClusterGPU> m_visibleClusters;
    int m_visibleCount = 0;
    int m_clusterCount = 0;

    // LOD counts for stats
    int m_individualCount = 0;
    int m_clusteredCount = 0;
    int m_texturedCount = 0;

    // Wind parameters
    glm::vec2 m_windDirection = {1.0f, 0.0f};
    float m_windStrength = 0.3f;

    // LOD configuration
    LOD::LODConfig m_lodConfig;

    bool createBladeGeometry();
    void updateInstanceBuffer();
    void buildGrassClusters(const glm::vec3& cameraPos);
};
