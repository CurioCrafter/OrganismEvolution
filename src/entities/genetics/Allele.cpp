#include "Allele.h"
#include "../../utils/Random.h"
#include <algorithm>
#include <cmath>

namespace genetics {

uint32_t Allele::nextId = 1;

Allele::Allele()
    : id(nextId++), value(0.0f), dominanceCoeff(0.5f),
      fitnessEffect(0.0f), expressionMod(1.0f),
      deleterious(false), origin(MutationType::NONE) {
}

Allele::Allele(float value, float dominanceCoeff)
    : id(nextId++), value(value), dominanceCoeff(dominanceCoeff),
      fitnessEffect(0.0f), expressionMod(1.0f),
      deleterious(false), origin(MutationType::NONE) {
}

Allele Allele::mutate(float strength) const {
    Allele mutant = *this;
    mutant.id = nextId++;

    float roll = Random::value();

    if (roll < 0.25f) {
        // Silent mutation - no phenotypic change
        mutant.origin = MutationType::POINT_SILENT;
        // Slight drift in expression
        mutant.expressionMod += Random::range(-0.02f, 0.02f);
    }
    else if (roll < 0.65f) {
        // Missense - small value change
        mutant.origin = MutationType::POINT_MISSENSE;
        mutant.value += Random::range(-strength, strength);

        // Small chance of fitness effect
        if (Random::chance(0.3f)) {
            mutant.fitnessEffect += Random::range(-0.05f, 0.02f);
            if (mutant.fitnessEffect < -0.1f) {
                mutant.deleterious = true;
            }
        }
    }
    else if (roll < 0.80f) {
        // Nonsense - severe reduction in function
        mutant.origin = MutationType::POINT_NONSENSE;
        mutant.value *= Random::range(0.1f, 0.5f);
        mutant.expressionMod *= Random::range(0.2f, 0.6f);
        mutant.fitnessEffect = Random::range(-0.3f, -0.05f);
        mutant.deleterious = true;
    }
    else if (roll < 0.90f) {
        // Regulatory mutation - expression change
        mutant.origin = MutationType::REGULATORY;
        mutant.expressionMod *= Random::range(0.5f, 1.5f);
        mutant.expressionMod = std::clamp(mutant.expressionMod, 0.1f, 2.0f);

        // Can affect fitness
        if (std::abs(mutant.expressionMod - 1.0f) > 0.3f) {
            mutant.fitnessEffect += Random::range(-0.1f, 0.05f);
        }
    }
    else {
        // Dominance change
        mutant.dominanceCoeff += Random::range(-0.2f, 0.2f);
        mutant.dominanceCoeff = std::clamp(mutant.dominanceCoeff, 0.0f, 1.0f);
    }

    return mutant;
}

float Allele::calculatePhenotype(const Allele& a1, const Allele& a2) {
    // General dominance model
    // When h = 0: phenotype = a1.value (a1 completely dominant)
    // When h = 0.5: phenotype = (a1.value + a2.value) / 2 (additive)
    // When h = 1: phenotype = a2.value (a2 completely dominant)

    float h = (a1.dominanceCoeff + a2.dominanceCoeff) / 2.0f;

    // Determine which allele is "dominant" based on coefficient
    float dominantValue, recessiveValue;
    if (a1.dominanceCoeff >= a2.dominanceCoeff) {
        dominantValue = a1.value;
        recessiveValue = a2.value;
    } else {
        dominantValue = a2.value;
        recessiveValue = a1.value;
    }

    // Phenotype calculation with dominance
    float phenotype = dominantValue * (1.0f - h) + recessiveValue * h;

    // Apply expression modifiers
    float avgExpression = (a1.expressionMod + a2.expressionMod) / 2.0f;
    phenotype *= avgExpression;

    return phenotype;
}

} // namespace genetics
