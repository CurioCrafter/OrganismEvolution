#pragma once

// Forge Engine - Quaternion
// Rotation representation with SIMD optimization

#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Forge::Math
{
    // ========================================================================
    // Quaternion
    // ========================================================================

    struct alignas(16) Quat
    {
        union {
            struct { f32 x, y, z, w; };
            __m128 simd;
        };

        // ====================================================================
        // Constructors
        // ====================================================================

        /// Default constructor - identity quaternion
        Quat() : simd(_mm_set_ps(1, 0, 0, 0)) {}

        /// Direct component constructor
        Quat(f32 x, f32 y, f32 z, f32 w) : simd(_mm_set_ps(w, z, y, x)) {}

        /// From SIMD register
        Quat(__m128 v) : simd(v) {}

        /// From axis-angle (radians)
        static Quat FromAxisAngle(const Vec3& axis, f32 radians)
        {
            f32 halfAngle = radians * 0.5f;
            f32 s = std::sin(halfAngle);
            f32 c = std::cos(halfAngle);
            Vec3 n = axis.Normalized();
            return Quat(n.x * s, n.y * s, n.z * s, c);
        }

        /// From Euler angles (radians, XYZ order)
        static Quat FromEuler(const Vec3& eulerRadians)
        {
            f32 cx = std::cos(eulerRadians.x * 0.5f);
            f32 sx = std::sin(eulerRadians.x * 0.5f);
            f32 cy = std::cos(eulerRadians.y * 0.5f);
            f32 sy = std::sin(eulerRadians.y * 0.5f);
            f32 cz = std::cos(eulerRadians.z * 0.5f);
            f32 sz = std::sin(eulerRadians.z * 0.5f);

            return Quat(
                sx * cy * cz - cx * sy * sz,
                cx * sy * cz + sx * cy * sz,
                cx * cy * sz - sx * sy * cz,
                cx * cy * cz + sx * sy * sz
            );
        }

        /// From rotation matrix (assumes orthonormal)
        static Quat FromMatrix(const Mat4& m);

        /// Look rotation (direction to look at)
        static Quat LookRotation(const Vec3& forward, const Vec3& up = Vec3::Up());

        // ====================================================================
        // Static Constructors
        // ====================================================================

        static Quat Identity() { return Quat(); }

        // ====================================================================
        // Properties
        // ====================================================================

        /// Length squared
        [[nodiscard]] f32 LengthSq() const
        {
            return x * x + y * y + z * z + w * w;
        }

        /// Length
        [[nodiscard]] f32 Length() const
        {
            return std::sqrt(LengthSq());
        }

        /// Normalized quaternion
        [[nodiscard]] Quat Normalized() const
        {
            f32 len = Length();
            if (len > 0)
            {
                f32 invLen = 1.0f / len;
                return Quat(_mm_mul_ps(simd, _mm_set1_ps(invLen)));
            }
            return Identity();
        }

        void Normalize() { *this = Normalized(); }

        /// Conjugate (inverse for unit quaternion)
        [[nodiscard]] Quat Conjugate() const
        {
            return Quat(-x, -y, -z, w);
        }

        /// Inverse
        [[nodiscard]] Quat Inverse() const
        {
            f32 lenSq = LengthSq();
            if (lenSq > 0)
            {
                f32 invLenSq = 1.0f / lenSq;
                return Quat(-x * invLenSq, -y * invLenSq, -z * invLenSq, w * invLenSq);
            }
            return Identity();
        }

        // ====================================================================
        // Operators
        // ====================================================================

        /// Quaternion multiplication (composition of rotations)
        [[nodiscard]] Quat operator*(const Quat& q) const
        {
            return Quat(
                w * q.x + x * q.w + y * q.z - z * q.y,
                w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w,
                w * q.w - x * q.x - y * q.y - z * q.z
            );
        }

        Quat& operator*=(const Quat& q) { *this = *this * q; return *this; }

        /// Rotate a vector
        [[nodiscard]] Vec3 operator*(const Vec3& v) const
        {
            // Optimized quaternion-vector rotation
            Vec3 qv(x, y, z);
            Vec3 uv = qv.Cross(v);
            Vec3 uuv = qv.Cross(uv);
            return v + (uv * w + uuv) * 2.0f;
        }

        /// Rotate a vector (alias for operator*)
        [[nodiscard]] Vec3 RotateVector(const Vec3& v) const
        {
            return (*this) * v;
        }

        /// Scalar multiplication
        [[nodiscard]] Quat operator*(f32 s) const
        {
            return Quat(_mm_mul_ps(simd, _mm_set1_ps(s)));
        }

        Quat& operator*=(f32 s)
        {
            simd = _mm_mul_ps(simd, _mm_set1_ps(s));
            return *this;
        }

        /// Addition
        [[nodiscard]] Quat operator+(const Quat& q) const
        {
            return Quat(_mm_add_ps(simd, q.simd));
        }

        Quat& operator+=(const Quat& q)
        {
            simd = _mm_add_ps(simd, q.simd);
            return *this;
        }

        /// Subtraction
        [[nodiscard]] Quat operator-(const Quat& q) const
        {
            return Quat(_mm_sub_ps(simd, q.simd));
        }

        Quat& operator-=(const Quat& q)
        {
            simd = _mm_sub_ps(simd, q.simd);
            return *this;
        }

        /// Negation
        [[nodiscard]] Quat operator-() const
        {
            return Quat(_mm_xor_ps(simd, _mm_set1_ps(-0.0f)));
        }

        /// Comparison
        bool operator==(const Quat& q) const
        {
            return _mm_movemask_ps(_mm_cmpeq_ps(simd, q.simd)) == 0xF;
        }

        bool operator!=(const Quat& q) const { return !(*this == q); }

        // ====================================================================
        // Conversion
        // ====================================================================

        /// Convert to rotation matrix
        [[nodiscard]] Mat4 ToMatrix() const
        {
            f32 xx = x * x, yy = y * y, zz = z * z;
            f32 xy = x * y, xz = x * z, yz = y * z;
            f32 wx = w * x, wy = w * y, wz = w * z;

            Mat4 m;
            m[0] = Vec4(1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0);
            m[1] = Vec4(2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0);
            m[2] = Vec4(2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0);
            m[3] = Vec4(0, 0, 0, 1);
            return m;
        }

        /// Convert to Euler angles (radians)
        [[nodiscard]] Vec3 ToEuler() const
        {
            Vec3 euler;

            // Roll (x-axis rotation)
            f32 sinr_cosp = 2 * (w * x + y * z);
            f32 cosr_cosp = 1 - 2 * (x * x + y * y);
            euler.x = std::atan2(sinr_cosp, cosr_cosp);

            // Pitch (y-axis rotation)
            f32 sinp = 2 * (w * y - z * x);
            if (std::abs(sinp) >= 1)
                euler.y = std::copysign(HALF_PI, sinp);
            else
                euler.y = std::asin(sinp);

            // Yaw (z-axis rotation)
            f32 siny_cosp = 2 * (w * z + x * y);
            f32 cosy_cosp = 1 - 2 * (y * y + z * z);
            euler.z = std::atan2(siny_cosp, cosy_cosp);

            return euler;
        }

        /// Get axis of rotation
        [[nodiscard]] Vec3 GetAxis() const
        {
            f32 s = std::sqrt(1.0f - w * w);
            if (s < 0.001f)
                return Vec3::UnitX();
            return Vec3(x / s, y / s, z / s);
        }

        /// Get angle of rotation (radians)
        [[nodiscard]] f32 GetAngle() const
        {
            return 2.0f * std::acos(Clamp(w, -1.0f, 1.0f));
        }

        // ====================================================================
        // Interpolation
        // ====================================================================

        /// Dot product
        [[nodiscard]] f32 Dot(const Quat& q) const
        {
            return x * q.x + y * q.y + z * q.z + w * q.w;
        }

        /// Spherical linear interpolation
        [[nodiscard]] static Quat Slerp(const Quat& a, const Quat& b, f32 t)
        {
            Quat q = b;
            f32 dot = a.Dot(b);

            // Ensure shortest path
            if (dot < 0.0f)
            {
                q = -q;
                dot = -dot;
            }

            // If very close, use linear interpolation to avoid numerical issues
            if (dot > 0.9995f)
            {
                return Quat(
                    a.x + t * (q.x - a.x),
                    a.y + t * (q.y - a.y),
                    a.z + t * (q.z - a.z),
                    a.w + t * (q.w - a.w)
                ).Normalized();
            }

            f32 theta = std::acos(dot);
            f32 sinTheta = std::sin(theta);
            f32 wa = std::sin((1.0f - t) * theta) / sinTheta;
            f32 wb = std::sin(t * theta) / sinTheta;

            return Quat(
                wa * a.x + wb * q.x,
                wa * a.y + wb * q.y,
                wa * a.z + wb * q.z,
                wa * a.w + wb * q.w
            );
        }

        /// Normalized linear interpolation (faster than slerp, similar results)
        [[nodiscard]] static Quat Nlerp(const Quat& a, const Quat& b, f32 t)
        {
            Quat q = b;

            // Ensure shortest path
            if (a.Dot(b) < 0.0f)
                q = -q;

            return Quat(
                a.x + t * (q.x - a.x),
                a.y + t * (q.y - a.y),
                a.z + t * (q.z - a.z),
                a.w + t * (q.w - a.w)
            ).Normalized();
        }

        // ====================================================================
        // Direction Vectors
        // ====================================================================

        /// Get forward direction (local +Z rotated by this quaternion)
        [[nodiscard]] Vec3 Forward() const { return *this * Vec3::Forward(); }

        /// Get right direction (local +X rotated by this quaternion)
        [[nodiscard]] Vec3 Right() const { return *this * Vec3::Right(); }

        /// Get up direction (local +Y rotated by this quaternion)
        [[nodiscard]] Vec3 Up() const { return *this * Vec3::Up(); }
    };

    inline Quat operator*(f32 s, const Quat& q) { return q * s; }

} // namespace Forge::Math
