#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <cmath>

class MetaballSystem {
public:
    struct Metaball {
        glm::vec3 position;
        float radius;
        float strength;

        Metaball(const glm::vec3& pos, float r, float s = 1.0f)
            : position(pos), radius(r), strength(s) {}
    };

    MetaballSystem() : threshold(1.0f) {}

    // Add a metaball to the system
    void addMetaball(const glm::vec3& position, float radius, float strength = 1.0f) {
        metaballs.push_back(Metaball(position, radius, strength));
    }

    // Clear all metaballs
    void clear() {
        metaballs.clear();
    }

    // Evaluate the potential field at a point
    // Returns sum of all metaball influences
    float evaluatePotential(const glm::vec3& point) const {
        float potential = 0.0f;

        for (const auto& ball : metaballs) {
            float distance = glm::length(point - ball.position);

            if (distance < ball.radius) {
                // Smooth falloff function: strength * (1 - (d/r)^2)^2
                float ratio = distance / ball.radius;
                float influence = 1.0f - ratio * ratio;
                potential += ball.strength * influence * influence;
            }
        }

        return potential;
    }

    // Calculate analytical gradient (6x faster than finite differences)
    // Gradient of f(r) = strength * (1 - (r/R)^2)^2 where r = distance
    // f'(r) = strength * 2 * (1 - (r/R)^2) * (-2r/R^2)
    glm::vec3 evaluateGradient(const glm::vec3& point) const {
        glm::vec3 gradient(0.0f);

        for (const auto& ball : metaballs) {
            glm::vec3 diff = point - ball.position;
            float distance = glm::length(diff);

            if (distance < ball.radius && distance > 0.0001f) {
                float ratio = distance / ball.radius;
                float ratio_sq = ratio * ratio;
                float influence = 1.0f - ratio_sq;

                // Derivative: 2 * strength * (1 - (r/R)^2) * (-2r/R^2)
                // = -4 * strength * (1 - (r/R)^2) * r / R^2
                float derivative = -4.0f * ball.strength * influence * distance / (ball.radius * ball.radius);

                // Gradient = derivative * direction
                glm::vec3 direction = diff / distance;
                gradient += derivative * direction;
            }
        }

        return gradient;
    }

    // Calculate normal vector at a point (uses analytical gradient)
    glm::vec3 calculateNormal(const glm::vec3& point) const {
        glm::vec3 gradient = evaluateGradient(point);

        // Normalize if non-zero
        float length = glm::length(gradient);
        if (length > 0.0001f) {
            return gradient / length;
        }

        return glm::vec3(0.0f, 1.0f, 0.0f); // Default up vector
    }

    // Get the threshold value (surface is where potential = threshold)
    float getThreshold() const { return threshold; }
    void setThreshold(float t) { threshold = t; }

    // Get number of metaballs
    size_t getMetaballCount() const { return metaballs.size(); }

    // Get metaball by index
    const Metaball& getMetaball(size_t index) const {
        return metaballs[index];
    }

    // Calculate bounding box of all metaballs
    void getBounds(glm::vec3& minBounds, glm::vec3& maxBounds) const {
        if (metaballs.empty()) {
            minBounds = maxBounds = glm::vec3(0.0f);
            return;
        }

        minBounds = metaballs[0].position - glm::vec3(metaballs[0].radius);
        maxBounds = metaballs[0].position + glm::vec3(metaballs[0].radius);

        for (const auto& ball : metaballs) {
            glm::vec3 ballMin = ball.position - glm::vec3(ball.radius);
            glm::vec3 ballMax = ball.position + glm::vec3(ball.radius);

            minBounds = glm::min(minBounds, ballMin);
            maxBounds = glm::max(maxBounds, ballMax);
        }
    }

private:
    std::vector<Metaball> metaballs;
    float threshold;
};
