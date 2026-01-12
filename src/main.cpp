/*
 * OrganismEvolution - Evolution Simulator
 * Main entry point
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/Shader.h"
#include "graphics/Camera.h"
#include "core/Simulation.h"

// Window dimensions
const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

// Camera and input
Camera camera(glm::vec3(0.0f, 80.0f, 150.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseCaptured = false;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Simulation
Simulation* simulation = nullptr;

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseCaptured) return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.processKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.processKeyboard(DOWN, deltaTime);

    // Simulation controls
    static bool pausePressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pausePressed) {
        simulation->togglePause();
        pausePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pausePressed = false;
    }

    static bool plusPressed = false;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && !plusPressed) {
        simulation->increaseSpeed();
        plusPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_RELEASE) {
        plusPressed = false;
    }

    static bool minusPressed = false;
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS && !minusPressed) {
        simulation->decreaseSpeed();
        minusPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_RELEASE) {
        minusPressed = false;
    }

    // Toggle mouse capture
    static bool tabPressed = false;
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
        mouseCaptured = !mouseCaptured;
        if (mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        tabPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE) {
        tabPressed = false;
    }
}

void renderText(GLFWwindow* window, const std::string& text) {
    // Simple console output for stats (will show in terminal)
    static float lastPrint = 0.0f;
    float currentTime = glfwGetTime();
    if (currentTime - lastPrint > 1.0f) {
        std::cout << "\r" << text << std::flush;
        lastPrint = currentTime;
    }
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "    OrganismEvolution - Evolution Simulator      " << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "\nInitializing..." << std::endl;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        "OrganismEvolution - Evolution Simulator",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Print OpenGL version
    std::cout << "\nOpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // Configure OpenGL
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // Load shaders
    std::cout << "\nLoading shaders..." << std::endl;
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Initialize simulation
    std::cout << "\nInitializing simulation..." << std::endl;
    simulation = new Simulation();
    simulation->init();

    std::cout << "\n==================================================" << std::endl;
    std::cout << "Simulation started!" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  WASD      - Move camera" << std::endl;
    std::cout << "  Mouse     - Look around (press TAB to toggle)" << std::endl;
    std::cout << "  Space     - Move camera up" << std::endl;
    std::cout << "  Ctrl      - Move camera down" << std::endl;
    std::cout << "  P         - Pause/Resume" << std::endl;
    std::cout << "  +/-       - Speed up/slow down" << std::endl;
    std::cout << "  TAB       - Toggle mouse capture" << std::endl;
    std::cout << "  ESC       - Exit" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "\n";

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Update simulation
        simulation->update(deltaTime);

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Set matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                               (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                                               0.1f, 1000.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        // Set lighting
        shader.setVec3("lightPos", glm::vec3(100.0f, 150.0f, 100.0f));
        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        // Render simulation
        simulation->render(camera);

        // Display stats
        std::stringstream stats;
        stats << "Population: " << std::setw(4) << simulation->getPopulation()
              << " | Generation: " << std::setw(3) << simulation->getGeneration()
              << " | Avg Fitness: " << std::fixed << std::setprecision(2)
              << simulation->getAverageFitness()
              << " | FPS: " << std::setw(3) << (int)(1.0f / deltaTime);
        renderText(window, stats.str());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete simulation;
    glfwTerminate();

    std::cout << "\n\n==================================================" << std::endl;
    std::cout << "Simulation ended. Thank you for using OrganismEvolution!" << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
