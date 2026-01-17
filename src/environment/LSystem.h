#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class LSystem {
public:
    LSystem(const std::string& axiom, float angle);

    // Add a production rule
    void addRule(char symbol, const std::string& replacement);

    // Generate string after n iterations
    std::string generate(int iterations) const;

    // Get angle for rotations
    float getAngle() const { return angle; }

private:
    std::string axiom;
    std::unordered_map<char, std::string> rules;
    float angle;
};
