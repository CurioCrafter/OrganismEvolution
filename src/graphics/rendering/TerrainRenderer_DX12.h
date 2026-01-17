#pragma once

// Terrain Renderer for OrganismEvolution
// Renders heightmap-based terrain chunks using DX12 via Forge RHI
// Features frustum culling, procedural generation, and biome-based coloring
//
// This renderer is self-contained and uses procedural noise for terrain generation.
// Creature height queries are handled by the separate Terrain class in environment/.

#include "../Camera.h"
#include "../Frustum.h"
#include "RHI/RHI.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <memory>

using namespace Forge;
using namespace Forge::RHI;

// Terrain configuration constants
namespace TerrainRendererConfig {
    constexpr int WORLD_SIZE = 2048;           // World units
    constexpr int CHUNK_SIZE = 64;             // Units per chunk
    constexpr int CHUNKS_PER_AXIS = 32;        // 32x32 grid
    constexpr int TOTAL_CHUNKS = 1024;         // Total chunk count
    constexpr float HEIGHT_SCALE = 30.0f;      // World height multiplier
    constexpr float WATER_LEVEL = 0.35f;       // Normalized water level
}

// Terrain constant buffer (256-byte aligned for DX12)
struct alignas(256) TerrainConstants {
    float viewProj[16];       // 64 bytes - combined view*proj matrix
    float world[16];          // 64 bytes - chunk world transform
    float cameraPos[4];       // 16 bytes - camera position + padding
    float lightDir[4];        // 16 bytes - sun direction + padding
    float lightColor[4];      // 16 bytes - sun color + intensity in w
    float terrainScale[4];    // 16 bytes - x=heightScale, y=chunkSize, z=waterLevel, w=time
    float texCoordScale[4];   // 16 bytes - texture tiling parameters
    float padding[8];         // 32 bytes - pad to 256
};
static_assert(sizeof(TerrainConstants) == 256, "TerrainConstants must be 256 bytes");

// Terrain vertex format for DX12
struct TerrainVertexDX12 {
    float position[3];   // World position XYZ
    float normal[3];     // Surface normal
    float color[3];      // Biome color RGB
    float texCoord[2];   // UV coordinates
};
static_assert(sizeof(TerrainVertexDX12) == 44, "TerrainVertexDX12 must be 44 bytes");

class TerrainRendererDX12 {
public:
    TerrainRendererDX12();
    ~TerrainRendererDX12();

    // Non-copyable
    TerrainRendererDX12(const TerrainRendererDX12&) = delete;
    TerrainRendererDX12& operator=(const TerrainRendererDX12&) = delete;

    // Initialize with device
    bool init(IDevice* device);

    // Shutdown and release resources
    void shutdown();

    // Update visible chunks based on camera position
    void update(const Camera& camera);

    // Render visible terrain chunks
    void render(
        ICommandList* cmdList,
        IPipeline* pipeline,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    // Render for shadow pass (depth only)
    void renderForShadow(
        ICommandList* cmdList,
        IPipeline* shadowPipeline,
        const glm::mat4& lightViewProj
    );

    // Statistics
    int getRenderedChunkCount() const { return m_renderedChunks; }
    int getCulledChunkCount() const { return m_culledChunks; }
    int getTotalVertices() const { return m_totalVertices; }
    int getTotalIndices() const { return m_totalIndices; }

    // Check if initialized
    bool isInitialized() const { return m_initialized; }

private:
    IDevice* m_device = nullptr;

    // Per-chunk mesh data
    struct ChunkMeshDX12 {
        UniquePtr<IBuffer> vertexBuffer;
        UniquePtr<IBuffer> indexBuffer;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        glm::vec3 boundsMin;
        glm::vec3 boundsMax;
        bool valid = false;
    };
    std::vector<ChunkMeshDX12> m_chunkMeshes;

    // Constant buffer for per-chunk rendering
    UniquePtr<IBuffer> m_constantBuffer;

    // Upload command list for static buffer uploads
    UniquePtr<ICommandList> m_uploadCommandList;
    UniquePtr<IFence> m_uploadFence;
    u64 m_uploadFenceValue = 0;

    // Statistics
    int m_renderedChunks = 0;
    int m_culledChunks = 0;
    int m_totalVertices = 0;
    int m_totalIndices = 0;

    bool m_initialized = false;

    // Internal helpers
    bool createChunkMeshes();
    bool createConstantBuffer();
    void updateConstants(
        const glm::mat4& viewProj,
        const glm::mat4& world,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        const glm::vec3& lightColor,
        float time
    );

    // Generate mesh data for a single chunk
    bool generateChunkMesh(
        int chunkX, int chunkZ,
        std::vector<TerrainVertexDX12>& vertices,
        std::vector<uint32_t>& indices
    );

    // Helper to get biome color based on height
    glm::vec3 getBiomeColor(float height, float slope) const;

    // Helper to generate height at a world position using procedural noise
    float generateHeight(float worldX, float worldZ) const;
};
