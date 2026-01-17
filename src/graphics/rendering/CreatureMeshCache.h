#pragma once

#include "../mesh/MeshData.h"
#include "../../entities/Genome.h"
#include "../../entities/CreatureType.h"
#include <unordered_map>
#include <memory>

// Key for mesh cache lookup
struct MeshKey {
    CreatureType type;
    int sizeCategory;    // 0-3: tiny, small, medium, large
    int speedCategory;   // 0-2: slow, medium, fast
    int bodyPlan;        // 0-4: quadruped, biped, hexapod, serpentine, avian
    int headShape;       // 0-4: round, elongated, flat, horned, crested
    int tailType;        // 0-5: none, short, long, bushy, spiked, finned
    int detailCategory;  // 0-63: extra morphology buckets

    MeshKey()
        : type(CreatureType::HERBIVORE),
          sizeCategory(0),
          speedCategory(0),
          bodyPlan(0),
          headShape(0),
          tailType(0),
          detailCategory(0) {}

    MeshKey(CreatureType t, int size, int speed, int plan, int head, int tail, int detail)
        : type(t),
          sizeCategory(size),
          speedCategory(speed),
          bodyPlan(plan),
          headShape(head),
          tailType(tail),
          detailCategory(detail) {}

    bool operator==(const MeshKey& other) const {
        return type == other.type &&
               sizeCategory == other.sizeCategory &&
               speedCategory == other.speedCategory &&
               bodyPlan == other.bodyPlan &&
               headShape == other.headShape &&
               tailType == other.tailType &&
               detailCategory == other.detailCategory;
    }
};

// Hash function for MeshKey
namespace std {
    template<>
    struct hash<MeshKey> {
        size_t operator()(const MeshKey& key) const {
            return ((size_t)key.type << 30) ^
                   ((size_t)key.sizeCategory << 26) ^
                   ((size_t)key.speedCategory << 23) ^
                   ((size_t)key.bodyPlan << 19) ^
                   ((size_t)key.headShape << 15) ^
                   ((size_t)key.tailType << 11) ^
                   (size_t)key.detailCategory;
        }
    };
}

class CreatureMeshCache {
public:
    CreatureMeshCache();
    ~CreatureMeshCache();

    // Get mesh key from genome
    static MeshKey getMeshKey(const Genome& genome, CreatureType type);

    // Get or generate mesh for a genome
    MeshData* getMesh(const Genome& genome, CreatureType type);

    // Preload common archetypes at startup
    void preloadArchetypes();

    // Get cache statistics
    int getCachedMeshCount() const { return static_cast<int>(cache.size()); }
    void printStatistics() const;

private:
    std::unordered_map<MeshKey, std::unique_ptr<MeshData>> cache;

    // Generate mesh for specific key
    MeshData* generateMesh(const MeshKey& key, const Genome& genome, CreatureType type);

    // Categorize genome values
    static int categorizeSize(float size);
    static int categorizeSpeed(float speed);
    static int categorizeDetail(float value, float minValue, float maxValue, int buckets);
};
