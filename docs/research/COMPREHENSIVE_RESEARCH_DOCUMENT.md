# Comprehensive Research Document: OrganismEvolution Simulator

## Table of Contents
1. [Procedural Creature Generation](#1-procedural-creature-generation)
2. [Marching Cubes Algorithm](#2-marching-cubes-algorithm)
3. [L-System Tree Generation](#3-l-system-tree-generation)
4. [Bush and Shrub Generation](#4-bush-and-shrub-generation)
5. [OpenGL Text Rendering](#5-opengl-text-rendering)
6. [Craig Reynolds' Steering Behaviors](#6-craig-reynolds-steering-behaviors)
7. [Predator-Prey Ecosystem Dynamics](#7-predator-prey-ecosystem-dynamics)
8. [Spatial Partitioning](#8-spatial-partitioning)
9. [Visual Differentiation Techniques](#9-visual-differentiation-techniques)

---

## 1. Procedural Creature Generation

### Sources
- [Procedural Creature Progress 2021-2024 (Runevision)](https://blog.runevision.com/2025/01/procedural-creature-progress-2021-2024.html)
- [Pudgy Pals Procedural Creature Generator](https://github.com/nmagarino/Pudgy-Pals-Procedural-Creature-Generator)
- [NCCA Bournemouth MSc Thesis](https://nccastaff.bournemouth.ac.uk/jmacey/MastersProject/MSc22/01/ProceduralCreatureGenerationandAnimationforGames.pdf)
- [Marching Cubes and Metaballs Blog](https://hiatus770.github.io/blog/programming/graphics/2024/05/25/Marching-Cubes-and-Metaballs.html)

### Metaball System

Metaballs are organic-looking n-dimensional isosurfaces that blend smoothly when in proximity.

**Mathematical Foundation:**
```
f(point) = SUM( radius^2 / distance^2 ) for each metaball

Isosurface exists where: f(point) >= threshold (typically 1.0)
```

**Influence Function Variants:**
```cpp
// Inverse square (gravity-like, smoother blending)
float influence = (radius * radius) / (distance * distance);

// Polynomial falloff (bounded influence radius)
float t = distance / radius;
float influence = (t < 1.0f) ? (1.0f - t*t) * (1.0f - t*t) : 0.0f;

// Wyvill blend (smooth with finite extent)
float b = distance / radius;
float influence = (b < 1.0f) ? (1.0f - b*b*b * (b * (b*6 - 15) + 10)) : 0.0f;
```

### Spine Generation (Pudgy Pals Method)

**De Casteljau's Algorithm for Spine:**
1. Create 4 control points defining creature's body curve
2. Connect points using cubic bezier spline
3. Place 8 metaballs at successive positions along spline
4. Assign random radii to each metaball
5. Translate smaller metaballs closer to maintain continuity

```cpp
struct CreatureSpine {
    std::vector<glm::vec3> controlPoints; // 4 points
    std::vector<Metaball> spineBalls;     // 8 metaballs

    glm::vec3 evaluateBezier(float t) {
        // De Casteljau's algorithm
        std::vector<glm::vec3> points = controlPoints;
        while (points.size() > 1) {
            std::vector<glm::vec3> newPoints;
            for (size_t i = 0; i < points.size() - 1; i++) {
                newPoints.push_back(glm::mix(points[i], points[i+1], t));
            }
            points = newPoints;
        }
        return points[0];
    }
};
```

### Limb Generation

**Placement Strategy:**
- Up to 4 pairs of procedurally generated limbs
- Tag system prevents misplacement (legs tag, arms tag, etc.)
- Walking creatures: legs tied to movement animation
- Flying creatures: wings attached at shoulder joints
- Swimming creatures: fins distributed for balance

**Limb Structure:**
```cpp
struct Limb {
    int segments;           // 2-4 segments per limb
    float lengths[4];       // Length of each segment
    float thicknesses[4];   // Thickness of each segment
    glm::vec3 jointAngles[4]; // Rotation at each joint
    LimbType type;          // LEG, ARM, WING, FIN, TAIL
};
```

### Parameter Reduction (Runevision Method)

**High-Level vs Low-Level Parameters:**
- Original: 503 low-level parameters (bone alignments, thicknesses)
- Reduced: 106 high-level meaningful parameters
- Method: Manual correlation analysis (NOT PCA - each PCA parameter influences many traits)

**Meaningful Parameters:**
```cpp
struct CreatureParameters {
    // Body proportions
    float bodyLength;      // 0.5 - 2.0
    float bodyWidth;       // 0.3 - 1.5
    float bodyHeight;      // 0.3 - 1.5

    // Limb configuration
    int legCount;          // 0, 2, 4, 6
    float legLength;       // 0.5 - 2.0
    float legThickness;    // 0.1 - 0.5

    // Head features
    float headSize;        // 0.3 - 1.5
    float eyeSize;         // 0.1 - 0.5
    float eyeSpacing;      // 0.2 - 0.8

    // Details
    float tailLength;      // 0.0 - 2.0
    int spikeCount;        // 0 - 12
    float earSize;         // 0.0 - 0.5
};
```

### Gradient Descent for Creature Matching

**Silhouette-Based Optimization:**
1. Capture silhouettes from multiple angles (front, side, top)
2. Compute Signed Distance Field (SDF) for each silhouette
3. Calculate penalty where procedural silhouettes diverge from reference
4. Use gradient descent to minimize penalty

```cpp
float computeSilhouettePenalty(const Silhouette& reference,
                                const Silhouette& procedural) {
    float penalty = 0.0f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float refDist = reference.sdf[y][x];
            float procDist = procedural.sdf[y][x];
            penalty += (refDist - procDist) * (refDist - procDist);
        }
    }
    return penalty;
}
```

---

## 2. Marching Cubes Algorithm

### Sources
- [Paul Bourke - Polygonising a Scalar Field](https://paulbourke.net/geometry/polygonise/)
- [CIS 700 Procedural Graphics Assignment](https://cis700-procedural-graphics.github.io/assignments/proj6-marchingcubes/)

### Cube Vertex Numbering

```
        4 --------- 5
       /|          /|
      / |         / |
     7 --------- 6  |
     |  |        |  |
     |  0 -------|-- 1
     | /         | /
     |/          |/
     3 --------- 2

Vertex positions:
0: (0, 0, 0)  4: (0, 1, 0)
1: (1, 0, 0)  5: (1, 1, 0)
2: (1, 0, 1)  6: (1, 1, 1)
3: (0, 0, 1)  7: (0, 1, 1)
```

### Edge Numbering

```
Edge 0:  vertex 0 to 1    Edge 6:  vertex 4 to 5
Edge 1:  vertex 1 to 2    Edge 7:  vertex 5 to 6
Edge 2:  vertex 2 to 3    Edge 8:  vertex 0 to 4
Edge 3:  vertex 3 to 0    Edge 9:  vertex 1 to 5
Edge 4:  vertex 4 to 7    Edge 10: vertex 2 to 6
Edge 5:  vertex 7 to 6    Edge 11: vertex 3 to 7
```

### Algorithm Steps

```cpp
void marchingCubes(ScalarField& field, float isoLevel, MeshData& mesh) {
    for (int z = 0; z < depth - 1; z++) {
        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width - 1; x++) {
                // Step 1: Sample 8 corners
                float values[8];
                for (int i = 0; i < 8; i++) {
                    values[i] = field.sample(x + cubeOffsets[i].x,
                                             y + cubeOffsets[i].y,
                                             z + cubeOffsets[i].z);
                }

                // Step 2: Build cube index
                int cubeIndex = 0;
                for (int i = 0; i < 8; i++) {
                    if (values[i] < isoLevel) cubeIndex |= (1 << i);
                }

                // Step 3: Skip if entirely inside or outside
                if (edgeTable[cubeIndex] == 0) continue;

                // Step 4: Interpolate edge vertices
                glm::vec3 edgeVerts[12];
                if (edgeTable[cubeIndex] & 1)    edgeVerts[0]  = interpolate(0, 1, values, isoLevel);
                if (edgeTable[cubeIndex] & 2)    edgeVerts[1]  = interpolate(1, 2, values, isoLevel);
                // ... etc for all 12 edges

                // Step 5: Generate triangles from lookup table
                for (int i = 0; triTable[cubeIndex][i] != -1; i += 3) {
                    mesh.addTriangle(
                        edgeVerts[triTable[cubeIndex][i]],
                        edgeVerts[triTable[cubeIndex][i+1]],
                        edgeVerts[triTable[cubeIndex][i+2]]
                    );
                }
            }
        }
    }
}

glm::vec3 interpolate(int v1, int v2, float* values, float isoLevel) {
    float t = (isoLevel - values[v1]) / (values[v2] - values[v1]);
    return glm::mix(cubePositions[v1], cubePositions[v2], t);
}
```

### Edge Table (First 16 entries)

```cpp
int edgeTable[256] = {
    0x000, 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    // ... 240 more entries
};
```

### Triangle Table Structure

```cpp
int triTable[256][16] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // 0
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},   // 1
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},   // 2
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},     // 3
    // ... 252 more configurations
};
```

---

## 3. L-System Tree Generation

### Sources
- [The Algorithmic Beauty of Plants](https://www.houdinikitchen.net/wp-content/uploads/2019/12/L-systems.pdf)
- [Allen Pike - Modeling Plants with L-Systems](https://allenpike.com/modeling-plants-with-l-systems/)
- [Generating Trees with L-Systems](https://gpfault.net/posts/generating-trees.txt.html)
- [3D L-Systems](https://csh.rit.edu/~aidan/portfolio/3DLSystems.shtml)
- [L3D GitHub Repository](https://github.com/abiusx/L3D)

### 3D Turtle Graphics Commands

**Orientation Vectors (H, L, U):**
- H = Heading (forward direction)
- L = Left vector
- U = Up vector
- Relationship: H x L = U (right-hand rule)

**Complete Command Set:**

| Symbol | Action | Rotation Matrix |
|--------|--------|-----------------|
| F | Move forward, drawing line | - |
| f | Move forward, no drawing | - |
| + | Turn left by angle δ | RU(δ) |
| - | Turn right by angle δ | RU(-δ) |
| & | Pitch down by angle δ | RL(δ) |
| ^ | Pitch up by angle δ | RL(-δ) |
| \\ | Roll left by angle δ | RH(δ) |
| / | Roll right by angle δ | RH(-δ) |
| \| | Turn around (180°) | RU(180°) |
| [ | Push state to stack | - |
| ] | Pop state from stack | - |

### Rotation Matrices

```cpp
// Rotation around Up vector (yaw/turn)
glm::mat3 rotateU(float angle) {
    float c = cos(angle), s = sin(angle);
    return glm::mat3(
        c, s, 0,
       -s, c, 0,
        0, 0, 1
    );
}

// Rotation around Left vector (pitch)
glm::mat3 rotateL(float angle) {
    float c = cos(angle), s = sin(angle);
    return glm::mat3(
        c, 0, -s,
        0, 1,  0,
        s, 0,  c
    );
}

// Rotation around Heading vector (roll)
glm::mat3 rotateH(float angle) {
    float c = cos(angle), s = sin(angle);
    return glm::mat3(
        1,  0, 0,
        0,  c, s,
        0, -s, c
    );
}
```

### Parametric L-Systems

**Syntax:** `Module(param1, param2, ...)`

**Example - Tree with Decreasing Segments:**
```
Axiom: A(1, 10)
Rules:
  A(l, w) : w > 0 -> F(l, w)[+A(l*0.7, w-1)][-A(l*0.7, w-1)]
  F(l, w) -> draw segment of length l and width w
```

### Stochastic L-Systems (Randomization)

**Rule Probabilities:**
```
F -> F[+F]F[-F]F   (probability 0.33)
F -> F[+F]F        (probability 0.33)
F -> F[-F]F        (probability 0.34)
```

**Parameter Variation:**
```cpp
float segmentLength = baseLength * (0.9f + Random::value() * 0.2f);  // +/- 10%
float branchAngle = baseAngle * (0.85f + Random::value() * 0.3f);    // +/- 15%
float thickness = baseThickness * (0.75f + Random::value() * 0.5f);  // +/- 25%
```

### Example Tree Grammars

**Realistic Oak:**
```
Axiom: F(1.0, 0.2)
Angle: 25-35 degrees (randomized)
Rules:
  F(l, w) -> F(l*0.8, w*0.7)[&(30)+F(l*0.6, w*0.6)]
                            [^(30)-F(l*0.6, w*0.6)]
                            [/(45)&(20)F(l*0.5, w*0.5)]
Iterations: 4-5
```

**Pine Tree:**
```
Axiom: F(1.0, 0.15)
Angle: 30-40 degrees
Rules:
  F(l, w) -> F(l*0.9, w*0.85)[&(35)+F(l*0.5, w*0.4)]
                              [&(35)-F(l*0.5, w*0.4)]
                              [&(35)/(90)+F(l*0.5, w*0.4)]
                              [&(35)/(90)-F(l*0.5, w*0.4)]
Iterations: 5-6
```

**Willow Tree:**
```
Axiom: F(1.0, 0.18)
Angle: 15-25 degrees (shallow for drooping)
Rules:
  F(l, w) -> F(l*0.85, w*0.75)[&(60)+F(l*0.7, w*0.5)]
                               [&(60)-F(l*0.7, w*0.5)]
Iterations: 5-7
```

### Mesh Generation from L-System

```cpp
void interpretLSystem(const std::string& lString, float angle, MeshData& mesh) {
    std::stack<TurtleState> stateStack;
    TurtleState turtle;
    turtle.position = glm::vec3(0, 0, 0);
    turtle.heading = glm::vec3(0, 1, 0);
    turtle.left = glm::vec3(-1, 0, 0);
    turtle.up = glm::vec3(0, 0, 1);
    turtle.width = 0.2f;

    for (char c : lString) {
        switch (c) {
            case 'F': {
                glm::vec3 newPos = turtle.position + turtle.heading * segmentLength;
                addCylinderSegment(mesh, turtle.position, newPos,
                                   turtle.width, turtle.width * 0.8f);
                turtle.position = newPos;
                turtle.width *= 0.8f;
                break;
            }
            case '+': applyRotation(turtle, rotateU(angle)); break;
            case '-': applyRotation(turtle, rotateU(-angle)); break;
            case '&': applyRotation(turtle, rotateL(angle)); break;
            case '^': applyRotation(turtle, rotateL(-angle)); break;
            case '\\': applyRotation(turtle, rotateH(angle)); break;
            case '/': applyRotation(turtle, rotateH(-angle)); break;
            case '[': stateStack.push(turtle); break;
            case ']': turtle = stateStack.top(); stateStack.pop(); break;
        }
    }
}
```

---

## 4. Bush and Shrub Generation

### Sources
- [NVIDIA GPU Gems - Vegetation in Crysis](https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-16-vegetation-procedural-animation-and-shading-crysis)
- [AMD GPUOpen - Procedural Grass Rendering](https://gpuopen.com/learn/mesh_shaders/mesh_shaders-procedural_grass_rendering/)
- [SpeedTree Documentation](https://unity.com/products/speedtree)
- [Procedural Plant Ecosystems](https://80.lv/articles/002sgr-generating-procedural-plant-ecosystems)

### Bush L-System Grammar

**Dense Bush:**
```
Axiom: X
Angle: 25-35 degrees
Rules:
  X -> F[+X][-X][&X][^X]
  F -> FF
Iterations: 3-4
Scale: 0.5-0.7 per iteration
```

**Flowering Shrub:**
```
Axiom: A
Angle: 30 degrees
Rules:
  A -> F[+A][-A][&B]
  B -> F[/B][\B][K]  // K = flower
  F -> FF
Iterations: 4
```

### Billboard vs. Mesh Bushes

**Billboard Approach (Performance):**
```cpp
struct BushBillboard {
    glm::vec3 position;
    float size;
    int textureIndex;  // Which bush sprite

    void render(const Camera& camera) {
        // Face camera on Y axis only (cylinder billboard)
        glm::vec3 toCamera = camera.position - position;
        toCamera.y = 0;
        float angle = atan2(toCamera.x, toCamera.z);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, angle, glm::vec3(0, 1, 0));
        model = glm::scale(model, glm::vec3(size));
        // Render quad with bush texture
    }
};
```

**Cross-Quad Approach (Better 3D):**
```cpp
void createBushMesh(MeshData& mesh, float size) {
    // Two intersecting quads at 90 degrees
    // Quad 1: aligned to X axis
    addQuad(mesh,
        glm::vec3(-size/2, 0, 0), glm::vec3(size/2, 0, 0),
        glm::vec3(size/2, size, 0), glm::vec3(-size/2, size, 0));

    // Quad 2: aligned to Z axis
    addQuad(mesh,
        glm::vec3(0, 0, -size/2), glm::vec3(0, 0, size/2),
        glm::vec3(0, size, size/2), glm::vec3(0, size, -size/2));

    // Optional: Add third quad at 45 degrees for fuller look
}
```

### Procedural Bush Mesh Generation

```cpp
struct BushGenerator {
    static MeshData generate(int branchCount, float size, unsigned int seed) {
        MeshData mesh;
        Random::seed(seed);

        glm::vec3 center(0, 0, 0);

        for (int i = 0; i < branchCount; i++) {
            // Random direction from center
            float theta = Random::range(0, 2 * PI);
            float phi = Random::range(PI * 0.3f, PI * 0.7f);  // Mostly upward

            glm::vec3 direction(
                sin(phi) * cos(theta),
                cos(phi),
                sin(phi) * sin(theta)
            );

            // Create branch
            float branchLength = size * Random::range(0.5f, 1.0f);
            float branchWidth = size * 0.03f;

            glm::vec3 endPos = center + direction * branchLength;
            addCylinder(mesh, center, endPos, branchWidth, branchWidth * 0.5f);

            // Add leaf cluster at end
            addLeafCluster(mesh, endPos, size * 0.2f);
        }

        return mesh;
    }
};
```

---

## 5. OpenGL Text Rendering

### Sources
- [LearnOpenGL - Text Rendering](https://learnopengl.com/In-Practice/Text-Rendering)
- [stb_truetype Tutorial](https://dev.to/shreyaspranav/how-to-render-truetype-fonts-in-opengl-using-stbtruetypeh-1p5k)
- [freetype-gl Library](https://github.com/rougier/freetype-gl)

### FreeType Setup

```cpp
#include <ft2build.h>
#include FT_FREETYPE_H

bool initFreeType(const char* fontPath) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not init FreeType Library" << std::endl;
        return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);  // 48 pixel height

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return true;
}
```

### Character Structure

```cpp
struct Character {
    GLuint textureID;    // Glyph texture
    glm::ivec2 size;     // Size of glyph
    glm::ivec2 bearing;  // Offset from baseline to left/top
    GLuint advance;      // Horizontal offset to next glyph
};

std::map<char, Character> characters;
```

### Loading Glyphs

```cpp
void loadCharacters(FT_Face face) {
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load glyph: " << c << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<GLuint>(face->glyph->advance.x)
        };
        characters.insert(std::pair<char, Character>(c, character));
    }
}
```

### Text Shaders

**Vertex Shader:**
```glsl
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
```

**Fragment Shader:**
```glsl
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
    float alpha = texture(text, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}
```

### Rendering Text

```cpp
void renderText(const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    textShader.use();
    textShader.setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    for (char c : text) {
        Character ch = characters[c];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.advance >> 6) * scale;  // Advance is in 1/64 pixels
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
}
```

### Alternative: stb_truetype (Header-Only)

```cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

void loadFontAtlas(const char* fontPath, int fontSize) {
    // Read font file
    FILE* fontFile = fopen(fontPath, "rb");
    fseek(fontFile, 0, SEEK_END);
    long size = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    unsigned char* fontBuffer = new unsigned char[size];
    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    // Bake font atlas
    unsigned char atlasData[512 * 512];
    stbtt_bakedchar charData[96];  // ASCII 32-127

    stbtt_BakeFontBitmap(fontBuffer, 0, fontSize,
                          atlasData, 512, 512, 32, 96, charData);

    // Create OpenGL texture from atlasData
    GLuint fontTexture;
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasData);

    delete[] fontBuffer;
}
```

---

## 6. Craig Reynolds' Steering Behaviors

### Sources
- [Craig Reynolds - Boids](https://www.red3d.com/cwr/boids/)
- [Original SIGGRAPH Paper (1987)](https://www.red3d.com/cwr/papers/1987/SIGGRAPH87.pdf)
- [Boids Algorithm Tutorial](https://vanhunteradams.com/Pico/Animal_Movement/Boids-algorithm.html)

### The Three Core Rules

**1. Separation (Avoidance)**
```cpp
glm::vec3 separate(const Creature& boid, const std::vector<Creature*>& neighbors) {
    glm::vec3 steering(0.0f);
    int count = 0;

    for (Creature* other : neighbors) {
        float distance = glm::length(boid.position - other->position);
        if (distance > 0 && distance < SEPARATION_RADIUS) {
            glm::vec3 diff = boid.position - other->position;
            diff = glm::normalize(diff) / distance;  // Weight by inverse distance
            steering += diff;
            count++;
        }
    }

    if (count > 0) {
        steering /= (float)count;
        steering = glm::normalize(steering) * MAX_SPEED;
        steering -= boid.velocity;
        steering = limit(steering, MAX_FORCE);
    }
    return steering;
}
```

**2. Alignment (Velocity Matching)**
```cpp
glm::vec3 align(const Creature& boid, const std::vector<Creature*>& neighbors) {
    glm::vec3 avgVelocity(0.0f);
    int count = 0;

    for (Creature* other : neighbors) {
        float distance = glm::length(boid.position - other->position);
        if (distance > 0 && distance < ALIGNMENT_RADIUS) {
            avgVelocity += other->velocity;
            count++;
        }
    }

    if (count > 0) {
        avgVelocity /= (float)count;
        avgVelocity = glm::normalize(avgVelocity) * MAX_SPEED;
        glm::vec3 steering = avgVelocity - boid.velocity;
        return limit(steering, MAX_FORCE);
    }
    return glm::vec3(0.0f);
}
```

**3. Cohesion (Centering)**
```cpp
glm::vec3 cohere(const Creature& boid, const std::vector<Creature*>& neighbors) {
    glm::vec3 centerOfMass(0.0f);
    int count = 0;

    for (Creature* other : neighbors) {
        float distance = glm::length(boid.position - other->position);
        if (distance > 0 && distance < COHESION_RADIUS) {
            centerOfMass += other->position;
            count++;
        }
    }

    if (count > 0) {
        centerOfMass /= (float)count;
        return seek(boid, centerOfMass);  // Steer towards center
    }
    return glm::vec3(0.0f);
}
```

### Recommended Weight Parameters

```cpp
// From various implementations
struct FlockingParams {
    float separationWeight = 1.5f;   // Highest - avoid crowding
    float alignmentWeight = 1.0f;    // Match neighbors
    float cohesionWeight = 1.0f;     // Stay with group

    float separationRadius = 8.0f;   // Keep this distance
    float alignmentRadius = 40.0f;   // Match velocity within this
    float cohesionRadius = 40.0f;    // Move toward center within this

    float maxSpeed = 6.0f;
    float minSpeed = 3.0f;
    float maxForce = 0.05f;
    float turnFactor = 0.2f;         // For boundary avoidance
};
```

### Neighborhood Definition

```cpp
bool isInNeighborhood(const Creature& boid, const Creature& other,
                       float radius, float fieldOfView) {
    glm::vec3 toOther = other.position - boid.position;
    float distance = glm::length(toOther);

    if (distance > radius || distance < 0.001f) return false;

    // Check if within field of view
    glm::vec3 heading = glm::normalize(boid.velocity);
    glm::vec3 toOtherNorm = glm::normalize(toOther);
    float angle = acos(glm::dot(heading, toOtherNorm));

    return angle < fieldOfView / 2.0f;  // Half angle on each side
}
```

### Inverse Square Distance Weighting

```cpp
// More natural, gravity-like damping
glm::vec3 separateInverseSquare(const Creature& boid,
                                 const std::vector<Creature*>& neighbors) {
    glm::vec3 steering(0.0f);

    for (Creature* other : neighbors) {
        glm::vec3 diff = boid.position - other->position;
        float distSq = glm::dot(diff, diff);
        if (distSq > 0.001f && distSq < SEPARATION_RADIUS * SEPARATION_RADIUS) {
            steering += diff / distSq;  // Inverse square falloff
        }
    }

    return limit(steering, MAX_FORCE);
}
```

---

## 7. Predator-Prey Ecosystem Dynamics

### Sources
- [Lotka-Volterra Model](https://teaching.smp.uq.edu.au/scims/Appl_analysis/Lotka_Volterra.html)
- [Unity 3D Ecosystem Research](https://www.mdpi.com/2227-9709/9/1/9)
- [Red Blob Games - Predator Prey](https://www.redblobgames.com/dynamics/predator-prey/)

### Lotka-Volterra Equations

**Differential Equations:**
```
dPrey/dt = (birthRate - predationRate * Predators) * Prey
dPredators/dt = (predationEfficiency * Prey - deathRate) * Predators
```

**Parameters:**
- `birthRate`: Prey reproduction rate (0.5-2.0)
- `predationRate`: How effectively predators catch prey (0.01-0.05)
- `predationEfficiency`: Energy gain per prey eaten (0.01-0.02)
- `deathRate`: Predator death rate without food (0.5-1.5)

### Agent-Based Implementation

```cpp
class Ecosystem {
    std::vector<Creature> herbivores;
    std::vector<Creature> carnivores;

    // Balance parameters (from research)
    const int INITIAL_HERBIVORES = 60;
    const int INITIAL_CARNIVORES = 15;
    const int MIN_HERBIVORES = 30;
    const int MIN_CARNIVORES = 5;
    const int MAX_HERBIVORES = 150;
    const int MAX_CARNIVORES = 50;

    // Target ratio: 4:1 herbivore:carnivore (Lotka-Volterra equilibrium)
    const float TARGET_RATIO = 4.0f;

    void balancePopulation() {
        int herbCount = countHerbivores();
        int carnCount = countCarnivores();

        // Prevent extinction
        if (herbCount < MIN_HERBIVORES) spawnHerbivores(MIN_HERBIVORES - herbCount + 5);
        if (carnCount < MIN_CARNIVORES) spawnCarnivores(MIN_CARNIVORES - carnCount + 2);

        // Prevent overpopulation
        if (herbCount > MAX_HERBIVORES) cullWeakest(HERBIVORE, herbCount - MAX_HERBIVORES);
        if (carnCount > MAX_CARNIVORES) cullWeakest(CARNIVORE, carnCount - MAX_CARNIVORES);
    }
};
```

### Hunting Mechanics

```cpp
void Carnivore::hunt(float deltaTime, std::vector<Creature*>& prey) {
    // Find nearest prey
    Creature* target = findNearest(prey, VISION_RANGE);

    if (target == nullptr) {
        // Wander when no prey visible
        applyWander(deltaTime);
        return;
    }

    float distance = glm::length(target->position - position);

    if (distance < ATTACK_RANGE && attackCooldown <= 0.0f) {
        // Attack!
        target->takeDamage(ATTACK_DAMAGE * deltaTime);

        if (!target->isAlive()) {
            // Gained a kill
            energy += KILL_ENERGY_GAIN;
            killCount++;
        }

        attackCooldown = ATTACK_COOLDOWN;
    } else {
        // Pursuit - predict intercept point
        glm::vec3 futurePos = target->position + target->velocity * PREDICTION_TIME;
        applySeek(futurePos);
    }
}
```

### Fleeing Mechanics

```cpp
void Herbivore::updateBehavior(float deltaTime,
                                const std::vector<Creature*>& predators,
                                const std::vector<glm::vec3>& food) {
    // Check for nearby predators
    Creature* threat = findNearestPredator(predators, VISION_RANGE);

    if (threat != nullptr) {
        float distance = glm::length(threat->position - position);

        if (distance < FLEE_DISTANCE) {
            // PANIC MODE
            fear = 1.0f;

            // Evasion - predict where predator will be
            glm::vec3 futurePos = threat->position + threat->velocity * PREDICTION_TIME;
            applyFlee(futurePos);

            // Speed boost (adrenaline)
            maxSpeed = BASE_SPEED * 1.4f;

            // Group together for safety
            applyCohesion(getHerbivoreNeighbors(), 2.0f);  // Stronger cohesion

            return;  // Skip food seeking when fleeing
        }
    }

    // Normal behavior - seek food
    fear = std::max(0.0f, fear - deltaTime * 0.5f);
    maxSpeed = BASE_SPEED;
    seekNearestFood(food);
}
```

### Reproduction Requirements

```cpp
bool Creature::canReproduce() const {
    if (type == HERBIVORE) {
        return energy > HERBIVORE_REPRODUCTION_THRESHOLD;  // e.g., 180
    } else {
        // Carnivores need both energy AND kills
        return energy > CARNIVORE_REPRODUCTION_THRESHOLD   // e.g., 170
               && killCount >= MIN_KILLS_TO_REPRODUCE;     // e.g., 2
    }
}

void Creature::reproduce() {
    if (type == HERBIVORE) {
        energy -= HERBIVORE_REPRODUCTION_COST;  // e.g., 80
    } else {
        energy -= CARNIVORE_REPRODUCTION_COST;  // e.g., 100
        killCount = 0;  // Reset kill requirement
    }
}
```

---

## 8. Spatial Partitioning

### Sources
- [Game Programming Patterns - Spatial Partition](https://gameprogrammingpatterns.com/spatial-partition.html)
- [Quadtree Implementation](https://carlosupc.github.io/Spatial-Partitioning-Quadtree/)
- [Spatial Partition Pattern](https://java-design-patterns.com/patterns/spatial-partition/)

### Grid-Based Partitioning

**Advantages:**
- Simple O(1) insertion and removal
- Constant memory footprint
- Fast position updates

**Disadvantages:**
- Performance degrades with clustering
- Fixed cell size may not suit all object distributions

```cpp
class SpatialGrid {
    static const int GRID_SIZE = 20;
    float cellWidth, cellHeight;
    std::vector<Creature*> cells[GRID_SIZE][GRID_SIZE];

public:
    SpatialGrid(float worldWidth, float worldHeight) {
        cellWidth = worldWidth / GRID_SIZE;
        cellHeight = worldHeight / GRID_SIZE;
    }

    void insert(Creature* creature) {
        int x = worldToGridX(creature->position.x);
        int z = worldToGridZ(creature->position.z);
        cells[x][z].push_back(creature);
    }

    std::vector<Creature*> query(glm::vec3 pos, float radius) {
        std::vector<Creature*> result;

        int minX = worldToGridX(pos.x - radius);
        int maxX = worldToGridX(pos.x + radius);
        int minZ = worldToGridZ(pos.z - radius);
        int maxZ = worldToGridZ(pos.z + radius);

        for (int x = minX; x <= maxX; x++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (x >= 0 && x < GRID_SIZE && z >= 0 && z < GRID_SIZE) {
                    for (Creature* c : cells[x][z]) {
                        float dist = glm::length(c->position - pos);
                        if (dist <= radius) {
                            result.push_back(c);
                        }
                    }
                }
            }
        }
        return result;
    }

    int worldToGridX(float x) {
        return std::clamp((int)((x + worldWidth/2) / cellWidth), 0, GRID_SIZE-1);
    }

    int worldToGridZ(float z) {
        return std::clamp((int)((z + worldHeight/2) / cellHeight), 0, GRID_SIZE-1);
    }
};
```

### Quadtree Implementation

```cpp
class QuadTree {
    static const int MAX_OBJECTS = 10;
    static const int MAX_LEVELS = 5;

    int level;
    std::vector<Creature*> objects;
    AABB bounds;
    QuadTree* nodes[4];

public:
    void insert(Creature* creature) {
        if (nodes[0] != nullptr) {
            int index = getIndex(creature);
            if (index != -1) {
                nodes[index]->insert(creature);
                return;
            }
        }

        objects.push_back(creature);

        if (objects.size() > MAX_OBJECTS && level < MAX_LEVELS) {
            if (nodes[0] == nullptr) split();

            int i = 0;
            while (i < objects.size()) {
                int index = getIndex(objects[i]);
                if (index != -1) {
                    nodes[index]->insert(objects[i]);
                    objects.erase(objects.begin() + i);
                } else {
                    i++;
                }
            }
        }
    }

    void split() {
        float halfW = bounds.width / 2;
        float halfH = bounds.height / 2;
        float x = bounds.x;
        float y = bounds.y;

        nodes[0] = new QuadTree(level + 1, {x + halfW, y, halfW, halfH});
        nodes[1] = new QuadTree(level + 1, {x, y, halfW, halfH});
        nodes[2] = new QuadTree(level + 1, {x, y + halfH, halfW, halfH});
        nodes[3] = new QuadTree(level + 1, {x + halfW, y + halfH, halfW, halfH});
    }

    std::vector<Creature*> retrieve(AABB area) {
        std::vector<Creature*> result;
        retrieveAll(area, result);
        return result;
    }
};
```

### Performance Comparison

| Operation | Brute Force | Grid | Quadtree |
|-----------|-------------|------|----------|
| Query All Pairs | O(n^2) | O(n * k) | O(n log n) |
| Insert | O(1) | O(1) | O(log n) |
| Update Position | O(1) | O(1) | O(log n) |
| Memory | O(n) | O(cells + n) | O(n log n) |

Where `k` = average objects per cell for grid.

---

## 9. Visual Differentiation Techniques

### Sources
- [Procedural Creature Generation MSc Thesis](https://nccastaff.bournemouth.ac.uk/jmacey/MastersProject/MSc22/01/ProceduralCreatureGenerationandAnimationforGames.pdf)
- [No Man's Sky Creature Generation Analysis](https://medium.com/@jmohon1986/procedural-creature-generation-e476851adc32)
- [Genetic Algorithms for Visual Patterns](https://pmc.ncbi.nlm.nih.gov/articles/PMC11098352/)

### Genome-to-Appearance Mapping

```cpp
struct VisualGenome {
    // Body shape
    float bodyLength;        // 0.5 - 2.0
    float bodyWidth;         // 0.3 - 1.5
    float bodyHeight;        // 0.3 - 1.5
    float bodyTaper;         // 0.0 - 1.0 (how much narrower at ends)

    // Limbs
    int limbPairs;           // 0 - 4
    float limbLength;        // 0.5 - 2.0
    float limbThickness;     // 0.1 - 0.5
    float limbSpread;        // 0.2 - 0.8 (angle from body)

    // Head
    float headSize;          // 0.3 - 1.5
    float neckLength;        // 0.0 - 1.0
    float eyeSize;           // 0.1 - 0.4
    float eyeSpacing;        // 0.2 - 0.8
    bool hasHorns;
    bool hasEars;

    // Tail
    float tailLength;        // 0.0 - 2.0
    float tailThickness;     // 0.05 - 0.3
    int tailSegments;        // 1 - 5

    // Surface details
    int spikeCount;          // 0 - 20
    float spikeLength;       // 0.1 - 0.5
    float patternScale;      // Pattern texture scale
    int patternType;         // 0=solid, 1=stripes, 2=spots, 3=patches

    // Colors
    glm::vec3 primaryColor;
    glm::vec3 secondaryColor;
    glm::vec3 accentColor;
    float colorContrast;     // 0.0 - 1.0
};
```

### Type-Based Visual Differentiation

**Herbivore Characteristics:**
- Rounder body shapes (defensive)
- Larger eyes (side-facing for wide field of view)
- Earth tones (greens, browns, tans)
- Shorter limbs relative to body
- No claws/fangs
- Possibly horns or protective features

**Carnivore Characteristics:**
- Elongated, streamlined body (pursuit)
- Forward-facing eyes (depth perception for hunting)
- Warning colors (reds, oranges, dark tones)
- Longer limbs (speed)
- Visible claws or fangs
- Muscular build

```cpp
void applyTypeTraits(VisualGenome& genome, CreatureType type) {
    if (type == HERBIVORE) {
        genome.bodyWidth *= 1.2f;       // Rounder
        genome.bodyTaper = 0.2f;        // Less streamlined
        genome.eyeSize *= 1.3f;         // Larger eyes
        genome.limbLength *= 0.8f;      // Shorter legs

        // Earth tone colors
        genome.primaryColor = glm::vec3(
            Random::range(0.3f, 0.5f),   // Low red
            Random::range(0.4f, 0.7f),   // Moderate green
            Random::range(0.2f, 0.4f)    // Low blue
        );
    } else {  // CARNIVORE
        genome.bodyWidth *= 0.8f;       // Sleeker
        genome.bodyTaper = 0.6f;        // Streamlined
        genome.eyeSize *= 0.9f;         // Slightly smaller
        genome.limbLength *= 1.2f;      // Longer legs

        // Warning colors
        genome.primaryColor = glm::vec3(
            Random::range(0.6f, 1.0f),   // High red
            Random::range(0.1f, 0.4f),   // Low green
            Random::range(0.0f, 0.2f)    // Very low blue
        );
    }
}
```

### Pattern Generation

```cpp
// Fragment shader for procedural patterns
vec3 applyPattern(vec3 baseColor, vec3 secondaryColor, vec2 uv, int patternType) {
    float pattern = 0.0;

    if (patternType == 1) {  // Stripes
        pattern = step(0.5, fract(uv.x * 10.0));
    }
    else if (patternType == 2) {  // Spots
        vec2 cell = floor(uv * 8.0);
        vec2 local = fract(uv * 8.0) - 0.5;
        float dist = length(local);
        pattern = 1.0 - smoothstep(0.2, 0.3, dist);
    }
    else if (patternType == 3) {  // Patches (noise-based)
        pattern = step(0.5, snoise(uv * 4.0));
    }

    return mix(baseColor, secondaryColor, pattern);
}
```

### Size-Based Metaball Configuration

```cpp
MeshData generateCreatureMesh(const VisualGenome& genome) {
    std::vector<Metaball> metaballs;

    // Body segments based on body length
    int bodySegments = 2 + (int)(genome.bodyLength * 2);
    float segmentSpacing = genome.bodyLength / bodySegments;

    for (int i = 0; i < bodySegments; i++) {
        float t = (float)i / (bodySegments - 1);
        float taper = 1.0f - genome.bodyTaper * (2.0f * abs(t - 0.5f));

        Metaball ball;
        ball.position = glm::vec3(0, 0, -genome.bodyLength/2 + i * segmentSpacing);
        ball.radius = genome.bodyWidth * 0.5f * taper;
        ball.strength = 1.0f;
        metaballs.push_back(ball);
    }

    // Head
    Metaball head;
    head.position = glm::vec3(0, genome.neckLength * 0.5f,
                               -genome.bodyLength/2 - genome.headSize * 0.5f);
    head.radius = genome.headSize * 0.5f;
    metaballs.push_back(head);

    // Limbs (as smaller metaballs or cylinders)
    for (int pair = 0; pair < genome.limbPairs; pair++) {
        float z = -genome.bodyLength/2 + (pair + 0.5f) * genome.bodyLength / genome.limbPairs;
        float spread = genome.limbSpread * genome.bodyWidth;

        // Left limb
        addLimbMetaballs(metaballs, glm::vec3(-spread, 0, z), genome);
        // Right limb
        addLimbMetaballs(metaballs, glm::vec3(spread, 0, z), genome);
    }

    // Generate mesh using marching cubes
    return marchingCubes(metaballs, RESOLUTION, ISO_LEVEL);
}
```

---

## Implementation Priorities

Based on this research, here are the recommended implementation priorities:

### Immediate Fixes

1. **Trees - Make 3D and Varied**
   - Add pitch (& ^) rotations to L-system interpretation
   - Increase segment count per cylinder (currently likely too few)
   - Add trunk thickness variation
   - Use different L-system rules per tree type

2. **Nametags - Add Actual Text**
   - Implement FreeType or stb_truetype
   - Create texture atlas for ASCII characters
   - Render creature ID as billboarded text

3. **Creature Visual Variety**
   - Map genome traits to metaball configurations
   - Add type-based color schemes (herbivore vs carnivore)
   - Vary limb count, body shape, and features

### New Features

4. **Bush Generation**
   - Use simplified L-system with dense branching
   - Cross-quad billboard approach for performance
   - Spawn in grass biome regions

---

## References Summary

### Primary Sources
- [Paul Bourke - Marching Cubes](https://paulbourke.net/geometry/polygonise/)
- [Craig Reynolds - Boids](https://www.red3d.com/cwr/boids/)
- [LearnOpenGL - Text Rendering](https://learnopengl.com/In-Practice/Text-Rendering)
- [Runevision - Procedural Creatures](https://blog.runevision.com/2025/01/procedural-creature-progress-2021-2024.html)
- [Game Programming Patterns - Spatial Partition](https://gameprogrammingpatterns.com/spatial-partition.html)

### Additional Resources
- [L3D GitHub - 3D L-Systems](https://github.com/abiusx/L3D)
- [Generating Trees with L-Systems](https://gpfault.net/posts/generating-trees.txt.html)
- [Unity 3D Ecosystem Research (MDPI)](https://www.mdpi.com/2227-9709/9/1/9)
- [AMD GPUOpen - Procedural Grass](https://gpuopen.com/learn/mesh_shaders/mesh_shaders-procedural_grass_rendering/)
