#include "Frustum.h"
#include <cmath>

void Frustum::normalizePlane(glm::vec4& plane) {
    float length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
    if (length > 0.0001f) {
        plane /= length;
    }
}

void Frustum::update(const glm::mat4& viewProjection) {
    // Extract frustum planes using the Gribb/Hartmann method.
    // For a row-major matrix M (which GLM uses column-major, so we access columns),
    // the planes are extracted as follows:
    //
    // Left:   row3 + row0
    // Right:  row3 - row0
    // Bottom: row3 + row1
    // Top:    row3 - row1
    // Near:   row3 + row2
    // Far:    row3 - row2
    //
    // Since GLM is column-major, we access M[col][row], so:
    // row0 = {M[0][0], M[1][0], M[2][0], M[3][0]}
    // row1 = {M[0][1], M[1][1], M[2][1], M[3][1]}
    // row2 = {M[0][2], M[1][2], M[2][2], M[3][2]}
    // row3 = {M[0][3], M[1][3], M[2][3], M[3][3]}

    const glm::mat4& m = viewProjection;

    // Left plane: row3 + row0
    planes[LEFT] = glm::vec4(
        m[0][3] + m[0][0],
        m[1][3] + m[1][0],
        m[2][3] + m[2][0],
        m[3][3] + m[3][0]
    );
    normalizePlane(planes[LEFT]);

    // Right plane: row3 - row0
    planes[RIGHT] = glm::vec4(
        m[0][3] - m[0][0],
        m[1][3] - m[1][0],
        m[2][3] - m[2][0],
        m[3][3] - m[3][0]
    );
    normalizePlane(planes[RIGHT]);

    // Bottom plane: row3 + row1
    planes[BOTTOM] = glm::vec4(
        m[0][3] + m[0][1],
        m[1][3] + m[1][1],
        m[2][3] + m[2][1],
        m[3][3] + m[3][1]
    );
    normalizePlane(planes[BOTTOM]);

    // Top plane: row3 - row1
    planes[TOP] = glm::vec4(
        m[0][3] - m[0][1],
        m[1][3] - m[1][1],
        m[2][3] - m[2][1],
        m[3][3] - m[3][1]
    );
    normalizePlane(planes[TOP]);

    // Near plane: row3 + row2
    planes[NEAR_PLANE] = glm::vec4(
        m[0][3] + m[0][2],
        m[1][3] + m[1][2],
        m[2][3] + m[2][2],
        m[3][3] + m[3][2]
    );
    normalizePlane(planes[NEAR_PLANE]);

    // Far plane: row3 - row2
    planes[FAR_PLANE] = glm::vec4(
        m[0][3] - m[0][2],
        m[1][3] - m[1][2],
        m[2][3] - m[2][2],
        m[3][3] - m[3][2]
    );
    normalizePlane(planes[FAR_PLANE]);
}

bool Frustum::isBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
    // For each plane, find the p-vertex (the corner of the AABB that is
    // furthest in the direction of the plane normal). If this vertex is
    // outside the plane, the entire box is outside.

    for (int i = 0; i < 6; i++) {
        const glm::vec4& plane = planes[i];

        // Find the p-vertex: for each axis, choose min or max based on
        // whether the plane normal component is positive or negative.
        glm::vec3 pVertex;
        pVertex.x = (plane.x >= 0.0f) ? max.x : min.x;
        pVertex.y = (plane.y >= 0.0f) ? max.y : min.y;
        pVertex.z = (plane.z >= 0.0f) ? max.z : min.z;

        // Calculate signed distance from p-vertex to plane
        float distance = plane.x * pVertex.x + plane.y * pVertex.y +
                        plane.z * pVertex.z + plane.w;

        // If p-vertex is outside (negative distance), the box is completely outside
        if (distance < 0.0f) {
            return false;
        }
    }

    // The box is at least partially inside all planes
    return true;
}

bool Frustum::isSphereVisible(const glm::vec3& center, float radius) const {
    // For each plane, check if the sphere is completely outside.
    // If the signed distance from center to plane is less than -radius,
    // the sphere is completely outside that plane.

    for (int i = 0; i < 6; i++) {
        const glm::vec4& plane = planes[i];

        // Signed distance from sphere center to plane
        float distance = plane.x * center.x + plane.y * center.y +
                        plane.z * center.z + plane.w;

        // If sphere is completely outside this plane
        if (distance < -radius) {
            return false;
        }
    }

    // Sphere is at least partially inside all planes
    return true;
}

bool Frustum::isPointVisible(const glm::vec3& point) const {
    for (int i = 0; i < 6; i++) {
        const glm::vec4& plane = planes[i];

        float distance = plane.x * point.x + plane.y * point.y +
                        plane.z * point.z + plane.w;

        if (distance < 0.0f) {
            return false;
        }
    }
    return true;
}

float Frustum::getDistanceToPlane(int planeIndex, const glm::vec3& point) const {
    if (planeIndex < 0 || planeIndex >= 6) {
        return 0.0f;
    }

    const glm::vec4& plane = planes[planeIndex];
    return plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w;
}

glm::vec3 Frustum::getPlaneNormal(int planeIndex) const {
    if (planeIndex < 0 || planeIndex >= 6) {
        return glm::vec3(0.0f);
    }

    return glm::vec3(planes[planeIndex].x, planes[planeIndex].y, planes[planeIndex].z);
}
