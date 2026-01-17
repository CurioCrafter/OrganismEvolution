#include "Chromosome.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>

namespace genetics {

uint32_t Chromosome::nextId = 1;

Chromosome::Chromosome()
    : id(nextId++), recombinationRate(0.02f) {
}

Chromosome::Chromosome(uint32_t id, int numGenes)
    : id(id), recombinationRate(0.02f) {
    genes.reserve(numGenes);
}

void Chromosome::initializeGenes(const std::vector<GeneType>& geneTypes) {
    genes.clear();
    genes.reserve(geneTypes.size());

    for (size_t i = 0; i < geneTypes.size(); i++) {
        genes.emplace_back(static_cast<uint32_t>(i), geneTypes[i]);
    }
}

void Chromosome::addGene(const Gene& gene) {
    genes.push_back(gene);
}

std::pair<Chromosome, Chromosome> Chromosome::recombine(const Chromosome& other) const {
    // Use double crossover by default (more realistic)
    if (Random::chance(0.7f)) {
        return doubleCrossover(other);
    } else {
        return singleCrossover(other);
    }
}

std::pair<Chromosome, Chromosome> Chromosome::singleCrossover(const Chromosome& other) const {
    Chromosome child1, child2;
    child1.recombinationRate = recombinationRate;
    child2.recombinationRate = other.recombinationRate;

    size_t minGenes = std::min(genes.size(), other.genes.size());
    if (minGenes == 0) return {child1, child2};

    // Single crossover point
    size_t crossoverPoint = Random::range(0, static_cast<int>(minGenes));

    // Build child chromosomes
    for (size_t i = 0; i < minGenes; i++) {
        if (i < crossoverPoint) {
            child1.addGene(genes[i]);
            child2.addGene(other.genes[i]);
        } else {
            child1.addGene(other.genes[i]);
            child2.addGene(genes[i]);
        }
    }

    // Handle any extra genes from longer chromosome
    for (size_t i = minGenes; i < genes.size(); i++) {
        if (Random::chance(0.5f)) child1.addGene(genes[i]);
        else child2.addGene(genes[i]);
    }
    for (size_t i = minGenes; i < other.genes.size(); i++) {
        if (Random::chance(0.5f)) child1.addGene(other.genes[i]);
        else child2.addGene(other.genes[i]);
    }

    return {child1, child2};
}

std::pair<Chromosome, Chromosome> Chromosome::doubleCrossover(const Chromosome& other) const {
    Chromosome child1, child2;
    child1.recombinationRate = (recombinationRate + other.recombinationRate) / 2.0f;
    child2.recombinationRate = child1.recombinationRate;

    size_t minGenes = std::min(genes.size(), other.genes.size());
    if (minGenes < 3) return singleCrossover(other);

    // Two crossover points
    int point1 = Random::range(1, static_cast<int>(minGenes) - 1);
    int point2 = Random::range(point1 + 1, static_cast<int>(minGenes));

    // Build child chromosomes
    bool swapped = false;
    for (size_t i = 0; i < minGenes; i++) {
        if (i == static_cast<size_t>(point1) || i == static_cast<size_t>(point2)) {
            swapped = !swapped;
        }

        if (!swapped) {
            child1.addGene(genes[i]);
            child2.addGene(other.genes[i]);
        } else {
            child1.addGene(other.genes[i]);
            child2.addGene(genes[i]);
        }
    }

    return {child1, child2};
}

void Chromosome::applyPointMutation(size_t geneIndex, float strength) {
    if (geneIndex >= genes.size()) return;
    genes[geneIndex].mutate(strength);
}

void Chromosome::applyInsertion(size_t position, const Gene& newGene) {
    if (position > genes.size()) position = genes.size();

    // Insertion can affect nearby genes
    if (position > 0 && Random::chance(0.2f)) {
        genes[position - 1].setExpressionLevel(
            genes[position - 1].getExpressionLevel() * Random::range(0.8f, 1.2f)
        );
    }

    genes.insert(genes.begin() + position, newGene);
}

void Chromosome::applyDeletion(size_t geneIndex) {
    if (geneIndex >= genes.size()) return;

    // Deletion can affect nearby genes
    if (geneIndex > 0 && Random::chance(0.3f)) {
        genes[geneIndex - 1].setExpressionLevel(
            genes[geneIndex - 1].getExpressionLevel() * Random::range(0.7f, 1.0f)
        );
    }

    genes.erase(genes.begin() + geneIndex);
}

void Chromosome::applyDuplication(size_t geneIndex) {
    if (geneIndex >= genes.size()) return;

    Gene duplicate = genes[geneIndex];

    // Duplicate may have reduced expression initially
    duplicate.setExpressionLevel(duplicate.getExpressionLevel() * Random::range(0.6f, 0.9f));

    // Insert tandem (70%) or elsewhere (30%)
    if (Random::chance(0.7f)) {
        genes.insert(genes.begin() + geneIndex + 1, duplicate);
    } else {
        size_t randomPos = Random::range(0, static_cast<int>(genes.size()));
        genes.insert(genes.begin() + randomPos, duplicate);
    }
}

void Chromosome::applyInversion(size_t start, size_t end) {
    if (start >= genes.size() || end >= genes.size() || start >= end) return;

    // Reverse the segment
    std::reverse(genes.begin() + start, genes.begin() + end + 1);

    // Breakpoints may cause expression changes
    if (Random::chance(0.3f)) {
        genes[start].setExpressionLevel(genes[start].getExpressionLevel() * 0.9f);
    }
    if (Random::chance(0.3f)) {
        genes[end].setExpressionLevel(genes[end].getExpressionLevel() * 0.9f);
    }
}

const Gene* Chromosome::getGeneByType(GeneType type) const {
    for (const auto& gene : genes) {
        if (gene.getType() == type) {
            return &gene;
        }
    }
    return nullptr;
}

Gene* Chromosome::getGeneByType(GeneType type) {
    for (auto& gene : genes) {
        if (gene.getType() == type) {
            return &gene;
        }
    }
    return nullptr;
}

float Chromosome::distanceTo(const Chromosome& other) const {
    if (genes.empty() || other.genes.empty()) return 1.0f;

    float totalDistance = 0.0f;
    size_t comparisons = 0;

    size_t minGenes = std::min(genes.size(), other.genes.size());

    for (size_t i = 0; i < minGenes; i++) {
        if (genes[i].getType() == other.genes[i].getType()) {
            // Compare allele values
            float diff1 = std::abs(genes[i].getAllele1().getValue() -
                                   other.genes[i].getAllele1().getValue());
            float diff2 = std::abs(genes[i].getAllele2().getValue() -
                                   other.genes[i].getAllele2().getValue());

            auto range = getGeneValueRange(genes[i].getType());
            float rangeSize = range.max - range.min;
            if (rangeSize > 0) {
                totalDistance += (diff1 + diff2) / (2.0f * rangeSize);
            }
            comparisons++;
        }
    }

    // Account for structural differences (different gene counts)
    float structuralDiff = std::abs(static_cast<float>(genes.size()) -
                                     static_cast<float>(other.genes.size())) /
                           std::max(genes.size(), other.genes.size());

    if (comparisons == 0) return structuralDiff + 0.5f;

    return (totalDistance / comparisons) * 0.8f + structuralDiff * 0.2f;
}

} // namespace genetics
