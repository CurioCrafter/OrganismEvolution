#pragma once

#include <glm/glm.hpp>

/**
 * @class Frustum
 * @brief Frustum culling implementation for efficient rendering.
 *
 * Extracts frustum planes from the view-projection matrix and provides
 * intersection tests for axis-aligned bounding boxes (AABB) and spheres.
 * This enables skipping off-screen objects before submitting them to the GPU.
 */
class Frustum {
public:
    /**
     * @brief Plane indices for clarity.
     * Note: Using NEAR_PLANE/FAR_PLANE to avoid Windows macro conflicts.
     */
    enum Plane {
        LEFT = 0,
        RIGHT = 1,
        BOTTOM = 2,
        TOP = 3,
        NEAR_PLANE = 4,
        FAR_PLANE = 5
    };

    /**
     * @brief Default constructor.
     */
    Frustum() = default;

    /**
     * @brief Extract frustum planes from a view-projection matrix.
     *
     * This method extracts the six frustum planes using the Gribb/Hartmann method.
     * The planes are stored in the format (A, B, C, D) where Ax + By + Cz + D = 0.
     * All planes are normalized so that the normal (A, B, C) has unit length.
     *
     * @param viewProjection The combined view-projection matrix.
     */
    void update(const glm::mat4& viewProjection);

    /**
     * @brief Test if an axis-aligned bounding box is visible.
     *
     * Uses the optimized "p-vertex" method: for each plane, find the corner
     * of the AABB that is most in the direction of the plane normal (p-vertex).
     * If this p-vertex is outside any plane, the box is completely outside.
     *
     * @param min The minimum corner of the AABB.
     * @param max The maximum corner of the AABB.
     * @return true if the box is at least partially inside the frustum.
     */
    bool isBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

    /**
     * @brief Test if a sphere is visible.
     *
     * Checks if the sphere intersects or is inside the frustum by computing
     * the signed distance from the sphere center to each plane.
     *
     * @param center The center of the sphere.
     * @param radius The radius of the sphere.
     * @return true if the sphere is at least partially inside the frustum.
     */
    bool isSphereVisible(const glm::vec3& center, float radius) const;

    /**
     * @brief Test if a point is inside the frustum.
     *
     * @param point The point to test.
     * @return true if the point is inside all frustum planes.
     */
    bool isPointVisible(const glm::vec3& point) const;

    /**
     * @brief Get the signed distance from a point to a specific plane.
     *
     * @param planeIndex The index of the plane (0-5).
     * @param point The point to test.
     * @return Positive if in front of plane, negative if behind.
     */
    float getDistanceToPlane(int planeIndex, const glm::vec3& point) const;

    /**
     * @brief Get the normal of a specific frustum plane.
     *
     * @param planeIndex The index of the plane (0-5).
     * @return The plane normal as a vec3.
     */
    glm::vec3 getPlaneNormal(int planeIndex) const;

private:
    /**
     * @brief The six frustum planes.
     *
     * Each plane is stored as (A, B, C, D) where the plane equation is:
     * Ax + By + Cz + D = 0
     *
     * The planes are normalized so that sqrt(A^2 + B^2 + C^2) = 1.
     * This makes distance calculations trivial: dist = Ax + By + Cz + D
     */
    glm::vec4 planes[6];

    /**
     * @brief Normalize a plane so its normal has unit length.
     *
     * @param plane The plane to normalize (modified in place).
     */
    static void normalizePlane(glm::vec4& plane);
};
