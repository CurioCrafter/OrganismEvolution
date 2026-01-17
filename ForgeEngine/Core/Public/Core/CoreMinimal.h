#pragma once

// Forge Engine - Minimal Core Includes
// Include this header for basic engine types and utilities

#include "Core/Platform.h"
#include "Core/Types.h"
#include "Core/Assert.h"

#include <utility>
#include <type_traits>
#include <memory>
#include <span>
#include <string_view>
#include <optional>
#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include <variant>
#include <string>

namespace Forge
{
    // ========================================================================
    // String Aliases
    // ========================================================================

    using String = std::string;
    using StringView = std::string_view;

    // ========================================================================
    // Container Aliases
    // ========================================================================

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Queue = std::queue<T>;

    template<typename K, typename V>
    using HashMap = std::unordered_map<K, V>;

    // ========================================================================
    // Span Alias
    // ========================================================================

    template<typename T>
    using Span = std::span<T>;

    // ========================================================================
    // Smart Pointer Aliases
    // ========================================================================

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    template<typename T, typename... Args>
    [[nodiscard]] UniquePtr<T> MakeUnique(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    [[nodiscard]] SharedPtr<T> MakeShared(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // ========================================================================
    // Optional Alias
    // ========================================================================

    template<typename T>
    using Optional = std::optional<T>;

    inline constexpr std::nullopt_t NullOpt = std::nullopt;

    // ========================================================================
    // Function Alias
    // ========================================================================

    template<typename Signature>
    using Function = std::function<Signature>;

    // ========================================================================
    // Non-Copyable / Non-Movable Base Classes
    // ========================================================================

    /// Inherit from this to make a class non-copyable
    class NonCopyable
    {
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;

        NonCopyable(NonCopyable&&) = default;
        NonCopyable& operator=(NonCopyable&&) = default;
    };

    /// Inherit from this to make a class non-movable
    class NonMovable
    {
    protected:
        NonMovable() = default;
        ~NonMovable() = default;

        NonMovable(const NonMovable&) = delete;
        NonMovable& operator=(const NonMovable&) = delete;

        NonMovable(NonMovable&&) = delete;
        NonMovable& operator=(NonMovable&&) = delete;
    };

    // ========================================================================
    // Scope Guard
    // ========================================================================

    /// RAII scope guard for cleanup on scope exit
    template<typename Func>
    class ScopeGuard
    {
    public:
        explicit ScopeGuard(Func&& func) noexcept
            : m_func(std::move(func))
            , m_active(true)
        {}

        ~ScopeGuard()
        {
            if (m_active)
            {
                m_func();
            }
        }

        ScopeGuard(ScopeGuard&& other) noexcept
            : m_func(std::move(other.m_func))
            , m_active(other.m_active)
        {
            other.Dismiss();
        }

        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) = delete;

        /// Cancel the scope guard - cleanup won't be called
        void Dismiss() noexcept { m_active = false; }

    private:
        Func m_func;
        bool m_active;
    };

    /// Helper to create scope guards with type deduction
    template<typename Func>
    [[nodiscard]] ScopeGuard<Func> MakeScopeGuard(Func&& func)
    {
        return ScopeGuard<Func>(std::forward<Func>(func));
    }

    // Macro for convenient scope guard creation
    #define FORGE_SCOPE_EXIT \
        auto FORGE_UNIQUE_NAME(scopeGuard_) = ::Forge::Detail::ScopeGuardHelper{} + [&]()

    namespace Detail
    {
        struct ScopeGuardHelper
        {
            template<typename Func>
            ScopeGuard<Func> operator+(Func&& func)
            {
                return ScopeGuard<Func>(std::forward<Func>(func));
            }
        };
    }

    // ========================================================================
    // Type Traits Helpers
    // ========================================================================

    /// Remove const, volatile, and reference from a type
    template<typename T>
    using RemoveCVRef = std::remove_cvref_t<T>;

    /// Check if a type is any of the given types
    template<typename T, typename... Types>
    inline constexpr bool IsAnyOf = (std::is_same_v<T, Types> || ...);

    // ========================================================================
    // Hash Combine
    // ========================================================================

    /// Combine hash values (for multi-field hashing)
    template<typename T>
    inline void HashCombine(usize& seed, const T& value)
    {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    // ========================================================================
    // Enum Flags
    // ========================================================================

    /// Enable bitwise operators for an enum class
    #define FORGE_ENABLE_ENUM_FLAGS(EnumType) \
        inline constexpr EnumType operator|(EnumType a, EnumType b) noexcept { \
            return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) | \
                                          static_cast<std::underlying_type_t<EnumType>>(b)); \
        } \
        inline constexpr EnumType operator&(EnumType a, EnumType b) noexcept { \
            return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) & \
                                          static_cast<std::underlying_type_t<EnumType>>(b)); \
        } \
        inline constexpr EnumType operator^(EnumType a, EnumType b) noexcept { \
            return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) ^ \
                                          static_cast<std::underlying_type_t<EnumType>>(b)); \
        } \
        inline constexpr EnumType operator~(EnumType a) noexcept { \
            return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(a)); \
        } \
        inline constexpr EnumType& operator|=(EnumType& a, EnumType b) noexcept { \
            return a = a | b; \
        } \
        inline constexpr EnumType& operator&=(EnumType& a, EnumType b) noexcept { \
            return a = a & b; \
        } \
        inline constexpr EnumType& operator^=(EnumType& a, EnumType b) noexcept { \
            return a = a ^ b; \
        } \
        inline constexpr bool HasFlag(EnumType flags, EnumType flag) noexcept { \
            return (flags & flag) == flag; \
        }

    // Alias for compatibility
    #define FORGE_DEFINE_ENUM_FLAG_OPERATORS(EnumType) FORGE_ENABLE_ENUM_FLAGS(EnumType)

    // ========================================================================
    // Result Type (Error Handling)
    // ========================================================================

    /// Simple result type for functions that can fail
    template<typename T, typename E = StringView>
    class Result
    {
    public:
        /// Create a success result
        static Result Ok(T value) { return Result(std::move(value)); }

        /// Create an error result
        static Result Err(E error) { return Result(std::move(error), false); }

        /// Check if result is success
        [[nodiscard]] bool IsOk() const noexcept { return m_isOk; }

        /// Check if result is error
        [[nodiscard]] bool IsErr() const noexcept { return !m_isOk; }

        /// Get the value (asserts on error)
        [[nodiscard]] T& Value() &
        {
            FORGE_ASSERT_MSG(m_isOk, "Attempted to get value from error result");
            return std::get<T>(m_data);
        }

        [[nodiscard]] const T& Value() const&
        {
            FORGE_ASSERT_MSG(m_isOk, "Attempted to get value from error result");
            return std::get<T>(m_data);
        }

        [[nodiscard]] T&& Value() &&
        {
            FORGE_ASSERT_MSG(m_isOk, "Attempted to get value from error result");
            return std::move(std::get<T>(m_data));
        }

        /// Get the error (asserts on success)
        [[nodiscard]] const E& Error() const
        {
            FORGE_ASSERT_MSG(!m_isOk, "Attempted to get error from success result");
            return std::get<E>(m_data);
        }

        /// Get value or default
        [[nodiscard]] T ValueOr(T defaultValue) const
        {
            return m_isOk ? std::get<T>(m_data) : std::move(defaultValue);
        }

        /// Boolean conversion
        explicit operator bool() const noexcept { return m_isOk; }

    private:
        Result(T value) : m_data(std::move(value)), m_isOk(true) {}
        Result(E error, bool) : m_data(std::move(error)), m_isOk(false) {}

        std::variant<T, E> m_data;
        bool m_isOk;
    };

    /// Specialization for void result
    template<typename E>
    class Result<void, E>
    {
    public:
        static Result Ok() { return Result(true); }
        static Result Err(E error) { return Result(std::move(error)); }

        [[nodiscard]] bool IsOk() const noexcept { return m_isOk; }
        [[nodiscard]] bool IsErr() const noexcept { return !m_isOk; }

        [[nodiscard]] const E& Error() const
        {
            FORGE_ASSERT_MSG(!m_isOk, "Attempted to get error from success result");
            return *m_error;
        }

        explicit operator bool() const noexcept { return m_isOk; }

    private:
        Result(bool) : m_isOk(true) {}
        Result(E error) : m_error(std::move(error)), m_isOk(false) {}

        Optional<E> m_error;
        bool m_isOk;
    };

} // namespace Forge
