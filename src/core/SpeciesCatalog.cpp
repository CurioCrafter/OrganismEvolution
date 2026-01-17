#include "SpeciesCatalog.h"
#include "Serializer.h"
#include "../entities/Genome.h"
#include "../entities/SpeciesNameGenerator.h"
#include "../entities/SpeciesNaming.h"
#include "../environment/PlanetTheme.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <chrono>
#include <iostream>

namespace discovery {

// ============================================================================
// Global Instance
// ============================================================================

static std::unique_ptr<SpeciesCatalog> g_catalog;

SpeciesCatalog& getCatalog() {
    if (!g_catalog) {
        g_catalog = std::make_unique<SpeciesCatalog>();
    }
    return *g_catalog;
}

// ============================================================================
// Rarity Utilities
// ============================================================================

const char* rarityToString(RarityTier rarity) {
    switch (rarity) {
        case RarityTier::COMMON: return "Common";
        case RarityTier::UNCOMMON: return "Uncommon";
        case RarityTier::RARE: return "Rare";
        case RarityTier::EPIC: return "Epic";
        case RarityTier::LEGENDARY: return "Legendary";
        case RarityTier::MYTHICAL: return "Mythical";
        default: return "Unknown";
    }
}

glm::vec3 rarityToColor(RarityTier rarity) {
    switch (rarity) {
        case RarityTier::COMMON:     return glm::vec3(0.6f, 0.6f, 0.6f);   // Gray
        case RarityTier::UNCOMMON:   return glm::vec3(0.2f, 0.8f, 0.2f);   // Green
        case RarityTier::RARE:       return glm::vec3(0.2f, 0.5f, 1.0f);   // Blue
        case RarityTier::EPIC:       return glm::vec3(0.8f, 0.3f, 0.9f);   // Purple
        case RarityTier::LEGENDARY:  return glm::vec3(1.0f, 0.6f, 0.1f);   // Orange
        case RarityTier::MYTHICAL:   return glm::vec3(1.0f, 0.9f, 0.3f);   // Gold
        default:                     return glm::vec3(1.0f, 1.0f, 1.0f);   // White
    }
}

// ============================================================================
// ScanProgress
// ============================================================================

float ScanProgress::getProximityMultiplier(float distance) {
    if (distance <= CLOSE_RANGE) {
        return 3.0f;  // 3x speed at close range
    } else if (distance <= MEDIUM_RANGE) {
        // Interpolate between 3x and 1.5x
        float t = (distance - CLOSE_RANGE) / (MEDIUM_RANGE - CLOSE_RANGE);
        return 3.0f - 1.5f * t;
    } else if (distance <= FAR_RANGE) {
        // Interpolate between 1.5x and 1x
        float t = (distance - MEDIUM_RANGE) / (FAR_RANGE - MEDIUM_RANGE);
        return 1.5f - 0.5f * t;
    }
    return 1.0f;  // Base speed at long range
}

void ScanProgress::reset() {
    targetCreatureId = -1;
    observationTime = 0.0f;
    proximityBonus = 0.0f;
    scanProgress = 0.0f;
    state = DiscoveryState::UNDISCOVERED;
}

// ============================================================================
// SpeciesDiscoveryEntry
// ============================================================================

void SpeciesDiscoveryEntry::write(Forge::BinaryWriter& writer) const {
    writer.write(speciesId);
    writer.writeString(commonName);
    writer.writeString(scientificName);

    writer.write(firstSeenTimestamp);
    writer.write(lastSeenTimestamp);
    writer.write(discoveryTimestamp);
    writer.write(firstSeenSimTime);

    writer.write(static_cast<uint8_t>(discoveryBiome));
    writer.write(discoveryLocation.x);
    writer.write(discoveryLocation.y);
    writer.write(discoveryLocation.z);

    writer.write(static_cast<uint32_t>(habitatBiomes.size()));
    for (auto biome : habitatBiomes) {
        writer.write(static_cast<uint8_t>(biome));
    }

    writer.write(static_cast<uint8_t>(creatureType));
    writer.write(static_cast<uint8_t>(rarity));
    writer.write(static_cast<uint8_t>(discoveryState));

    writer.write(sampleCount);
    writer.write(generationsObserved);
    writer.write(averageSize);
    writer.write(averageSpeed);
    writer.write(averageLifespan);

    for (int i = 0; i < 5; ++i) {
        writer.writeBool(traitsUnlocked[i]);
    }

    writer.write(primaryColor.r);
    writer.write(primaryColor.g);
    writer.write(primaryColor.b);
    writer.write(secondaryColor.r);
    writer.write(secondaryColor.g);
    writer.write(secondaryColor.b);
    writer.write(textureHash);

    writer.writeString(userNotes);

    // Scan progress
    writer.write(scanProgress.observationTime);
    writer.write(scanProgress.scanProgress);
}

void SpeciesDiscoveryEntry::read(Forge::BinaryReader& reader, uint32_t version) {
    speciesId = reader.read<uint32_t>();
    commonName = reader.readString(1024);
    scientificName = reader.readString(1024);

    firstSeenTimestamp = reader.read<uint64_t>();
    lastSeenTimestamp = reader.read<uint64_t>();
    discoveryTimestamp = reader.read<uint64_t>();
    firstSeenSimTime = reader.read<float>();

    discoveryBiome = static_cast<BiomeType>(reader.read<uint8_t>());
    discoveryLocation.x = reader.read<float>();
    discoveryLocation.y = reader.read<float>();
    discoveryLocation.z = reader.read<float>();

    uint32_t biomeCount = reader.read<uint32_t>();
    habitatBiomes.resize(biomeCount);
    for (uint32_t i = 0; i < biomeCount; ++i) {
        habitatBiomes[i] = static_cast<BiomeType>(reader.read<uint8_t>());
    }

    creatureType = static_cast<CreatureType>(reader.read<uint8_t>());
    rarity = static_cast<RarityTier>(reader.read<uint8_t>());
    discoveryState = static_cast<DiscoveryState>(reader.read<uint8_t>());

    sampleCount = reader.read<uint32_t>();
    generationsObserved = reader.read<uint32_t>();
    averageSize = reader.read<float>();
    averageSpeed = reader.read<float>();
    averageLifespan = reader.read<float>();

    for (int i = 0; i < 5; ++i) {
        traitsUnlocked[i] = reader.readBool();
    }

    primaryColor.r = reader.read<float>();
    primaryColor.g = reader.read<float>();
    primaryColor.b = reader.read<float>();
    secondaryColor.r = reader.read<float>();
    secondaryColor.g = reader.read<float>();
    secondaryColor.b = reader.read<float>();
    textureHash = reader.read<uint32_t>();

    userNotes = reader.readString(4096);

    scanProgress.observationTime = reader.read<float>();
    scanProgress.scanProgress = reader.read<float>();
    scanProgress.targetSpeciesId = speciesId;
    scanProgress.state = discoveryState;

    (void)version; // Reserved for future format changes
}

std::string SpeciesDiscoveryEntry::getDisplayName() const {
    if (!commonName.empty()) {
        return commonName;
    }
    return "Unknown Species #" + std::to_string(speciesId);
}

float SpeciesDiscoveryEntry::getDiscoveryProgress() const {
    switch (discoveryState) {
        case DiscoveryState::UNDISCOVERED: return 0.0f;
        case DiscoveryState::DETECTED: return 0.15f + scanProgress.scanProgress * 0.35f;
        case DiscoveryState::SCANNING: return 0.15f + scanProgress.scanProgress * 0.35f;
        case DiscoveryState::PARTIAL: return 0.50f + scanProgress.scanProgress * 0.50f;
        case DiscoveryState::COMPLETE: return 1.0f;
        default: return 0.0f;
    }
}

int SpeciesDiscoveryEntry::getUnlockedTraitCount() const {
    int count = 0;
    for (int i = 0; i < 5; ++i) {
        if (traitsUnlocked[i]) ++count;
    }
    return count;
}

// ============================================================================
// DiscoveryStatistics
// ============================================================================

float DiscoveryStatistics::getCompletionPercent(uint32_t totalSpeciesInWorld) const {
    if (totalSpeciesInWorld == 0) return 100.0f;
    return (static_cast<float>(speciesDiscovered) / static_cast<float>(totalSpeciesInWorld)) * 100.0f;
}

void DiscoveryStatistics::write(Forge::BinaryWriter& writer) const {
    writer.write(speciesDiscovered);
    writer.write(speciesPartiallyScanned);
    writer.write(totalSightings);
    writer.write(totalScanTime);

    for (int i = 0; i < 6; ++i) {
        writer.write(rarityCount[i]);
    }

    writer.write(static_cast<uint32_t>(biomeDiscoveries.size()));
    for (const auto& [biome, count] : biomeDiscoveries) {
        writer.write(static_cast<uint8_t>(biome));
        writer.write(count);
    }

    writer.write(static_cast<uint32_t>(typeDiscoveries.size()));
    for (const auto& [type, count] : typeDiscoveries) {
        writer.write(static_cast<uint8_t>(type));
        writer.write(count);
    }

    writer.writeBool(firstDiscovery);
    writer.writeBool(tenDiscoveries);
    writer.writeBool(fiftyDiscoveries);
    writer.writeBool(hundredDiscoveries);
    writer.writeBool(allRaritiesFound);
    writer.writeBool(allBiomesExplored);
}

void DiscoveryStatistics::read(Forge::BinaryReader& reader, uint32_t version) {
    speciesDiscovered = reader.read<uint32_t>();
    speciesPartiallyScanned = reader.read<uint32_t>();
    totalSightings = reader.read<uint32_t>();
    totalScanTime = reader.read<float>();

    for (int i = 0; i < 6; ++i) {
        rarityCount[i] = reader.read<uint32_t>();
    }

    uint32_t biomeCount = reader.read<uint32_t>();
    biomeDiscoveries.clear();
    for (uint32_t i = 0; i < biomeCount; ++i) {
        auto biome = static_cast<BiomeType>(reader.read<uint8_t>());
        auto count = reader.read<uint32_t>();
        biomeDiscoveries[biome] = count;
    }

    uint32_t typeCount = reader.read<uint32_t>();
    typeDiscoveries.clear();
    for (uint32_t i = 0; i < typeCount; ++i) {
        auto type = static_cast<CreatureType>(reader.read<uint8_t>());
        auto count = reader.read<uint32_t>();
        typeDiscoveries[type] = count;
    }

    firstDiscovery = reader.readBool();
    tenDiscoveries = reader.readBool();
    fiftyDiscoveries = reader.readBool();
    hundredDiscoveries = reader.readBool();
    allRaritiesFound = reader.readBool();
    allBiomesExplored = reader.readBool();

    (void)version; // Reserved for future format changes
}

// ============================================================================
// SpeciesCatalog
// ============================================================================

SpeciesCatalog::SpeciesCatalog() = default;

bool SpeciesCatalog::recordSighting(uint32_t speciesId,
                                    const Genome& genome,
                                    CreatureType type,
                                    BiomeType biome,
                                    const glm::vec3& position,
                                    int creatureId,
                                    int generation,
                                    float simulationTime) {
    bool isNewSighting = false;
    m_statistics.totalSightings++;

    auto it = m_entries.find(speciesId);
    if (it == m_entries.end()) {
        // First sighting of this species
        createEntry(speciesId, genome, type);
        it = m_entries.find(speciesId);
        isNewSighting = true;

        // Set discovery location and biome
        it->second.discoveryBiome = biome;
        it->second.discoveryLocation = position;
        it->second.habitatBiomes.push_back(biome);
        it->second.firstSeenSimTime = simulationTime;

        // Emit detection event
        emitEvent(DiscoveryEvent::Type::SPECIES_DETECTED, it->second, "New species detected!");
    }

    auto& entry = it->second;

    // Update last seen
    entry.lastSeenTimestamp = static_cast<uint64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    // Update sample count
    entry.sampleCount++;

    // Update generation tracking
    if (static_cast<uint32_t>(generation) > entry.generationsObserved) {
        entry.generationsObserved = generation;
    }

    // Update habitat biomes if new
    auto biomeIt = std::find(entry.habitatBiomes.begin(), entry.habitatBiomes.end(), biome);
    if (biomeIt == entry.habitatBiomes.end()) {
        entry.habitatBiomes.push_back(biome);
    }

    // Update stats from genome
    updateEntryStats(entry, genome, generation);

    return isNewSighting;
}

bool SpeciesCatalog::updateScan(uint32_t speciesId,
                                float deltaTime,
                                float distance,
                                bool isTargeted) {
    auto it = m_entries.find(speciesId);
    if (it == m_entries.end()) {
        return false;  // Species not yet sighted
    }

    auto& entry = it->second;
    if (entry.discoveryState == DiscoveryState::COMPLETE) {
        return false;  // Already fully discovered
    }

    // Calculate effective scan time
    float multiplier = ScanProgress::getProximityMultiplier(distance);
    if (isTargeted) {
        multiplier *= 1.5f;  // Bonus for actively targeting
    }

    float effectiveTime = deltaTime * multiplier;
    entry.scanProgress.observationTime += effectiveTime;
    m_statistics.totalScanTime += deltaTime;

    // Check for state transitions
    DiscoveryState previousState = entry.discoveryState;
    DiscoveryState newState = previousState;

    float totalTime = entry.scanProgress.observationTime;

    if (totalTime >= ScanProgress::COMPLETE_THRESHOLD) {
        newState = DiscoveryState::COMPLETE;
        entry.scanProgress.scanProgress = 1.0f;
    } else if (totalTime >= ScanProgress::PARTIAL_THRESHOLD) {
        newState = DiscoveryState::PARTIAL;
        entry.scanProgress.scanProgress =
            (totalTime - ScanProgress::PARTIAL_THRESHOLD) /
            (ScanProgress::COMPLETE_THRESHOLD - ScanProgress::PARTIAL_THRESHOLD);
    } else if (totalTime >= ScanProgress::DETECTED_THRESHOLD) {
        newState = entry.discoveryState == DiscoveryState::UNDISCOVERED
            ? DiscoveryState::DETECTED : entry.discoveryState;
        entry.scanProgress.scanProgress =
            (totalTime - ScanProgress::DETECTED_THRESHOLD) /
            (ScanProgress::PARTIAL_THRESHOLD - ScanProgress::DETECTED_THRESHOLD);
    }

    // Handle state change
    if (newState != previousState) {
        entry.discoveryState = newState;
        entry.scanProgress.state = newState;
        unlockTraits(entry, newState);
        updateStatistics(entry, previousState);

        // Emit appropriate event
        if (newState == DiscoveryState::COMPLETE) {
            entry.discoveryTimestamp = static_cast<uint64_t>(
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
            emitEvent(DiscoveryEvent::Type::SPECIES_DISCOVERED, entry,
                "Species fully discovered: " + entry.commonName);

            // Check for rarity event
            if (entry.rarity >= RarityTier::RARE) {
                emitEvent(DiscoveryEvent::Type::RARITY_FOUND, entry,
                    "Discovered " + std::string(rarityToString(entry.rarity)) + " species!");
            }
        } else if (newState == DiscoveryState::PARTIAL) {
            emitEvent(DiscoveryEvent::Type::SPECIES_PARTIAL_SCAN, entry,
                "Partial scan complete for: " + entry.commonName);
        }

        checkMilestones();
        return true;
    }

    return false;
}

void SpeciesCatalog::forceDiscovery(uint32_t speciesId) {
    auto it = m_entries.find(speciesId);
    if (it == m_entries.end()) {
        return;
    }

    auto& entry = it->second;
    DiscoveryState previousState = entry.discoveryState;

    entry.discoveryState = DiscoveryState::COMPLETE;
    entry.scanProgress.state = DiscoveryState::COMPLETE;
    entry.scanProgress.scanProgress = 1.0f;
    entry.scanProgress.observationTime = ScanProgress::COMPLETE_THRESHOLD;
    entry.discoveryTimestamp = static_cast<uint64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    unlockTraits(entry, DiscoveryState::COMPLETE);
    updateStatistics(entry, previousState);
    checkMilestones();
}

const SpeciesDiscoveryEntry* SpeciesCatalog::getEntry(uint32_t speciesId) const {
    auto it = m_entries.find(speciesId);
    return it != m_entries.end() ? &it->second : nullptr;
}

SpeciesDiscoveryEntry* SpeciesCatalog::getMutableEntry(uint32_t speciesId) {
    auto it = m_entries.find(speciesId);
    return it != m_entries.end() ? &it->second : nullptr;
}

bool SpeciesCatalog::isDiscovered(uint32_t speciesId) const {
    auto it = m_entries.find(speciesId);
    return it != m_entries.end() && it->second.discoveryState == DiscoveryState::COMPLETE;
}

bool SpeciesCatalog::isPartiallyDiscovered(uint32_t speciesId) const {
    auto it = m_entries.find(speciesId);
    return it != m_entries.end() && it->second.discoveryState >= DiscoveryState::PARTIAL;
}

DiscoveryState SpeciesCatalog::getDiscoveryState(uint32_t speciesId) const {
    auto it = m_entries.find(speciesId);
    return it != m_entries.end() ? it->second.discoveryState : DiscoveryState::UNDISCOVERED;
}

std::vector<const SpeciesDiscoveryEntry*> SpeciesCatalog::getEntriesByBiome(BiomeType biome) const {
    std::vector<const SpeciesDiscoveryEntry*> result;
    for (const auto& [id, entry] : m_entries) {
        auto it = std::find(entry.habitatBiomes.begin(), entry.habitatBiomes.end(), biome);
        if (it != entry.habitatBiomes.end()) {
            result.push_back(&entry);
        }
    }
    return result;
}

std::vector<const SpeciesDiscoveryEntry*> SpeciesCatalog::getEntriesByRarity(RarityTier rarity) const {
    std::vector<const SpeciesDiscoveryEntry*> result;
    for (const auto& [id, entry] : m_entries) {
        if (entry.rarity == rarity) {
            result.push_back(&entry);
        }
    }
    return result;
}

std::vector<const SpeciesDiscoveryEntry*> SpeciesCatalog::getEntriesByType(CreatureType type) const {
    std::vector<const SpeciesDiscoveryEntry*> result;
    for (const auto& [id, entry] : m_entries) {
        if (entry.creatureType == type) {
            result.push_back(&entry);
        }
    }
    return result;
}

std::vector<const SpeciesDiscoveryEntry*> SpeciesCatalog::getDiscoveredEntries() const {
    std::vector<const SpeciesDiscoveryEntry*> result;
    for (const auto& [id, entry] : m_entries) {
        if (entry.discoveryState == DiscoveryState::COMPLETE) {
            result.push_back(&entry);
        }
    }
    return result;
}

std::vector<const SpeciesDiscoveryEntry*> SpeciesCatalog::getRecentDiscoveries(uint32_t count) const {
    std::vector<const SpeciesDiscoveryEntry*> all;
    for (const auto& [id, entry] : m_entries) {
        if (entry.discoveryState >= DiscoveryState::DETECTED) {
            all.push_back(&entry);
        }
    }

    // Sort by last seen timestamp (most recent first)
    std::sort(all.begin(), all.end(),
        [](const SpeciesDiscoveryEntry* a, const SpeciesDiscoveryEntry* b) {
            return a->lastSeenTimestamp > b->lastSeenTimestamp;
        });

    if (all.size() > count) {
        all.resize(count);
    }
    return all;
}

void SpeciesCatalog::setActiveScanTarget(uint32_t speciesId, int creatureId) {
    m_activeScanSpeciesId = speciesId;
    m_activeScanCreatureId = creatureId;

    auto it = m_entries.find(speciesId);
    if (it != m_entries.end()) {
        it->second.scanProgress.targetCreatureId = creatureId;
    }
}

void SpeciesCatalog::clearActiveScanTarget() {
    m_activeScanSpeciesId = 0;
    m_activeScanCreatureId = -1;
}

const ScanProgress* SpeciesCatalog::getActiveScanProgress() const {
    if (m_activeScanSpeciesId == 0) {
        return nullptr;
    }

    auto it = m_entries.find(m_activeScanSpeciesId);
    return it != m_entries.end() ? &it->second.scanProgress : nullptr;
}

RarityTier SpeciesCatalog::calculateRarity(const Genome& genome, CreatureType type) {
    float score = calculateRarityScore(genome, type);

    if (score >= 85.0f) return RarityTier::MYTHICAL;
    if (score >= 70.0f) return RarityTier::LEGENDARY;
    if (score >= 55.0f) return RarityTier::EPIC;
    if (score >= 40.0f) return RarityTier::RARE;
    if (score >= 25.0f) return RarityTier::UNCOMMON;
    return RarityTier::COMMON;
}

float SpeciesCatalog::calculateRarityScore(const Genome& genome, CreatureType type) {
    float score = 0.0f;

    // Size extremes (+0-15 points)
    if (genome.size < 0.5f || genome.size > 1.8f) {
        score += 15.0f * (genome.size < 0.5f ? (0.5f - genome.size) / 0.3f : (genome.size - 1.5f) / 0.5f);
    }

    // Speed extremes (+0-15 points)
    if (genome.speed < 6.0f || genome.speed > 18.0f) {
        score += 15.0f;
    }

    // Special abilities (+0-20 points each)
    if (genome.hasBioluminescence) {
        score += 20.0f * genome.biolumIntensity;
    }
    if (genome.echolocationAbility > 0.5f) {
        score += 15.0f * genome.echolocationAbility;
    }
    if (genome.electricDischarge > 0.5f) {
        score += 20.0f * genome.electricDischarge;
    }
    if (genome.venomPotency > 0.5f) {
        score += 15.0f * genome.venomPotency;
    }

    // Color uniqueness (+0-10 points)
    float colorIntensity = std::max({genome.color.r, genome.color.g, genome.color.b});
    float colorSaturation = colorIntensity - std::min({genome.color.r, genome.color.g, genome.color.b});
    if (colorSaturation > 0.6f) {
        score += 10.0f * colorSaturation;
    }

    // Type-specific bonuses
    if (type == CreatureType::AQUATIC_APEX) {
        score += 10.0f;  // Apex predators are rarer
    }
    if (type == CreatureType::AERIAL_PREDATOR) {
        score += 8.0f;
    }

    // Neural complexity (+0-10 points)
    if (!genome.neuralWeights.empty()) {
        float complexity = 0.0f;
        for (float w : genome.neuralWeights) {
            complexity += std::abs(w);
        }
        complexity /= genome.neuralWeights.size();
        score += 10.0f * std::min(complexity, 1.0f);
    }

    // Camouflage (+0-8 points)
    if (genome.camouflageLevel > 0.6f) {
        score += 8.0f * genome.camouflageLevel;
    }

    // Flight traits for flying creatures
    if (isFlying(type)) {
        if (genome.hoveringAbility > 0.7f) {
            score += 12.0f;  // Hovering is rare
        }
        if (genome.glideRatio > 0.7f) {
            score += 8.0f;
        }
    }

    // Aquatic traits for water creatures
    if (isAquatic(type)) {
        if (genome.preferredDepth > 0.7f) {
            score += 10.0f;  // Deep-sea creatures are rarer
        }
    }

    return std::clamp(score, 0.0f, 100.0f);
}

void SpeciesCatalog::save(Forge::BinaryWriter& writer) const {
    writer.write(CATALOG_MAGIC);
    writer.write(CATALOG_VERSION);

    writer.write(static_cast<uint32_t>(m_entries.size()));
    for (const auto& [id, entry] : m_entries) {
        entry.write(writer);
    }

    m_statistics.write(writer);
}

bool SpeciesCatalog::load(Forge::BinaryReader& reader) {
    uint32_t magic = reader.read<uint32_t>();
    if (magic != CATALOG_MAGIC) {
        std::cerr << "ERROR: Invalid catalog file magic number" << std::endl;
        return false;
    }

    uint32_t version = reader.read<uint32_t>();
    if (version > CATALOG_VERSION) {
        std::cerr << "ERROR: Catalog file version " << version << " is newer than supported version " << CATALOG_VERSION << std::endl;
        return false;
    }

    clear();

    uint32_t entryCount = reader.read<uint32_t>();
    for (uint32_t i = 0; i < entryCount; ++i) {
        SpeciesDiscoveryEntry entry;
        entry.read(reader, version);
        m_entries[entry.speciesId] = std::move(entry);
    }

    m_statistics.read(reader, version);

    return true;
}

std::string SpeciesCatalog::exportToJson() const {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"catalogVersion\": " << CATALOG_VERSION << ",\n";
    ss << "  \"statistics\": {\n";
    ss << "    \"speciesDiscovered\": " << m_statistics.speciesDiscovered << ",\n";
    ss << "    \"totalSightings\": " << m_statistics.totalSightings << ",\n";
    ss << "    \"totalScanTime\": " << m_statistics.totalScanTime << "\n";
    ss << "  },\n";
    ss << "  \"entries\": [\n";

    bool first = true;
    for (const auto& [id, entry] : m_entries) {
        if (!first) ss << ",\n";
        first = false;

        ss << "    {\n";
        ss << "      \"speciesId\": " << entry.speciesId << ",\n";
        ss << "      \"commonName\": \"" << entry.commonName << "\",\n";
        ss << "      \"scientificName\": \"" << entry.scientificName << "\",\n";
        ss << "      \"rarity\": \"" << rarityToString(entry.rarity) << "\",\n";
        ss << "      \"discoveryState\": " << static_cast<int>(entry.discoveryState) << ",\n";
        ss << "      \"sampleCount\": " << entry.sampleCount << ",\n";
        ss << "      \"discoveryProgress\": " << entry.getDiscoveryProgress() << "\n";
        ss << "    }";
    }

    ss << "\n  ]\n";
    ss << "}\n";

    return ss.str();
}

void SpeciesCatalog::clear() {
    m_entries.clear();
    m_statistics = DiscoveryStatistics();
    m_activeScanSpeciesId = 0;
    m_activeScanCreatureId = -1;
    m_recentEvents.clear();
}

void SpeciesCatalog::renameSpecies(uint32_t speciesId, const std::string& newName) {
    auto it = m_entries.find(speciesId);
    if (it != m_entries.end()) {
        it->second.commonName = newName;
    }
}

void SpeciesCatalog::setUserNotes(uint32_t speciesId, const std::string& notes) {
    auto it = m_entries.find(speciesId);
    if (it != m_entries.end()) {
        it->second.userNotes = notes;
    }
}

void SpeciesCatalog::createEntry(uint32_t speciesId, const Genome& genome, CreatureType type) {
    SpeciesDiscoveryEntry entry;
    entry.speciesId = speciesId;
    entry.creatureType = type;

    // Generate name using the naming system
    auto& namingSystem = naming::getNamingSystem();
    naming::CreatureTraits traits;
    traits.primaryColor = genome.color;
    traits.size = genome.size;
    traits.speed = genome.speed;
    traits.isPredator = isPredator(type);
    traits.isNocturnal = genome.nocturnalFlight > 0.5f;
    traits.livesInWater = isAquatic(type);
    traits.canFly = isFlying(type);
    traits.hasWings = isFlying(type);
    traits.hasFins = isAquatic(type);

    const auto& speciesName = namingSystem.getOrCreateSpeciesName(speciesId, traits);
    entry.commonName = speciesName.commonName;
    entry.scientificName = speciesName.scientificName;

    // Calculate rarity
    entry.rarity = calculateRarity(genome, type);

    // Set colors
    entry.primaryColor = genome.color;

    // Calculate secondary color from pattern
    if (genome.patternType > 0) {
        float hueOffset = genome.patternSecondaryHue;
        entry.secondaryColor = glm::vec3(
            std::fmod(genome.color.r + hueOffset, 1.0f),
            genome.color.g * 0.8f,
            genome.color.b * 0.9f
        );
    } else {
        entry.secondaryColor = genome.color * 0.7f;
    }

    // Generate texture hash for identification
    entry.textureHash = static_cast<uint32_t>(
        speciesId * 31 +
        static_cast<uint32_t>(genome.size * 1000) +
        static_cast<uint32_t>(genome.color.r * 255) * 65536 +
        static_cast<uint32_t>(genome.color.g * 255) * 256 +
        static_cast<uint32_t>(genome.color.b * 255)
    );

    // Set initial discovery state
    entry.discoveryState = DiscoveryState::DETECTED;
    entry.scanProgress.targetSpeciesId = speciesId;
    entry.scanProgress.state = DiscoveryState::DETECTED;

    // Set timestamps
    auto now = std::chrono::system_clock::now();
    entry.firstSeenTimestamp = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(now));
    entry.lastSeenTimestamp = entry.firstSeenTimestamp;

    // Initial unlock (basic info)
    entry.traitsUnlocked[0] = true;

    m_entries[speciesId] = std::move(entry);
}

void SpeciesCatalog::updateEntryStats(SpeciesDiscoveryEntry& entry, const Genome& genome, int generation) {
    // Rolling average for size and speed
    float n = static_cast<float>(entry.sampleCount);
    entry.averageSize = (entry.averageSize * (n - 1) + genome.size) / n;
    entry.averageSpeed = (entry.averageSpeed * (n - 1) + genome.speed) / n;

    // Track highest generation
    if (static_cast<uint32_t>(generation) > entry.generationsObserved) {
        entry.generationsObserved = generation;
    }
}

void SpeciesCatalog::unlockTraits(SpeciesDiscoveryEntry& entry, DiscoveryState newState) {
    switch (newState) {
        case DiscoveryState::DETECTED:
            entry.traitsUnlocked[0] = true;  // Basic (type, color)
            break;
        case DiscoveryState::PARTIAL:
            entry.traitsUnlocked[0] = true;
            entry.traitsUnlocked[1] = true;  // Physical (size, speed)
            entry.traitsUnlocked[2] = true;  // Behavioral (diet, movement)
            break;
        case DiscoveryState::COMPLETE:
            for (int i = 0; i < 5; ++i) {
                entry.traitsUnlocked[i] = true;
            }
            break;
        default:
            break;
    }
}

void SpeciesCatalog::updateStatistics(const SpeciesDiscoveryEntry& entry, DiscoveryState previousState) {
    // Update discovery counts
    if (entry.discoveryState == DiscoveryState::COMPLETE &&
        previousState != DiscoveryState::COMPLETE) {
        m_statistics.speciesDiscovered++;
        m_statistics.rarityCount[static_cast<int>(entry.rarity)]++;
        m_statistics.biomeDiscoveries[entry.discoveryBiome]++;
        m_statistics.typeDiscoveries[entry.creatureType]++;
    }

    if (entry.discoveryState >= DiscoveryState::PARTIAL &&
        previousState < DiscoveryState::PARTIAL) {
        m_statistics.speciesPartiallyScanned++;
    }
}

void SpeciesCatalog::emitEvent(DiscoveryEvent::Type type, const SpeciesDiscoveryEntry& entry, const std::string& message) {
    DiscoveryEvent event;
    event.type = type;
    event.speciesId = entry.speciesId;
    event.rarity = entry.rarity;
    event.speciesName = entry.commonName;
    event.message = message.empty() ? entry.commonName : message;
    event.timestamp = static_cast<uint64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    // Add to recent events (capped)
    m_recentEvents.push_back(event);
    if (m_recentEvents.size() > MAX_RECENT_EVENTS) {
        m_recentEvents.erase(m_recentEvents.begin());
    }

    // Invoke callback if set
    if (m_eventCallback) {
        m_eventCallback(event);
    }
}

void SpeciesCatalog::checkMilestones() {
    DiscoveryStatistics& stats = m_statistics;

    // Check discovery count milestones
    if (!stats.firstDiscovery && stats.speciesDiscovered >= 1) {
        stats.firstDiscovery = true;
        DiscoveryEvent event;
        event.type = DiscoveryEvent::Type::MILESTONE_REACHED;
        event.message = "First Discovery! You've cataloged your first species.";
        m_recentEvents.push_back(event);
        if (m_eventCallback) m_eventCallback(event);
    }

    if (!stats.tenDiscoveries && stats.speciesDiscovered >= 10) {
        stats.tenDiscoveries = true;
        DiscoveryEvent event;
        event.type = DiscoveryEvent::Type::MILESTONE_REACHED;
        event.message = "Amateur Naturalist: 10 species discovered!";
        m_recentEvents.push_back(event);
        if (m_eventCallback) m_eventCallback(event);
    }

    if (!stats.fiftyDiscoveries && stats.speciesDiscovered >= 50) {
        stats.fiftyDiscoveries = true;
        DiscoveryEvent event;
        event.type = DiscoveryEvent::Type::MILESTONE_REACHED;
        event.message = "Field Researcher: 50 species discovered!";
        m_recentEvents.push_back(event);
        if (m_eventCallback) m_eventCallback(event);
    }

    if (!stats.hundredDiscoveries && stats.speciesDiscovered >= 100) {
        stats.hundredDiscoveries = true;
        DiscoveryEvent event;
        event.type = DiscoveryEvent::Type::MILESTONE_REACHED;
        event.message = "Master Cataloger: 100 species discovered!";
        m_recentEvents.push_back(event);
        if (m_eventCallback) m_eventCallback(event);
    }

    // Check if all rarities found
    if (!stats.allRaritiesFound) {
        bool allFound = true;
        for (int i = 0; i < 6; ++i) {
            if (stats.rarityCount[i] == 0) {
                allFound = false;
                break;
            }
        }
        if (allFound) {
            stats.allRaritiesFound = true;
            DiscoveryEvent event;
            event.type = DiscoveryEvent::Type::MILESTONE_REACHED;
            event.message = "Rarity Hunter: Found species of every rarity tier!";
            m_recentEvents.push_back(event);
            if (m_eventCallback) m_eventCallback(event);
        }
    }
}

std::string SpeciesCatalog::generateThemedName(const Genome& genome, CreatureType type, BiomeType biome) const {
    // If no planet theme, fall back to standard naming
    if (!m_planetTheme) {
        return "";  // Will use standard naming
    }

    // Use the planet theme to influence naming
    // This is a hook for future planet-themed naming integration
    const auto& themeData = m_planetTheme->getData();

    // For now, just return empty to use standard naming
    // Future: use themeData.name and biome to generate themed prefixes
    (void)themeData;
    (void)genome;
    (void)type;
    (void)biome;

    return "";
}

} // namespace discovery
