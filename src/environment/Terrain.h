#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Terrain {
public:
    Terrain(int width, int depth, float scale = 1.0f);
    ~Terrain();

    void generate(unsigned int seed);
    void render();
    float getHeight(float x, float z) const;
    bool isWater(float x, float z) const;

    int getWidth() const { return width; }
    int getDepth() const { return depth; }
    float getScale() const { return scale; }

private:
    int width;
    int depth;
    float scale;
    float waterLevel;

    std::vector<float> heightMap;
    GLuint VAO, VBO, EBO;
    unsigned int indexCount;

    void setupMesh();
    glm::vec3 getTerrainColor(float height);
};
