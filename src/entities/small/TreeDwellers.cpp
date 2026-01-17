#include "TreeDwellers.h"
#include "../../environment/VegetationManager.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace small {

// =============================================================================
// TreeDwellerSystem Implementation
// =============================================================================

TreeDwellerSystem::TreeDwellerSystem()
    : vegManager_(nullptr)
    , nextTreeID_(1)
    , nextNestID_(1) {
}

TreeDwellerSystem::~TreeDwellerSystem() = default;

void TreeDwellerSystem::initialize(VegetationManager* vegManager) {
    vegManager_ = vegManager;
    updateTreeStructures();
}

void TreeDwellerSystem::updateTreeStructures() {
    trees_.clear();

    if (!vegManager_) return;

    // Get trees from vegetation manager
    const auto& treeInstances = vegManager_->getTrees();

    trees_.reserve(treeInstances.size());

    for (const auto& tree : treeInstances) {
        NavigableTree navTree;
        navTree.id = nextTreeID_++;
        navTree.basePosition = { tree.position.x, tree.position.y, tree.position.z };

        // Estimate height and canopy based on tree scale
        navTree.height = tree.scale * 6.0f;  // Trees have ~6x scale multiplier
        navTree.canopyRadius = tree.scale * 3.0f;

        // Build internal structure
        buildTreeStructure(navTree, navTree.basePosition, navTree.height, navTree.canopyRadius);

        trees_.push_back(std::move(navTree));
    }
}

void TreeDwellerSystem::buildTreeStructure(NavigableTree& tree, const XMFLOAT3& position,
                                            float height, float canopyRadius) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
    std::uniform_real_distribution<float> varianceDist(0.8f, 1.2f);

    tree.nodes.clear();

    // Create trunk nodes
    int trunkSegments = static_cast<int>(height / 0.5f);
    float segmentHeight = height / trunkSegments;

    for (int i = 0; i < trunkSegments; ++i) {
        TreeNode node;
        node.id = static_cast<uint32_t>(tree.nodes.size() + 1);
        node.treeID = tree.id;
        node.type = TreeNode::Type::TRUNK;
        node.position = {
            position.x,
            position.y + i * segmentHeight,
            position.z
        };
        node.direction = { 0, 1, 0 };
        node.radius = 0.2f * (1.0f - (float)i / trunkSegments * 0.5f);  // Taper
        node.length = segmentHeight;
        node.parentID = (i > 0) ? static_cast<uint32_t>(tree.nodes.size()) : 0;
        node.hasNest = false;
        node.nestOwner = 0;
        node.foodValue = 0.0f;

        tree.nodes.push_back(node);

        // Add branches at intervals (starting from 1/3 up)
        if (i > trunkSegments / 3) {
            // Number of branches increases towards middle
            int branchCount = 2 + (i < trunkSegments * 2 / 3 ? 1 : 0);
            float startAngle = angleDist(rng);

            for (int b = 0; b < branchCount; ++b) {
                float angle = startAngle + b * (6.28318f / branchCount);
                float branchLength = canopyRadius * 0.6f * varianceDist(rng);

                // Create main branch
                TreeNode branch;
                branch.id = static_cast<uint32_t>(tree.nodes.size() + 1);
                branch.treeID = tree.id;
                branch.type = TreeNode::Type::BRANCH;
                branch.position = node.position;
                branch.direction = {
                    cosf(angle) * 0.7f,
                    0.3f * varianceDist(rng),
                    sinf(angle) * 0.7f
                };
                branch.radius = node.radius * 0.3f;
                branch.length = branchLength;
                branch.parentID = node.id;
                branch.hasNest = false;
                branch.nestOwner = 0;
                branch.foodValue = 0.0f;

                tree.nodes.push_back(branch);
                tree.branchNodes.push_back(branch.id);

                // Mark as nestable
                if (branch.radius > 0.05f) {
                    tree.nestableNodes.push_back(branch.id);
                }

                // Add sub-branches and leaves
                generateBranches(tree, branch.id, 2, angle, branchLength * 0.6f);
            }
        }
    }
}

void TreeDwellerSystem::generateBranches(NavigableTree& tree, uint32_t parentID,
                                          int depth, float angle, float length) {
    if (depth <= 0 || length < 0.1f) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> varianceDist(0.7f, 1.0f);

    // Find parent node
    TreeNode* parent = nullptr;
    for (auto& node : tree.nodes) {
        if (node.id == parentID) {
            parent = &node;
            break;
        }
    }
    if (!parent) return;

    // End position of parent branch
    XMFLOAT3 endPos = {
        parent->position.x + parent->direction.x * parent->length,
        parent->position.y + parent->direction.y * parent->length,
        parent->position.z + parent->direction.z * parent->length
    };

    // Create 2-3 sub-branches
    int subBranches = 2 + (depth > 1 ? 1 : 0);
    for (int i = 0; i < subBranches; ++i) {
        float subAngle = angle + angleDist(rng) + i * 0.8f;

        TreeNode::Type type = (depth == 1) ? TreeNode::Type::TWIG : TreeNode::Type::BRANCH;

        TreeNode node;
        node.id = static_cast<uint32_t>(tree.nodes.size() + 1);
        node.treeID = tree.id;
        node.type = type;
        node.position = endPos;
        node.direction = {
            cosf(subAngle) * 0.6f + parent->direction.x * 0.4f,
            0.2f + parent->direction.y * 0.3f,
            sinf(subAngle) * 0.6f + parent->direction.z * 0.4f
        };

        // Normalize direction
        float dirLen = sqrtf(node.direction.x * node.direction.x +
                            node.direction.y * node.direction.y +
                            node.direction.z * node.direction.z);
        node.direction.x /= dirLen;
        node.direction.y /= dirLen;
        node.direction.z /= dirLen;

        node.radius = parent->radius * 0.5f;
        node.length = length * varianceDist(rng);
        node.parentID = parentID;
        node.hasNest = false;
        node.nestOwner = 0;

        // Leaves/food at tips
        if (depth == 1) {
            node.foodValue = 10.0f;
            tree.leafNodes.push_back(node.id);
        } else {
            node.foodValue = 0.0f;
            tree.branchNodes.push_back(node.id);
        }

        tree.nodes.push_back(node);

        // Recurse for sub-branches
        generateBranches(tree, node.id, depth - 1, subAngle, length * 0.6f);
    }

    // Add leaf cluster at end of twigs
    if (depth == 1) {
        TreeNode leaf;
        leaf.id = static_cast<uint32_t>(tree.nodes.size() + 1);
        leaf.treeID = tree.id;
        leaf.type = TreeNode::Type::LEAF_CLUSTER;
        leaf.position = {
            endPos.x + parent->direction.x * length * 0.8f,
            endPos.y + parent->direction.y * length * 0.8f,
            endPos.z + parent->direction.z * length * 0.8f
        };
        leaf.direction = parent->direction;
        leaf.radius = 0.3f;
        leaf.length = 0.5f;
        leaf.parentID = parentID;
        leaf.hasNest = false;
        leaf.nestOwner = 0;
        leaf.foodValue = 20.0f;

        tree.nodes.push_back(leaf);
        tree.leafNodes.push_back(leaf.id);
    }
}

NavigableTree* TreeDwellerSystem::findNearestTree(const XMFLOAT3& position, float maxRadius) {
    NavigableTree* nearest = nullptr;
    float minDist = maxRadius * maxRadius;

    for (auto& tree : trees_) {
        float dx = tree.basePosition.x - position.x;
        float dz = tree.basePosition.z - position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < minDist) {
            minDist = distSq;
            nearest = &tree;
        }
    }

    return nearest;
}

std::vector<NavigableTree*> TreeDwellerSystem::findTreesInRange(const XMFLOAT3& position, float radius) {
    std::vector<NavigableTree*> result;
    float radiusSq = radius * radius;

    for (auto& tree : trees_) {
        float dx = tree.basePosition.x - position.x;
        float dz = tree.basePosition.z - position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq <= radiusSq) {
            result.push_back(&tree);
        }
    }

    return result;
}

TreeNode* TreeDwellerSystem::findNearestBranch(NavigableTree* tree, const XMFLOAT3& position) {
    if (!tree) return nullptr;

    TreeNode* nearest = nullptr;
    float minDist = FLT_MAX;

    for (uint32_t branchID : tree->branchNodes) {
        for (auto& node : tree->nodes) {
            if (node.id == branchID) {
                float dx = node.position.x - position.x;
                float dy = node.position.y - position.y;
                float dz = node.position.z - position.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq < minDist) {
                    minDist = distSq;
                    nearest = &node;
                }
                break;
            }
        }
    }

    return nearest;
}

TreeNode* TreeDwellerSystem::findNearestLeafCluster(NavigableTree* tree, const XMFLOAT3& position) {
    if (!tree) return nullptr;

    TreeNode* nearest = nullptr;
    float minDist = FLT_MAX;

    for (uint32_t leafID : tree->leafNodes) {
        for (auto& node : tree->nodes) {
            if (node.id == leafID) {
                float dx = node.position.x - position.x;
                float dy = node.position.y - position.y;
                float dz = node.position.z - position.z;
                float distSq = dx * dx + dy * dy + dz * dz;

                if (distSq < minDist) {
                    minDist = distSq;
                    nearest = &node;
                }
                break;
            }
        }
    }

    return nearest;
}

TreePath TreeDwellerSystem::findPathToTree(const XMFLOAT3& startPos, NavigableTree* tree) {
    TreePath path;
    if (!tree) return path;

    path.treeID = tree->id;
    path.totalDistance = 0.0f;
    path.totalDifficulty = 0.0f;

    // Simple path: start -> tree base -> first branch
    ClimbPathPoint p1;
    p1.position = startPos;
    p1.surfaceNormal = { 0, 1, 0 };
    p1.difficulty = 0.1f;
    p1.isRest = true;
    path.points.push_back(p1);

    ClimbPathPoint p2;
    p2.position = tree->basePosition;
    p2.surfaceNormal = { 0, 1, 0 };
    p2.difficulty = 0.2f;
    p2.isRest = true;
    path.points.push_back(p2);

    // Add trunk climbing points
    for (const auto& node : tree->nodes) {
        if (node.type == TreeNode::Type::TRUNK) {
            ClimbPathPoint p;
            p.position = node.position;
            p.surfaceNormal = { -node.direction.x, 0, -node.direction.z };
            p.difficulty = 0.3f;
            p.isRest = false;
            path.points.push_back(p);
        }
    }

    // Calculate total distance
    for (size_t i = 1; i < path.points.size(); ++i) {
        float dx = path.points[i].position.x - path.points[i-1].position.x;
        float dy = path.points[i].position.y - path.points[i-1].position.y;
        float dz = path.points[i].position.z - path.points[i-1].position.z;
        path.totalDistance += sqrtf(dx * dx + dy * dy + dz * dz);
        path.totalDifficulty += path.points[i].difficulty;
    }

    return path;
}

TreePath TreeDwellerSystem::findPathOnTree(NavigableTree* tree, const XMFLOAT3& startPos,
                                            const XMFLOAT3& targetPos) {
    TreePath path;
    if (!tree) return path;

    path.treeID = tree->id;

    // Simple A* through tree nodes would go here
    // For now, direct path with some waypoints

    ClimbPathPoint start;
    start.position = startPos;
    start.surfaceNormal = { 0, 1, 0 };
    start.difficulty = 0.2f;
    start.isRest = false;
    path.points.push_back(start);

    // Find intermediate nodes
    TreeNode* nearStart = findNearestBranch(tree, startPos);
    TreeNode* nearEnd = findNearestBranch(tree, targetPos);

    if (nearStart) {
        ClimbPathPoint p;
        p.position = nearStart->position;
        p.surfaceNormal = { -nearStart->direction.x, 0, -nearStart->direction.z };
        p.difficulty = 0.3f;
        p.isRest = true;
        path.points.push_back(p);
    }

    if (nearEnd && nearEnd != nearStart) {
        ClimbPathPoint p;
        p.position = nearEnd->position;
        p.surfaceNormal = { -nearEnd->direction.x, 0, -nearEnd->direction.z };
        p.difficulty = 0.3f;
        p.isRest = true;
        path.points.push_back(p);
    }

    ClimbPathPoint end;
    end.position = targetPos;
    end.surfaceNormal = { 0, 1, 0 };
    end.difficulty = 0.2f;
    end.isRest = true;
    path.points.push_back(end);

    // Calculate totals
    for (size_t i = 1; i < path.points.size(); ++i) {
        float dx = path.points[i].position.x - path.points[i-1].position.x;
        float dy = path.points[i].position.y - path.points[i-1].position.y;
        float dz = path.points[i].position.z - path.points[i-1].position.z;
        path.totalDistance += sqrtf(dx * dx + dy * dy + dz * dz);
        path.totalDifficulty += path.points[i].difficulty;
    }

    return path;
}

TreePath TreeDwellerSystem::findPathBetweenTrees(NavigableTree* from, NavigableTree* to) {
    TreePath path;
    if (!from || !to) return path;

    // Find closest branches between trees for jumping
    TreeNode* fromBranch = nullptr;
    TreeNode* toBranch = nullptr;
    float minDist = FLT_MAX;

    for (uint32_t fromID : from->branchNodes) {
        for (auto& fromNode : from->nodes) {
            if (fromNode.id != fromID) continue;

            for (uint32_t toID : to->branchNodes) {
                for (auto& toNode : to->nodes) {
                    if (toNode.id != toID) continue;

                    float dx = toNode.position.x - fromNode.position.x;
                    float dy = toNode.position.y - fromNode.position.y;
                    float dz = toNode.position.z - fromNode.position.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < minDist) {
                        minDist = distSq;
                        fromBranch = &fromNode;
                        toBranch = &toNode;
                    }
                }
            }
        }
    }

    if (fromBranch && toBranch) {
        path.treeID = to->id;

        ClimbPathPoint p1;
        p1.position = fromBranch->position;
        p1.difficulty = 0.5f;  // Jumping is hard
        p1.isRest = true;
        path.points.push_back(p1);

        ClimbPathPoint p2;
        p2.position = toBranch->position;
        p2.difficulty = 0.5f;
        p2.isRest = true;
        path.points.push_back(p2);

        path.totalDistance = sqrtf(minDist);
        path.totalDifficulty = 1.0f;
    }

    return path;
}

TreeNest* TreeDwellerSystem::createNest(SmallCreature* creature, NavigableTree* tree, NestType type) {
    if (!creature || !tree) return nullptr;

    // Find suitable node
    TreeNode* nestNode = nullptr;

    for (uint32_t nodeID : tree->nestableNodes) {
        for (auto& node : tree->nodes) {
            if (node.id == nodeID && !node.hasNest) {
                nestNode = &node;
                break;
            }
        }
        if (nestNode) break;
    }

    if (!nestNode) return nullptr;

    TreeNest nest;
    nest.id = nextNestID_++;
    nest.treeID = tree->id;
    nest.nodeID = nestNode->id;
    nest.type = type;
    nest.position = nestNode->position;
    nest.ownerID = creature->id;
    nest.colonyID = creature->colonyID;
    nest.integrity = 100.0f;
    nest.foodStored = 0.0f;
    nest.occupants = 1;
    nest.eggs = 0;

    nestNode->hasNest = true;
    nestNode->nestOwner = creature->id;

    nests_.push_back(nest);
    return &nests_.back();
}

TreeNest* TreeDwellerSystem::findNest(uint32_t nestID) {
    for (auto& nest : nests_) {
        if (nest.id == nestID) return &nest;
    }
    return nullptr;
}

TreeNest* TreeDwellerSystem::findNestByOwner(uint32_t ownerID) {
    for (auto& nest : nests_) {
        if (nest.ownerID == ownerID) return &nest;
    }
    return nullptr;
}

std::vector<TreeNest*> TreeDwellerSystem::findNestsInTree(uint32_t treeID) {
    std::vector<TreeNest*> result;
    for (auto& nest : nests_) {
        if (nest.treeID == treeID) {
            result.push_back(&nest);
        }
    }
    return result;
}

void TreeDwellerSystem::destroyNest(uint32_t nestID) {
    for (auto it = nests_.begin(); it != nests_.end(); ++it) {
        if (it->id == nestID) {
            // Clear node's nest flag
            for (auto& tree : trees_) {
                if (tree.id == it->treeID) {
                    for (auto& node : tree.nodes) {
                        if (node.id == it->nodeID) {
                            node.hasNest = false;
                            node.nestOwner = 0;
                            break;
                        }
                    }
                    break;
                }
            }

            nests_.erase(it);
            return;
        }
    }
}

void TreeDwellerSystem::update(float deltaTime, SmallCreatureManager& manager) {
    // Update nest integrity
    for (auto& nest : nests_) {
        nest.integrity -= deltaTime * 0.01f;  // Slow decay

        // Destroy very degraded nests
        if (nest.integrity <= 0.0f) {
            destroyNest(nest.id);
        }
    }

    // Regrow food on leaves
    for (auto& tree : trees_) {
        for (auto& node : tree.nodes) {
            if (node.type == TreeNode::Type::LEAF_CLUSTER) {
                node.foodValue = std::min(node.foodValue + deltaTime * 0.1f, 20.0f);
            }
        }
    }
}

bool TreeDwellerSystem::canUseTree(SmallCreatureType type) {
    // Only creatures that PRIMARILY live in trees should use trees
    // This prevents ground insects from climbing trees just because they CAN climb
    auto props = getProperties(type);

    // Check if tree/canopy is the primary habitat
    if (props.primaryHabitat == HabitatType::CANOPY ||
        props.primaryHabitat == HabitatType::TREE_TRUNK) {
        return true;
    }

    // Specific arboreal species that may have ground listed as primary but use trees
    switch (type) {
        case SmallCreatureType::SQUIRREL_TREE:
        case SmallCreatureType::TREE_FROG:
        case SmallCreatureType::GECKO:
        case SmallCreatureType::CHAMELEON:
        case SmallCreatureType::SPIDER_ORB_WEAVER:  // Web builders use trees
            return true;

        // Bats roost in trees
        case SmallCreatureType::BAT_SMALL:
        case SmallCreatureType::BAT_LARGE:
            return true;

        // Caterpillars/butterflies may use trees for pupation
        case SmallCreatureType::BUTTERFLY:
        case SmallCreatureType::MOTH:
            return true;

        default:
            break;
    }

    // Ground insects (ants, beetles, etc.) should NOT use trees
    // even if they have canClimb capability - they forage on ground
    return false;
}

bool TreeDwellerSystem::canBuildNest(SmallCreatureType type) {
    return type == SmallCreatureType::SQUIRREL_TREE ||
           type == SmallCreatureType::SPIDER_ORB_WEAVER ||
           type == SmallCreatureType::BUTTERFLY ||
           type == SmallCreatureType::MOTH;
}

NestType TreeDwellerSystem::getPreferredNestType(SmallCreatureType type) {
    if (type == SmallCreatureType::SQUIRREL_TREE) return NestType::TREE_HOLLOW;
    if (isSpider(type)) return NestType::WEB;
    if (type == SmallCreatureType::BUTTERFLY || type == SmallCreatureType::MOTH) {
        return NestType::COCOON;
    }
    return NestType::LEAF_NEST;
}

XMFLOAT3 TreeDwellerSystem::getClimbingTarget(SmallCreature* creature, NavigableTree* tree) {
    if (!creature || !tree) return { 0, 0, 0 };

    // Find node above current position
    TreeNode* target = nullptr;
    float minAboveDist = FLT_MAX;

    for (const auto& node : tree->nodes) {
        if (node.position.y <= creature->position.y) continue;

        float dx = node.position.x - creature->position.x;
        float dy = node.position.y - creature->position.y;
        float dz = node.position.z - creature->position.z;
        float dist = dx * dx + dy * dy + dz * dz;

        if (dist < minAboveDist) {
            minAboveDist = dist;
            target = const_cast<TreeNode*>(&node);
        }
    }

    if (target) {
        return target->position;
    }

    // If at top, find branch
    TreeNode* branch = findNearestBranch(tree, creature->position);
    if (branch) {
        return branch->position;
    }

    return creature->position;
}

XMFLOAT3 TreeDwellerSystem::getBranchJumpTarget(SmallCreature* creature, NavigableTree* tree) {
    if (!creature || !tree) return { 0, 0, 0 };

    // Find branch at similar height but different position
    TreeNode* target = nullptr;
    float bestScore = 0.0f;

    for (uint32_t branchID : tree->branchNodes) {
        for (const auto& node : tree->nodes) {
            if (node.id != branchID) continue;

            float heightDiff = fabsf(node.position.y - creature->position.y);
            if (heightDiff > 2.0f) continue;  // Too far vertically

            float dx = node.position.x - creature->position.x;
            float dz = node.position.z - creature->position.z;
            float horizDist = sqrtf(dx * dx + dz * dz);

            if (horizDist < 0.5f || horizDist > 3.0f) continue;  // Too close or too far

            // Score: prefer branches at similar height, moderate distance
            float score = 1.0f / (1.0f + heightDiff) * horizDist;

            if (score > bestScore) {
                bestScore = score;
                target = const_cast<TreeNode*>(&node);
            }
        }
    }

    if (target) {
        return target->position;
    }

    return creature->position;
}

float TreeDwellerSystem::getFoodAtNode(TreeNode* node) {
    if (!node) return 0.0f;
    return node->foodValue;
}

void TreeDwellerSystem::consumeFoodAtNode(TreeNode* node, float amount) {
    if (!node) return;
    node->foodValue = std::max(0.0f, node->foodValue - amount);
}

// =============================================================================
// SquirrelBehavior Implementation
// =============================================================================

XMFLOAT3 SquirrelBehavior::calculateTreeMovement(SmallCreature* squirrel,
                                                  NavigableTree* tree,
                                                  const XMFLOAT3& target) {
    if (!squirrel || !tree) return { 0, 0, 0 };

    float dx = target.x - squirrel->position.x;
    float dy = target.y - squirrel->position.y;
    float dz = target.z - squirrel->position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.01f) return { 0, 0, 0 };

    // Squirrels move fast on trees
    auto props = getProperties(squirrel->type);
    float speed = props.baseSpeed * 1.5f * squirrel->genome->speed;

    return {
        (dx / dist) * speed,
        (dy / dist) * speed,
        (dz / dist) * speed
    };
}

void SquirrelBehavior::cacheFoodNearTree(SmallCreature* squirrel, NavigableTree* tree,
                                          std::vector<FoodCache>& caches) {
    if (!squirrel || !tree) return;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-3.0f, 3.0f);

    FoodCache cache;
    cache.position = {
        tree->basePosition.x + dist(rng),
        tree->basePosition.y,  // Underground
        tree->basePosition.z + dist(rng)
    };
    cache.amount = 5.0f;
    cache.age = 0.0f;

    caches.push_back(cache);
}

SquirrelBehavior::FoodCache* SquirrelBehavior::findCachedFood(SmallCreature* squirrel,
                                                               std::vector<FoodCache>& caches) {
    if (!squirrel || caches.empty()) return nullptr;

    // Squirrels remember ~80% of their caches
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> memory(0.0f, 1.0f);

    FoodCache* nearest = nullptr;
    float minDist = FLT_MAX;

    for (auto& cache : caches) {
        if (memory(rng) > 0.8f) continue;  // Forgot this cache

        float dx = cache.position.x - squirrel->position.x;
        float dz = cache.position.z - squirrel->position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < minDist) {
            minDist = distSq;
            nearest = &cache;
        }
    }

    return nearest;
}

bool SquirrelBehavior::shouldBuildNest(SmallCreature* squirrel) {
    if (!squirrel) return false;

    // Build nest when energy high and no existing nest
    return squirrel->energy > 80.0f && squirrel->nestID == 0;
}

TreeNode* SquirrelBehavior::findNestingSite(NavigableTree* tree) {
    if (!tree) return nullptr;

    // Prefer thick branches high up
    TreeNode* best = nullptr;
    float bestScore = 0.0f;

    for (auto& node : tree->nodes) {
        if (node.type != TreeNode::Type::BRANCH) continue;
        if (node.hasNest) continue;

        float heightScore = node.position.y / tree->height;
        float thicknessScore = node.radius;
        float score = heightScore * 0.7f + thicknessScore * 0.3f;

        if (score > bestScore) {
            bestScore = score;
            best = &node;
        }
    }

    return best;
}

XMFLOAT3 SquirrelBehavior::evadeOnTree(SmallCreature* squirrel,
                                        NavigableTree* tree,
                                        const XMFLOAT3& threatPos) {
    if (!squirrel || !tree) return { 0, 0, 0 };

    // Move to opposite side of tree from threat
    float dx = squirrel->position.x - threatPos.x;
    float dz = squirrel->position.z - threatPos.z;
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 0.01f) {
        dx = 1.0f;
        dz = 0.0f;
        dist = 1.0f;
    }

    // Also move upward
    return {
        (dx / dist) * 2.0f,
        2.0f,
        (dz / dist) * 2.0f
    };
}

bool SquirrelBehavior::canJumpToTree(SmallCreature* squirrel,
                                      NavigableTree* from, NavigableTree* to) {
    if (!squirrel || !from || !to) return false;

    // Calculate distance between closest branches
    float minDist = FLT_MAX;

    for (uint32_t fromID : from->branchNodes) {
        for (auto& fromNode : from->nodes) {
            if (fromNode.id != fromID) continue;

            for (uint32_t toID : to->branchNodes) {
                for (auto& toNode : to->nodes) {
                    if (toNode.id != toID) continue;

                    float dx = toNode.position.x - fromNode.position.x;
                    float dy = toNode.position.y - fromNode.position.y;
                    float dz = toNode.position.z - fromNode.position.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < minDist) {
                        minDist = distSq;
                    }
                }
            }
        }
    }

    // Squirrels can jump about 3 meters
    return sqrtf(minDist) < 3.0f;
}

XMFLOAT3 SquirrelBehavior::calculateTreeJump(SmallCreature* squirrel,
                                              NavigableTree* target) {
    if (!squirrel || !target) return { 0, 0, 0 };

    // Find nearest branch on target tree
    XMFLOAT3 targetPos = target->basePosition;
    float minDist = FLT_MAX;

    for (uint32_t branchID : target->branchNodes) {
        for (auto& node : target->nodes) {
            if (node.id != branchID) continue;

            float dx = node.position.x - squirrel->position.x;
            float dy = node.position.y - squirrel->position.y;
            float dz = node.position.z - squirrel->position.z;
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq < minDist) {
                minDist = distSq;
                targetPos = node.position;
            }
        }
    }

    // Calculate jump velocity
    float dx = targetPos.x - squirrel->position.x;
    float dy = targetPos.y - squirrel->position.y;
    float dz = targetPos.z - squirrel->position.z;
    float horizDist = sqrtf(dx * dx + dz * dz);

    float jumpSpeed = 5.0f;
    float angle = 0.6f;  // About 35 degrees

    return {
        (dx / horizDist) * jumpSpeed * cosf(angle),
        jumpSpeed * sinf(angle) + dy * 0.3f,
        (dz / horizDist) * jumpSpeed * cosf(angle)
    };
}

// =============================================================================
// TreeFrogBehavior Implementation
// =============================================================================

XMFLOAT3 TreeFrogBehavior::calculateClimb(SmallCreature* frog,
                                           NavigableTree* tree,
                                           const XMFLOAT3& target) {
    if (!frog || !tree) return { 0, 0, 0 };

    float dx = target.x - frog->position.x;
    float dy = target.y - frog->position.y;
    float dz = target.z - frog->position.z;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist < 0.01f) return { 0, 0, 0 };

    // Tree frogs climb slowly but can go anywhere
    auto props = getProperties(frog->type);
    float speed = props.baseSpeed * 0.5f;

    return {
        (dx / dist) * speed,
        (dy / dist) * speed,
        (dz / dist) * speed
    };
}

void TreeFrogBehavior::performMatingCall(SmallCreature* frog) {
    // Would trigger audio/visual effect
    // For now, just set a flag or state
    if (frog && frog->isMale()) {
        frog->matingUrge = 1.0f;  // Signal readiness
    }
}

SmallCreature* TreeFrogBehavior::findInsectPrey(SmallCreature* frog,
                                                 MicroSpatialGrid& grid,
                                                 float range) {
    if (!frog) return nullptr;

    return grid.findNearest(frog->position, range,
        [](SmallCreature* c) {
            return c->isAlive() && isFlyingInsect(c->type);
        });
}

XMFLOAT3 TreeFrogBehavior::calculateTongueStrike(SmallCreature* frog,
                                                  SmallCreature* prey) {
    if (!frog || !prey) return { 0, 0, 0 };

    // Direction to prey
    float dx = prey->position.x - frog->position.x;
    float dy = prey->position.y - frog->position.y;
    float dz = prey->position.z - frog->position.z;

    // Tongue is instant, return direction for animation
    return { dx, dy, dz };
}

// =============================================================================
// GeckoBehavior Implementation
// =============================================================================

XMFLOAT3 GeckoBehavior::calculateWallMovement(SmallCreature* gecko,
                                               const XMFLOAT3& surfaceNormal,
                                               const XMFLOAT3& target) {
    if (!gecko) return { 0, 0, 0 };

    // Calculate direction along surface
    float dx = target.x - gecko->position.x;
    float dy = target.y - gecko->position.y;
    float dz = target.z - gecko->position.z;

    // Project onto surface
    float dot = dx * surfaceNormal.x + dy * surfaceNormal.y + dz * surfaceNormal.z;

    XMFLOAT3 surfaceDir = {
        dx - dot * surfaceNormal.x,
        dy - dot * surfaceNormal.y,
        dz - dot * surfaceNormal.z
    };

    float len = sqrtf(surfaceDir.x * surfaceDir.x +
                     surfaceDir.y * surfaceDir.y +
                     surfaceDir.z * surfaceDir.z);

    if (len > 0.01f) {
        auto props = getProperties(gecko->type);
        float speed = props.baseSpeed * gecko->genome->speed;

        return {
            (surfaceDir.x / len) * speed,
            (surfaceDir.y / len) * speed,
            (surfaceDir.z / len) * speed
        };
    }

    return { 0, 0, 0 };
}

SmallCreature* GeckoBehavior::ambushPrey(SmallCreature* gecko,
                                          MicroSpatialGrid& grid) {
    if (!gecko) return nullptr;

    // Wait for prey to come close
    return grid.findNearest(gecko->position, 0.3f,
        [](SmallCreature* c) {
            return c->isAlive() && isInsect(c->type);
        });
}

bool GeckoBehavior::shouldDropTail(SmallCreature* gecko, float threatLevel) {
    // Geckos drop tail when fear > 0.9 and being grabbed
    return gecko && gecko->fear > 0.9f && threatLevel > 0.8f;
}

// =============================================================================
// TreeSpiderBehavior Implementation
// =============================================================================

std::vector<TreeSpiderBehavior::WebAnchor> TreeSpiderBehavior::planWebConstruction(
    SmallCreature* spider, NavigableTree* tree) {
    std::vector<WebAnchor> anchors;
    if (!spider || !tree) return anchors;

    // Find 3-4 branch points to anchor web
    std::vector<TreeNode*> candidates;

    for (auto& node : tree->nodes) {
        if (node.type != TreeNode::Type::BRANCH && node.type != TreeNode::Type::TWIG) continue;

        float dx = node.position.x - spider->position.x;
        float dy = node.position.y - spider->position.y;
        float dz = node.position.z - spider->position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);

        if (dist > 0.3f && dist < 2.0f) {
            candidates.push_back(&node);
        }
    }

    // Select anchors forming rough polygon
    for (size_t i = 0; i < std::min((size_t)4, candidates.size()); ++i) {
        WebAnchor anchor;
        anchor.position = candidates[i]->position;
        anchor.nodeID = candidates[i]->id;
        anchor.strength = 1.0f;
        anchors.push_back(anchor);
    }

    return anchors;
}

XMFLOAT3 TreeSpiderBehavior::getWebCenter(const std::vector<WebAnchor>& anchors) {
    if (anchors.empty()) return { 0, 0, 0 };

    XMFLOAT3 center = { 0, 0, 0 };
    for (const auto& anchor : anchors) {
        center.x += anchor.position.x;
        center.y += anchor.position.y;
        center.z += anchor.position.z;
    }

    float count = static_cast<float>(anchors.size());
    return {
        center.x / count,
        center.y / count,
        center.z / count
    };
}

bool TreeSpiderBehavior::detectWebVibration(SmallCreature* spider,
                                             const std::vector<WebAnchor>& anchors,
                                             MicroSpatialGrid& grid) {
    if (!spider || anchors.empty()) return false;

    XMFLOAT3 webCenter = getWebCenter(anchors);

    // Check for flying insects near web
    auto prey = grid.findNearest(webCenter, 1.0f,
        [](SmallCreature* c) {
            return c->isAlive() && isFlyingInsect(c->type);
        });

    return prey != nullptr;
}

// =============================================================================
// CaterpillarBehavior Implementation
// =============================================================================

void CaterpillarBehavior::consumeLeaf(SmallCreature* caterpillar, TreeNode* leafNode) {
    if (!caterpillar || !leafNode) return;

    float consumption = 0.1f * caterpillar->genome->metabolism;
    leafNode->foodValue -= consumption;
    caterpillar->energy += consumption * 10.0f;
}

XMFLOAT3 CaterpillarBehavior::findPupationSite(SmallCreature* caterpillar,
                                                 NavigableTree* tree) {
    if (!caterpillar || !tree) return { 0, 0, 0 };

    // Find protected spot under branch
    for (auto& node : tree->nodes) {
        if (node.type != TreeNode::Type::BRANCH) continue;

        // Check if sheltered
        bool hasCover = false;
        for (auto& other : tree->nodes) {
            if (other.position.y > node.position.y &&
                fabsf(other.position.x - node.position.x) < 0.5f &&
                fabsf(other.position.z - node.position.z) < 0.5f) {
                hasCover = true;
                break;
            }
        }

        if (hasCover) {
            return node.position;
        }
    }

    return caterpillar->position;
}

TreeNest* CaterpillarBehavior::createCocoon(SmallCreature* caterpillar,
                                             TreeDwellerSystem& system,
                                             NavigableTree* tree) {
    if (!caterpillar || !tree) return nullptr;

    return system.createNest(caterpillar, tree, NestType::COCOON);
}

} // namespace small
