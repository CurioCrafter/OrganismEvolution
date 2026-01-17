#pragma once

// Forge Engine - Vector Math
// SIMD-optimized vector types for high-performance math

#include "Core/CoreMinimal.h"
#include <cmath>
#include <algorithm>
#include <immintrin.h>

namespace Forge::Math
{
    // ========================================================================
    // Forward Declarations
    // ========================================================================

    struct Vec2;
    struct Vec3;
    struct Vec4;

    // ========================================================================
    // Vec2 - 2D Vector
    // ========================================================================

    struct Vec2
    {
        f32 x, y;

        // Constructors
        constexpr Vec2() : x(0), y(0) {}
        constexpr Vec2(f32 scalar) : x(scalar), y(scalar) {}
        constexpr Vec2(f32 x, f32 y) : x(x), y(y) {}

        // Static constructors
        static constexpr Vec2 Zero() { return Vec2(0, 0); }
        static constexpr Vec2 One() { return Vec2(1, 1); }
        static constexpr Vec2 UnitX() { return Vec2(1, 0); }
        static constexpr Vec2 UnitY() { return Vec2(0, 1); }

        // Accessors
        f32& operator[](usize i) { return (&x)[i]; }
        const f32& operator[](usize i) const { return (&x)[i]; }

        // Arithmetic operators
        Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
        Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
        Vec2 operator*(const Vec2& v) const { return Vec2(x * v.x, y * v.y); }
        Vec2 operator/(const Vec2& v) const { return Vec2(x / v.x, y / v.y); }
        Vec2 operator*(f32 s) const { return Vec2(x * s, y * s); }
        Vec2 operator/(f32 s) const { f32 inv = 1.0f / s; return Vec2(x * inv, y * inv); }
        Vec2 operator-() const { return Vec2(-x, -y); }

        Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
        Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; return *this; }
        Vec2& operator*=(const Vec2& v) { x *= v.x; y *= v.y; return *this; }
        Vec2& operator/=(const Vec2& v) { x /= v.x; y /= v.y; return *this; }
        Vec2& operator*=(f32 s) { x *= s; y *= s; return *this; }
        Vec2& operator/=(f32 s) { f32 inv = 1.0f / s; x *= inv; y *= inv; return *this; }

        // Comparison
        bool operator==(const Vec2& v) const { return x == v.x && y == v.y; }
        bool operator!=(const Vec2& v) const { return !(*this == v); }

        // Operations
        [[nodiscard]] f32 Dot(const Vec2& v) const { return x * v.x + y * v.y; }
        [[nodiscard]] f32 Cross(const Vec2& v) const { return x * v.y - y * v.x; }
        [[nodiscard]] f32 LengthSq() const { return x * x + y * y; }
        [[nodiscard]] f32 Length() const { return std::sqrt(LengthSq()); }
        [[nodiscard]] Vec2 Normalized() const { f32 len = Length(); return len > 0 ? *this / len : Zero(); }
        void Normalize() { *this = Normalized(); }

        [[nodiscard]] Vec2 Perpendicular() const { return Vec2(-y, x); }
        [[nodiscard]] Vec2 Reflect(const Vec2& normal) const { return *this - normal * 2.0f * Dot(normal); }

        [[nodiscard]] static Vec2 Lerp(const Vec2& a, const Vec2& b, f32 t) {
            return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
        }

        [[nodiscard]] static f32 Distance(const Vec2& a, const Vec2& b) {
            return (b - a).Length();
        }

        [[nodiscard]] static f32 DistanceSq(const Vec2& a, const Vec2& b) {
            return (b - a).LengthSq();
        }
    };

    inline Vec2 operator*(f32 s, const Vec2& v) { return v * s; }

    // ========================================================================
    // Vec3 - 3D Vector
    // ========================================================================

    struct alignas(16) Vec3
    {
        f32 x, y, z;
        f32 _pad; // Padding for SIMD alignment

        // Constructors
        constexpr Vec3() : x(0), y(0), z(0), _pad(0) {}
        constexpr Vec3(f32 scalar) : x(scalar), y(scalar), z(scalar), _pad(0) {}
        constexpr Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z), _pad(0) {}
        constexpr Vec3(const Vec2& v, f32 z) : x(v.x), y(v.y), z(z), _pad(0) {}

        // Static constructors
        static constexpr Vec3 Zero() { return Vec3(0, 0, 0); }
        static constexpr Vec3 One() { return Vec3(1, 1, 1); }
        static constexpr Vec3 UnitX() { return Vec3(1, 0, 0); }
        static constexpr Vec3 UnitY() { return Vec3(0, 1, 0); }
        static constexpr Vec3 UnitZ() { return Vec3(0, 0, 1); }
        static constexpr Vec3 Up() { return Vec3(0, 1, 0); }
        static constexpr Vec3 Down() { return Vec3(0, -1, 0); }
        static constexpr Vec3 Forward() { return Vec3(0, 0, 1); }
        static constexpr Vec3 Back() { return Vec3(0, 0, -1); }
        static constexpr Vec3 Right() { return Vec3(1, 0, 0); }
        static constexpr Vec3 Left() { return Vec3(-1, 0, 0); }

        // Accessors
        f32& operator[](usize i) { return (&x)[i]; }
        const f32& operator[](usize i) const { return (&x)[i]; }

        // Convert to Vec2
        Vec2 XY() const { return Vec2(x, y); }
        Vec2 XZ() const { return Vec2(x, z); }

        // SIMD load/store
        __m128 ToSIMD() const { return _mm_load_ps(&x); }
        static Vec3 FromSIMD(__m128 v) { Vec3 result; _mm_store_ps(&result.x, v); return result; }

        // Arithmetic operators
        Vec3 operator+(const Vec3& v) const {
            return FromSIMD(_mm_add_ps(ToSIMD(), v.ToSIMD()));
        }
        Vec3 operator-(const Vec3& v) const {
            return FromSIMD(_mm_sub_ps(ToSIMD(), v.ToSIMD()));
        }
        Vec3 operator*(const Vec3& v) const {
            return FromSIMD(_mm_mul_ps(ToSIMD(), v.ToSIMD()));
        }
        Vec3 operator/(const Vec3& v) const {
            return FromSIMD(_mm_div_ps(ToSIMD(), v.ToSIMD()));
        }
        Vec3 operator*(f32 s) const {
            return FromSIMD(_mm_mul_ps(ToSIMD(), _mm_set1_ps(s)));
        }
        Vec3 operator/(f32 s) const {
            return FromSIMD(_mm_div_ps(ToSIMD(), _mm_set1_ps(s)));
        }
        Vec3 operator-() const {
            return FromSIMD(_mm_xor_ps(ToSIMD(), _mm_set1_ps(-0.0f)));
        }

        Vec3& operator+=(const Vec3& v) { *this = *this + v; return *this; }
        Vec3& operator-=(const Vec3& v) { *this = *this - v; return *this; }
        Vec3& operator*=(const Vec3& v) { *this = *this * v; return *this; }
        Vec3& operator/=(const Vec3& v) { *this = *this / v; return *this; }
        Vec3& operator*=(f32 s) { *this = *this * s; return *this; }
        Vec3& operator/=(f32 s) { *this = *this / s; return *this; }

        // Comparison
        bool operator==(const Vec3& v) const { return x == v.x && y == v.y && z == v.z; }
        bool operator!=(const Vec3& v) const { return !(*this == v); }

        // Operations
        [[nodiscard]] f32 Dot(const Vec3& v) const {
            __m128 mul = _mm_mul_ps(ToSIMD(), v.ToSIMD());
            __m128 hadd1 = _mm_hadd_ps(mul, mul);
            __m128 hadd2 = _mm_hadd_ps(hadd1, hadd1);
            return _mm_cvtss_f32(hadd2);
        }

        [[nodiscard]] Vec3 Cross(const Vec3& v) const {
            return Vec3(
                y * v.z - z * v.y,
                z * v.x - x * v.z,
                x * v.y - y * v.x
            );
        }

        [[nodiscard]] f32 LengthSq() const { return Dot(*this); }
        [[nodiscard]] f32 LengthSquared() const { return LengthSq(); }  // Alias
        [[nodiscard]] f32 Length() const { return std::sqrt(LengthSq()); }

        [[nodiscard]] Vec3 Normalized() const {
            f32 len = Length();
            return len > 0 ? *this / len : Zero();
        }

        void Normalize() { *this = Normalized(); }

        [[nodiscard]] Vec3 Reflect(const Vec3& normal) const {
            return *this - normal * 2.0f * Dot(normal);
        }

        [[nodiscard]] static Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t) {
            return a + (b - a) * t;
        }

        [[nodiscard]] static f32 Distance(const Vec3& a, const Vec3& b) {
            return (b - a).Length();
        }

        [[nodiscard]] static f32 DistanceSq(const Vec3& a, const Vec3& b) {
            return (b - a).LengthSq();
        }

        [[nodiscard]] static f32 Angle(const Vec3& from, const Vec3& to) {
            f32 dot = from.Normalized().Dot(to.Normalized());
            dot = std::clamp(dot, -1.0f, 1.0f);
            return std::acos(dot);
        }

        [[nodiscard]] static Vec3 Min(const Vec3& a, const Vec3& b) {
            return FromSIMD(_mm_min_ps(a.ToSIMD(), b.ToSIMD()));
        }

        [[nodiscard]] static Vec3 Max(const Vec3& a, const Vec3& b) {
            return FromSIMD(_mm_max_ps(a.ToSIMD(), b.ToSIMD()));
        }
    };

    inline Vec3 operator*(f32 s, const Vec3& v) { return v * s; }

    // ========================================================================
    // Vec4 - 4D Vector / SIMD Vector
    // ========================================================================

    struct alignas(16) Vec4
    {
        union {
            struct { f32 x, y, z, w; };
            struct { f32 r, g, b, a; };
            __m128 simd;
        };

        // Constructors
        Vec4() : simd(_mm_setzero_ps()) {}
        Vec4(f32 scalar) : simd(_mm_set1_ps(scalar)) {}
        Vec4(f32 x, f32 y, f32 z, f32 w) : simd(_mm_set_ps(w, z, y, x)) {}
        Vec4(const Vec3& v, f32 w) : simd(_mm_set_ps(w, v.z, v.y, v.x)) {}
        Vec4(const Vec2& v, f32 z, f32 w) : simd(_mm_set_ps(w, z, v.y, v.x)) {}
        Vec4(__m128 v) : simd(v) {}

        // Static constructors
        static Vec4 Zero() { return Vec4(); }
        static Vec4 One() { return Vec4(1.0f); }
        static Vec4 UnitX() { return Vec4(1, 0, 0, 0); }
        static Vec4 UnitY() { return Vec4(0, 1, 0, 0); }
        static Vec4 UnitZ() { return Vec4(0, 0, 1, 0); }
        static Vec4 UnitW() { return Vec4(0, 0, 0, 1); }

        // Accessors
        f32& operator[](usize i) { return (&x)[i]; }
        const f32& operator[](usize i) const { return (&x)[i]; }

        // Convert to lower dimensions
        Vec2 XY() const { return Vec2(x, y); }
        Vec3 XYZ() const { return Vec3(x, y, z); }

        // Arithmetic operators
        Vec4 operator+(const Vec4& v) const { return Vec4(_mm_add_ps(simd, v.simd)); }
        Vec4 operator-(const Vec4& v) const { return Vec4(_mm_sub_ps(simd, v.simd)); }
        Vec4 operator*(const Vec4& v) const { return Vec4(_mm_mul_ps(simd, v.simd)); }
        Vec4 operator/(const Vec4& v) const { return Vec4(_mm_div_ps(simd, v.simd)); }
        Vec4 operator*(f32 s) const { return Vec4(_mm_mul_ps(simd, _mm_set1_ps(s))); }
        Vec4 operator/(f32 s) const { return Vec4(_mm_div_ps(simd, _mm_set1_ps(s))); }
        Vec4 operator-() const { return Vec4(_mm_xor_ps(simd, _mm_set1_ps(-0.0f))); }

        Vec4& operator+=(const Vec4& v) { simd = _mm_add_ps(simd, v.simd); return *this; }
        Vec4& operator-=(const Vec4& v) { simd = _mm_sub_ps(simd, v.simd); return *this; }
        Vec4& operator*=(const Vec4& v) { simd = _mm_mul_ps(simd, v.simd); return *this; }
        Vec4& operator/=(const Vec4& v) { simd = _mm_div_ps(simd, v.simd); return *this; }
        Vec4& operator*=(f32 s) { simd = _mm_mul_ps(simd, _mm_set1_ps(s)); return *this; }
        Vec4& operator/=(f32 s) { simd = _mm_div_ps(simd, _mm_set1_ps(s)); return *this; }

        // Comparison
        bool operator==(const Vec4& v) const {
            return _mm_movemask_ps(_mm_cmpeq_ps(simd, v.simd)) == 0xF;
        }
        bool operator!=(const Vec4& v) const { return !(*this == v); }

        // Operations
        [[nodiscard]] f32 Dot(const Vec4& v) const {
            __m128 mul = _mm_mul_ps(simd, v.simd);
            __m128 hadd1 = _mm_hadd_ps(mul, mul);
            __m128 hadd2 = _mm_hadd_ps(hadd1, hadd1);
            return _mm_cvtss_f32(hadd2);
        }

        [[nodiscard]] f32 LengthSq() const { return Dot(*this); }
        [[nodiscard]] f32 Length() const { return std::sqrt(LengthSq()); }

        [[nodiscard]] Vec4 Normalized() const {
            f32 len = Length();
            return len > 0 ? *this / len : Zero();
        }

        void Normalize() { *this = Normalized(); }

        [[nodiscard]] static Vec4 Lerp(const Vec4& a, const Vec4& b, f32 t) {
            return a + (b - a) * t;
        }
    };

    inline Vec4 operator*(f32 s, const Vec4& v) { return v * s; }

    // ========================================================================
    // Common Math Functions
    // ========================================================================

    inline constexpr f32 PI = 3.14159265358979323846f;
    inline constexpr f32 TWO_PI = 6.28318530717958647693f;
    inline constexpr f32 HALF_PI = 1.57079632679489661923f;
    inline constexpr f32 DEG_TO_RAD = PI / 180.0f;
    inline constexpr f32 RAD_TO_DEG = 180.0f / PI;

    [[nodiscard]] inline f32 Radians(f32 degrees) { return degrees * DEG_TO_RAD; }
    [[nodiscard]] inline f32 Degrees(f32 radians) { return radians * RAD_TO_DEG; }

    template<typename T>
    [[nodiscard]] constexpr T Min(T a, T b) { return a < b ? a : b; }

    template<typename T>
    [[nodiscard]] constexpr T Max(T a, T b) { return a > b ? a : b; }

    template<typename T>
    [[nodiscard]] constexpr T Clamp(T value, T min, T max) {
        return value < min ? min : (value > max ? max : value);
    }

    template<typename T>
    [[nodiscard]] constexpr T Lerp(T a, T b, f32 t) {
        return a + (b - a) * t;
    }

    [[nodiscard]] inline f32 InverseLerp(f32 a, f32 b, f32 value) {
        return (value - a) / (b - a);
    }

    [[nodiscard]] inline f32 Remap(f32 value, f32 inMin, f32 inMax, f32 outMin, f32 outMax) {
        return Lerp(outMin, outMax, InverseLerp(inMin, inMax, value));
    }

    [[nodiscard]] inline f32 SmoothStep(f32 edge0, f32 edge1, f32 x) {
        f32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    [[nodiscard]] inline bool NearlyEqual(f32 a, f32 b, f32 epsilon = F32_EPSILON) {
        return std::abs(a - b) <= epsilon;
    }

    [[nodiscard]] inline f32 Pow(f32 base, f32 exponent) {
        return std::pow(base, exponent);
    }

    [[nodiscard]] inline f32 Sqrt(f32 value) {
        return std::sqrt(value);
    }

    [[nodiscard]] inline f32 Abs(f32 value) {
        return std::abs(value);
    }

    [[nodiscard]] inline f32 Sin(f32 radians) {
        return std::sin(radians);
    }

    [[nodiscard]] inline f32 Cos(f32 radians) {
        return std::cos(radians);
    }

    [[nodiscard]] inline f32 Tan(f32 radians) {
        return std::tan(radians);
    }

    [[nodiscard]] inline f32 Asin(f32 value) {
        return std::asin(value);
    }

    [[nodiscard]] inline f32 Acos(f32 value) {
        return std::acos(value);
    }

    [[nodiscard]] inline f32 Atan(f32 value) {
        return std::atan(value);
    }

    [[nodiscard]] inline f32 Atan2(f32 y, f32 x) {
        return std::atan2(y, x);
    }

    [[nodiscard]] inline f32 Exp(f32 value) {
        return std::exp(value);
    }

    [[nodiscard]] inline f32 Log(f32 value) {
        return std::log(value);
    }

    [[nodiscard]] inline f32 Floor(f32 value) {
        return std::floor(value);
    }

    [[nodiscard]] inline f32 Ceil(f32 value) {
        return std::ceil(value);
    }

    [[nodiscard]] inline f32 Fmod(f32 x, f32 y) {
        return std::fmod(x, y);
    }

    // TAU is 2*PI (also known as TWO_PI)
    inline constexpr f32 TAU = TWO_PI;

} // namespace Forge::Math
