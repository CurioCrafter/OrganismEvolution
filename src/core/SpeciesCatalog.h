#pragma once

// SpeciesCatalog - No Man's Sky style discovery system
// Tracks discovered species, scan progress, and rarity tiers

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include "../entities/CreatureType.h"
#include "../environment/BiomeSystem.h"

// Forward declarations
class Genome;
class PlanetTheme;
namespace Forge {
    class BinaryWriter;
    class BinaryReader;
}

namespace discovery {

// ============================================================================
// Rarity Tiers - Based on genome complexity and morphology variance
// ============================================================================

enum class RarityTier : uint8_t {
    COMMON = 0,       // Basic traits, high population
    UNCOMMON = 1,     // Some specialized traits
    RARE = 2,         // Multiple specialized traits
    EPIC = 3,         // Unusual combinations
    LEGENDARY = 4,    // Extremely rare morphology/behavior
    MYTHICAL = 5      // One-of-a-kind or extreme mutations
};

// Get rarity tier name
const char* rarityToString(RarityTier rarity);

// Get rarity tier color (for UI)
glm::vec3 rarityToColor(RarityTier rarity);

// ============================================================================
// Discovery State - Track scan progress
// ============================================================================

enum class DiscoveryState : uint8_t {
    UNDISCOVERED = 0, // Not yet seen
    DETECTED = 1,     // Seen briefly, silhouette only
    SCANNING = 2,     // Currently being scanned
    PARTIAL = 3,      // Some details unlocked
    COMPLETE = 4      // Fully discovered
};

// ============================================================================
// Scan Progress - Per-creature or per-species scan tracking
// ============================================================================

struct ScanProgress {
    uint32_t targetSpeciesId = 0;
    int targetCreatureId = -1;      // Current creature being scanned (-1 = none)

    float observationTime = 0.0f;   // Total time observing this species
    float proximityBonus = 0.0f;    // Bonus from being close
    float scanProgress = 0.0f;      // 0-1 progress toward next discovery level

    DiscoveryState state = DiscoveryState::UNDISCOVERED;

    // Thresholds (in seconds of observation)
    static constexpr float DETECTED_THRESHOLD = 0.5f;   // Brief glimpse
    static constexpr float PARTIAL_THRESHOLD = 3.0f;    // Some details
    static constexpr float COMPLETE_THRESHOLD = 8.0f;   // Full discovery

    // Proximity multipliers
    static constexpr float CLOSE_RANGE = 10.0f;         // High bonus range
    static constexpr float MEDIUM_RANGE = 30.0f;        // Medium bonus range
    static constexpr float FAR_RANGE = 60.0f;           // Minimal bonus range

    // Calculate proximity multiplier (1.0 - 3.0x)
    static float getProximityMultiplier(float distance);

    // Reset for new target
    void reset();

    // Check if scan is complete
    bool isComplete() const { return state == DiscoveryState::COMPLETE; }
};

// ============================================================================
// Species Discovery Entry - Catalog record for a discovered species
// ============================================================================

struct SpeciesDiscoveryEntry {
    // Identity
    uint32_t speciesId = 0;
    std::string commonName;
    std::string scientificName;

    // Discovery metadata
    uint64_t firstSeenTimestamp = 0;    // Unix timestamp when first seen
    uint64_t lastSeenTimestamp = 0;     // Unix timestamp of last sighting
    uint64_t discoveryTimestamp = 0;    // Unix timestamp of full discovery
    float firstSeenSimTime = 0.0f;      // Simulation time when first seen

    // Location data
    BiomeType discoveryBiome = BiomeType::GRASSLAND;
    glm::vec3 discoveryLocation{0.0f};
    std::vector<BiomeType> habitatBiomes;  // All biomes where seen

    // Classification
    CreatureType creatureType = CreatureType::GRAZER;
    RarityTier rarity = RarityTier::COMMON;
    DiscoveryState discoveryState = DiscoveryState::UNDISCOVERED;

    // Statistics
    uint32_t sampleCount = 0;           // Number of individuals observed
    uint32_t generationsObserved = 0;   // Highest generation seen
    float averageSize = 1.0f;
    float averageSpeed = 10.0f;
    float averageLifespan = 0.0f;

    // Traits summary (unlocked progressively)
    bool traitsUnlocked[5] = {false, false, false, false, false};
    // 0: Basic (type, color)
    // 1: Physical (size, speed)
    // 2: Behavioral (diet, movement)
    // 3: Environmental (habitat, rarity)
    // 4: Advanced (neural complexity, special abilities)

    // Visual identification
    glm::vec3 primaryColor{0.5f};
    glm::vec3 secondaryColor{0.5f};
    uint32_t textureHash = 0;           // For visual identification

    // Notes (user can edit)
    std::string userNotes;

    // Scan tracking
    ScanProgress scanProgress;

    // Serialization
    void write(Forge::BinaryWriter& writer) const;
    void read(Forge::BinaryReader& reader, uint32_t version);

    // Helper methods
    std::string getDisplayName() const;
    std::string getRarityString() const { return rarityToString(rarity); }
    glm::vec3 getRarityColor() const { return rarityToColor(rarity); }
    float getDiscoveryProgress() const;
    bool isFullyDiscovered() const { return discoveryState == DiscoveryState::COMPLETE; }
    int getUnlockedTraitCount() const;
};

// ============================================================================
// Discovery Statistics - Session and global stats
// ============================================================================

struct DiscoveryStatistics {
    // Session stats
    uint32_t speciesDiscovered = 0;
    uint32_t speciesPartiallyScanned = 0;
    uint32_t totalSightings = 0;
    float totalScanTime = 0.0f;

    // Rarity breakdown
    uint32_t rarityCount[6] = {0, 0, 0, 0, 0, 0};

    // Biome breakdown
    std::unordered_map<BiomeType, uint32_t> biomeDiscoveries;

    // Type breakdown
    std::unordered_map<CreatureType, uint32_t> typeDiscoveries;

    // Achievements/milestones
    bool firstDiscovery = false;
    bool tenDiscoveries = false;
    bool fiftyDiscoveries = false;
    bool hundredDiscoveries = false;
    bool allRaritiesFound = false;
    bool allBiomesExplored = false;

    // Get completion percentage
    float getCompletionPercent(uint32_t totalSpeciesInWorld) const;

    // Serialization
    void write(Forge::BinaryWriter& writer) const;
    void read(Forge::BinaryReader& reader, uint32_t version);
};

// ============================================================================
// Discovery Event - For callbacks/notifications
// ============================================================================

struct DiscoveryEvent {
    enum class Type {
        SPECIES_DETECTED,       // First sighting
        SPECIES_PARTIAL_SCAN,   // Partial info unlocked
        SPECIES_DISCOVERED,     // Full discovery
        TRAIT_UNLOCKED,         // New trait tier unlocked
        RARITY_FOUND,           // Found rare+ species
        MILESTONE_REACHED       // Achievement unlocked
    };

    Type type;
    uint32_t speciesId = 0;
    RarityTier rarity = RarityTier::COMMON;
    std::string speciesName;
    std::string message;
    uint64_t timestamp = 0;
};

// ============================================================================
// Species Catalog - Main discovery system
// ============================================================================

class SpeciesCatalog {
public:
    SpeciesCatalog();
    ~SpeciesCatalog() = default;

    // ========================================
    // Core Operations
    // ========================================

    // Record a species sighting (call when creature is visible to camera)
    // Returns true if this is a new sighting
    bool recordSighting(uint32_t speciesId,
                       const Genome& genome,
                       CreatureType type,
                       BiomeType biome,
                       const glm::vec3& position,
                       int creatureId,
                       int generation,
                       float simulationTime);

    // Update scan progress (call while observing a creature)
    // Returns true if discovery state changed
    bool updateScan(uint32_t speciesId,
                   float deltaTime,
                   float distance,
                   bool isTargeted = false);

    // Complete discovery instantly (cheat/debug)
    void forceDiscovery(uint32_t speciesId);

    // ========================================
    // Query Operations
    // ========================================

    // Get discovery entry (returns nullptr if not in catalog)
    const SpeciesDiscoveryEntry* getEntry(uint32_t speciesId) const;
    SpeciesDiscoveryEntry* getMutableEntry(uint32_t speciesId);

    // Check discovery state
    bool isDiscovered(uint32_t speciesId) const;
    bool isPartiallyDiscovered(uint32_t speciesId) const;
    DiscoveryState getDiscoveryState(uint32_t speciesId) const;

    // Get all entries
    const std::unordered_map<uint32_t, SpeciesDiscoveryEntry>& getAllEntries() const { return m_entries; }

    // Get entries by filter
    std::vector<const SpeciesDiscoveryEntry*> getEntriesByBiome(BiomeType biome) const;
    std::vector<const SpeciesDiscoveryEntry*> getEntriesByRarity(RarityTier rarity) const;
    std::vector<const SpeciesDiscoveryEntry*> getEntriesByType(CreatureType type) const;
    std::vector<const SpeciesDiscoveryEntry*> getDiscoveredEntries() const;
    std::vector<const SpeciesDiscoveryEntry*> getRecentDiscoveries(uint32_t count) const;

    // Statistics
    const DiscoveryStatistics& getStatistics() const { return m_statistics; }
    uint32_t getDiscoveredCount() const { return m_statistics.speciesDiscovered; }
    uint32_t getTotalEntries() const { return static_cast<uint32_t>(m_entries.size()); }

    // ========================================
    // Active Scan Management
    // ========================================

    // Set the currently targeted species/creature for scanning
    void setActiveScanTarget(uint32_t speciesId, int creatureId = -1);
    void clearActiveScanTarget();

    // Get active scan info
    uint32_t getActiveScanSpeciesId() const { return m_activeScanSpeciesId; }
    int getActiveScanCreatureId() const { return m_activeScanCreatureId; }
    const ScanProgress* getActiveScanProgress() const;
    bool hasActiveScan() const { return m_activeScanSpeciesId != 0; }

    // ========================================
    // Rarity Calculation
    // ========================================

    // Calculate rarity from genome traits
    static RarityTier calculateRarity(const Genome& genome, CreatureType type);

    // Get rarity score (0-100) for detailed UI
    static float calculateRarityScore(const Genome& genome, CreatureType type);

    // ========================================
    // Event System
    // ========================================

    using EventCallback = std::function<void(const DiscoveryEvent&)>;
    void setEventCallback(EventCallback callback) { m_eventCallback = callback; }

    // Get recent events for UI
    const std::vector<DiscoveryEvent>& getRecentEvents() const { return m_recentEvents; }
    void clearRecentEvents() { m_recentEvents.clear(); }

    // ========================================
    // Planet Theme Integration
    // ========================================

    // Set planet theme for themed naming (optional)
    void setPlanetTheme(const PlanetTheme* theme) { m_planetTheme = theme; }

    // ========================================
    // Persistence
    // ========================================

    // Save/load catalog
    void save(Forge::BinaryWriter& writer) const;
    bool load(Forge::BinaryReader& reader);

    // Export to JSON (for debug/sharing)
    std::string exportToJson() const;

    // Clear all data
    void clear();

    // ========================================
    // User Customization
    // ========================================

    // Rename a species (user-defined name)
    void renameSpecies(uint32_t speciesId, const std::string& newName);

    // Add user notes
    void setUserNotes(uint32_t speciesId, const std::string& notes);

private:
    // Internal helpers
    void createEntry(uint32_t speciesId, const Genome& genome, CreatureType type);
    void updateEntryStats(SpeciesDiscoveryEntry& entry, const Genome& genome, int generation);
    void unlockTraits(SpeciesDiscoveryEntry& entry, DiscoveryState newState);
    void updateStatistics(const SpeciesDiscoveryEntry& entry, DiscoveryState previousState);
    void emitEvent(DiscoveryEvent::Type type, const SpeciesDiscoveryEntry& entry, const std::string& message = "");
    void checkMilestones();

    // Generate themed name based on planet
    std::string generateThemedName(const Genome& genome, CreatureType type, BiomeType biome) const;

    // Data
    std::unordered_map<uint32_t, SpeciesDiscoveryEntry> m_entries;
    DiscoveryStatistics m_statistics;

    // Active scan tracking
    uint32_t m_activeScanSpeciesId = 0;
    int m_activeScanCreatureId = -1;

    // Event system
    EventCallback m_eventCallback;
    std::vector<DiscoveryEvent> m_recentEvents;
    static constexpr size_t MAX_RECENT_EVENTS = 50;

    // Planet theme reference (optional)
    const PlanetTheme* m_planetTheme = nullptr;

    // Save format version
    static constexpr uint32_t CATALOG_VERSION = 1;
    static constexpr uint32_t CATALOG_MAGIC = 0x43415443; // "CATC"
};

// ============================================================================
// Global Catalog Accessor
// ============================================================================

SpeciesCatalog& getCatalog();

} // namespace discovery
