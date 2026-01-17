#include "Species.h"
#include "../Creature.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <numeric>

namespace genetics {

SpeciesId Species::nextId = 1;

// Species implementation
Species::Species()
    : id(nextId++), name("Species_" + std::to_string(id)),
      foundingLineage(0), foundingGeneration(0),
      extinct(false), extinctionGeneration(-1) {
}

Species::Species(SpeciesId id, const std::string& name)
    : id(id), name(name), foundingLineage(0), foundingGeneration(0),
      extinct(false), extinctionGeneration(-1) {
}

void Species::markExtinct(int generation) {
    extinct = true;
    extinctionGeneration = generation;
    members.clear();
}

void Species::addMember(Creature* creature) {
    if (std::find(members.begin(), members.end(), creature) == members.end()) {
        members.push_back(creature);
    }
}

void Species::removeMember(Creature* creature) {
    auto it = std::find(members.begin(), members.end(), creature);
    if (it != members.end()) {
        members.erase(it);
    }
}

void Species::updateStatistics(const std::vector<Creature*>& memberList) {
    members.clear();
    for (Creature* c : memberList) {
        if (c && c->isAlive()) {
            members.push_back(c);
        }
    }

    stats.size = static_cast<int>(members.size());

    if (stats.size == 0) {
        stats.averageHeterozygosity = 0;
        stats.averageFitness = 0;
        stats.averageGeneticLoad = 0;
        return;
    }

    // Track historical minimum for bottleneck detection
    if (stats.size < stats.historicalMinimum) {
        stats.historicalMinimum = stats.size;
        stats.generationsSinceBottleneck = 0;
    } else {
        stats.generationsSinceBottleneck++;
    }

    // Calculate averages
    float totalHet = 0, totalFit = 0, totalLoad = 0;

    for (Creature* c : members) {
        const DiploidGenome& genome = c->getDiploidGenome();
        totalHet += genome.getHeterozygosity();
        totalFit += c->getFitness();
        totalLoad += genome.getGeneticLoad();
    }

    stats.averageHeterozygosity = totalHet / stats.size;
    stats.averageFitness = totalFit / stats.size;
    stats.averageGeneticLoad = totalLoad / stats.size;

    // Effective population size (rough estimate based on variance)
    stats.effectivePopulationSize = stats.size * stats.averageHeterozygosity;

    // Update niche as average of members
    if (!members.empty()) {
        float dietSum = 0, habitatSum = 0, activitySum = 0;
        for (Creature* c : members) {
            EcologicalNiche n = c->getDiploidGenome().getEcologicalNiche();
            dietSum += n.dietSpecialization;
            habitatSum += n.habitatPreference;
            activitySum += n.activityTime;
        }
        niche.dietSpecialization = dietSum / stats.size;
        niche.habitatPreference = habitatSum / stats.size;
        niche.activityTime = activitySum / stats.size;
    }

    updateAlleleFrequencies(members);
}

float Species::getAlleleFrequency(uint32_t alleleId) const {
    auto it = alleleFrequencies.find(alleleId);
    return (it != alleleFrequencies.end()) ? it->second : 0.0f;
}

void Species::updateAlleleFrequencies(const std::vector<Creature*>& memberList) {
    alleleFrequencies.clear();

    if (memberList.empty()) return;

    std::map<uint32_t, int> alleleCounts;
    int totalAlleles = 0;

    for (Creature* c : memberList) {
        const DiploidGenome& genome = c->getDiploidGenome();

        for (size_t p = 0; p < genome.getChromosomeCount(); p++) {
            const auto& pair = genome.getChromosomePair(p);

            // Count alleles from both chromosomes in the pair (maternal and paternal)
            for (const auto& gene : pair.first.getGenes()) {
                alleleCounts[gene.getAllele1().getId()]++;
                alleleCounts[gene.getAllele2().getId()]++;
                totalAlleles += 2;
            }
            for (const auto& gene : pair.second.getGenes()) {
                alleleCounts[gene.getAllele1().getId()]++;
                alleleCounts[gene.getAllele2().getId()]++;
                totalAlleles += 2;
            }
        }
    }

    for (const auto& [alleleId, count] : alleleCounts) {
        alleleFrequencies[alleleId] = static_cast<float>(count) / totalAlleles;
    }
}

float Species::getReproductiveIsolation(SpeciesId otherId) const {
    auto it = reproductiveIsolation.find(otherId);
    return (it != reproductiveIsolation.end()) ? it->second.totalIsolation : 0.0f;
}

void Species::setReproductiveIsolation(SpeciesId otherId, float isolation) {
    IsolationData& data = reproductiveIsolation[otherId];
    data.totalIsolation = std::clamp(isolation, 0.0f, 1.0f);
}

bool Species::canInterbreedWith(const Species& other) const {
    float isolation = getReproductiveIsolation(other.id);
    return isolation < 0.9f;  // Allow some interbreeding unless strongly isolated
}

DiploidGenome Species::getRepresentativeGenome() const {
    if (members.empty()) {
        return DiploidGenome();
    }

    // Return genome of most typical member (closest to average)
    Creature* bestMatch = members[0];
    float bestScore = std::numeric_limits<float>::max();

    for (Creature* c : members) {
        float score = 0;
        const DiploidGenome& g = c->getDiploidGenome();

        // Score based on deviation from species averages
        score += std::abs(g.getHeterozygosity() - stats.averageHeterozygosity);
        score += std::abs(g.getGeneticLoad() - stats.averageGeneticLoad);

        if (score < bestScore) {
            bestScore = score;
            bestMatch = c;
        }
    }

    return bestMatch->getDiploidGenome();
}

float Species::distanceTo(const Species& other) const {
    if (members.empty() || other.members.empty()) {
        return 1.0f;
    }

    // Compare representative genomes
    DiploidGenome rep1 = getRepresentativeGenome();
    DiploidGenome rep2 = other.getRepresentativeGenome();

    return rep1.distanceTo(rep2);
}

glm::vec3 Species::getColor() const {
    // Generate unique color based on species ID
    float hue = fmod(id * 137.508f, 360.0f) / 360.0f;

    // HSV to RGB conversion
    float s = 0.8f, v = 0.9f;
    int hi = static_cast<int>(hue * 6);
    float f = hue * 6 - hi;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (hi % 6) {
        case 0: return glm::vec3(v, t, p);
        case 1: return glm::vec3(q, v, p);
        case 2: return glm::vec3(p, v, t);
        case 3: return glm::vec3(p, q, v);
        case 4: return glm::vec3(t, p, v);
        case 5: return glm::vec3(v, p, q);
        default: return glm::vec3(1.0f);
    }
}

// PhylogeneticTree implementation
PhylogeneticTree::PhylogeneticTree()
    : rootId(0), nextNodeId(1) {
}

uint64_t PhylogeneticTree::addRoot(SpeciesId speciesId, int generation) {
    PhyloNode node;
    node.id = nextNodeId++;
    node.parentId = 0;
    node.speciesId = speciesId;
    node.generation = generation;
    node.branchLength = 0;
    node.isExtant = true;

    nodes[node.id] = node;
    speciesToNode[speciesId] = node.id;
    rootId = node.id;

    return node.id;
}

uint64_t PhylogeneticTree::addSpeciation(SpeciesId parentSpecies, SpeciesId childSpecies, int generation) {
    auto parentIt = speciesToNode.find(parentSpecies);
    if (parentIt == speciesToNode.end()) {
        // Parent not in tree, add as root
        return addRoot(childSpecies, generation);
    }

    // Store parent node ID before any map modifications
    uint64_t parentNodeId = parentIt->second;

    // Get parent info before modifying map (to avoid iterator/reference invalidation)
    int parentGeneration = nodes[parentNodeId].generation;

    PhyloNode childNode;
    childNode.id = nextNodeId++;
    childNode.parentId = parentNodeId;
    childNode.speciesId = childSpecies;
    childNode.generation = generation;
    childNode.branchLength = static_cast<float>(generation - parentGeneration);
    childNode.isExtant = true;

    // Insert child node first, then update parent's children
    nodes[childNode.id] = childNode;
    speciesToNode[childSpecies] = childNode.id;

    // Now safe to update parent (map won't be modified again)
    nodes[parentNodeId].childrenIds.push_back(childNode.id);

    return childNode.id;
}

void PhylogeneticTree::markExtinction(SpeciesId species, int generation) {
    auto it = speciesToNode.find(species);
    if (it != speciesToNode.end()) {
        nodes[it->second].isExtant = false;
    }
}

SpeciesId PhylogeneticTree::getMostRecentCommonAncestor(SpeciesId sp1, SpeciesId sp2) const {
    auto it1 = speciesToNode.find(sp1);
    auto it2 = speciesToNode.find(sp2);

    if (it1 == speciesToNode.end() || it2 == speciesToNode.end()) {
        return 0;
    }

    // Collect ancestors of sp1
    std::vector<SpeciesId> ancestors1;
    uint64_t nodeId = it1->second;
    while (nodeId != 0) {
        const PhyloNode* node = getNode(nodeId);
        if (!node) break;
        ancestors1.push_back(node->speciesId);
        nodeId = node->parentId;
    }

    // Walk up sp2's ancestors and find first match
    nodeId = it2->second;
    while (nodeId != 0) {
        const PhyloNode* node = getNode(nodeId);
        if (!node) break;

        if (std::find(ancestors1.begin(), ancestors1.end(), node->speciesId) != ancestors1.end()) {
            return node->speciesId;
        }
        nodeId = node->parentId;
    }

    return 0;
}

float PhylogeneticTree::getEvolutionaryDistance(SpeciesId sp1, SpeciesId sp2) const {
    auto it1 = speciesToNode.find(sp1);
    auto it2 = speciesToNode.find(sp2);

    if (it1 == speciesToNode.end() || it2 == speciesToNode.end()) {
        return std::numeric_limits<float>::max();
    }

    // Find MRCA and sum branch lengths
    SpeciesId mrca = getMostRecentCommonAncestor(sp1, sp2);
    if (mrca == 0) return std::numeric_limits<float>::max();

    float dist1 = 0, dist2 = 0;

    // Distance from sp1 to MRCA
    uint64_t nodeId = it1->second;
    while (nodeId != 0) {
        const PhyloNode* node = getNode(nodeId);
        if (!node) break;
        if (node->speciesId == mrca) break;
        dist1 += node->branchLength;
        nodeId = node->parentId;
    }

    // Distance from sp2 to MRCA
    nodeId = it2->second;
    while (nodeId != 0) {
        const PhyloNode* node = getNode(nodeId);
        if (!node) break;
        if (node->speciesId == mrca) break;
        dist2 += node->branchLength;
        nodeId = node->parentId;
    }

    return dist1 + dist2;
}

std::vector<SpeciesId> PhylogeneticTree::getDescendants(SpeciesId ancestor) const {
    std::vector<SpeciesId> descendants;

    auto it = speciesToNode.find(ancestor);
    if (it != speciesToNode.end()) {
        collectDescendants(it->second, descendants);
    }

    return descendants;
}

void PhylogeneticTree::collectDescendants(uint64_t nodeId, std::vector<SpeciesId>& result) const {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return;

    for (uint64_t childId : it->second.childrenIds) {
        auto childIt = nodes.find(childId);
        if (childIt != nodes.end()) {
            result.push_back(childIt->second.speciesId);
            collectDescendants(childId, result);
        }
    }
}

std::vector<SpeciesId> PhylogeneticTree::getExtantSpecies() const {
    std::vector<SpeciesId> extant;
    for (const auto& [nodeId, node] : nodes) {
        if (node.isExtant) {
            extant.push_back(node.speciesId);
        }
    }
    return extant;
}

int PhylogeneticTree::getSpeciationCount() const {
    return nodes.size() > 0 ? static_cast<int>(nodes.size() - 1) : 0;  // Exclude root
}

int PhylogeneticTree::getExtinctionCount() const {
    int count = 0;
    for (const auto& [nodeId, node] : nodes) {
        if (!node.isExtant) count++;
    }
    return count;
}

const PhyloNode* PhylogeneticTree::getNode(uint64_t nodeId) const {
    auto it = nodes.find(nodeId);
    return (it != nodes.end()) ? &it->second : nullptr;
}

const PhyloNode* PhylogeneticTree::getNodeForSpecies(SpeciesId speciesId) const {
    auto it = speciesToNode.find(speciesId);
    if (it == speciesToNode.end()) return nullptr;
    return getNode(it->second);
}

std::string PhylogeneticTree::toNewick() const {
    if (rootId == 0) return ";";
    return nodeToNewick(rootId) + ";";
}

std::string PhylogeneticTree::nodeToNewick(uint64_t nodeId) const {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return "";

    const PhyloNode& node = it->second;
    std::stringstream ss;

    if (!node.childrenIds.empty()) {
        ss << "(";
        for (size_t i = 0; i < node.childrenIds.size(); i++) {
            if (i > 0) ss << ",";
            ss << nodeToNewick(node.childrenIds[i]);
        }
        ss << ")";
    }

    ss << "Species_" << node.speciesId;
    if (node.branchLength > 0) {
        ss << ":" << node.branchLength;
    }

    return ss.str();
}

void PhylogeneticTree::exportNewick(const std::string& filename) const {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << toNewick() << std::endl;
        file.close();
    }
}

float PhylogeneticTree::getTreeDepth() const {
    float maxDepth = 0;
    for (const auto& [nodeId, node] : nodes) {
        float depth = 0;
        uint64_t current = nodeId;
        while (current != 0) {
            const PhyloNode* n = getNode(current);
            if (!n) break;
            depth += n->branchLength;
            current = n->parentId;
        }
        maxDepth = std::max(maxDepth, depth);
    }
    return maxDepth;
}

float PhylogeneticTree::getAverageBranchLength() const {
    if (nodes.empty()) return 0;

    float totalLength = 0;
    int count = 0;
    for (const auto& [nodeId, node] : nodes) {
        if (node.branchLength > 0) {
            totalLength += node.branchLength;
            count++;
        }
    }

    return count > 0 ? totalLength / count : 0;
}

// SpeciationTracker implementation
SpeciationTracker::SpeciationTracker()
    : speciesThreshold(0.15f),
      minPopulationForSpecies(10),
      generationsForSpeciation(50),
      speciationEvents(0),
      extinctionEvents(0) {
}

void SpeciationTracker::update(std::vector<Creature*>& creatures, int currentGeneration) {
    // Remove dead creatures from species tracking
    for (auto& sp : species) {
        if (!sp->isExtinct()) {
            std::vector<Creature*> alive;
            for (Creature* c : creatures) {
                if (c && c->isAlive() && c->getDiploidGenome().getSpeciesId() == sp->getId()) {
                    alive.push_back(c);
                }
            }
            sp->updateStatistics(alive);
        }
    }

    // Assign unassigned creatures
    for (Creature* c : creatures) {
        if (c && c->isAlive() && c->getDiploidGenome().getSpeciesId() == 0) {
            assignToSpecies(c);
        }
    }

    // Check for speciation events
    checkForSpeciation(creatures, currentGeneration);

    // Check for extinctions
    checkForExtinction(currentGeneration);
}

Species* SpeciationTracker::getSpecies(SpeciesId id) {
    for (auto& sp : species) {
        if (sp->getId() == id) return sp.get();
    }
    return nullptr;
}

const Species* SpeciationTracker::getSpecies(SpeciesId id) const {
    for (const auto& sp : species) {
        if (sp->getId() == id) return sp.get();
    }
    return nullptr;
}

std::vector<Species*> SpeciationTracker::getActiveSpecies() {
    std::vector<Species*> active;
    for (auto& sp : species) {
        if (!sp->isExtinct()) {
            active.push_back(sp.get());
        }
    }
    return active;
}

std::vector<const Species*> SpeciationTracker::getActiveSpecies() const {
    std::vector<const Species*> active;
    for (const auto& sp : species) {
        if (!sp->isExtinct()) {
            active.push_back(sp.get());
        }
    }
    return active;
}

std::vector<const Species*> SpeciationTracker::getExtinctSpecies() const {
    std::vector<const Species*> extinct;
    for (const auto& sp : species) {
        if (sp->isExtinct()) {
            extinct.push_back(sp.get());
        }
    }
    return extinct;
}

int SpeciationTracker::getActiveSpeciesCount() const {
    int count = 0;
    for (const auto& sp : species) {
        if (!sp->isExtinct()) count++;
    }
    return count;
}

int SpeciationTracker::getTotalSpeciesCount() const {
    return static_cast<int>(species.size());
}

int SpeciationTracker::getSpeciationEventCount() const {
    return speciationEvents;
}

int SpeciationTracker::getExtinctionEventCount() const {
    return extinctionEvents;
}

std::string SpeciationTracker::generateSpeciesName(int index) {
    // Generate Latin-like species names
    static const char* prefixes[] = {
        "Mega", "Micro", "Proto", "Neo", "Pseudo", "Para", "Epi",
        "Hyper", "Ultra", "Super", "Trans", "Meta"
    };
    static const char* roots[] = {
        "saurus", "therium", "morpha", "phyla", "genus", "cephalus",
        "dactyl", "pteryx", "raptor", "mimus", "venator", "cursor"
    };

    int prefixIdx = index % 12;
    int rootIdx = (index / 12) % 12;
    int num = index / 144;

    std::string name = std::string(prefixes[prefixIdx]) + roots[rootIdx];
    if (num > 0) {
        name += "_" + std::to_string(num);
    }

    return name;
}

std::vector<std::vector<float>> SpeciationTracker::buildDistanceMatrix(
    const std::vector<Creature*>& creatures) {

    size_t n = creatures.size();
    std::vector<std::vector<float>> matrix(n, std::vector<float>(n, 0.0f));

    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j < n; j++) {
            float dist = creatures[i]->getDiploidGenome().distanceTo(
                creatures[j]->getDiploidGenome());
            matrix[i][j] = dist;
            matrix[j][i] = dist;
        }
    }

    return matrix;
}

std::vector<int> SpeciationTracker::clusterByDistance(
    const std::vector<std::vector<float>>& distances) {

    size_t n = distances.size();
    if (n == 0) return {};

    std::vector<int> clusters(n, -1);
    int nextCluster = 0;

    // Simple single-linkage clustering
    for (size_t i = 0; i < n; i++) {
        if (clusters[i] >= 0) continue;

        clusters[i] = nextCluster;
        std::vector<size_t> toProcess = {i};

        while (!toProcess.empty()) {
            size_t current = toProcess.back();
            toProcess.pop_back();

            for (size_t j = 0; j < n; j++) {
                if (clusters[j] < 0 && distances[current][j] < speciesThreshold) {
                    clusters[j] = nextCluster;
                    toProcess.push_back(j);
                }
            }
        }

        nextCluster++;
    }

    return clusters;
}

void SpeciationTracker::checkForSpeciation(std::vector<Creature*>& creatures, int generation) {
    if (creatures.size() < static_cast<size_t>(minPopulationForSpecies * 2)) {
        return;  // Not enough creatures for potential speciation
    }

    // Group creatures by current species
    std::map<SpeciesId, std::vector<Creature*>> bySpecies;
    for (Creature* c : creatures) {
        if (c && c->isAlive()) {
            bySpecies[c->getDiploidGenome().getSpeciesId()].push_back(c);
        }
    }

    // Check each species for potential splits
    for (auto& [spId, members] : bySpecies) {
        if (members.size() < static_cast<size_t>(minPopulationForSpecies * 2)) {
            continue;
        }

        // Build distance matrix for this species
        auto distances = buildDistanceMatrix(members);
        auto clusters = clusterByDistance(distances);

        // Count cluster sizes
        std::map<int, std::vector<Creature*>> clusterMembers;
        for (size_t i = 0; i < members.size(); i++) {
            clusterMembers[clusters[i]].push_back(members[i]);
        }

        // Check if any cluster is large enough and distinct enough
        for (auto& [clusterId, clusterCreatures] : clusterMembers) {
            if (clusterCreatures.size() >= static_cast<size_t>(minPopulationForSpecies)) {
                // Check genetic distance from parent species
                float avgDistance = 0;
                int comparisons = 0;

                for (Creature* c1 : clusterCreatures) {
                    for (Creature* c2 : members) {
                        if (std::find(clusterCreatures.begin(), clusterCreatures.end(), c2)
                            == clusterCreatures.end()) {
                            avgDistance += c1->getDiploidGenome().distanceTo(c2->getDiploidGenome());
                            comparisons++;
                        }
                    }
                }

                if (comparisons > 0) {
                    avgDistance /= comparisons;

                    if (avgDistance > speciesThreshold) {
                        // Speciation event!
                        Species* newSpecies = createSpecies(clusterCreatures, generation);

                        if (newSpecies && spId > 0) {
                            tree.addSpeciation(spId, newSpecies->getId(), generation);
                            speciationEvents++;

                            std::cout << "[SPECIATION] " << newSpecies->getName()
                                      << " emerged from Species_" << spId
                                      << " (generation " << generation << ", "
                                      << clusterCreatures.size() << " individuals)" << std::endl;
                        }
                    }
                }
            }
        }
    }
}

void SpeciationTracker::checkForExtinction(int generation) {
    for (auto& sp : species) {
        if (!sp->isExtinct() && sp->getStats().size == 0) {
            sp->markExtinct(generation);
            tree.markExtinction(sp->getId(), generation);
            extinctionEvents++;

            std::cout << "[EXTINCTION] " << sp->getName()
                      << " went extinct (generation " << generation << ")" << std::endl;
        }
    }
}

void SpeciationTracker::assignToSpecies(Creature* creature) {
    if (!creature) return;

    DiploidGenome& genome = const_cast<DiploidGenome&>(creature->getDiploidGenome());

    // Find closest existing species
    Species* closest = nullptr;
    float closestDist = std::numeric_limits<float>::max();

    for (auto& sp : species) {
        if (sp->isExtinct()) continue;

        float dist = genome.distanceTo(sp->getRepresentativeGenome());
        if (dist < closestDist && dist < speciesThreshold) {
            closestDist = dist;
            closest = sp.get();
        }
    }

    if (closest) {
        genome.setSpeciesId(closest->getId());
        closest->addMember(creature);
    } else {
        // Create new species for this creature (if enough unassigned)
        // For now, assign to species 0 (unassigned)
        genome.setSpeciesId(0);
    }
}

Species* SpeciationTracker::createSpecies(const std::vector<Creature*>& founders, int generation,
                                          SpeciesId parentId, SpeciationCause cause) {
    if (founders.empty()) return nullptr;

    auto newSpecies = std::make_unique<Species>();
    newSpecies->setName(generateSpeciesName(static_cast<int>(species.size())));
    newSpecies->setFoundingGeneration(generation);

    if (!founders.empty()) {
        newSpecies->setFoundingLineage(founders[0]->getDiploidGenome().getLineageId());
    }

    SpeciesId newId = newSpecies->getId();

    // Assign founders to new species
    for (Creature* c : founders) {
        DiploidGenome& genome = const_cast<DiploidGenome&>(c->getDiploidGenome());
        genome.setSpeciesId(newId);
        newSpecies->addMember(c);
    }

    newSpecies->updateStatistics(founders);

    Species* ptr = newSpecies.get();
    species.push_back(std::move(newSpecies));

    // Add to tree if first species
    if (species.size() == 1) {
        tree.addRoot(newId, generation);
    }

    return ptr;
}

} // namespace genetics
