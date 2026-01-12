#pragma once

#include <glm/glm.hpp>

class Food {
public:
    Food(const glm::vec3& position);

    const glm::vec3& getPosition() const { return position; }
    bool isActive() const { return active; }
    void consume() { active = false; }

private:
    glm::vec3 position;
    bool active;
};
