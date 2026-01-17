#pragma once

#include "Gene.h"
#include <vector>
#include <utility>
#include <cstdint>

namespace genetics {

// A chromosome is an ordered collection of genes
class Chromosome {
public:
    Chromosome();
    Chromosome(uint32_t id, int numGenes);

    // Initialize with specific gene types
    void initializeGenes(const std::vector<GeneType>& geneTypes);

    // Getters
    uint32_t getId() const { return id; }
    size_t getGeneCount() const { return genes.size(); }
    const Gene& getGene(size_t index) const { return genes[index]; }
    Gene& getGene(size_t index) { return genes[index]; }
    const std::vector<Gene>& getGenes() const { return genes; }
    std::vector<Gene>& getGenes() { return genes; }
    float getRecombinationRate() const { return recombinationRate; }

    // Setters
    void setRecombinationRate(float rate) { recombinationRate = rate; }

    // Add genes
    void addGene(const Gene& gene);

    // Crossover during meiosis - returns two recombined chromosomes
    std::pair<Chromosome, Chromosome> recombine(const Chromosome& other) const;

    // Single crossover point
    std::pair<Chromosome, Chromosome> singleCrossover(const Chromosome& other) const;

    // Double crossover (more realistic)
    std::pair<Chromosome, Chromosome> doubleCrossover(const Chromosome& other) const;

    // Mutation operators
    void applyPointMutation(size_t geneIndex, float strength);
    void applyInsertion(size_t position, const Gene& newGene);
    void applyDeletion(size_t geneIndex);
    void applyDuplication(size_t geneIndex);
    void applyInversion(size_t start, size_t end);

    // Get gene by type (returns first matching)
    const Gene* getGeneByType(GeneType type) const;
    Gene* getGeneByType(GeneType type);

    // Calculate total genetic distance to another chromosome
    float distanceTo(const Chromosome& other) const;

private:
    uint32_t id;
    std::vector<Gene> genes;
    float recombinationRate;  // Probability of crossover per gene

    static uint32_t nextId;
};

} // namespace genetics
