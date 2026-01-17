#include "CreatureMeshCache.h"
#include "../procedural/CreatureBuilder.h"
#include "../procedural/MarchingCubes.h"
#include "../procedural/MetaballSystem.h"
#include <iostream>
#include <algorithm>

CreatureMeshCache::CreatureMeshCache() {
    std::cout << "CreatureMeshCache initialized" << std::endl;
}

CreatureMeshCache::~CreatureMeshCache() {
    cache.clear();
    std::cout << "CreatureMeshCache destroyed, cleaned up " << cache.size() << " meshes" << std::endl;
}

int CreatureMeshCache::categorizeSize(float size) {
    // 0.5-2.0 range -> 0-3 categories
    if (size < 0.8f) return 0;      // Tiny
    if (size < 1.2f) return 1;      // Small
    if (size < 1.6f) return 2;      // Medium
    return 3;                        // Large
}

int CreatureMeshCache::categorizeSpeed(float speed) {
    // 5-20 range -> 0-2 categories
    if (speed < 10.0f) return 0;    // Slow
    if (speed < 15.0f) return 1;    // Medium
    return 2;                        // Fast
}

int CreatureMeshCache::categorizeDetail(float value, float minValue, float maxValue, int buckets) {
    if (buckets <= 1) return 0;
    float t = (value - minValue) / (maxValue - minValue);
    t = std::clamp(t, 0.0f, 0.999f);
    return static_cast<int>(t * static_cast<float>(buckets));
}

MeshKey CreatureMeshCache::getMeshKey(const Genome& genome, CreatureType type) {
    BodyPlan bodyPlan = CreatureBuilder::determineBodyPlan(genome, type);
    HeadShape headShape = CreatureBuilder::determineHeadShape(genome, type);
    TailType tailType = CreatureBuilder::determineTailType(genome, type);
    int efficiencyBucket = categorizeDetail(genome.efficiency, 0.5f, 1.5f, 4);
    int visionBucket = categorizeDetail(genome.visionRange, 10.0f, 50.0f, 4);
    int motionBucket = categorizeDetail(genome.motionDetection, 0.3f, 0.8f, 4);
    int detailCategory = (efficiencyBucket << 4) | (visionBucket << 2) | motionBucket;

    return MeshKey(
        type,
        categorizeSize(genome.size),
        categorizeSpeed(genome.speed),
        static_cast<int>(bodyPlan),
        static_cast<int>(headShape),
        static_cast<int>(tailType),
        detailCategory
    );
}

MeshData* CreatureMeshCache::getMesh(const Genome& genome, CreatureType type) {
    MeshKey key = getMeshKey(genome, type);

    // Check cache
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second.get();
    }

    // Generate new mesh
    std::cout << "Generating new creature mesh: type=" << (int)type
              << " size=" << key.sizeCategory
              << " speed=" << key.speedCategory
              << " body=" << key.bodyPlan
              << " head=" << key.headShape
              << " tail=" << key.tailType
              << " detail=" << key.detailCategory
              << std::endl;

    return generateMesh(key, genome, type);
}

MeshData* CreatureMeshCache::generateMesh(const MeshKey& key, const Genome& genome, CreatureType type) {
    // Create metaball system
    MetaballSystem metaballs;
    CreatureBuilder::buildCreatureMetaballs(metaballs, genome, type);

    // Set appropriate resolution based on size
    int resolution = 16; // Base resolution
    if (key.sizeCategory >= 2) {
        resolution = 20; // Higher resolution for larger creatures
    }

    // Generate mesh using marching cubes
    // CRITICAL: isovalue must be lower than typical metaball strengths (0.4-1.0)
    // Using 0.5f extracts surfaces at ~50% potential, capturing most geometry
    MeshData meshData = MarchingCubes::generateMesh(metaballs, resolution, 0.5f);

    // Get type name for logging
    const char* typeName = "Unknown";
    switch (type) {
        case CreatureType::HERBIVORE: typeName = "Herbivore"; break;
        case CreatureType::CARNIVORE: typeName = "Carnivore"; break;
        case CreatureType::AQUATIC: typeName = "Aquatic"; break;
        case CreatureType::FLYING: typeName = "Flying"; break;
        default: typeName = "Other"; break;
    }

    // Validate mesh data and use fallback if generation failed
    bool useFallback = false;
    if (meshData.vertices.empty()) {
        std::cerr << "[MESH ERROR] " << typeName << " mesh has 0 vertices after marching cubes!" << std::endl;
        useFallback = true;
    }
    if (meshData.indices.empty()) {
        std::cerr << "[MESH ERROR] " << typeName << " mesh has 0 indices after marching cubes!" << std::endl;
        useFallback = true;
    } else if (meshData.indices.size() % 3 != 0) {
        std::cerr << "[MESH ERROR] " << typeName << " mesh indices count (" << meshData.indices.size()
                  << ") is not divisible by 3!" << std::endl;
        useFallback = true;
    }

    // USE FALLBACK SPHERE if marching cubes failed
    if (useFallback) {
        std::cerr << "[MESH FALLBACK] Using sphere mesh for " << typeName << " - creatures WILL be visible!" << std::endl;
        // Scale sphere based on genome size category
        float sphereRadius = 0.8f + (key.sizeCategory * 0.3f);  // 0.8, 1.1, 1.4, 1.7 for size categories 0-3
        meshData = MarchingCubes::generateFallbackSphere(sphereRadius, 16, 12);
    }

    // Debug output for mesh generation
    std::cout << "[MESH] " << typeName << " mesh: " << meshData.vertices.size()
              << " vertices, " << meshData.indices.size() << " indices"
              << (useFallback ? " [FALLBACK SPHERE]" : "") << std::endl;

    // Upload to GPU
    meshData.upload();

    // Store in cache
    auto uniqueMesh = std::make_unique<MeshData>(std::move(meshData));
    MeshData* meshPtr = uniqueMesh.get();
    cache[key] = std::move(uniqueMesh);

    std::cout << "[MESH] Cached " << typeName << " archetype (size=" << key.sizeCategory
              << ", speed=" << key.speedCategory << ", body=" << key.bodyPlan
              << ", head=" << key.headShape << ", tail=" << key.tailType
              << ", detail=" << key.detailCategory << ")" << std::endl;

    return meshPtr;
}

void CreatureMeshCache::preloadArchetypes() {
    std::cout << "Preloading creature archetypes..." << std::endl;

    // Create representative genomes for each category
    int meshesGenerated = 0;

    // Preload land creatures (herbivores and carnivores)
    for (int typeIdx = 0; typeIdx < 2; typeIdx++) {
        CreatureType type = (typeIdx == 0) ? CreatureType::HERBIVORE : CreatureType::CARNIVORE;

        for (int sizeIdx = 0; sizeIdx < 4; sizeIdx++) {
            for (int speedIdx = 0; speedIdx < 3; speedIdx++) {
                // Create a representative genome
                Genome genome;
                genome.randomize();

                // Set genome to match category
                switch (sizeIdx) {
                    case 0: genome.size = 0.6f; break;
                    case 1: genome.size = 1.0f; break;
                    case 2: genome.size = 1.4f; break;
                    case 3: genome.size = 1.8f; break;
                }

                switch (speedIdx) {
                    case 0: genome.speed = 7.0f; break;
                    case 1: genome.speed = 12.5f; break;
                    case 2: genome.speed = 18.0f; break;
                }

                // Generate and cache mesh
                getMesh(genome, type);
                meshesGenerated++;
            }
        }
    }

    // Preload flying creature archetypes
    std::cout << "Preloading flying creature archetypes..." << std::endl;
    for (int sizeIdx = 0; sizeIdx < 4; sizeIdx++) {
        for (int speedIdx = 0; speedIdx < 3; speedIdx++) {
            Genome genome;
            genome.randomizeFlying();

            // Flying creatures are smaller and faster
            switch (sizeIdx) {
                case 0: genome.size = 0.4f; break;
                case 1: genome.size = 0.55f; break;
                case 2: genome.size = 0.7f; break;
                case 3: genome.size = 0.8f; break;
            }

            switch (speedIdx) {
                case 0: genome.speed = 16.0f; break;
                case 1: genome.speed = 20.0f; break;
                case 2: genome.speed = 24.0f; break;
            }

            getMesh(genome, CreatureType::FLYING);
            meshesGenerated++;
        }
    }

    std::cout << "Preloaded " << meshesGenerated << " creature archetypes" << std::endl;
    printStatistics();
}

void CreatureMeshCache::printStatistics() const {
    std::cout << "\n=== Creature Mesh Cache Statistics ===" << std::endl;
    std::cout << "Total cached meshes: " << cache.size() << std::endl;

    size_t totalVertices = 0;
    size_t totalIndices = 0;

    for (const auto& pair : cache) {
        totalVertices += pair.second->vertices.size();
        totalIndices += pair.second->indices.size();
    }

    std::cout << "Total vertices: " << totalVertices << std::endl;
    std::cout << "Total indices: " << totalIndices << std::endl;
    size_t averageVertices = cache.empty() ? 0u : totalVertices / cache.size();
    std::cout << "Average vertices per mesh: " << averageVertices << std::endl;
    std::cout << "========================================\n" << std::endl;
}
