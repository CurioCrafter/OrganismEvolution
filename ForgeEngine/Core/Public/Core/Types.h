#pragma once

// Forge Engine - Fundamental Types
// These type aliases provide consistent sizing across platforms

#include <cstdint>
#include <cstddef>
#include <limits>

namespace Forge
{
    // ========================================================================
    // Integer Types
    // ========================================================================

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using usize = std::size_t;
    using isize = std::ptrdiff_t;

    // ========================================================================
    // Floating Point Types
    // ========================================================================

    using f32 = float;
    using f64 = double;

    static_assert(sizeof(f32) == 4, "f32 must be 4 bytes");
    static_assert(sizeof(f64) == 8, "f64 must be 8 bytes");

    // ========================================================================
    // Byte Type
    // Note: Using 'Byte' (capitalized) to avoid conflicts with Windows SDK 'byte'
    // ========================================================================

    using Byte = std::byte;
    // Legacy alias - avoid using in new code to prevent Windows SDK conflicts
    // using byte = std::byte;

    // ========================================================================
    // Pointer Types
    // ========================================================================

    using uptr = std::uintptr_t;
    using iptr = std::intptr_t;

    // ========================================================================
    // Null Pointer Type
    // ========================================================================

    using nullptr_t = std::nullptr_t;

    // ========================================================================
    // Limits
    // ========================================================================

    inline constexpr u8  U8_MAX  = std::numeric_limits<u8>::max();
    inline constexpr u16 U16_MAX = std::numeric_limits<u16>::max();
    inline constexpr u32 U32_MAX = std::numeric_limits<u32>::max();
    inline constexpr u64 U64_MAX = std::numeric_limits<u64>::max();

    inline constexpr i8  I8_MIN  = std::numeric_limits<i8>::min();
    inline constexpr i8  I8_MAX  = std::numeric_limits<i8>::max();
    inline constexpr i16 I16_MIN = std::numeric_limits<i16>::min();
    inline constexpr i16 I16_MAX = std::numeric_limits<i16>::max();
    inline constexpr i32 I32_MIN = std::numeric_limits<i32>::min();
    inline constexpr i32 I32_MAX = std::numeric_limits<i32>::max();
    inline constexpr i64 I64_MIN = std::numeric_limits<i64>::min();
    inline constexpr i64 I64_MAX = std::numeric_limits<i64>::max();

    inline constexpr f32 F32_MIN = std::numeric_limits<f32>::lowest();
    inline constexpr f32 F32_MAX = std::numeric_limits<f32>::max();
    inline constexpr f32 F32_EPSILON = std::numeric_limits<f32>::epsilon();

    inline constexpr f64 F64_MIN = std::numeric_limits<f64>::lowest();
    inline constexpr f64 F64_MAX = std::numeric_limits<f64>::max();
    inline constexpr f64 F64_EPSILON = std::numeric_limits<f64>::epsilon();

    // ========================================================================
    // Invalid/Sentinel Values
    // ========================================================================

    inline constexpr u32 INVALID_INDEX = U32_MAX;
    inline constexpr u64 INVALID_HANDLE = U64_MAX;

    // ========================================================================
    // Alignment Helpers
    // ========================================================================

    /// Align a value up to the specified alignment (must be power of 2)
    template<typename T>
    [[nodiscard]] constexpr T AlignUp(T value, T alignment) noexcept
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    /// Align a value down to the specified alignment (must be power of 2)
    template<typename T>
    [[nodiscard]] constexpr T AlignDown(T value, T alignment) noexcept
    {
        return value & ~(alignment - 1);
    }

    /// Check if a value is aligned to the specified alignment
    template<typename T>
    [[nodiscard]] constexpr bool IsAligned(T value, T alignment) noexcept
    {
        return (value & (alignment - 1)) == 0;
    }

    /// Check if a pointer is aligned
    [[nodiscard]] inline bool IsPtrAligned(const void* ptr, usize alignment) noexcept
    {
        return (reinterpret_cast<uptr>(ptr) & (alignment - 1)) == 0;
    }

    // ========================================================================
    // Bit Manipulation
    // ========================================================================

    /// Check if value is a power of 2
    template<typename T>
    [[nodiscard]] constexpr bool IsPowerOf2(T value) noexcept
    {
        return value > 0 && (value & (value - 1)) == 0;
    }

    /// Get next power of 2 >= value
    [[nodiscard]] constexpr u32 NextPowerOf2(u32 value) noexcept
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value + 1;
    }

    [[nodiscard]] constexpr u64 NextPowerOf2(u64 value) noexcept
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        return value + 1;
    }

    // ========================================================================
    // Utility
    // ========================================================================

    /// Convert kilobytes to bytes
    [[nodiscard]] constexpr usize KB(usize n) noexcept { return n * 1024; }

    /// Convert megabytes to bytes
    [[nodiscard]] constexpr usize MB(usize n) noexcept { return n * 1024 * 1024; }

    /// Convert gigabytes to bytes
    [[nodiscard]] constexpr usize GB(usize n) noexcept { return n * 1024 * 1024 * 1024; }

} // namespace Forge
