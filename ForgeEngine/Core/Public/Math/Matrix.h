#pragma once

// Forge Engine - Matrix Math
// Column-major 4x4 matrix with SIMD optimization

#include "Math/Vector.h"

namespace Forge::Math
{
    // ========================================================================
    // Mat4 - 4x4 Matrix (Column-Major)
    // ========================================================================

    struct alignas(64) Mat4
    {
        // Column vectors (column-major storage)
        Vec4 columns[4];

        // ====================================================================
        // Constructors
        // ====================================================================

        /// Default constructor - identity matrix
        Mat4()
        {
            columns[0] = Vec4(1, 0, 0, 0);
            columns[1] = Vec4(0, 1, 0, 0);
            columns[2] = Vec4(0, 0, 1, 0);
            columns[3] = Vec4(0, 0, 0, 1);
        }

        /// Diagonal matrix
        explicit Mat4(f32 diagonal)
        {
            columns[0] = Vec4(diagonal, 0, 0, 0);
            columns[1] = Vec4(0, diagonal, 0, 0);
            columns[2] = Vec4(0, 0, diagonal, 0);
            columns[3] = Vec4(0, 0, 0, diagonal);
        }

        /// Construct from columns
        Mat4(const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3)
        {
            columns[0] = c0;
            columns[1] = c1;
            columns[2] = c2;
            columns[3] = c3;
        }

        /// Construct from individual elements (row-major input for convenience)
        Mat4(f32 m00, f32 m01, f32 m02, f32 m03,
             f32 m10, f32 m11, f32 m12, f32 m13,
             f32 m20, f32 m21, f32 m22, f32 m23,
             f32 m30, f32 m31, f32 m32, f32 m33)
        {
            columns[0] = Vec4(m00, m10, m20, m30);
            columns[1] = Vec4(m01, m11, m21, m31);
            columns[2] = Vec4(m02, m12, m22, m32);
            columns[3] = Vec4(m03, m13, m23, m33);
        }

        // ====================================================================
        // Static Constructors
        // ====================================================================

        static Mat4 Identity() { return Mat4(); }

        static Mat4 Zero()
        {
            Mat4 m;
            m.columns[0] = Vec4::Zero();
            m.columns[1] = Vec4::Zero();
            m.columns[2] = Vec4::Zero();
            m.columns[3] = Vec4::Zero();
            return m;
        }

        // ====================================================================
        // Accessors
        // ====================================================================

        Vec4& operator[](usize col) { return columns[col]; }
        const Vec4& operator[](usize col) const { return columns[col]; }

        /// Get element (column, row)
        f32& At(usize col, usize row) { return columns[col][row]; }
        const f32& At(usize col, usize row) const { return columns[col][row]; }

        /// Get row vector
        [[nodiscard]] Vec4 Row(usize row) const
        {
            return Vec4(columns[0][row], columns[1][row], columns[2][row], columns[3][row]);
        }

        /// Get column vector
        [[nodiscard]] const Vec4& Column(usize col) const { return columns[col]; }

        /// Get raw data pointer
        f32* Data() { return &columns[0].x; }
        const f32* Data() const { return &columns[0].x; }

        // ====================================================================
        // Matrix Operations
        // ====================================================================

        /// Matrix multiplication
        [[nodiscard]] Mat4 operator*(const Mat4& m) const
        {
            Mat4 result;
            for (int col = 0; col < 4; ++col)
            {
                __m128 x = _mm_set1_ps(m.columns[col].x);
                __m128 y = _mm_set1_ps(m.columns[col].y);
                __m128 z = _mm_set1_ps(m.columns[col].z);
                __m128 w = _mm_set1_ps(m.columns[col].w);

                __m128 r = _mm_mul_ps(columns[0].simd, x);
                r = _mm_add_ps(r, _mm_mul_ps(columns[1].simd, y));
                r = _mm_add_ps(r, _mm_mul_ps(columns[2].simd, z));
                r = _mm_add_ps(r, _mm_mul_ps(columns[3].simd, w));

                result.columns[col].simd = r;
            }
            return result;
        }

        Mat4& operator*=(const Mat4& m) { *this = *this * m; return *this; }

        /// Matrix-vector multiplication
        [[nodiscard]] Vec4 operator*(const Vec4& v) const
        {
            __m128 x = _mm_set1_ps(v.x);
            __m128 y = _mm_set1_ps(v.y);
            __m128 z = _mm_set1_ps(v.z);
            __m128 w = _mm_set1_ps(v.w);

            __m128 r = _mm_mul_ps(columns[0].simd, x);
            r = _mm_add_ps(r, _mm_mul_ps(columns[1].simd, y));
            r = _mm_add_ps(r, _mm_mul_ps(columns[2].simd, z));
            r = _mm_add_ps(r, _mm_mul_ps(columns[3].simd, w));

            return Vec4(r);
        }

        /// Transform point (w=1)
        [[nodiscard]] Vec3 TransformPoint(const Vec3& p) const
        {
            Vec4 result = *this * Vec4(p, 1.0f);
            return result.XYZ();
        }

        /// Transform direction (w=0)
        [[nodiscard]] Vec3 TransformDirection(const Vec3& d) const
        {
            Vec4 result = *this * Vec4(d, 0.0f);
            return result.XYZ();
        }

        /// Scalar multiplication
        [[nodiscard]] Mat4 operator*(f32 s) const
        {
            Mat4 result;
            __m128 scalar = _mm_set1_ps(s);
            result.columns[0].simd = _mm_mul_ps(columns[0].simd, scalar);
            result.columns[1].simd = _mm_mul_ps(columns[1].simd, scalar);
            result.columns[2].simd = _mm_mul_ps(columns[2].simd, scalar);
            result.columns[3].simd = _mm_mul_ps(columns[3].simd, scalar);
            return result;
        }

        Mat4& operator*=(f32 s) { *this = *this * s; return *this; }

        /// Matrix addition
        [[nodiscard]] Mat4 operator+(const Mat4& m) const
        {
            Mat4 result;
            result.columns[0].simd = _mm_add_ps(columns[0].simd, m.columns[0].simd);
            result.columns[1].simd = _mm_add_ps(columns[1].simd, m.columns[1].simd);
            result.columns[2].simd = _mm_add_ps(columns[2].simd, m.columns[2].simd);
            result.columns[3].simd = _mm_add_ps(columns[3].simd, m.columns[3].simd);
            return result;
        }

        Mat4& operator+=(const Mat4& m) { *this = *this + m; return *this; }

        /// Matrix subtraction
        [[nodiscard]] Mat4 operator-(const Mat4& m) const
        {
            Mat4 result;
            result.columns[0].simd = _mm_sub_ps(columns[0].simd, m.columns[0].simd);
            result.columns[1].simd = _mm_sub_ps(columns[1].simd, m.columns[1].simd);
            result.columns[2].simd = _mm_sub_ps(columns[2].simd, m.columns[2].simd);
            result.columns[3].simd = _mm_sub_ps(columns[3].simd, m.columns[3].simd);
            return result;
        }

        Mat4& operator-=(const Mat4& m) { *this = *this - m; return *this; }

        // ====================================================================
        // Matrix Properties
        // ====================================================================

        /// Transpose
        [[nodiscard]] Mat4 Transposed() const
        {
            Mat4 result;
            __m128 t0 = _mm_unpacklo_ps(columns[0].simd, columns[1].simd);
            __m128 t1 = _mm_unpackhi_ps(columns[0].simd, columns[1].simd);
            __m128 t2 = _mm_unpacklo_ps(columns[2].simd, columns[3].simd);
            __m128 t3 = _mm_unpackhi_ps(columns[2].simd, columns[3].simd);

            result.columns[0].simd = _mm_movelh_ps(t0, t2);
            result.columns[1].simd = _mm_movehl_ps(t2, t0);
            result.columns[2].simd = _mm_movelh_ps(t1, t3);
            result.columns[3].simd = _mm_movehl_ps(t3, t1);
            return result;
        }

        void Transpose() { *this = Transposed(); }

        /// Determinant
        [[nodiscard]] f32 Determinant() const;

        /// Inverse (returns identity if not invertible)
        [[nodiscard]] Mat4 Inversed() const;

        void Invert() { *this = Inversed(); }

        // ====================================================================
        // Transform Matrices
        // ====================================================================

        /// Translation matrix
        [[nodiscard]] static Mat4 Translation(const Vec3& t)
        {
            Mat4 m;
            m.columns[3] = Vec4(t, 1.0f);
            return m;
        }

        /// Scale matrix
        [[nodiscard]] static Mat4 Scale(const Vec3& s)
        {
            Mat4 m;
            m.columns[0].x = s.x;
            m.columns[1].y = s.y;
            m.columns[2].z = s.z;
            return m;
        }

        /// Uniform scale matrix
        [[nodiscard]] static Mat4 Scale(f32 s)
        {
            return Scale(Vec3(s, s, s));
        }

        /// Rotation around X axis (radians)
        [[nodiscard]] static Mat4 RotationX(f32 radians)
        {
            f32 c = std::cos(radians);
            f32 s = std::sin(radians);
            Mat4 m;
            m.columns[1] = Vec4(0, c, s, 0);
            m.columns[2] = Vec4(0, -s, c, 0);
            return m;
        }

        /// Rotation around Y axis (radians)
        [[nodiscard]] static Mat4 RotationY(f32 radians)
        {
            f32 c = std::cos(radians);
            f32 s = std::sin(radians);
            Mat4 m;
            m.columns[0] = Vec4(c, 0, -s, 0);
            m.columns[2] = Vec4(s, 0, c, 0);
            return m;
        }

        /// Rotation around Z axis (radians)
        [[nodiscard]] static Mat4 RotationZ(f32 radians)
        {
            f32 c = std::cos(radians);
            f32 s = std::sin(radians);
            Mat4 m;
            m.columns[0] = Vec4(c, s, 0, 0);
            m.columns[1] = Vec4(-s, c, 0, 0);
            return m;
        }

        /// Rotation around arbitrary axis (radians)
        [[nodiscard]] static Mat4 Rotation(const Vec3& axis, f32 radians);

        /// Rotation from Euler angles (radians, XYZ order)
        [[nodiscard]] static Mat4 RotationEuler(const Vec3& eulerRadians)
        {
            return RotationZ(eulerRadians.z) * RotationY(eulerRadians.y) * RotationX(eulerRadians.x);
        }

        // ====================================================================
        // View/Projection Matrices
        // ====================================================================

        /// Look-at view matrix
        [[nodiscard]] static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
        {
            Vec3 f = (target - eye).Normalized();  // Forward
            Vec3 r = f.Cross(up).Normalized();     // Right
            Vec3 u = r.Cross(f);                   // Up (recalculated)

            Mat4 m;
            m.columns[0] = Vec4(r.x, u.x, -f.x, 0);
            m.columns[1] = Vec4(r.y, u.y, -f.y, 0);
            m.columns[2] = Vec4(r.z, u.z, -f.z, 0);
            m.columns[3] = Vec4(-r.Dot(eye), -u.Dot(eye), f.Dot(eye), 1);
            return m;
        }

        /// Perspective projection (symmetric frustum)
        /// @param fovY Field of view in radians (vertical)
        /// @param aspect Aspect ratio (width / height)
        /// @param nearZ Near clip plane
        /// @param farZ Far clip plane
        [[nodiscard]] static Mat4 Perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ)
        {
            f32 tanHalfFov = std::tan(fovY * 0.5f);
            f32 zRange = farZ - nearZ;

            Mat4 m = Mat4::Zero();
            m.columns[0].x = 1.0f / (aspect * tanHalfFov);
            m.columns[1].y = 1.0f / tanHalfFov;
            m.columns[2].z = -(farZ + nearZ) / zRange;
            m.columns[2].w = -1.0f;
            m.columns[3].z = -(2.0f * farZ * nearZ) / zRange;
            return m;
        }

        /// Perspective projection with reversed-Z (better depth precision)
        [[nodiscard]] static Mat4 PerspectiveReversedZ(f32 fovY, f32 aspect, f32 nearZ, f32 farZ);

        /// Orthographic projection
        [[nodiscard]] static Mat4 Orthographic(f32 left, f32 right, f32 bottom, f32 top,
                                                f32 nearZ, f32 farZ);

        // ====================================================================
        // Decomposition
        // ====================================================================

        /// Extract translation from matrix
        [[nodiscard]] Vec3 GetTranslation() const { return columns[3].XYZ(); }

        /// Extract scale from matrix (assumes no shear)
        [[nodiscard]] Vec3 GetScale() const
        {
            return Vec3(
                columns[0].XYZ().Length(),
                columns[1].XYZ().Length(),
                columns[2].XYZ().Length()
            );
        }

        /// Set translation component
        void SetTranslation(const Vec3& t) { columns[3] = Vec4(t, 1.0f); }

        /// Comparison
        bool operator==(const Mat4& m) const
        {
            return columns[0] == m.columns[0] &&
                   columns[1] == m.columns[1] &&
                   columns[2] == m.columns[2] &&
                   columns[3] == m.columns[3];
        }

        bool operator!=(const Mat4& m) const { return !(*this == m); }
    };

    inline Mat4 operator*(f32 s, const Mat4& m) { return m * s; }

} // namespace Forge::Math
