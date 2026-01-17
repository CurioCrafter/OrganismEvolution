#include "LSystem.h"

LSystem::LSystem(const std::string& axiom, float angle)
    : axiom(axiom), angle(angle) {
}

void LSystem::addRule(char symbol, const std::string& replacement) {
    rules[symbol] = replacement;
}

std::string LSystem::generate(int iterations) const {
    std::string current = axiom;

    for (int i = 0; i < iterations; i++) {
        std::string next;
        next.reserve(current.length() * 2); // Pre-allocate for performance

        for (char c : current) {
            auto it = rules.find(c);
            if (it != rules.end()) {
                next += it->second;
            } else {
                next += c;
            }
        }

        current = std::move(next);
    }

    return current;
}
