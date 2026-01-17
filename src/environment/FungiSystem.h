#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
class Terrain;
class ClimateSystem;
class SeasonManager;
class DecomposerSystem;
class DX12Device;
struct ID3D12GraphicsCommandList;

// Types of fungi
enum class FungusType {
    // Edible mushrooms
    BUTTON_MUSHROOM,
    OYSTER_MUSHROOM,
    CHANTERELLE,
    MOREL,
    PORCINI,

    // Poisonous mushrooms
    DEATH_CAP,
    FLY_AGARIC,
    DESTROYING_ANGEL,

    // Bracket/shelf fungi
    TURKEY_TAIL,
    ARTISTS_CONK,
    CHICKEN_OF_WOODS,

    // Bioluminescent fungi
    GHOST_MUSHROOM,
    JACK_O_LANTERN,
    FOXFIRE,

    // Alien fungi
    CRYSTAL_SPORE,
    PLASMA_CAP,
    TENDRIL_FUNGUS,
    HIVEMIND_CLUSTER,

    // Specialized types
    PUFFBALL,
    STINKHORN,
    CORAL_FUNGUS,

    COUNT
};

// Toxicity levels
enum class ToxicityLevel {
    EDIBLE,
    MILDLY_TOXIC,
    TOXIC,
    DEADLY
};

// Single mushroom instance
struct MushroomInstance {
    glm::vec3 position;
    float rotation;
    float scale;
    FungusType type;

    // Growth state
    float age;              // Days since spawn
    float maturity;         // 0-1, affects size and spore production
    float health;           // 0-1, decay affects this

    // Properties
    bool isBioluminescent;
    float glowIntensity;
    glm::vec3 glowColor;

    // Nutrition for creatures
    float nutritionalValue;
    ToxicityLevel toxicity;

    // Spore production
    float sporeTimer;
    int sporesProduced;
};

// Mycelium network node
struct MyceliumNode {
    glm::vec3 position;
    std::vector<int> connections;  // Indices to connected nodes

    float nutrientLevel;           // Nutrients being transported
    float decompositionRate;       // How fast it breaks down matter
    bool isActive;

    // Visualization
    float thickness;
    glm::vec3 color;
};

// Mycelium network connecting mushrooms underground
struct MyceliumNetwork {
    std::vector<MyceliumNode> nodes;

    // Network properties
    float totalNutrients;
    float decompositionPower;      // Total decomposition capability
    int mushroomCount;

    // Spatial bounds
    glm::vec3 center;
    float radius;

    // Network ID for tracking
    int networkId;
};

// Decomposing matter (dead creatures, fallen trees, etc.)
struct DecomposingMatter {
    glm::vec3 position;
    float remainingMass;           // How much is left to decompose
    float decompositionRate;       // Current rate of breakdown
    float nutrientsReleased;       // Total nutrients released to soil

    // Type of matter (affects decomposition rate and nutrients)
    enum class MatterType {
        DEAD_CREATURE,
        FALLEN_TREE,
        LEAF_LITTER,
        DEAD_VEGETATION
    } type;

    // Visual
    float decayProgress;           // 0-1 for rendering
};

// Fungus species configuration
struct FungusConfig {
    FungusType type;
    std::string name;

    // Environmental preferences
    float preferredMoisture;       // 0-1
    float preferredTemperature;    // Celsius
    float minLight;                // 0-1 (most fungi like dark)
    float maxLight;

    // Growth parameters
    float growthRate;              // Base growth speed
    float maxSize;                 // Maximum scale
    float lifespan;                // Days until natural death

    // Spore production
    float sporeProductionRate;     // Spores per day
    float sporeSpreadRadius;       // How far spores can travel

    // Nutrition/danger
    float nutritionalValue;
    ToxicityLevel toxicity;

    // Visual properties
    glm::vec3 capColor;
    glm::vec3 stemColor;
    glm::vec3 gillColor;
    bool isBioluminescent;
    glm::vec3 glowColor;
    float glowIntensity;

    // Decomposition
    float decompositionPower;      // How much it helps break down matter
};

// Get configuration for fungus type
FungusConfig getFungusConfig(FungusType type);

class FungiSystem {
public:
    FungiSystem();
    ~FungiSystem();

    // Initialize with terrain and systems
    void initialize(const Terrain* terrain, DX12Device* device);

    // Set related systems
    void setClimateSystem(ClimateSystem* climate) { climateSystem = climate; }
    void setSeasonManager(SeasonManager* season) { seasonManager = season; }
    void setDecomposerSystem(DecomposerSystem* decomposer) { decomposerSystem = decomposer; }

    // Generate initial fungi placement
    void generate(unsigned int seed);

    // Update fungi growth, spore spread, decomposition
    void update(float deltaTime);

    // Render fungi
    void render(ID3D12GraphicsCommandList* commandList, const glm::vec3& cameraPos);

    // ========================================================================
    // Decomposition Interface
    // ========================================================================

    // Add decomposing matter (called when creature dies, tree falls, etc.)
    void addDecomposingMatter(const glm::vec3& position, float mass, DecomposingMatter::MatterType type);

    // Get nutrients available at position (for plants)
    float getNutrientsAt(const glm::vec3& position, float radius) const;

    // ========================================================================
    // Creature Interaction
    // ========================================================================

    // Find nearest mushroom for eating
    const MushroomInstance* findNearestMushroom(const glm::vec3& position, float radius) const;

    // Consume mushroom (returns nutrition, negative if poisonous)
    float consumeMushroom(const glm::vec3& position, float radius);

    // Get all edible mushroom positions (for creature AI)
    std::vector<glm::vec3> getEdibleMushroomPositions() const;

    // Get bioluminescent fungi (for night lighting calculations)
    std::vector<std::pair<glm::vec3, glm::vec3>> getBioluminescentPositions() const; // pos, color

    // ========================================================================
    // Statistics
    // ========================================================================

    size_t getMushroomCount() const { return mushrooms.size(); }
    size_t getNetworkCount() const { return networks.size(); }
    float getTotalDecompositionPower() const;
    float getTotalNutrients() const;

    // Get instances for rendering
    const std::vector<MushroomInstance>& getMushrooms() const { return mushrooms; }
    const std::vector<MyceliumNetwork>& getNetworks() const { return networks; }

private:
    const Terrain* terrain = nullptr;
    DX12Device* dx12Device = nullptr;
    ClimateSystem* climateSystem = nullptr;
    SeasonManager* seasonManager = nullptr;
    DecomposerSystem* decomposerSystem = nullptr;

    // Mushroom instances
    std::vector<MushroomInstance> mushrooms;

    // Mycelium networks
    std::vector<MyceliumNetwork> networks;
    int nextNetworkId = 0;

    // Decomposing matter being processed
    std::vector<DecomposingMatter> decomposingMatter;

    // Soil nutrient grid (simplified)
    std::vector<std::vector<float>> soilNutrients;
    int nutrientGridSize = 50;
    float nutrientTileSize = 20.0f;

    // Update timers
    float sporeSpreadTimer = 0.0f;
    float networkUpdateTimer = 0.0f;

    // Generation helpers
    void generateMushroomCluster(const glm::vec3& center, FungusType type, int count, unsigned int seed);
    bool isSuitableForFungi(float x, float z) const;
    FungusType selectFungusTypeForBiome(int biomeType) const;

    // Update helpers
    void updateMushroomGrowth(float deltaTime);
    void updateSporeSpread(float deltaTime);
    void updateMyceliumNetworks(float deltaTime);
    void updateDecomposition(float deltaTime);

    // Network management
    void createNetwork(const glm::vec3& center, float radius);
    void expandNetwork(int networkId, const glm::vec3& position);
    void connectMushroomsToNetworks();

    // Nutrient helpers
    void releaseNutrients(const glm::vec3& position, float amount);
    std::pair<int, int> worldToNutrientGrid(float x, float z) const;

    // Spawning
    void spawnMushroom(const glm::vec3& position, FungusType type);
    void trySpawnFromSpore(const glm::vec3& origin, FungusType type, float distance);
};
