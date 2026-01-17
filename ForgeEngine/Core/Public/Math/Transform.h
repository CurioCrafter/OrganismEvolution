#pragma once

// Forge Engine - Transform
// Position, Rotation, Scale combined transform

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Quaternion.h"

namespace Forge::Math
{
    // ========================================================================
    // Transform
    // ========================================================================

    /// Transform combining position, rotation, and scale
    /// This is the standard way to represent object transforms in the engine
    struct Transform
    {
        Vec3 position;
        Quat rotation;
        Vec3 scale;

        // ====================================================================
        // Constructors
        // ====================================================================

        /// Default constructor - identity transform
        Transform()
            : position(Vec3::Zero())
            , rotation(Quat::Identity())
            , scale(Vec3::One())
        {}

        /// Construct from components
        Transform(const Vec3& pos, const Quat& rot = Quat::Identity(),
                  const Vec3& scl = Vec3::One())
            : position(pos)
            , rotation(rot)
            , scale(scl)
        {}

        /// Construct from position only
        explicit Transform(const Vec3& pos)
            : position(pos)
            , rotation(Quat::Identity())
            , scale(Vec3::One())
        {}

        /// Construct from matrix (decomposes TRS)
        explicit Transform(const Mat4& matrix);

        // ====================================================================
        // Static Constructors
        // ====================================================================

        static Transform Identity() { return Transform(); }

        // ====================================================================
        // Matrix Conversion
        // ====================================================================

        /// Convert to 4x4 transformation matrix
        [[nodiscard]] Mat4 ToMatrix() const
        {
            Mat4 t = Mat4::Translation(position);
            Mat4 r = rotation.ToMatrix();
            Mat4 s = Mat4::Scale(scale);
            return t * r * s;
        }

        /// Convert to matrix (same as ToMatrix)
        explicit operator Mat4() const { return ToMatrix(); }

        // ====================================================================
        // Transform Operations
        // ====================================================================

        /// Transform a point
        [[nodiscard]] Vec3 TransformPoint(const Vec3& point) const
        {
            return position + rotation * (scale * point);
        }

        /// Transform a direction (ignores translation)
        [[nodiscard]] Vec3 TransformDirection(const Vec3& direction) const
        {
            return rotation * direction;
        }

        /// Transform a vector (includes scale, ignores translation)
        [[nodiscard]] Vec3 TransformVector(const Vec3& vector) const
        {
            return rotation * (scale * vector);
        }

        /// Inverse transform a point
        [[nodiscard]] Vec3 InverseTransformPoint(const Vec3& point) const
        {
            Vec3 p = point - position;
            p = rotation.Conjugate() * p;
            return p / scale;
        }

        /// Inverse transform a direction
        [[nodiscard]] Vec3 InverseTransformDirection(const Vec3& direction) const
        {
            return rotation.Conjugate() * direction;
        }

        // ====================================================================
        // Composition
        // ====================================================================

        /// Combine transforms (this * other = parent * child)
        [[nodiscard]] Transform operator*(const Transform& other) const
        {
            Transform result;
            result.scale = scale * other.scale;
            result.rotation = rotation * other.rotation;
            result.position = position + rotation * (scale * other.position);
            return result;
        }

        Transform& operator*=(const Transform& other)
        {
            *this = *this * other;
            return *this;
        }

        /// Inverse transform
        [[nodiscard]] Transform Inverse() const
        {
            Transform result;
            result.scale = Vec3(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
            result.rotation = rotation.Conjugate();
            result.position = result.rotation * (-position * result.scale);
            return result;
        }

        // ====================================================================
        // Direction Vectors
        // ====================================================================

        /// Get forward direction (local +Z)
        [[nodiscard]] Vec3 Forward() const { return rotation.Forward(); }

        /// Get right direction (local +X)
        [[nodiscard]] Vec3 Right() const { return rotation.Right(); }

        /// Get up direction (local +Y)
        [[nodiscard]] Vec3 Up() const { return rotation.Up(); }

        // ====================================================================
        // Modification
        // ====================================================================

        /// Translate by offset
        void Translate(const Vec3& offset)
        {
            position += offset;
        }

        /// Translate in local space
        void TranslateLocal(const Vec3& offset)
        {
            position += rotation * offset;
        }

        /// Rotate by quaternion
        void Rotate(const Quat& q)
        {
            rotation = q * rotation;
        }

        /// Rotate around axis (radians)
        void RotateAround(const Vec3& axis, f32 radians)
        {
            Rotate(Quat::FromAxisAngle(axis, radians));
        }

        /// Look at target
        void LookAt(const Vec3& target, const Vec3& up = Vec3::Up())
        {
            Vec3 forward = (target - position).Normalized();
            rotation = Quat::LookRotation(forward, up);
        }

        // ====================================================================
        // Interpolation
        // ====================================================================

        /// Linear interpolation
        [[nodiscard]] static Transform Lerp(const Transform& a, const Transform& b, f32 t)
        {
            return Transform(
                Vec3::Lerp(a.position, b.position, t),
                Quat::Slerp(a.rotation, b.rotation, t),
                Vec3::Lerp(a.scale, b.scale, t)
            );
        }

        // ====================================================================
        // Comparison
        // ====================================================================

        bool operator==(const Transform& other) const
        {
            return position == other.position &&
                   rotation == other.rotation &&
                   scale == other.scale;
        }

        bool operator!=(const Transform& other) const
        {
            return !(*this == other);
        }
    };

    // ========================================================================
    // AABB - Axis-Aligned Bounding Box
    // ========================================================================

    struct AABB
    {
        Vec3 min;
        Vec3 max;

        // ====================================================================
        // Constructors
        // ====================================================================

        /// Empty AABB
        AABB()
            : min(Vec3(F32_MAX))
            , max(Vec3(F32_MIN))
        {}

        /// Construct from min/max points
        AABB(const Vec3& minPoint, const Vec3& maxPoint)
            : min(minPoint)
            , max(maxPoint)
        {}

        /// Construct from center and half-extents
        static AABB FromCenterExtents(const Vec3& center, const Vec3& extents)
        {
            return AABB(center - extents, center + extents);
        }

        // ====================================================================
        // Properties
        // ====================================================================

        [[nodiscard]] Vec3 Center() const { return (min + max) * 0.5f; }
        [[nodiscard]] Vec3 Size() const { return max - min; }
        [[nodiscard]] Vec3 Extents() const { return Size() * 0.5f; }

        [[nodiscard]] bool IsValid() const
        {
            return min.x <= max.x && min.y <= max.y && min.z <= max.z;
        }

        // ====================================================================
        // Operations
        // ====================================================================

        /// Expand to include point
        void Encapsulate(const Vec3& point)
        {
            min = Vec3(Min(min.x, point.x), Min(min.y, point.y), Min(min.z, point.z));
            max = Vec3(Max(max.x, point.x), Max(max.y, point.y), Max(max.z, point.z));
        }

        /// Expand to include another AABB
        void Encapsulate(const AABB& other)
        {
            Encapsulate(other.min);
            Encapsulate(other.max);
        }

        /// Expand by amount
        void Expand(f32 amount)
        {
            min -= Vec3(amount);
            max += Vec3(amount);
        }

        /// Check if contains point
        [[nodiscard]] bool Contains(const Vec3& point) const
        {
            return point.x >= min.x && point.x <= max.x &&
                   point.y >= min.y && point.y <= max.y &&
                   point.z >= min.z && point.z <= max.z;
        }

        /// Check if intersects another AABB
        [[nodiscard]] bool Intersects(const AABB& other) const
        {
            return min.x <= other.max.x && max.x >= other.min.x &&
                   min.y <= other.max.y && max.y >= other.min.y &&
                   min.z <= other.max.z && max.z >= other.min.z;
        }

        /// Get closest point on AABB to given point
        [[nodiscard]] Vec3 ClosestPoint(const Vec3& point) const
        {
            return Vec3(
                Clamp(point.x, min.x, max.x),
                Clamp(point.y, min.y, max.y),
                Clamp(point.z, min.z, max.z)
            );
        }

        /// Transform AABB (returns axis-aligned bounds of transformed AABB)
        [[nodiscard]] AABB Transformed(const Mat4& matrix) const;
    };

    // ========================================================================
    // Ray
    // ========================================================================

    struct Ray
    {
        Vec3 origin;
        Vec3 direction;

        Ray() : origin(Vec3::Zero()), direction(Vec3::Forward()) {}
        Ray(const Vec3& origin, const Vec3& direction)
            : origin(origin)
            , direction(direction.Normalized())
        {}

        /// Get point along ray at distance t
        [[nodiscard]] Vec3 GetPoint(f32 t) const
        {
            return origin + direction * t;
        }

        /// Intersect with AABB
        [[nodiscard]] bool Intersects(const AABB& aabb, f32& tMin, f32& tMax) const;

        /// Intersect with plane
        [[nodiscard]] bool Intersects(const Vec3& planeNormal, f32 planeDistance, f32& t) const;
    };

    // ========================================================================
    // Plane
    // ========================================================================

    struct Plane
    {
        Vec3 normal;
        f32 distance;

        Plane() : normal(Vec3::Up()), distance(0) {}
        Plane(const Vec3& normal, f32 distance) : normal(normal), distance(distance) {}

        /// Construct from point on plane and normal
        Plane(const Vec3& point, const Vec3& normal)
            : normal(normal)
            , distance(normal.Dot(point))
        {}

        /// Construct from three points
        static Plane FromPoints(const Vec3& a, const Vec3& b, const Vec3& c)
        {
            Vec3 normal = (b - a).Cross(c - a).Normalized();
            return Plane(a, normal);
        }

        /// Signed distance from point to plane (positive = front side)
        [[nodiscard]] f32 SignedDistance(const Vec3& point) const
        {
            return normal.Dot(point) - distance;
        }

        /// Project point onto plane
        [[nodiscard]] Vec3 Project(const Vec3& point) const
        {
            return point - normal * SignedDistance(point);
        }
    };

} // namespace Forge::Math
