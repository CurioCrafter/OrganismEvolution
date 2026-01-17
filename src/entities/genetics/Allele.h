#pragma once

#include <cstdint>
#include <string>

namespace genetics {

// Types of mutations that can create new alleles
enum class MutationType {
    NONE,
    POINT_SILENT,
    POINT_MISSENSE,
    POINT_NONSENSE,
    INSERTION,
    DELETION,
    DUPLICATION,
    INVERSION,
    REGULATORY
};

// An allele is a variant of a gene at a particular locus
class Allele {
public:
    Allele();
    Allele(float value, float dominanceCoeff = 0.5f);

    // Create mutated copy
    Allele mutate(float strength) const;

    // Getters
    uint32_t getId() const { return id; }
    float getValue() const { return value; }
    float getDominanceCoeff() const { return dominanceCoeff; }
    float getFitnessEffect() const { return fitnessEffect; }
    float getExpressionMod() const { return expressionMod; }
    bool isDeleterious() const { return deleterious; }
    MutationType getOrigin() const { return origin; }

    // Setters
    void setValue(float v) { value = v; }
    void setDominanceCoeff(float d) { dominanceCoeff = d; }
    void setFitnessEffect(float f) { fitnessEffect = f; }
    void setDeleterious(bool d) { deleterious = d; }
    void setExpressionMod(float e) { expressionMod = e; }

    // Comparison
    bool operator==(const Allele& other) const { return id == other.id; }
    bool operator!=(const Allele& other) const { return id != other.id; }

    // Calculate phenotypic value when combined with another allele
    static float calculatePhenotype(const Allele& a1, const Allele& a2);

private:
    uint32_t id;
    float value;            // The allele's effect on the trait
    float dominanceCoeff;   // 0 = recessive, 0.5 = additive, 1 = dominant
    float fitnessEffect;    // Selection coefficient (-1 to 1)
    float expressionMod;    // Modifier to gene expression level
    bool deleterious;       // Whether this is harmful
    MutationType origin;    // How this allele arose

    static uint32_t nextId;
};

} // namespace genetics
