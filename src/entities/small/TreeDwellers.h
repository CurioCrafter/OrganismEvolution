#pragma once

#include "SmallCreatures.h"
#include <DirectXMath.h>
#include <vector>
#include <memory>

// Forward declarations
class VegetationManager;

namespace small {

using namespace DirectX;

// Tree structure for creature navigation
struct TreeNode {
    enum class Type : uint8_t {
        TRUNK,
        BRANCH,
        TWIG,
        LEAF_CLUSTER
    };

    uint32_t id;
    uint32_t treeID;
    Type type;
    XMFLOAT3 position;
    XMFLOAT3 direction;        // Direction of branch growth
    float radius;              // Branch thickness
    float length;              // Branch length

    uint32_t parentID;         // Parent node (0 for trunk base)
    std::vector<uint32_t> children;

    // Creature interaction
    bool hasNest;
    uint32_t nestOwner;        // Creature ID
    float foodValue;           // Leaves/fruit available
};

// Simplified tree structure for creature navigation
struct NavigableTree {
    uint32_t id;
    XMFLOAT3 basePosition;
    float height;
    float canopyRadius;

    std::vector<TreeNode> nodes;

    // Quick access
    std::vector<uint32_t> branchNodes;
    std::vector<uint32_t> leafNodes;
    std::vector<uint32_t> nestableNodes;  // Good for nesting
};

// Nest types
enum class NestType : uint8_t {
    TREE_HOLLOW,      // Inside tree (squirrels, owls)
    BRANCH_NEST,      // Built on branch (birds, squirrels)
    LEAF_NEST,        // In leaves (insects)
    BARK_NEST,        // Under bark (beetles, spiders)
    WEB,              // Spider web
    COCOON            // Caterpillar/moth cocoon
};

// Nest structure
struct TreeNest {
    uint32_t id;
    uint32_t treeID;
    uint32_t nodeID;           // Which part of tree
    NestType type;
    XMFLOAT3 position;

    uint32_t ownerID;          // Creature that built it
    uint32_t colonyID;         // If colonial (bees, ants)

    float integrity;           // Structural health
    float foodStored;
    int occupants;
    int eggs;
};

// Tree climbing path point
struct ClimbPathPoint {
    XMFLOAT3 position;
    XMFLOAT3 surfaceNormal;    // For grip calculation
    float difficulty;          // 0-1, how hard to traverse
    bool isRest;               // Good resting spot
};

// Tree traversal path
struct TreePath {
    uint32_t treeID;
    std::vector<ClimbPathPoint> points;
    float totalDistance;
    float totalDifficulty;
};

// Main tree dwelling system
class TreeDwellerSystem {
public:
    TreeDwellerSystem();
    ~TreeDwellerSystem();

    // Initialize from vegetation manager
    void initialize(VegetationManager* vegManager);

    // Update tree structures (call when trees change)
    void updateTreeStructures();

    // Tree queries
    NavigableTree* findNearestTree(const XMFLOAT3& position, float maxRadius = 50.0f);
    std::vector<NavigableTree*> findTreesInRange(const XMFLOAT3& position, float radius);
    TreeNode* findNearestBranch(NavigableTree* tree, const XMFLOAT3& position);
    TreeNode* findNearestLeafCluster(NavigableTree* tree, const XMFLOAT3& position);

    // Path finding
    TreePath findPathToTree(const XMFLOAT3& startPos, NavigableTree* tree);
    TreePath findPathOnTree(NavigableTree* tree, const XMFLOAT3& startPos,
                             const XMFLOAT3& targetPos);
    TreePath findPathBetweenTrees(NavigableTree* from, NavigableTree* to);

    // Nest management
    TreeNest* createNest(SmallCreature* creature, NavigableTree* tree, NestType type);
    TreeNest* findNest(uint32_t nestID);
    TreeNest* findNestByOwner(uint32_t ownerID);
    std::vector<TreeNest*> findNestsInTree(uint32_t treeID);
    void destroyNest(uint32_t nestID);

    // Update creatures on trees
    void update(float deltaTime, SmallCreatureManager& manager);

    // Can creature type use trees?
    static bool canUseTree(SmallCreatureType type);
    static bool canBuildNest(SmallCreatureType type);
    static NestType getPreferredNestType(SmallCreatureType type);

    // Creature-tree interactions
    XMFLOAT3 getClimbingTarget(SmallCreature* creature, NavigableTree* tree);
    XMFLOAT3 getBranchJumpTarget(SmallCreature* creature, NavigableTree* tree);
    float getFoodAtNode(TreeNode* node);
    void consumeFoodAtNode(TreeNode* node, float amount);

    // Statistics
    size_t getTreeCount() const { return trees_.size(); }
    size_t getNestCount() const { return nests_.size(); }

private:
    // Build navigable structure from vegetation
    void buildTreeStructure(NavigableTree& tree, const XMFLOAT3& position,
                            float height, float canopyRadius);

    // Generate procedural branch nodes
    void generateBranches(NavigableTree& tree, uint32_t parentID,
                          int depth, float angle, float length);

    VegetationManager* vegManager_;

    std::vector<NavigableTree> trees_;
    std::vector<TreeNest> nests_;

    uint32_t nextTreeID_;
    uint32_t nextNestID_;
};

// Squirrel-specific tree behavior
class SquirrelBehavior {
public:
    // Tree navigation
    static XMFLOAT3 calculateTreeMovement(SmallCreature* squirrel,
                                          NavigableTree* tree,
                                          const XMFLOAT3& target);

    // Food caching
    struct FoodCache {
        XMFLOAT3 position;
        float amount;
        float age;          // Older caches may be forgotten
    };

    static void cacheFoodNearTree(SmallCreature* squirrel, NavigableTree* tree,
                                   std::vector<FoodCache>& caches);
    static FoodCache* findCachedFood(SmallCreature* squirrel,
                                      std::vector<FoodCache>& caches);

    // Nest building
    static bool shouldBuildNest(SmallCreature* squirrel);
    static TreeNode* findNestingSite(NavigableTree* tree);

    // Predator evasion
    static XMFLOAT3 evadeOnTree(SmallCreature* squirrel,
                                 NavigableTree* tree,
                                 const XMFLOAT3& threatPos);

    // Inter-tree jumping
    static bool canJumpToTree(SmallCreature* squirrel,
                               NavigableTree* from, NavigableTree* to);
    static XMFLOAT3 calculateTreeJump(SmallCreature* squirrel,
                                       NavigableTree* target);
};

// Tree frog behavior
class TreeFrogBehavior {
public:
    // Climbing with toe pads
    static XMFLOAT3 calculateClimb(SmallCreature* frog,
                                    NavigableTree* tree,
                                    const XMFLOAT3& target);

    // Calling from trees (mating)
    static void performMatingCall(SmallCreature* frog);

    // Hunting from tree
    static SmallCreature* findInsectPrey(SmallCreature* frog,
                                          MicroSpatialGrid& grid,
                                          float range);
    static XMFLOAT3 calculateTongueStrike(SmallCreature* frog,
                                           SmallCreature* prey);
};

// Gecko behavior
class GeckoBehavior {
public:
    // Vertical surface movement
    static XMFLOAT3 calculateWallMovement(SmallCreature* gecko,
                                           const XMFLOAT3& surfaceNormal,
                                           const XMFLOAT3& target);

    // Hunting insects
    static SmallCreature* ambushPrey(SmallCreature* gecko,
                                      MicroSpatialGrid& grid);

    // Tail autotomy (escape mechanism)
    static bool shouldDropTail(SmallCreature* gecko, float threatLevel);
};

// Bark-dwelling insects
class BarkDwellerBehavior {
public:
    // Under-bark movement
    static XMFLOAT3 calculateBarkMovement(SmallCreature* insect,
                                           TreeNode* currentNode);

    // Bark beetle specific
    static void createGallery(SmallCreature* beetle, NavigableTree* tree);

    // Finding food under bark
    static XMFLOAT3 findBarkFood(SmallCreature* insect, NavigableTree* tree);
};

// Spider web in trees
class TreeSpiderBehavior {
public:
    // Web attachment points
    struct WebAnchor {
        XMFLOAT3 position;
        uint32_t nodeID;
        float strength;
    };

    // Build web between branches
    static std::vector<WebAnchor> planWebConstruction(SmallCreature* spider,
                                                       NavigableTree* tree);

    // Sit and wait on web
    static XMFLOAT3 getWebCenter(const std::vector<WebAnchor>& anchors);

    // Detect vibration (prey)
    static bool detectWebVibration(SmallCreature* spider,
                                    const std::vector<WebAnchor>& anchors,
                                    MicroSpatialGrid& grid);
};

// Caterpillar/moth behavior
class CaterpillarBehavior {
public:
    // Leaf consumption
    static void consumeLeaf(SmallCreature* caterpillar, TreeNode* leafNode);

    // Finding pupation site
    static XMFLOAT3 findPupationSite(SmallCreature* caterpillar,
                                      NavigableTree* tree);

    // Cocoon creation
    static TreeNest* createCocoon(SmallCreature* caterpillar,
                                   TreeDwellerSystem& system,
                                   NavigableTree* tree);
};

} // namespace small
