#include "CreatureType.h"

CreatureTraits CreatureTraits::getTraitsFor(CreatureType type) {
    CreatureTraits traits;
    traits.type = type;

    switch (type) {
        case CreatureType::GRAZER:
            traits.diet = DietType::GRASS_ONLY;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 40.0f;
            traits.huntingEfficiency = 0.0f;
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = true;
            traits.isTerritorial = false;
            traits.canClimb = false;
            traits.canDigest[0] = true;   // grass
            traits.canDigest[1] = false;  // leaves
            traits.canDigest[2] = false;  // fruit
            traits.parasiteResistance = 0.4f;
            break;

        case CreatureType::BROWSER:
            traits.diet = DietType::BROWSE_ONLY;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 35.0f;
            traits.huntingEfficiency = 0.0f;
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = true;
            traits.isTerritorial = false;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = true;   // leaves
            traits.canDigest[2] = true;   // fruit (incidental)
            traits.parasiteResistance = 0.5f;
            break;

        case CreatureType::FRUGIVORE:
            traits.diet = DietType::FRUIT_ONLY;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 30.0f;
            traits.huntingEfficiency = 0.0f;
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = false;
            traits.canClimb = true;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = true;   // fruit
            traits.parasiteResistance = 0.3f;
            break;

        case CreatureType::SMALL_PREDATOR:
            traits.diet = DietType::SMALL_PREY;
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 2.0f;
            traits.attackDamage = 10.0f;
            traits.fleeDistance = 25.0f;  // Flees from apex predators
            traits.huntingEfficiency = 0.10f;
            traits.minPreySize = 0.2f;
            traits.maxPreySize = 1.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = true;
            traits.canClimb = true;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.6f;
            break;

        case CreatureType::OMNIVORE:
            traits.diet = DietType::OMNIVORE_FLEX;
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 2.5f;
            traits.attackDamage = 12.0f;
            traits.fleeDistance = 20.0f;  // Only flees from apex predators
            traits.huntingEfficiency = 0.08f;
            traits.minPreySize = 0.2f;
            traits.maxPreySize = 1.2f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = true;
            traits.canClimb = true;
            traits.canDigest[0] = true;
            traits.canDigest[1] = true;
            traits.canDigest[2] = true;
            traits.parasiteResistance = 0.7f;
            break;

        case CreatureType::APEX_PREDATOR:
            traits.diet = DietType::ALL_PREY;
            traits.trophicLevel = TrophicLevel::TERTIARY_CONSUMER;
            traits.attackRange = 3.0f;
            traits.attackDamage = 20.0f;
            traits.fleeDistance = 0.0f;  // Never flees
            traits.huntingEfficiency = 0.08f;
            traits.minPreySize = 0.5f;
            traits.maxPreySize = 2.5f;
            traits.isPackHunter = true;
            traits.isHerdAnimal = false;
            traits.isTerritorial = true;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.8f;
            break;

        case CreatureType::SCAVENGER:
            traits.diet = DietType::CARRION;
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 30.0f;
            traits.huntingEfficiency = 0.15f;  // Efficient at processing carrion
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = false;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.9f;  // Evolved immunity
            break;

        case CreatureType::PARASITE:
            traits.diet = DietType::PARASITE_DRAIN;
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 1.0f;
            traits.attackDamage = 0.5f;  // Low damage, sustained drain
            traits.fleeDistance = 0.0f;
            traits.huntingEfficiency = 0.20f;
            traits.minPreySize = 0.5f;
            traits.maxPreySize = 10.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = false;
            traits.canClimb = true;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 1.0f;  // Immune to other parasites
            break;

        case CreatureType::CLEANER:
            traits.diet = DietType::CLEANER_SERVICE;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 20.0f;
            traits.huntingEfficiency = 0.25f;  // Gets energy from cleaning
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = false;
            traits.canClimb = true;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.8f;
            break;

        // =====================================
        // AQUATIC CREATURE TYPES
        // =====================================

        case CreatureType::AQUATIC:
        case CreatureType::AQUATIC_HERBIVORE:
            // Small fish - minnow/guppy analog
            traits.diet = DietType::AQUATIC_ALGAE;
            traits.trophicLevel = TrophicLevel::PRIMARY_CONSUMER;
            traits.attackRange = 0.0f;
            traits.attackDamage = 0.0f;
            traits.fleeDistance = 25.0f;
            traits.huntingEfficiency = 0.0f;
            traits.minPreySize = 0.0f;
            traits.maxPreySize = 0.0f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = true;  // Schools together
            traits.isTerritorial = false;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.3f;
            break;

        case CreatureType::AQUATIC_PREDATOR:
            // Predatory fish - bass/pike analog
            traits.diet = DietType::AQUATIC_SMALL_PREY;
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 2.0f;
            traits.attackDamage = 12.0f;
            traits.fleeDistance = 15.0f;  // Flees from sharks
            traits.huntingEfficiency = 0.12f;
            traits.minPreySize = 0.2f;
            traits.maxPreySize = 0.8f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;  // Mostly solitary
            traits.isTerritorial = true;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.5f;
            break;

        case CreatureType::AQUATIC_APEX:
            // Apex aquatic predator - shark analog
            traits.diet = DietType::AQUATIC_ALL_PREY;
            traits.trophicLevel = TrophicLevel::TERTIARY_CONSUMER;
            traits.attackRange = 3.5f;
            traits.attackDamage = 25.0f;
            traits.fleeDistance = 0.0f;  // Never flees
            traits.huntingEfficiency = 0.10f;
            traits.minPreySize = 0.3f;
            traits.maxPreySize = 1.5f;
            traits.isPackHunter = false;  // Solitary hunter
            traits.isHerdAnimal = false;
            traits.isTerritorial = true;
            traits.canClimb = false;
            traits.canDigest[0] = false;
            traits.canDigest[1] = false;
            traits.canDigest[2] = false;
            traits.parasiteResistance = 0.7f;
            break;

        case CreatureType::AMPHIBIAN:
            // Amphibian - frog/salamander analog
            traits.diet = DietType::OMNIVORE_FLEX;  // Eats insects and plants
            traits.trophicLevel = TrophicLevel::SECONDARY_CONSUMER;
            traits.attackRange = 1.5f;
            traits.attackDamage = 5.0f;
            traits.fleeDistance = 25.0f;
            traits.huntingEfficiency = 0.08f;
            traits.minPreySize = 0.1f;
            traits.maxPreySize = 0.5f;
            traits.isPackHunter = false;
            traits.isHerdAnimal = false;
            traits.isTerritorial = false;
            traits.canClimb = true;
            traits.canDigest[0] = false;
            traits.canDigest[1] = true;  // Can eat some plants
            traits.canDigest[2] = true;
            traits.parasiteResistance = 0.4f;
            break;

        default:
            // Default to grazer traits
            break;
    }

    return traits;
}
