#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include "TreeGenerator.h"
#include "../utils/Random.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cmath>
#include <algorithm>

// ============================================================================
// Tree Species Configurations
// ============================================================================

TreeSpeciesConfig getTreeSpeciesConfig(TreeType type) {
    TreeSpeciesConfig config;
    config.type = type;
    config.isAlien = false;
    config.isBioluminescent = false;
    config.alienIntensity = 0.0f;

    switch (type) {
        // Temperate trees
        case TreeType::OAK:
            config.name = "Oak";
            config.maxAge = 500.0f;
            config.growthRate = 0.8f;
            config.maxHeight = 25.0f;
            config.canopySpread = 20.0f;
            config.minTemperature = -10.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.8f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.6f;
            config.producesFruit = true;
            config.fruitEnergyValue = 3.0f;
            config.fruitSeasonStart = 270.0f;
            config.fruitSeasonLength = 60.0f;
            config.isDeciduous = true;
            config.isEvergreen = false;
            config.hasFlowers = false;
            config.autumnColor = glm::vec3(0.8f, 0.4f, 0.1f);
            break;

        case TreeType::MAPLE:
            config.name = "Maple";
            config.maxAge = 300.0f;
            config.growthRate = 1.0f;
            config.maxHeight = 20.0f;
            config.canopySpread = 15.0f;
            config.minTemperature = -20.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.8f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.5f;
            config.producesFruit = true;
            config.fruitEnergyValue = 2.0f;
            config.fruitSeasonStart = 240.0f;
            config.fruitSeasonLength = 45.0f;
            config.isDeciduous = true;
            config.isEvergreen = false;
            config.hasFlowers = true;
            config.flowerColor = glm::vec3(0.9f, 0.9f, 0.3f);
            config.autumnColor = glm::vec3(0.95f, 0.2f, 0.05f);
            break;

        case TreeType::CHERRY_BLOSSOM:
            config.name = "Cherry Blossom";
            config.maxAge = 100.0f;
            config.growthRate = 1.2f;
            config.maxHeight = 12.0f;
            config.canopySpread = 10.0f;
            config.minTemperature = -5.0f;
            config.maxTemperature = 30.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.4f;
            config.producesFruit = true;
            config.fruitEnergyValue = 5.0f;
            config.fruitSeasonStart = 150.0f;
            config.fruitSeasonLength = 30.0f;
            config.isDeciduous = true;
            config.isEvergreen = false;
            config.hasFlowers = true;
            config.flowerColor = glm::vec3(1.0f, 0.75f, 0.8f);
            config.autumnColor = glm::vec3(0.9f, 0.5f, 0.2f);
            break;

        case TreeType::APPLE:
            config.name = "Apple";
            config.maxAge = 100.0f;
            config.growthRate = 1.1f;
            config.maxHeight = 10.0f;
            config.canopySpread = 8.0f;
            config.minTemperature = -15.0f;
            config.maxTemperature = 32.0f;
            config.minMoisture = 0.4f;
            config.maxMoisture = 0.7f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.4f;
            config.producesFruit = true;
            config.fruitEnergyValue = 8.0f;
            config.fruitSeasonStart = 210.0f;
            config.fruitSeasonLength = 60.0f;
            config.isDeciduous = true;
            config.isEvergreen = false;
            config.hasFlowers = true;
            config.flowerColor = glm::vec3(1.0f, 1.0f, 1.0f);
            config.autumnColor = glm::vec3(0.85f, 0.6f, 0.2f);
            break;

        case TreeType::PINE:
            config.name = "Pine";
            config.maxAge = 400.0f;
            config.growthRate = 0.7f;
            config.maxHeight = 30.0f;
            config.canopySpread = 8.0f;
            config.minTemperature = -30.0f;
            config.maxTemperature = 25.0f;
            config.minMoisture = 0.2f;
            config.maxMoisture = 0.7f;
            config.minElevation = 0.2f;
            config.maxElevation = 0.8f;
            config.producesFruit = true;
            config.fruitEnergyValue = 4.0f;
            config.fruitSeasonStart = 180.0f;
            config.fruitSeasonLength = 90.0f;
            config.isDeciduous = false;
            config.isEvergreen = true;
            config.hasFlowers = false;
            break;

        // Tropical trees
        case TreeType::BANYAN:
            config.name = "Banyan";
            config.maxAge = 1000.0f;
            config.growthRate = 0.6f;
            config.maxHeight = 30.0f;
            config.canopySpread = 40.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 45.0f;
            config.minMoisture = 0.6f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.3f;
            config.producesFruit = true;
            config.fruitEnergyValue = 4.0f;
            config.fruitSeasonStart = 90.0f;
            config.fruitSeasonLength = 180.0f;
            config.isDeciduous = false;
            config.isEvergreen = true;
            config.hasFlowers = false;
            break;

        case TreeType::COCONUT_PALM:
            config.name = "Coconut Palm";
            config.maxAge = 80.0f;
            config.growthRate = 1.5f;
            config.maxHeight = 25.0f;
            config.canopySpread = 6.0f;
            config.minTemperature = 20.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.15f;
            config.producesFruit = true;
            config.fruitEnergyValue = 15.0f;
            config.fruitSeasonStart = 0.0f;
            config.fruitSeasonLength = 365.0f;
            config.isDeciduous = false;
            config.isEvergreen = true;
            config.hasFlowers = true;
            config.flowerColor = glm::vec3(0.95f, 0.9f, 0.7f);
            break;

        case TreeType::BAMBOO:
            config.name = "Bamboo";
            config.maxAge = 50.0f;
            config.growthRate = 3.0f;
            config.maxHeight = 20.0f;
            config.canopySpread = 3.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 38.0f;
            config.minMoisture = 0.5f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.4f;
            config.producesFruit = false;
            config.isDeciduous = false;
            config.isEvergreen = true;
            config.hasFlowers = false;
            break;

        // Alien trees
        case TreeType::CRYSTAL_TREE:
            config.name = "Crystal Tree";
            config.isAlien = true;
            config.isBioluminescent = true;
            config.alienIntensity = 0.8f;
            config.maxAge = 10000.0f;
            config.growthRate = 0.1f;
            config.maxHeight = 15.0f;
            config.canopySpread = 8.0f;
            config.minTemperature = -50.0f;
            config.maxTemperature = 50.0f;
            config.minMoisture = 0.0f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 1.0f;
            config.producesFruit = false;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        case TreeType::BIOLUMINESCENT_TREE:
            config.name = "Bioluminescent Tree";
            config.isAlien = true;
            config.isBioluminescent = true;
            config.alienIntensity = 1.0f;
            config.maxAge = 500.0f;
            config.growthRate = 0.5f;
            config.maxHeight = 20.0f;
            config.canopySpread = 12.0f;
            config.minTemperature = 5.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.6f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.5f;
            config.producesFruit = true;
            config.fruitEnergyValue = 10.0f;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        case TreeType::FLOATING_TREE:
            config.name = "Floating Tree";
            config.isAlien = true;
            config.isBioluminescent = true;
            config.alienIntensity = 0.6f;
            config.maxAge = 200.0f;
            config.growthRate = 0.3f;
            config.maxHeight = 10.0f;
            config.canopySpread = 15.0f;
            config.minTemperature = 0.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 1.0f;
            config.producesFruit = true;
            config.fruitEnergyValue = 12.0f;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        case TreeType::TENDRIL_TREE:
            config.name = "Tendril Tree";
            config.isAlien = true;
            config.isBioluminescent = false;
            config.alienIntensity = 0.4f;
            config.maxAge = 150.0f;
            config.growthRate = 0.8f;
            config.maxHeight = 18.0f;
            config.canopySpread = 10.0f;
            config.minTemperature = 10.0f;
            config.maxTemperature = 40.0f;
            config.minMoisture = 0.7f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.4f;
            config.producesFruit = false;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        case TreeType::SPORE_TREE:
            config.name = "Spore Tree";
            config.isAlien = true;
            config.isBioluminescent = true;
            config.alienIntensity = 0.5f;
            config.maxAge = 80.0f;
            config.growthRate = 1.2f;
            config.maxHeight = 12.0f;
            config.canopySpread = 8.0f;
            config.minTemperature = 15.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.8f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.3f;
            config.producesFruit = false;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        case TreeType::PLASMA_TREE:
            config.name = "Plasma Tree";
            config.isAlien = true;
            config.isBioluminescent = true;
            config.alienIntensity = 1.5f;
            config.maxAge = 50.0f;
            config.growthRate = 2.0f;
            config.maxHeight = 8.0f;
            config.canopySpread = 6.0f;
            config.minTemperature = -100.0f;
            config.maxTemperature = 100.0f;
            config.minMoisture = 0.0f;
            config.maxMoisture = 1.0f;
            config.minElevation = 0.0f;
            config.maxElevation = 1.0f;
            config.producesFruit = false;
            config.isDeciduous = false;
            config.isEvergreen = true;
            break;

        default:
            // Default configuration for other trees
            config.name = "Tree";
            config.maxAge = 200.0f;
            config.growthRate = 1.0f;
            config.maxHeight = 15.0f;
            config.canopySpread = 10.0f;
            config.minTemperature = -10.0f;
            config.maxTemperature = 35.0f;
            config.minMoisture = 0.3f;
            config.maxMoisture = 0.8f;
            config.minElevation = 0.0f;
            config.maxElevation = 0.7f;
            config.producesFruit = false;
            config.isDeciduous = true;
            config.isEvergreen = false;
            config.hasFlowers = false;
            config.autumnColor = glm::vec3(0.7f, 0.5f, 0.2f);
            break;
    }

    return config;
}

bool isAlienTreeType(TreeType type) {
    return type == TreeType::CRYSTAL_TREE ||
           type == TreeType::BIOLUMINESCENT_TREE ||
           type == TreeType::FLOATING_TREE ||
           type == TreeType::TENDRIL_TREE ||
           type == TreeType::SPORE_TREE ||
           type == TreeType::PLASMA_TREE;
}

glm::vec3 getSeasonalLeafColor(TreeType type, TreeSeasonalState state, float progress) {
    TreeSpeciesConfig config = getTreeSpeciesConfig(type);

    // Get base leaf color
    glm::vec3 baseColor;
    switch (type) {
        case TreeType::OAK:
            baseColor = glm::vec3(0.15f, 0.5f, 0.15f);
            break;
        case TreeType::MAPLE:
            baseColor = glm::vec3(0.2f, 0.55f, 0.15f);
            break;
        case TreeType::CHERRY_BLOSSOM:
            baseColor = glm::vec3(0.2f, 0.5f, 0.18f);
            break;
        case TreeType::BIRCH:
            baseColor = glm::vec3(0.3f, 0.55f, 0.2f);
            break;
        default:
            baseColor = glm::vec3(0.2f, 0.5f, 0.2f);
    }

    switch (state) {
        case TreeSeasonalState::DORMANT:
            return glm::vec3(0.0f); // No leaves

        case TreeSeasonalState::BUDDING:
            // Light green buds emerging
            return glm::mix(glm::vec3(0.4f, 0.5f, 0.2f), baseColor, progress);

        case TreeSeasonalState::FLOWERING:
            // Mix of leaves and flowers
            if (config.hasFlowers) {
                return glm::mix(baseColor, config.flowerColor, 0.3f * (1.0f - progress));
            }
            return baseColor;

        case TreeSeasonalState::FULL_FOLIAGE:
            return baseColor;

        case TreeSeasonalState::FRUITING:
            // Slightly darker, mature leaves
            return baseColor * 0.95f;

        case TreeSeasonalState::AUTUMN_COLORS:
            // Transition to autumn colors
            return glm::mix(baseColor, config.autumnColor, progress);

        case TreeSeasonalState::LEAF_DROP:
            // Fading autumn colors
            return glm::mix(config.autumnColor, glm::vec3(0.5f, 0.35f, 0.15f), progress);

        default:
            return baseColor;
    }
}

// TurtleState rotation methods - proper 3D turtle graphics

void TurtleState::rotateU(float angle) {
    // Rotate around Up vector (yaw - turn left/right)
    float c = cos(angle);
    float s = sin(angle);

    glm::vec3 newHeading = heading * c + left * s;
    glm::vec3 newLeft = left * c - heading * s;

    heading = glm::normalize(newHeading);
    left = glm::normalize(newLeft);
}

void TurtleState::rotateL(float angle) {
    // Rotate around Left vector (pitch - tilt up/down)
    // This is THE KEY for 3D trees!
    float c = cos(angle);
    float s = sin(angle);

    glm::vec3 newHeading = heading * c - up * s;
    glm::vec3 newUp = up * c + heading * s;

    heading = glm::normalize(newHeading);
    up = glm::normalize(newUp);
}

void TurtleState::rotateH(float angle) {
    // Rotate around Heading vector (roll)
    float c = cos(angle);
    float s = sin(angle);

    glm::vec3 newLeft = left * c + up * s;
    glm::vec3 newUp = up * c - left * s;

    left = glm::normalize(newLeft);
    up = glm::normalize(newUp);
}

// L-System definitions with 3D commands - optimized for reasonable tree sizes
LSystem TreeGenerator::createLSystem(TreeType type) {
    switch (type) {
        case TreeType::OAK:
            // Oak tree - broad, spreading canopy with 3D branching
            {
                LSystem lsys("FA", 28.0f);
                lsys.addRule('A', "[&FL!A]////[&FL!A]////[&FL!A]");
                lsys.addRule('F', "S[//&F]");
                lsys.addRule('S', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::PINE:
            // Pine tree - conical shape with horizontal branches
            {
                LSystem lsys("FFA", 25.0f);
                lsys.addRule('A', "[&&B]////[&&B]////[&&B]FA");
                lsys.addRule('B', "[+F][-F]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::WILLOW:
            // Willow - drooping branches with cascading effect
            {
                LSystem lsys("FFA", 15.0f);
                lsys.addRule('A', "[&&&W]////[&&&W]////[&&&W]");
                lsys.addRule('W', "F[&&W]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::BIRCH:
            // Birch - slender trunk, delicate branching
            {
                LSystem lsys("FFA", 22.0f);
                lsys.addRule('A', "[&FL!A]///[&FL!A]///[&FL!A]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::PALM:
            // Palm - single tall trunk with fronds at top
            {
                LSystem lsys("FFFFA", 45.0f);
                lsys.addRule('A', "[&&&L][//&&&L][////&&&L][//////&&&L][////////&&&L]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::MANGROVE:
            // Mangrove - complex root structure, spreading crown
            {
                LSystem lsys("FA", 30.0f);
                lsys.addRule('A', "[&&FL!A]///[&&FL!A]///[&&FL!A]");
                lsys.addRule('F', "F[//&F]");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::KAPOK:
            // Kapok - massive emergent rainforest tree
            {
                LSystem lsys("FFFA", 25.0f);
                lsys.addRule('A', "[&FL!A]////[&FL!A]////[&FL!A]");
                lsys.addRule('F', "FF");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::CACTUS_SAGUARO:
            // Saguaro cactus - tall with arms
            {
                LSystem lsys("FFFA", 90.0f);
                lsys.addRule('A', "[+FF][−FF]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::CACTUS_BARREL:
            // Barrel cactus - short and round
            {
                LSystem lsys("F", 0.0f);
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::JOSHUA_TREE:
            // Joshua tree - spiky branching
            {
                LSystem lsys("FFA", 35.0f);
                lsys.addRule('A', "[&FL][//&FL][////&FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::SPRUCE:
            // Spruce - dense conical shape
            {
                LSystem lsys("FFA", 20.0f);
                lsys.addRule('A', "[&&B]//[&&B]//[&&B]//[&&B]FA");
                lsys.addRule('B', "[+F][-F]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::FIR:
            // Fir - similar to spruce but more layered
            {
                LSystem lsys("FFFA", 18.0f);
                lsys.addRule('A', "[&&B]///[&&B]///[&&B]FA");
                lsys.addRule('B', "[+FF][-FF]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::ACACIA:
            // Acacia - flat umbrella canopy
            {
                LSystem lsys("FFFA", 12.0f);
                lsys.addRule('A', "[&&FL][//&&FL][////&&FL][//////&&FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::BAOBAB:
            // Baobab - thick trunk, sparse branches
            {
                LSystem lsys("FA", 40.0f);
                lsys.addRule('A', "[&FL][//&FL][////&FL]");
                lsys.addRule('F', "FF");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::CYPRESS:
            // Cypress - tall, narrow, swamp-adapted
            {
                LSystem lsys("FFFA", 15.0f);
                lsys.addRule('A', "[&FL]//[&FL]//[&FL]A");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::JUNIPER:
            // Juniper - gnarled, twisted
            {
                LSystem lsys("FA", 35.0f);
                lsys.addRule('A', "[+&FL!A][-&FL!A]");
                lsys.addRule('F', "F/F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::ALPINE_FIR:
            // Alpine fir - compact, wind-shaped
            {
                LSystem lsys("FFA", 22.0f);
                lsys.addRule('A', "[&&B]//[&&B]//[&&B]FA");
                lsys.addRule('B', "[+F][-F]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::BUSH:
            // Bush - dense, rounded, many short branches
            {
                LSystem lsys("FFFA", 35.0f);
                lsys.addRule('A', "[+FB][-FB][&FB][^FB]//A");
                lsys.addRule('B', "[+F][-F]");
                lsys.addRule('F', "F");
                return lsys;
            }

        // New temperate trees
        case TreeType::MAPLE:
            // Maple - broad canopy with palmate branching
            {
                LSystem lsys("FA", 32.0f);
                lsys.addRule('A', "[&FL!A]///[&FL!A]///[&FL!A]///[&FL!A]");
                lsys.addRule('F', "S[//&F]");
                lsys.addRule('S', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::CHERRY_BLOSSOM:
            // Cherry blossom - elegant spreading branches
            {
                LSystem lsys("FFA", 25.0f);
                lsys.addRule('A', "[&&FL!A]//[&&FL!A]//[&&FL!A]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::APPLE:
            // Apple tree - compact, fruit-bearing
            {
                LSystem lsys("FA", 30.0f);
                lsys.addRule('A', "[&FL!A]////[&FL!A]////[&FL!A]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        // New tropical trees
        case TreeType::BANYAN:
            // Banyan - massive spreading with aerial roots
            {
                LSystem lsys("FFA", 20.0f);
                lsys.addRule('A', "[&&FL!A][//&&FL!A][////&&FL!A][//////&&FL!A]");
                lsys.addRule('F', "FF[//&F]");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::COCONUT_PALM:
            // Coconut palm - tall single trunk, large fronds
            {
                LSystem lsys("FFFFFA", 50.0f);
                lsys.addRule('A', "[&&&L][//&&&L][////&&&L][//////&&&L][////////&&&L][//////////&&&L]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::BAMBOO:
            // Bamboo - segmented vertical growth
            {
                LSystem lsys("FFFFFFFF", 5.0f);
                lsys.addRule('F', "F[+L][-L]");
                lsys.addRule('L', "");
                return lsys;
            }

        // New desert trees
        case TreeType::PRICKLY_PEAR:
            // Prickly pear cactus - paddle-like segments
            {
                LSystem lsys("FA", 45.0f);
                lsys.addRule('A', "[&FA][//&FA]");
                lsys.addRule('F', "F");
                return lsys;
            }

        // New boreal
        case TreeType::LARCH:
            // Larch - deciduous conifer
            {
                LSystem lsys("FFFA", 22.0f);
                lsys.addRule('A', "[&&B]///[&&B]///[&&B]FA");
                lsys.addRule('B', "[+FL][-FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        // New savanna
        case TreeType::UMBRELLA_THORN:
            // Umbrella thorn - flat top like acacia
            {
                LSystem lsys("FFFA", 10.0f);
                lsys.addRule('A', "[&&FL][//&&FL][////&&FL][//////&&FL][////////&&FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        // New swamp trees
        case TreeType::BALD_CYPRESS:
            // Bald cypress - buttressed base, feathery foliage
            {
                LSystem lsys("FFFFA", 18.0f);
                lsys.addRule('A', "[&FL]//[&FL]//[&FL]//[&FL]A");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::WATER_OAK:
            // Water oak - spreading branches
            {
                LSystem lsys("FFA", 26.0f);
                lsys.addRule('A', "[&FL!A]////[&FL!A]////[&FL!A]");
                lsys.addRule('F', "F[//&F]");
                lsys.addRule('L', "");
                return lsys;
            }

        // New mountain trees
        case TreeType::MOUNTAIN_ASH:
            // Mountain ash - compound leaves, berries
            {
                LSystem lsys("FFA", 28.0f);
                lsys.addRule('A', "[&FL!A]///[&FL!A]///[&FL!A]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::BRISTLECONE_PINE:
            // Bristlecone pine - ancient, gnarled
            {
                LSystem lsys("FFA", 40.0f);
                lsys.addRule('A', "[+&FL!A][-&FL!A]");
                lsys.addRule('F', "F/F\\F");
                lsys.addRule('L', "");
                return lsys;
            }

        // Bush variants
        case TreeType::FLOWERING_BUSH:
            {
                LSystem lsys("FFFA", 38.0f);
                lsys.addRule('A', "[+FB][-FB][&FB][^FB]//A");
                lsys.addRule('B', "[+FL][-FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::BERRY_BUSH:
            {
                LSystem lsys("FFA", 32.0f);
                lsys.addRule('A', "[+FB][-FB][&FB][^FB]//A");
                lsys.addRule('B', "[+FL][-FL]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        // Alien trees - unique L-systems for bizarre shapes
        case TreeType::CRYSTAL_TREE:
            // Crystal - angular, geometric branching
            {
                LSystem lsys("FA", 60.0f);
                lsys.addRule('A', "[+FA][-FA][&FA][^FA]");
                lsys.addRule('F', "FF");
                return lsys;
            }

        case TreeType::BIOLUMINESCENT_TREE:
            // Bioluminescent - organic with glowing nodes
            {
                LSystem lsys("FFA", 30.0f);
                lsys.addRule('A', "[&FL!A]////[&FL!A]////[&FL!A]");
                lsys.addRule('F', "F[//&F]");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::FLOATING_TREE:
            // Floating - inverted canopy, hanging roots
            {
                LSystem lsys("FA", 25.0f);
                lsys.addRule('A', "[^FL!A]////[^FL!A]////[^FL!A]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::TENDRIL_TREE:
            // Tendril - many twisting appendages
            {
                LSystem lsys("FFFA", 20.0f);
                lsys.addRule('A', "[&T]//[&T]//[&T]//[&T]//[&T]");
                lsys.addRule('T', "F/F\\F/F[&T]");
                lsys.addRule('F', "F");
                return lsys;
            }

        case TreeType::SPORE_TREE:
            // Spore - mushroom-like with cap
            {
                LSystem lsys("FFFFA", 15.0f);
                lsys.addRule('A', "[&&L][//&&L][////&&L][//////&&L][////////&&L]");
                lsys.addRule('F', "F");
                lsys.addRule('L', "");
                return lsys;
            }

        case TreeType::PLASMA_TREE:
            // Plasma - chaotic energy patterns
            {
                LSystem lsys("FA", 45.0f);
                lsys.addRule('A', "[+&FA][-^FA][/&FA][\\^FA]");
                lsys.addRule('F', "F");
                return lsys;
            }

        default:
            break;
    }

    // Default fallback - simple 3D tree
    LSystem lsys("FFA", 25.0f);
    lsys.addRule('A', "[&FA]////[&FA]");
    lsys.addRule('F', "F");
    return lsys;
}

MeshData TreeGenerator::generateTree(TreeType type, unsigned int seed) {
    // Use seed for variation
    if (seed != 0) {
        Random::init();
    }

    LSystem lsys = createLSystem(type);

    // Different iterations for different tree types
    int iterations;
    float baseRadius;
    float segmentLength;

    switch (type) {
        case TreeType::OAK:
            iterations = 3;
            baseRadius = 0.08f + Random::value() * 0.04f;
            segmentLength = 0.4f + Random::value() * 0.2f;
            break;

        case TreeType::PINE:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.5f + Random::value() * 0.2f;
            break;

        case TreeType::WILLOW:
            iterations = 3;
            baseRadius = 0.06f + Random::value() * 0.03f;
            segmentLength = 0.35f + Random::value() * 0.15f;
            break;

        case TreeType::BIRCH:
            iterations = 3;
            baseRadius = 0.04f + Random::value() * 0.02f;
            segmentLength = 0.45f + Random::value() * 0.15f;
            break;

        case TreeType::PALM:
            iterations = 2;
            baseRadius = 0.12f + Random::value() * 0.04f;
            segmentLength = 0.8f + Random::value() * 0.3f;
            break;

        case TreeType::MANGROVE:
            iterations = 3;
            baseRadius = 0.06f + Random::value() * 0.03f;
            segmentLength = 0.3f + Random::value() * 0.15f;
            break;

        case TreeType::KAPOK:
            iterations = 3;
            baseRadius = 0.15f + Random::value() * 0.08f;
            segmentLength = 0.6f + Random::value() * 0.3f;
            break;

        case TreeType::CACTUS_SAGUARO:
            iterations = 2;
            baseRadius = 0.2f + Random::value() * 0.1f;
            segmentLength = 0.8f + Random::value() * 0.4f;
            break;

        case TreeType::CACTUS_BARREL:
            iterations = 1;
            baseRadius = 0.3f + Random::value() * 0.15f;
            segmentLength = 0.4f + Random::value() * 0.2f;
            break;

        case TreeType::JOSHUA_TREE:
            iterations = 3;
            baseRadius = 0.08f + Random::value() * 0.04f;
            segmentLength = 0.35f + Random::value() * 0.15f;
            break;

        case TreeType::SPRUCE:
            iterations = 4;
            baseRadius = 0.06f + Random::value() * 0.03f;
            segmentLength = 0.4f + Random::value() * 0.15f;
            break;

        case TreeType::FIR:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.45f + Random::value() * 0.15f;
            break;

        case TreeType::ACACIA:
            iterations = 2;
            baseRadius = 0.1f + Random::value() * 0.05f;
            segmentLength = 0.5f + Random::value() * 0.25f;
            break;

        case TreeType::BAOBAB:
            iterations = 2;
            baseRadius = 0.25f + Random::value() * 0.15f;
            segmentLength = 0.4f + Random::value() * 0.2f;
            break;

        case TreeType::CYPRESS:
            iterations = 3;
            baseRadius = 0.07f + Random::value() * 0.03f;
            segmentLength = 0.5f + Random::value() * 0.2f;
            break;

        case TreeType::JUNIPER:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.25f + Random::value() * 0.15f;
            break;

        case TreeType::ALPINE_FIR:
            iterations = 3;
            baseRadius = 0.04f + Random::value() * 0.02f;
            segmentLength = 0.35f + Random::value() * 0.15f;
            break;

        case TreeType::BUSH:
            iterations = 3;
            baseRadius = 0.03f + Random::value() * 0.02f;
            segmentLength = 0.15f + Random::value() * 0.1f;
            break;

        // New temperate trees
        case TreeType::MAPLE:
            iterations = 3;
            baseRadius = 0.07f + Random::value() * 0.04f;
            segmentLength = 0.38f + Random::value() * 0.2f;
            break;

        case TreeType::CHERRY_BLOSSOM:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.3f + Random::value() * 0.15f;
            break;

        case TreeType::APPLE:
            iterations = 3;
            baseRadius = 0.06f + Random::value() * 0.03f;
            segmentLength = 0.25f + Random::value() * 0.12f;
            break;

        // New tropical trees
        case TreeType::BANYAN:
            iterations = 3;
            baseRadius = 0.12f + Random::value() * 0.06f;
            segmentLength = 0.45f + Random::value() * 0.25f;
            break;

        case TreeType::COCONUT_PALM:
            iterations = 2;
            baseRadius = 0.15f + Random::value() * 0.05f;
            segmentLength = 1.0f + Random::value() * 0.4f;
            break;

        case TreeType::BAMBOO:
            iterations = 2;
            baseRadius = 0.04f + Random::value() * 0.02f;
            segmentLength = 0.6f + Random::value() * 0.3f;
            break;

        // New desert
        case TreeType::PRICKLY_PEAR:
            iterations = 3;
            baseRadius = 0.15f + Random::value() * 0.08f;
            segmentLength = 0.3f + Random::value() * 0.15f;
            break;

        // New boreal
        case TreeType::LARCH:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.42f + Random::value() * 0.18f;
            break;

        // New savanna
        case TreeType::UMBRELLA_THORN:
            iterations = 2;
            baseRadius = 0.08f + Random::value() * 0.04f;
            segmentLength = 0.55f + Random::value() * 0.25f;
            break;

        // New swamp
        case TreeType::BALD_CYPRESS:
            iterations = 3;
            baseRadius = 0.1f + Random::value() * 0.05f;
            segmentLength = 0.5f + Random::value() * 0.2f;
            break;

        case TreeType::WATER_OAK:
            iterations = 3;
            baseRadius = 0.07f + Random::value() * 0.04f;
            segmentLength = 0.35f + Random::value() * 0.18f;
            break;

        // New mountain
        case TreeType::MOUNTAIN_ASH:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.32f + Random::value() * 0.15f;
            break;

        case TreeType::BRISTLECONE_PINE:
            iterations = 3;
            baseRadius = 0.06f + Random::value() * 0.04f;
            segmentLength = 0.2f + Random::value() * 0.12f;
            break;

        // Bush variants
        case TreeType::FLOWERING_BUSH:
            iterations = 3;
            baseRadius = 0.025f + Random::value() * 0.015f;
            segmentLength = 0.12f + Random::value() * 0.08f;
            break;

        case TreeType::BERRY_BUSH:
            iterations = 3;
            baseRadius = 0.03f + Random::value() * 0.02f;
            segmentLength = 0.14f + Random::value() * 0.08f;
            break;

        // Alien trees
        case TreeType::CRYSTAL_TREE:
            iterations = 3;
            baseRadius = 0.08f + Random::value() * 0.05f;
            segmentLength = 0.35f + Random::value() * 0.2f;
            break;

        case TreeType::BIOLUMINESCENT_TREE:
            iterations = 3;
            baseRadius = 0.07f + Random::value() * 0.04f;
            segmentLength = 0.4f + Random::value() * 0.2f;
            break;

        case TreeType::FLOATING_TREE:
            iterations = 3;
            baseRadius = 0.06f + Random::value() * 0.03f;
            segmentLength = 0.3f + Random::value() * 0.15f;
            break;

        case TreeType::TENDRIL_TREE:
            iterations = 3;
            baseRadius = 0.05f + Random::value() * 0.03f;
            segmentLength = 0.25f + Random::value() * 0.12f;
            break;

        case TreeType::SPORE_TREE:
            iterations = 2;
            baseRadius = 0.12f + Random::value() * 0.06f;
            segmentLength = 0.5f + Random::value() * 0.25f;
            break;

        case TreeType::PLASMA_TREE:
            iterations = 3;
            baseRadius = 0.04f + Random::value() * 0.03f;
            segmentLength = 0.2f + Random::value() * 0.12f;
            break;

        default:
            iterations = 3;
            baseRadius = 0.05f;
            segmentLength = 0.4f;
    }

    std::string lString = lsys.generate(iterations);

    MeshData mesh = interpretLSystem(lString, lsys.getAngle(), baseRadius, segmentLength, type);

    return mesh;
}

MeshData TreeGenerator::generateBush(unsigned int seed) {
    return generateTree(TreeType::BUSH, seed);
}

MeshData TreeGenerator::interpretLSystem(
    const std::string& lString,
    float angle,
    float baseRadius,
    float segmentLength,
    TreeType type
) {
    MeshData mesh;
    std::stack<TurtleState> stateStack;

    TurtleState turtle;
    turtle.radius = baseRadius;

    float angleRad = glm::radians(angle);

    // Bark color variations
    glm::vec3 barkColor;
    switch (type) {
        case TreeType::OAK:
            barkColor = glm::vec3(0.35f, 0.25f, 0.15f);  // Dark brown
            break;
        case TreeType::PINE:
            barkColor = glm::vec3(0.4f, 0.28f, 0.18f);   // Reddish brown
            break;
        case TreeType::WILLOW:
            barkColor = glm::vec3(0.45f, 0.38f, 0.25f);  // Light brown
            break;
        case TreeType::BIRCH:
            barkColor = glm::vec3(0.85f, 0.82f, 0.78f);  // White birch bark
            break;
        case TreeType::PALM:
            barkColor = glm::vec3(0.5f, 0.4f, 0.3f);     // Tan trunk
            break;
        case TreeType::MANGROVE:
            barkColor = glm::vec3(0.35f, 0.3f, 0.2f);    // Dark greenish brown
            break;
        case TreeType::KAPOK:
            barkColor = glm::vec3(0.45f, 0.4f, 0.35f);   // Gray-brown
            break;
        case TreeType::CACTUS_SAGUARO:
        case TreeType::CACTUS_BARREL:
            barkColor = glm::vec3(0.3f, 0.5f, 0.25f);    // Cactus green
            break;
        case TreeType::JOSHUA_TREE:
            barkColor = glm::vec3(0.45f, 0.38f, 0.28f);  // Desert tan
            break;
        case TreeType::SPRUCE:
        case TreeType::FIR:
        case TreeType::ALPINE_FIR:
            barkColor = glm::vec3(0.35f, 0.25f, 0.2f);   // Reddish brown
            break;
        case TreeType::ACACIA:
            barkColor = glm::vec3(0.4f, 0.32f, 0.22f);   // Tan-brown
            break;
        case TreeType::BAOBAB:
            barkColor = glm::vec3(0.55f, 0.48f, 0.42f);  // Gray-tan
            break;
        case TreeType::CYPRESS:
            barkColor = glm::vec3(0.38f, 0.32f, 0.25f);  // Dark reddish
            break;
        case TreeType::JUNIPER:
            barkColor = glm::vec3(0.42f, 0.35f, 0.28f);  // Weathered brown
            break;
        case TreeType::BUSH:
            barkColor = glm::vec3(0.3f, 0.22f, 0.12f);   // Dark
            break;

        // New temperate trees
        case TreeType::MAPLE:
            barkColor = glm::vec3(0.4f, 0.32f, 0.22f);   // Gray-brown
            break;
        case TreeType::CHERRY_BLOSSOM:
            barkColor = glm::vec3(0.45f, 0.3f, 0.25f);   // Dark reddish brown
            break;
        case TreeType::APPLE:
            barkColor = glm::vec3(0.38f, 0.28f, 0.18f);  // Brown
            break;

        // New tropical
        case TreeType::BANYAN:
            barkColor = glm::vec3(0.5f, 0.45f, 0.38f);   // Light gray-brown
            break;
        case TreeType::COCONUT_PALM:
            barkColor = glm::vec3(0.55f, 0.42f, 0.32f);  // Light tan
            break;
        case TreeType::BAMBOO:
            barkColor = glm::vec3(0.45f, 0.55f, 0.35f);  // Green-tan
            break;

        // New desert
        case TreeType::PRICKLY_PEAR:
            barkColor = glm::vec3(0.25f, 0.45f, 0.2f);   // Cactus green
            break;

        // New boreal
        case TreeType::LARCH:
            barkColor = glm::vec3(0.4f, 0.3f, 0.22f);    // Brown
            break;

        // New savanna
        case TreeType::UMBRELLA_THORN:
            barkColor = glm::vec3(0.35f, 0.28f, 0.18f);  // Dark brown
            break;

        // New swamp
        case TreeType::BALD_CYPRESS:
            barkColor = glm::vec3(0.42f, 0.35f, 0.28f);  // Reddish-gray
            break;
        case TreeType::WATER_OAK:
            barkColor = glm::vec3(0.38f, 0.3f, 0.2f);    // Dark brown
            break;

        // New mountain
        case TreeType::MOUNTAIN_ASH:
            barkColor = glm::vec3(0.5f, 0.42f, 0.35f);   // Smooth gray
            break;
        case TreeType::BRISTLECONE_PINE:
            barkColor = glm::vec3(0.45f, 0.38f, 0.32f);  // Weathered gray-brown
            break;

        // Bush variants
        case TreeType::FLOWERING_BUSH:
        case TreeType::BERRY_BUSH:
            barkColor = glm::vec3(0.32f, 0.24f, 0.14f);  // Dark brown
            break;

        // Alien trees - unique colors
        case TreeType::CRYSTAL_TREE:
            barkColor = glm::vec3(0.6f, 0.7f, 0.85f);    // Crystalline blue-white
            break;
        case TreeType::BIOLUMINESCENT_TREE:
            barkColor = glm::vec3(0.2f, 0.35f, 0.25f);   // Dark green-black
            break;
        case TreeType::FLOATING_TREE:
            barkColor = glm::vec3(0.5f, 0.45f, 0.55f);   // Purple-gray
            break;
        case TreeType::TENDRIL_TREE:
            barkColor = glm::vec3(0.35f, 0.25f, 0.3f);   // Dark purple-brown
            break;
        case TreeType::SPORE_TREE:
            barkColor = glm::vec3(0.45f, 0.35f, 0.25f);  // Tan-brown
            break;
        case TreeType::PLASMA_TREE:
            barkColor = glm::vec3(0.15f, 0.15f, 0.2f);   // Near-black with purple tint
            break;

        default:
            barkColor = glm::vec3(0.4f, 0.3f, 0.2f);
    }

    // Leaf color variations
    glm::vec3 baseLeafColor;
    switch (type) {
        case TreeType::OAK:
            baseLeafColor = glm::vec3(0.15f, 0.5f, 0.15f);   // Rich green
            break;
        case TreeType::PINE:
            baseLeafColor = glm::vec3(0.1f, 0.35f, 0.15f);   // Dark green
            break;
        case TreeType::WILLOW:
            baseLeafColor = glm::vec3(0.35f, 0.55f, 0.25f);  // Yellow-green
            break;
        case TreeType::BIRCH:
            baseLeafColor = glm::vec3(0.3f, 0.55f, 0.2f);    // Light green
            break;
        case TreeType::PALM:
            baseLeafColor = glm::vec3(0.2f, 0.5f, 0.15f);    // Palm frond green
            break;
        case TreeType::MANGROVE:
            baseLeafColor = glm::vec3(0.15f, 0.45f, 0.15f);  // Dark tropical green
            break;
        case TreeType::KAPOK:
            baseLeafColor = glm::vec3(0.18f, 0.48f, 0.18f);  // Deep green
            break;
        case TreeType::CACTUS_SAGUARO:
        case TreeType::CACTUS_BARREL:
            baseLeafColor = glm::vec3(0.3f, 0.5f, 0.25f);    // Cactus green (no leaves)
            break;
        case TreeType::JOSHUA_TREE:
            baseLeafColor = glm::vec3(0.35f, 0.45f, 0.25f);  // Spiky yellow-green
            break;
        case TreeType::SPRUCE:
        case TreeType::FIR:
        case TreeType::ALPINE_FIR:
            baseLeafColor = glm::vec3(0.1f, 0.32f, 0.15f);   // Dark conifer green
            break;
        case TreeType::ACACIA:
            baseLeafColor = glm::vec3(0.25f, 0.5f, 0.2f);    // Savanna green
            break;
        case TreeType::BAOBAB:
            baseLeafColor = glm::vec3(0.22f, 0.45f, 0.18f);  // Sparse green
            break;
        case TreeType::CYPRESS:
            baseLeafColor = glm::vec3(0.15f, 0.4f, 0.2f);    // Swamp green
            break;
        case TreeType::JUNIPER:
            baseLeafColor = glm::vec3(0.2f, 0.38f, 0.22f);   // Blue-green
            break;
        case TreeType::BUSH:
            baseLeafColor = glm::vec3(0.2f, 0.55f, 0.2f);    // Bright green
            break;

        // New temperate trees
        case TreeType::MAPLE:
            baseLeafColor = glm::vec3(0.2f, 0.55f, 0.15f);   // Bright green (turns red in fall)
            break;
        case TreeType::CHERRY_BLOSSOM:
            baseLeafColor = glm::vec3(1.0f, 0.75f, 0.8f);    // Pink blossoms
            break;
        case TreeType::APPLE:
            baseLeafColor = glm::vec3(0.18f, 0.5f, 0.15f);   // Rich green
            break;

        // New tropical
        case TreeType::BANYAN:
            baseLeafColor = glm::vec3(0.12f, 0.42f, 0.12f);  // Deep tropical green
            break;
        case TreeType::COCONUT_PALM:
            baseLeafColor = glm::vec3(0.2f, 0.52f, 0.18f);   // Palm green
            break;
        case TreeType::BAMBOO:
            baseLeafColor = glm::vec3(0.25f, 0.55f, 0.2f);   // Light green
            break;

        // New desert
        case TreeType::PRICKLY_PEAR:
            baseLeafColor = glm::vec3(0.25f, 0.45f, 0.2f);   // Cactus green (no separate leaves)
            break;

        // New boreal
        case TreeType::LARCH:
            baseLeafColor = glm::vec3(0.2f, 0.48f, 0.15f);   // Light conifer green
            break;

        // New savanna
        case TreeType::UMBRELLA_THORN:
            baseLeafColor = glm::vec3(0.22f, 0.45f, 0.18f);  // Dry season green
            break;

        // New swamp
        case TreeType::BALD_CYPRESS:
            baseLeafColor = glm::vec3(0.18f, 0.42f, 0.18f);  // Feathery green
            break;
        case TreeType::WATER_OAK:
            baseLeafColor = glm::vec3(0.15f, 0.48f, 0.15f);  // Dark green
            break;

        // New mountain
        case TreeType::MOUNTAIN_ASH:
            baseLeafColor = glm::vec3(0.2f, 0.5f, 0.18f);    // Green (red berries separate)
            break;
        case TreeType::BRISTLECONE_PINE:
            baseLeafColor = glm::vec3(0.12f, 0.35f, 0.15f);  // Dark needle green
            break;

        // Bush variants
        case TreeType::FLOWERING_BUSH:
            baseLeafColor = glm::vec3(0.9f, 0.4f, 0.5f);     // Pink flowers
            break;
        case TreeType::BERRY_BUSH:
            baseLeafColor = glm::vec3(0.18f, 0.5f, 0.18f);   // Green with berries
            break;

        // Alien trees - unique colors
        case TreeType::CRYSTAL_TREE:
            baseLeafColor = glm::vec3(0.7f, 0.85f, 1.0f);    // Ice blue crystal
            break;
        case TreeType::BIOLUMINESCENT_TREE:
            baseLeafColor = glm::vec3(0.3f, 1.0f, 0.5f);     // Glowing green
            break;
        case TreeType::FLOATING_TREE:
            baseLeafColor = glm::vec3(0.6f, 0.4f, 0.8f);     // Purple-lavender
            break;
        case TreeType::TENDRIL_TREE:
            baseLeafColor = glm::vec3(0.5f, 0.2f, 0.4f);     // Dark magenta
            break;
        case TreeType::SPORE_TREE:
            baseLeafColor = glm::vec3(0.8f, 0.7f, 0.4f);     // Tan-orange spores
            break;
        case TreeType::PLASMA_TREE:
            baseLeafColor = glm::vec3(0.4f, 0.2f, 1.0f);     // Electric purple-blue
            break;

        default:
            baseLeafColor = glm::vec3(0.2f, 0.5f, 0.2f);
    }

    bool addedTrunk = false;

    for (size_t i = 0; i < lString.length(); i++) {
        char c = lString[i];

        // Add randomness to angles for natural look
        float randomAngle = angleRad * (0.8f + Random::value() * 0.4f);

        switch (c) {
            case 'F': case 'S': {
                // Draw forward - create branch segment
                float currentLength = segmentLength;
                if (c == 'S') currentLength *= 0.5f;  // Short segment

                // Add some random variation to segment length
                currentLength *= (0.85f + Random::value() * 0.3f);

                glm::vec3 endPos = turtle.position + turtle.heading * currentLength;
                float endRadius = turtle.radius * 0.75f;  // Taper

                // Minimum radius
                if (endRadius < 0.01f) endRadius = 0.01f;

                addBranch(mesh, turtle.position, endPos, turtle.radius, endRadius, barkColor);

                turtle.position = endPos;
                turtle.radius = endRadius;
                addedTrunk = true;
                break;
            }

            case '+': {
                // Rotate around Up - turn right
                turtle.rotateU(randomAngle);
                break;
            }

            case '-': {
                // Rotate around Up - turn left
                turtle.rotateU(-randomAngle);
                break;
            }

            case '&': {
                // Rotate around Left - pitch down (key for 3D!)
                turtle.rotateL(randomAngle);
                break;
            }

            case '^': {
                // Rotate around Left - pitch up
                turtle.rotateL(-randomAngle);
                break;
            }

            case '\\': {
                // Roll right
                turtle.rotateH(randomAngle);
                break;
            }

            case '/': {
                // Roll left
                turtle.rotateH(-randomAngle);
                break;
            }

            case '|': {
                // Turn around (180 degrees)
                turtle.rotateU(glm::pi<float>());
                break;
            }

            case '!': {
                // Decrease diameter
                turtle.radius *= 0.7f;
                break;
            }

            case '[': {
                // Push state - start branch
                stateStack.push(turtle);
                turtle.depth++;
                turtle.radius *= 0.7f;  // Branches are thinner
                break;
            }

            case ']': {
                // Pop state - end branch, possibly add leaves
                if (!stateStack.empty()) {
                    // Add foliage at branch tips
                    if (turtle.depth > 1) {
                        glm::vec3 leafColor = baseLeafColor;
                        // Add color variation
                        leafColor.r += (Random::value() - 0.5f) * 0.15f;
                        leafColor.g += (Random::value() - 0.5f) * 0.2f;
                        leafColor.b += (Random::value() - 0.5f) * 0.1f;

                        float leafSize = 0.3f + Random::value() * 0.4f;

                        // Choose foliage type based on tree type
                        if (type == TreeType::PINE || type == TreeType::SPRUCE ||
                            type == TreeType::FIR || type == TreeType::ALPINE_FIR ||
                            type == TreeType::CYPRESS) {
                            addPineNeedles(mesh, turtle.position, turtle.heading, leafSize, leafColor);
                        } else if (type == TreeType::BUSH) {
                            addBushFoliage(mesh, turtle.position, leafSize * 0.8f, leafColor);
                        } else if (type == TreeType::CACTUS_SAGUARO || type == TreeType::CACTUS_BARREL) {
                            // Cacti have no leaves - skip foliage
                        } else if (type == TreeType::PALM) {
                            // Palm fronds - elongated leaf clusters
                            addLeafCluster(mesh, turtle.position, leafSize * 1.5f, leafColor);
                        } else if (type == TreeType::ACACIA || type == TreeType::BAOBAB) {
                            // Flat, spreading canopy
                            addLeafCluster(mesh, turtle.position, leafSize * 1.2f, leafColor);
                        } else if (type == TreeType::JOSHUA_TREE) {
                            // Spiky clusters
                            addPineNeedles(mesh, turtle.position, turtle.heading, leafSize * 0.8f, leafColor);
                        } else {
                            addLeafCluster(mesh, turtle.position, leafSize, leafColor);
                        }
                    }

                    turtle = stateStack.top();
                    stateStack.pop();
                }
                break;
            }

            case 'L': {
                // Explicit leaf marker
                if (turtle.depth > 0) {
                    glm::vec3 leafColor = baseLeafColor;
                    leafColor.r += (Random::value() - 0.5f) * 0.15f;
                    leafColor.g += (Random::value() - 0.5f) * 0.2f;

                    float leafSize = 0.25f + Random::value() * 0.35f;
                    addLeafCluster(mesh, turtle.position, leafSize, leafColor);
                }
                break;
            }

            // Skip unrecognized characters (A, B, W, etc. are just placeholders)
            default:
                break;
        }
    }

    return mesh;
}

void TreeGenerator::addBranch(
    MeshData& mesh,
    const glm::vec3& start,
    const glm::vec3& end,
    float startRadius,
    float endRadius,
    const glm::vec3& color,
    int segments
) {
    if (glm::length(end - start) < 0.001f) return;

    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 perpendicular;

    // Find perpendicular vector
    if (std::abs(direction.y) < 0.9f) {
        perpendicular = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    } else {
        perpendicular = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size());

    // Create cylinder with caps for better appearance
    for (int i = 0; i <= segments; i++) {
        float theta = (float)i / segments * 2.0f * glm::pi<float>();

        // Rotate perpendicular around direction
        glm::vec3 offset = rotateAroundAxis(perpendicular, direction, theta);

        // Start ring
        Vertex vStart;
        vStart.position = start + offset * startRadius;
        vStart.normal = glm::normalize(offset);
        vStart.texCoord = glm::vec2(color.r, color.g);  // Store color
        mesh.vertices.push_back(vStart);

        // End ring
        Vertex vEnd;
        vEnd.position = end + offset * endRadius;
        vEnd.normal = glm::normalize(offset);
        vEnd.texCoord = glm::vec2(color.r * 0.85f, color.g * 0.85f);  // Slightly darker
        mesh.vertices.push_back(vEnd);
    }

    // Create triangles for cylinder sides
    for (int i = 0; i < segments; i++) {
        unsigned int i0 = baseIndex + i * 2;
        unsigned int i1 = i0 + 1;
        unsigned int i2 = i0 + 2;
        unsigned int i3 = i0 + 3;

        mesh.indices.push_back(i0);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i1);

        mesh.indices.push_back(i1);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i3);
    }
}

void TreeGenerator::addLeafCluster(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color
) {
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size());

    // Create icosahedron for organic leaf cluster shape
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    glm::vec3 icoVerts[] = {
        glm::normalize(glm::vec3(-1,  t,  0)),
        glm::normalize(glm::vec3( 1,  t,  0)),
        glm::normalize(glm::vec3(-1, -t,  0)),
        glm::normalize(glm::vec3( 1, -t,  0)),
        glm::normalize(glm::vec3( 0, -1,  t)),
        glm::normalize(glm::vec3( 0,  1,  t)),
        glm::normalize(glm::vec3( 0, -1, -t)),
        glm::normalize(glm::vec3( 0,  1, -t)),
        glm::normalize(glm::vec3( t,  0, -1)),
        glm::normalize(glm::vec3( t,  0,  1)),
        glm::normalize(glm::vec3(-t,  0, -1)),
        glm::normalize(glm::vec3(-t,  0,  1))
    };

    // Add some organic distortion
    for (int i = 0; i < 12; i++) {
        float distort = 0.8f + Random::value() * 0.4f;
        Vertex v;
        v.position = position + icoVerts[i] * size * distort;
        v.normal = glm::normalize(icoVerts[i]);
        v.texCoord = glm::vec2(color.r, color.g);
        mesh.vertices.push_back(v);
    }

    // Icosahedron indices (20 triangles)
    unsigned int indices[] = {
        0, 11, 5,  0, 5, 1,   0, 1, 7,   0, 7, 10,  0, 10, 11,
        1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,   3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,   2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1
    };

    for (int i = 0; i < 60; i++) {
        mesh.indices.push_back(baseIndex + indices[i]);
    }
}

void TreeGenerator::addPineNeedles(
    MeshData& mesh,
    const glm::vec3& position,
    const glm::vec3& direction,
    float size,
    const glm::vec3& color
) {
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size());

    // Create cone shape for pine needle cluster
    int segments = 6;
    float coneHeight = size * 1.5f;
    float coneRadius = size * 0.6f;

    // Tip vertex
    Vertex tip;
    tip.position = position + glm::normalize(direction) * coneHeight * 0.3f;
    tip.normal = glm::normalize(direction);
    tip.texCoord = glm::vec2(color.r, color.g);
    mesh.vertices.push_back(tip);

    // Find perpendicular
    glm::vec3 perp;
    if (std::abs(direction.y) < 0.9f) {
        perp = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    } else {
        perp = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    // Base ring
    for (int i = 0; i < segments; i++) {
        float theta = (float)i / segments * 2.0f * glm::pi<float>();
        glm::vec3 offset = rotateAroundAxis(perp, direction, theta) * coneRadius;

        Vertex v;
        v.position = position - glm::normalize(direction) * coneHeight * 0.2f + offset;
        v.normal = glm::normalize(offset - direction * 0.5f);
        v.texCoord = glm::vec2(color.r * 0.8f, color.g * 0.8f);
        mesh.vertices.push_back(v);
    }

    // Create triangles from tip to base
    for (int i = 0; i < segments; i++) {
        int next = (i + 1) % segments;
        mesh.indices.push_back(baseIndex);  // Tip
        mesh.indices.push_back(baseIndex + 1 + i);
        mesh.indices.push_back(baseIndex + 1 + next);
    }
}

void TreeGenerator::addBushFoliage(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color
) {
    // Many overlapping leaf clusters for dense, full bush appearance
    int clusterCount = 6 + (int)(Random::value() * 4);  // 6-10 clusters

    for (int i = 0; i < clusterCount; i++) {
        // Spread clusters in a dome shape
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>() * 0.5f;  // Upper hemisphere
        float r = size * (0.3f + Random::value() * 0.7f);

        glm::vec3 offset(
            r * sin(phi) * cos(theta),
            r * cos(phi) * 0.6f + size * 0.2f,  // Bias upward
            r * sin(phi) * sin(theta)
        );

        glm::vec3 clusterColor = color;
        clusterColor.r += (Random::value() - 0.5f) * 0.08f;
        clusterColor.g += (Random::value() - 0.5f) * 0.15f;
        clusterColor.b += (Random::value() - 0.5f) * 0.05f;

        float clusterSize = size * (0.35f + Random::value() * 0.25f);
        addLeafCluster(mesh, position + offset, clusterSize, clusterColor);
    }
}

glm::vec3 TreeGenerator::rotateAroundAxis(
    const glm::vec3& vec,
    const glm::vec3& axis,
    float angle
) {
    return glm::rotate(vec, angle, axis);
}

// ============================================================================
// New Generation Methods
// ============================================================================

MeshData TreeGenerator::generateTreeWithState(
    TreeType type,
    TreeGrowthStage growth,
    TreeSeasonalState season,
    float leafDensity,
    const glm::vec3& leafColorTint,
    unsigned int seed
) {
    if (seed != 0) {
        Random::init();
    }

    // Adjust parameters based on growth stage
    float growthScale = 1.0f;
    switch (growth) {
        case TreeGrowthStage::SEEDLING:
            growthScale = 0.15f;
            break;
        case TreeGrowthStage::SAPLING:
            growthScale = 0.4f;
            break;
        case TreeGrowthStage::MATURE:
            growthScale = 1.0f;
            break;
        case TreeGrowthStage::OLD_GROWTH:
            growthScale = 1.3f;
            break;
        case TreeGrowthStage::DYING:
            growthScale = 1.1f;
            leafDensity *= 0.3f;
            break;
        case TreeGrowthStage::DEAD:
            growthScale = 0.9f;
            leafDensity = 0.0f;
            break;
    }

    // Adjust leaf density based on season for deciduous trees
    TreeSpeciesConfig config = getTreeSpeciesConfig(type);
    if (config.isDeciduous) {
        switch (season) {
            case TreeSeasonalState::DORMANT:
                leafDensity = 0.0f;
                break;
            case TreeSeasonalState::BUDDING:
                leafDensity *= 0.3f;
                break;
            case TreeSeasonalState::FLOWERING:
                leafDensity *= 0.6f;
                break;
            case TreeSeasonalState::FULL_FOLIAGE:
                // Full density
                break;
            case TreeSeasonalState::FRUITING:
                leafDensity *= 0.95f;
                break;
            case TreeSeasonalState::AUTUMN_COLORS:
                leafDensity *= 0.8f;
                break;
            case TreeSeasonalState::LEAF_DROP:
                leafDensity *= 0.3f;
                break;
        }
    }

    // Generate base tree
    MeshData mesh = generateTree(type, seed);

    // Scale mesh based on growth
    for (auto& vertex : mesh.vertices) {
        vertex.position *= growthScale;
    }

    return mesh;
}

MeshData TreeGenerator::generateDeadTree(TreeType type, float decayProgress, unsigned int seed) {
    if (seed != 0) {
        Random::init();
    }

    LSystem lsys = createLSystem(type);

    // Fewer iterations for dead trees (broken branches)
    int iterations = 2;
    float baseRadius = 0.06f + Random::value() * 0.04f;
    float segmentLength = 0.35f + Random::value() * 0.2f;

    std::string lString = lsys.generate(iterations);

    MeshData mesh;
    std::stack<TurtleState> stateStack;
    TurtleState turtle;
    turtle.radius = baseRadius;

    float angleRad = glm::radians(lsys.getAngle());

    // Dead tree colors - gray and weathered
    glm::vec3 deadBarkColor = glm::mix(
        glm::vec3(0.4f, 0.35f, 0.3f),
        glm::vec3(0.25f, 0.22f, 0.2f),
        decayProgress
    );

    for (size_t i = 0; i < lString.length(); i++) {
        char c = lString[i];
        float randomAngle = angleRad * (0.7f + Random::value() * 0.6f);

        switch (c) {
            case 'F': case 'S': {
                float currentLength = segmentLength;
                if (c == 'S') currentLength *= 0.5f;
                currentLength *= (0.7f + Random::value() * 0.5f);

                // Some branches are broken
                if (Random::value() < decayProgress * 0.4f) {
                    currentLength *= 0.3f;
                }

                glm::vec3 endPos = turtle.position + turtle.heading * currentLength;
                float endRadius = turtle.radius * 0.72f;
                if (endRadius < 0.01f) endRadius = 0.01f;

                addBranch(mesh, turtle.position, endPos, turtle.radius, endRadius, deadBarkColor);

                turtle.position = endPos;
                turtle.radius = endRadius;
                break;
            }
            case '+': turtle.rotateU(randomAngle); break;
            case '-': turtle.rotateU(-randomAngle); break;
            case '&': turtle.rotateL(randomAngle); break;
            case '^': turtle.rotateL(-randomAngle); break;
            case '\\': turtle.rotateH(randomAngle); break;
            case '/': turtle.rotateH(-randomAngle); break;
            case '[':
                stateStack.push(turtle);
                turtle.depth++;
                turtle.radius *= 0.7f;
                break;
            case ']':
                if (!stateStack.empty()) {
                    turtle = stateStack.top();
                    stateStack.pop();
                }
                break;
        }
    }

    return mesh;
}

MeshData TreeGenerator::generateFloweringBush(const glm::vec3& flowerColor, unsigned int seed) {
    MeshData mesh = generateTree(TreeType::FLOWERING_BUSH, seed);

    // Add flower clusters
    int flowerCount = 8 + (int)(Random::value() * 6);
    for (int i = 0; i < flowerCount; i++) {
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>() * 0.4f;
        float r = 0.3f + Random::value() * 0.4f;

        glm::vec3 flowerPos(
            r * sin(phi) * cos(theta),
            0.3f + r * cos(phi),
            r * sin(phi) * sin(theta)
        );

        glm::vec3 petalColor = flowerColor;
        petalColor.r += (Random::value() - 0.5f) * 0.2f;
        petalColor.g += (Random::value() - 0.5f) * 0.1f;
        petalColor.b += (Random::value() - 0.5f) * 0.2f;

        addFlowerCluster(mesh, flowerPos, 0.08f + Random::value() * 0.06f, petalColor, glm::vec3(1.0f, 0.9f, 0.3f));
    }

    return mesh;
}

MeshData TreeGenerator::generateBerryBush(float fruitDensity, const glm::vec3& berryColor, unsigned int seed) {
    MeshData mesh = generateTree(TreeType::BERRY_BUSH, seed);

    // Add berry clusters
    int berryClusterCount = (int)(12 * fruitDensity);
    for (int i = 0; i < berryClusterCount; i++) {
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>() * 0.5f;
        float r = 0.2f + Random::value() * 0.35f;

        glm::vec3 clusterPos(
            r * sin(phi) * cos(theta),
            0.2f + r * cos(phi),
            r * sin(phi) * sin(theta)
        );

        addBerryClusters(mesh, clusterPos, 0.05f, berryColor, fruitDensity);
    }

    return mesh;
}

// ============================================================================
// Alien Tree Generation
// ============================================================================

MeshData TreeGenerator::generateCrystalTree(const glm::vec3& crystalColor, float complexity, unsigned int seed) {
    if (seed != 0) Random::init();

    MeshData mesh;

    // Main crystal trunk
    glm::vec3 trunkColor = crystalColor * 0.7f;
    int segments = 5 + (int)(complexity * 3);

    glm::vec3 pos(0.0f);
    glm::vec3 dir(0.0f, 1.0f, 0.0f);
    float radius = 0.12f;

    for (int i = 0; i < segments; i++) {
        float segLen = 0.4f + Random::value() * 0.3f;
        glm::vec3 endPos = pos + dir * segLen;

        addBranch(mesh, pos, endPos, radius, radius * 0.85f, trunkColor, 5);

        // Add crystal formations at nodes
        if (i > 0 && Random::value() < 0.6f) {
            int crystalCount = 2 + (int)(Random::value() * 3);
            for (int c = 0; c < crystalCount; c++) {
                float angle = (float)c / crystalCount * 2.0f * glm::pi<float>();
                glm::vec3 crystalDir(cos(angle) * 0.7f, 0.4f, sin(angle) * 0.7f);
                glm::vec3 crystalPos = pos + crystalDir * (0.1f + Random::value() * 0.2f);
                float crystalSize = 0.1f + Random::value() * 0.15f;

                addCrystalFormation(mesh, crystalPos, crystalSize, crystalColor, 5 + (int)(complexity * 3));
            }
        }

        pos = endPos;
        radius *= 0.85f;

        // Slight direction change
        dir.x += (Random::value() - 0.5f) * 0.3f;
        dir.z += (Random::value() - 0.5f) * 0.3f;
        dir = glm::normalize(dir);
    }

    // Top crystal cluster
    int topCrystals = 4 + (int)(complexity * 4);
    for (int i = 0; i < topCrystals; i++) {
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * 0.5f;

        glm::vec3 crystalDir(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
        glm::vec3 crystalPos = pos + crystalDir * (0.1f + Random::value() * 0.15f);
        float crystalSize = 0.15f + Random::value() * 0.2f;

        glm::vec3 variedColor = crystalColor;
        variedColor.r += (Random::value() - 0.5f) * 0.2f;
        variedColor.g += (Random::value() - 0.5f) * 0.2f;
        variedColor.b += (Random::value() - 0.5f) * 0.1f;

        addCrystalFormation(mesh, crystalPos, crystalSize, variedColor, 6);
    }

    return mesh;
}

MeshData TreeGenerator::generateBioluminescentTree(const glm::vec3& glowColor, float glowIntensity, unsigned int seed) {
    // Start with a regular tree structure
    MeshData mesh = generateTree(TreeType::BIOLUMINESCENT_TREE, seed);

    // Add glowing orbs at branch tips
    int orbCount = 15 + (int)(glowIntensity * 10);
    for (int i = 0; i < orbCount; i++) {
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>() * 0.6f;
        float r = 0.5f + Random::value() * 1.0f;

        glm::vec3 orbPos(
            r * sin(phi) * cos(theta),
            0.5f + r * cos(phi),
            r * sin(phi) * sin(theta)
        );

        glm::vec3 orbColor = glowColor * (0.8f + glowIntensity * 0.5f);
        orbColor.r += (Random::value() - 0.5f) * 0.2f;
        orbColor.g += (Random::value() - 0.5f) * 0.1f;

        float orbSize = 0.06f + Random::value() * 0.08f * glowIntensity;
        addGlowingOrb(mesh, orbPos, orbSize, orbColor);
    }

    return mesh;
}

MeshData TreeGenerator::generateFloatingTree(float floatHeight, unsigned int seed) {
    if (seed != 0) Random::init();

    MeshData mesh;

    // Create inverted root system (hanging down)
    glm::vec3 baseColor(0.5f, 0.45f, 0.55f);
    glm::vec3 pos(0.0f, floatHeight, 0.0f);

    // Central trunk going up
    glm::vec3 trunkEnd = pos + glm::vec3(0.0f, 1.5f, 0.0f);
    addBranch(mesh, pos, trunkEnd, 0.1f, 0.08f, baseColor);

    // Canopy
    int branchCount = 6;
    for (int i = 0; i < branchCount; i++) {
        float angle = (float)i / branchCount * 2.0f * glm::pi<float>();
        glm::vec3 branchDir(cos(angle) * 0.5f, 0.3f, sin(angle) * 0.5f);
        glm::vec3 branchEnd = trunkEnd + branchDir * (0.6f + Random::value() * 0.3f);

        addBranch(mesh, trunkEnd, branchEnd, 0.05f, 0.03f, baseColor);

        // Floating leaves
        glm::vec3 leafColor(0.6f, 0.4f, 0.8f);
        addLeafCluster(mesh, branchEnd, 0.2f + Random::value() * 0.15f, leafColor);
    }

    // Hanging roots (going down)
    int rootCount = 8;
    for (int i = 0; i < rootCount; i++) {
        float angle = (float)i / rootCount * 2.0f * glm::pi<float>() + Random::value() * 0.3f;
        float rootLen = 0.8f + Random::value() * 0.5f;

        glm::vec3 rootDir(cos(angle) * 0.2f, -1.0f, sin(angle) * 0.2f);
        rootDir = glm::normalize(rootDir);

        glm::vec3 rootEnd = pos + rootDir * rootLen;
        addTendril(mesh, pos, rootEnd, 0.02f + Random::value() * 0.02f, baseColor * 0.8f, 8);
    }

    return mesh;
}

MeshData TreeGenerator::generateTendrilTree(int tendrilCount, unsigned int seed) {
    if (seed != 0) Random::init();

    MeshData mesh;

    glm::vec3 baseColor(0.35f, 0.25f, 0.3f);
    glm::vec3 tendrilColor(0.5f, 0.2f, 0.4f);

    // Central trunk
    glm::vec3 trunkEnd(0.0f, 2.0f, 0.0f);
    addBranch(mesh, glm::vec3(0.0f), trunkEnd, 0.15f, 0.1f, baseColor);

    // Generate tendrils
    for (int i = 0; i < tendrilCount; i++) {
        float baseHeight = 0.5f + Random::value() * 1.5f;
        glm::vec3 tendrilStart(0.0f, baseHeight, 0.0f);

        float angle = (float)i / tendrilCount * 2.0f * glm::pi<float>() + Random::value() * 0.5f;
        float tendrilLen = 1.0f + Random::value() * 1.5f;

        // Curved tendril path
        glm::vec3 control1 = tendrilStart + glm::vec3(cos(angle) * 0.3f, 0.2f, sin(angle) * 0.3f);
        glm::vec3 end = tendrilStart + glm::vec3(cos(angle) * tendrilLen, Random::value() * 0.5f - 0.25f, sin(angle) * tendrilLen);

        glm::vec3 variedColor = tendrilColor;
        variedColor.r += (Random::value() - 0.5f) * 0.15f;
        variedColor.b += (Random::value() - 0.5f) * 0.15f;

        addTendril(mesh, tendrilStart, end, 0.02f + Random::value() * 0.03f, variedColor, 12);

        // Small buds at tendril tips
        addGlowingOrb(mesh, end, 0.04f + Random::value() * 0.03f, variedColor * 1.5f);
    }

    return mesh;
}

MeshData TreeGenerator::generateSporeTree(float sporeCloudDensity, unsigned int seed) {
    if (seed != 0) Random::init();

    MeshData mesh;

    glm::vec3 stalkColor(0.45f, 0.35f, 0.25f);
    glm::vec3 capColor(0.7f, 0.5f, 0.3f);
    glm::vec3 sporeColor(0.8f, 0.7f, 0.4f);

    // Thick stalk
    float stalkHeight = 1.5f + Random::value() * 0.5f;
    addBranch(mesh, glm::vec3(0.0f), glm::vec3(0.0f, stalkHeight, 0.0f), 0.2f, 0.18f, stalkColor, 8);

    // Mushroom cap (multiple overlapping domes)
    glm::vec3 capCenter(0.0f, stalkHeight, 0.0f);
    int capSegments = 8;
    float capRadius = 0.8f + Random::value() * 0.3f;

    for (int ring = 0; ring < 3; ring++) {
        float ringRadius = capRadius * (1.0f - ring * 0.25f);
        float ringHeight = stalkHeight + ring * 0.1f;

        for (int i = 0; i < capSegments; i++) {
            float angle = (float)i / capSegments * 2.0f * glm::pi<float>();
            glm::vec3 pos(cos(angle) * ringRadius, ringHeight, sin(angle) * ringRadius);

            glm::vec3 variedCapColor = capColor;
            variedCapColor.r += (Random::value() - 0.5f) * 0.1f;
            variedCapColor.g += (Random::value() - 0.5f) * 0.1f;

            addLeafCluster(mesh, pos, 0.25f + Random::value() * 0.15f, variedCapColor);
        }
    }

    // Spore clouds
    int sporeCloudCount = (int)(5 * sporeCloudDensity);
    for (int i = 0; i < sporeCloudCount; i++) {
        float angle = Random::value() * 2.0f * glm::pi<float>();
        float dist = capRadius * 0.5f + Random::value() * capRadius * 0.8f;
        float height = stalkHeight - 0.3f + Random::value() * 0.6f;

        glm::vec3 cloudPos(cos(angle) * dist, height, sin(angle) * dist);
        addSporeCloud(mesh, cloudPos, 0.15f + Random::value() * 0.1f, sporeColor, (int)(20 * sporeCloudDensity));
    }

    return mesh;
}

// ============================================================================
// Helper Geometry Methods
// ============================================================================

void TreeGenerator::addFlowerCluster(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& petalColor,
    const glm::vec3& centerColor
) {
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size());

    // Flower center
    Vertex center;
    center.position = position;
    center.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    center.texCoord = glm::vec2(centerColor.r, centerColor.g);
    mesh.vertices.push_back(center);

    // Petals in a circle
    int petalCount = 5 + (int)(Random::value() * 3);
    for (int i = 0; i < petalCount; i++) {
        float angle = (float)i / petalCount * 2.0f * glm::pi<float>();
        float petalDist = size * (0.8f + Random::value() * 0.4f);

        glm::vec3 petalPos = position + glm::vec3(cos(angle) * petalDist, 0.02f, sin(angle) * petalDist);

        Vertex petal;
        petal.position = petalPos;
        petal.normal = glm::normalize(glm::vec3(cos(angle) * 0.3f, 1.0f, sin(angle) * 0.3f));
        petal.texCoord = glm::vec2(petalColor.r, petalColor.g);
        mesh.vertices.push_back(petal);
    }

    // Connect petals to center
    for (int i = 0; i < petalCount; i++) {
        int next = (i + 1) % petalCount;
        mesh.indices.push_back(baseIndex);
        mesh.indices.push_back(baseIndex + 1 + i);
        mesh.indices.push_back(baseIndex + 1 + next);
    }
}

void TreeGenerator::addFruit(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color
) {
    // Simple sphere for fruit
    addLeafCluster(mesh, position, size, color);
}

void TreeGenerator::addBerryClusters(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& berryColor,
    float density
) {
    int berryCount = (int)(5 * density) + 2;
    for (int i = 0; i < berryCount; i++) {
        glm::vec3 offset(
            (Random::value() - 0.5f) * size * 2,
            (Random::value() - 0.5f) * size,
            (Random::value() - 0.5f) * size * 2
        );

        glm::vec3 variedColor = berryColor;
        variedColor.r += (Random::value() - 0.5f) * 0.15f;

        addFruit(mesh, position + offset, size * 0.3f, variedColor);
    }
}

void TreeGenerator::addCrystalFormation(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color,
    int facets
) {
    unsigned int baseIndex = static_cast<unsigned int>(mesh.vertices.size());

    // Crystal point (top)
    Vertex tip;
    tip.position = position + glm::vec3(0.0f, size, 0.0f);
    tip.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    tip.texCoord = glm::vec2(color.r * 1.2f, color.g * 1.2f);  // Brighter at tip
    mesh.vertices.push_back(tip);

    // Crystal base (bottom point)
    Vertex base;
    base.position = position - glm::vec3(0.0f, size * 0.3f, 0.0f);
    base.normal = glm::vec3(0.0f, -1.0f, 0.0f);
    base.texCoord = glm::vec2(color.r * 0.8f, color.g * 0.8f);
    mesh.vertices.push_back(base);

    // Middle ring
    float midRadius = size * 0.3f;
    for (int i = 0; i < facets; i++) {
        float angle = (float)i / facets * 2.0f * glm::pi<float>();
        glm::vec3 pos = position + glm::vec3(cos(angle) * midRadius, 0.0f, sin(angle) * midRadius);

        Vertex v;
        v.position = pos;
        v.normal = glm::normalize(glm::vec3(cos(angle), 0.0f, sin(angle)));
        v.texCoord = glm::vec2(color.r, color.g);
        mesh.vertices.push_back(v);
    }

    // Connect to top
    for (int i = 0; i < facets; i++) {
        int next = (i + 1) % facets;
        mesh.indices.push_back(baseIndex);  // Tip
        mesh.indices.push_back(baseIndex + 2 + i);
        mesh.indices.push_back(baseIndex + 2 + next);
    }

    // Connect to bottom
    for (int i = 0; i < facets; i++) {
        int next = (i + 1) % facets;
        mesh.indices.push_back(baseIndex + 1);  // Base
        mesh.indices.push_back(baseIndex + 2 + next);
        mesh.indices.push_back(baseIndex + 2 + i);
    }
}

void TreeGenerator::addGlowingOrb(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color
) {
    // Orbs are just bright-colored leaf clusters
    glm::vec3 glowColor = color;
    glowColor = glm::clamp(glowColor * 1.5f, glm::vec3(0.0f), glm::vec3(1.0f));
    addLeafCluster(mesh, position, size, glowColor);
}

void TreeGenerator::addTendril(
    MeshData& mesh,
    const glm::vec3& start,
    const glm::vec3& end,
    float thickness,
    const glm::vec3& color,
    int segments
) {
    glm::vec3 direction = end - start;
    float length = glm::length(direction);
    if (length < 0.001f) return;

    direction = glm::normalize(direction);

    glm::vec3 pos = start;
    float segmentLen = length / segments;
    float currentThickness = thickness;

    for (int i = 0; i < segments; i++) {
        // Add some waviness
        glm::vec3 offset(0.0f);
        if (i > 0 && i < segments - 1) {
            float wave = sin((float)i / segments * glm::pi<float>() * 2.0f) * 0.05f;
            glm::vec3 perp = glm::cross(direction, glm::vec3(0, 1, 0));
            if (glm::length(perp) < 0.1f) perp = glm::cross(direction, glm::vec3(1, 0, 0));
            perp = glm::normalize(perp);
            offset = perp * wave;
        }

        glm::vec3 nextPos = pos + direction * segmentLen + offset;
        float nextThickness = currentThickness * 0.92f;

        addBranch(mesh, pos, nextPos, currentThickness, nextThickness, color, 5);

        pos = nextPos;
        currentThickness = nextThickness;
    }
}

void TreeGenerator::addSporeCloud(
    MeshData& mesh,
    const glm::vec3& position,
    float radius,
    const glm::vec3& color,
    int particleCount
) {
    for (int i = 0; i < particleCount; i++) {
        // Random position within sphere
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>();
        float r = Random::value() * radius;

        glm::vec3 sporePos = position + glm::vec3(
            r * sin(phi) * cos(theta),
            r * cos(phi),
            r * sin(phi) * sin(theta)
        );

        // Tiny spore particle
        float sporeSize = 0.01f + Random::value() * 0.015f;
        addGlowingOrb(mesh, sporePos, sporeSize, color);
    }
}

void TreeGenerator::addSparseLeaves(
    MeshData& mesh,
    const glm::vec3& position,
    float size,
    const glm::vec3& color,
    float density
) {
    int leafCount = std::max(1, (int)(6 * density));

    for (int i = 0; i < leafCount; i++) {
        float theta = Random::value() * 2.0f * glm::pi<float>();
        float phi = Random::value() * glm::pi<float>() * 0.5f;
        float r = size * (0.5f + Random::value() * 0.5f);

        glm::vec3 offset(
            r * sin(phi) * cos(theta),
            r * cos(phi),
            r * sin(phi) * sin(theta)
        );

        float leafSize = size * (0.3f + Random::value() * 0.2f);
        addLeafCluster(mesh, position + offset, leafSize, color);
    }
}

void TreeGenerator::addDecayedBranch(
    MeshData& mesh,
    const glm::vec3& start,
    const glm::vec3& end,
    float startRadius,
    float endRadius,
    float decayLevel
) {
    // Weathered gray-brown color
    glm::vec3 decayColor = glm::mix(
        glm::vec3(0.4f, 0.35f, 0.28f),
        glm::vec3(0.25f, 0.22f, 0.18f),
        decayLevel
    );

    // Irregular shape for decayed branches
    float irregularity = decayLevel * 0.3f;
    glm::vec3 modifiedEnd = end;
    modifiedEnd.x += (Random::value() - 0.5f) * irregularity;
    modifiedEnd.z += (Random::value() - 0.5f) * irregularity;

    addBranch(mesh, start, modifiedEnd, startRadius, endRadius, decayColor, 5);
}

std::vector<glm::vec3> TreeGenerator::getFruitPositions(TreeType type, const MeshData& treeMesh, unsigned int seed) {
    std::vector<glm::vec3> fruitPositions;

    if (seed != 0) Random::init();

    TreeSpeciesConfig config = getTreeSpeciesConfig(type);
    if (!config.producesFruit) return fruitPositions;

    // Estimate canopy bounds from mesh
    glm::vec3 minBounds(FLT_MAX), maxBounds(-FLT_MAX);
    for (const auto& vertex : treeMesh.vertices) {
        minBounds = glm::min(minBounds, vertex.position);
        maxBounds = glm::max(maxBounds, vertex.position);
    }

    // Generate fruit positions in upper half of tree
    float minY = (minBounds.y + maxBounds.y) * 0.5f;
    int fruitCount = 5 + (int)(Random::value() * 10);

    for (int i = 0; i < fruitCount; i++) {
        glm::vec3 pos;
        pos.x = minBounds.x + Random::value() * (maxBounds.x - minBounds.x);
        pos.y = minY + Random::value() * (maxBounds.y - minY);
        pos.z = minBounds.z + Random::value() * (maxBounds.z - minBounds.z);

        // Only add if within reasonable distance from center
        float horizDist = glm::length(glm::vec2(pos.x, pos.z));
        float maxDist = (maxBounds.x - minBounds.x) * 0.4f;
        if (horizDist < maxDist) {
            fruitPositions.push_back(pos);
        }
    }

    return fruitPositions;
}

glm::vec3 TreeGenerator::getBarkColor(TreeType type, float damage) {
    glm::vec3 baseColor;
    switch (type) {
        case TreeType::OAK: baseColor = glm::vec3(0.35f, 0.25f, 0.15f); break;
        case TreeType::BIRCH: baseColor = glm::vec3(0.85f, 0.82f, 0.78f); break;
        case TreeType::PINE: baseColor = glm::vec3(0.4f, 0.28f, 0.18f); break;
        default: baseColor = glm::vec3(0.4f, 0.3f, 0.2f); break;
    }

    // Damaged bark is darker and more gray
    if (damage > 0.0f) {
        glm::vec3 damagedColor(0.3f, 0.28f, 0.25f);
        baseColor = glm::mix(baseColor, damagedColor, damage);
    }

    return baseColor;
}

glm::vec3 TreeGenerator::getLeafColor(TreeType type, TreeSeasonalState season) {
    return getSeasonalLeafColor(type, season, 0.5f);
}
