#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Frustum.h"

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 25.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 50.0f, 100.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    glm::mat4 getViewMatrix();
    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    void updateCameraVectors();

    /**
     * @brief Update the frustum from the current view-projection matrix.
     * @param aspectRatio The aspect ratio (width/height) for the projection.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     */
    void updateFrustum(float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

    /**
     * @brief Get the frustum for culling tests.
     * @return Reference to the camera's frustum.
     */
    const Frustum& getFrustum() const { return frustum; }

    /**
     * @brief Get the projection matrix.
     * @param aspectRatio The aspect ratio for the projection.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     * @return The projection matrix.
     */
    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f) const;

    /**
     * @brief Get the combined view-projection matrix.
     * @param aspectRatio The aspect ratio for the projection.
     * @param nearPlane The near clipping plane distance.
     * @param farPlane The far clipping plane distance.
     * @return The combined view-projection matrix.
     */
    glm::mat4 getViewProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f);

private:
    Frustum frustum;
};
