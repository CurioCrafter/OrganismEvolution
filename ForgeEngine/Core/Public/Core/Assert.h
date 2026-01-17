#pragma once

// Forge Engine - Assertion System

#include "Core/Platform.h"
#include "Core/Types.h"

#include <cstdio>
#include <cstdlib>

namespace Forge
{
    // ========================================================================
    // Assertion Handler
    // ========================================================================

    /// Function signature for custom assertion handlers
    using AssertHandler = void(*)(const char* expression, const char* message,
                                   const char* file, int line, const char* function);

    /// Set a custom assertion handler (returns previous handler)
    FORGE_API AssertHandler SetAssertHandler(AssertHandler handler);

    /// Get the current assertion handler
    FORGE_API AssertHandler GetAssertHandler();

    namespace Detail
    {
        /// Default assertion handler - prints to stderr and triggers debug break
        inline void DefaultAssertHandler(const char* expression, const char* message,
                                         const char* file, int line, const char* function)
        {
            std::fprintf(stderr,
                "\n"
                "================================================================================\n"
                "ASSERTION FAILED!\n"
                "================================================================================\n"
                "Expression: %s\n"
                "Message:    %s\n"
                "File:       %s\n"
                "Line:       %d\n"
                "Function:   %s\n"
                "================================================================================\n\n",
                expression,
                message ? message : "(none)",
                file,
                line,
                function
            );
            std::fflush(stderr);

            FORGE_DEBUG_BREAK();
            std::abort();
        }

        /// Internal assertion function
        FORGE_NOINLINE inline void AssertFailed(const char* expression, const char* message,
                                                 const char* file, int line, const char* function)
        {
            static AssertHandler s_handler = DefaultAssertHandler;
            if (s_handler)
            {
                s_handler(expression, message, file, line, function);
            }
            else
            {
                DefaultAssertHandler(expression, message, file, line, function);
            }
        }

    } // namespace Detail

} // namespace Forge

// ============================================================================
// Assert Macros
// ============================================================================

#if FORGE_DEBUG

    /// Basic assertion - only active in debug builds
    #define FORGE_ASSERT(expr) \
        do { \
            if (FORGE_UNLIKELY(!(expr))) { \
                ::Forge::Detail::AssertFailed(#expr, nullptr, __FILE__, __LINE__, FORGE_FUNCSIG); \
            } \
        } while(0)

    /// Assertion with message
    #define FORGE_ASSERT_MSG(expr, msg) \
        do { \
            if (FORGE_UNLIKELY(!(expr))) { \
                ::Forge::Detail::AssertFailed(#expr, msg, __FILE__, __LINE__, FORGE_FUNCSIG); \
            } \
        } while(0)

    /// Debug-only code block
    #define FORGE_DEBUG_ONLY(code) code

#else

    #define FORGE_ASSERT(expr) FORGE_ASSUME(expr)
    #define FORGE_ASSERT_MSG(expr, msg) FORGE_ASSUME(expr)
    #define FORGE_DEBUG_ONLY(code)

#endif

// ============================================================================
// Verify Macros (Always Active)
// ============================================================================

/// Verification - always active, even in release builds
#define FORGE_VERIFY(expr) \
    do { \
        if (FORGE_UNLIKELY(!(expr))) { \
            ::Forge::Detail::AssertFailed(#expr, nullptr, __FILE__, __LINE__, FORGE_FUNCSIG); \
        } \
    } while(0)

/// Verification with message
#define FORGE_VERIFY_MSG(expr, msg) \
    do { \
        if (FORGE_UNLIKELY(!(expr))) { \
            ::Forge::Detail::AssertFailed(#expr, msg, __FILE__, __LINE__, FORGE_FUNCSIG); \
        } \
    } while(0)

// ============================================================================
// Check Macros (Soft Assertions)
// ============================================================================

#if FORGE_DEBUG

    /// Soft check - evaluates expression and returns false on failure (debug only logs)
    #define FORGE_CHECK(expr) \
        [&]() -> bool { \
            if (FORGE_UNLIKELY(!(expr))) { \
                std::fprintf(stderr, "[CHECK FAILED] %s at %s:%d\n", #expr, __FILE__, __LINE__); \
                return false; \
            } \
            return true; \
        }()

#else

    #define FORGE_CHECK(expr) (!!(expr))

#endif

// ============================================================================
// Static Assert
// ============================================================================

#define FORGE_STATIC_ASSERT(expr, msg) static_assert(expr, msg)

// ============================================================================
// Compile-Time Checks
// ============================================================================

/// Mark code path as not yet implemented
#define FORGE_NOT_IMPLEMENTED() \
    do { \
        ::Forge::Detail::AssertFailed("NOT_IMPLEMENTED", "This code path is not yet implemented", \
                                       __FILE__, __LINE__, FORGE_FUNCSIG); \
    } while(0)

/// Mark code path that should never be reached
#define FORGE_UNREACHABLE_CODE() \
    do { \
        ::Forge::Detail::AssertFailed("UNREACHABLE", "This code path should never be reached", \
                                       __FILE__, __LINE__, FORGE_FUNCSIG); \
        FORGE_UNREACHABLE(); \
    } while(0)

/// Mark a switch case default that should never be hit
#define FORGE_INVALID_ENUM_VALUE(value) \
    do { \
        ::Forge::Detail::AssertFailed("INVALID_ENUM", "Invalid enum value encountered", \
                                       __FILE__, __LINE__, FORGE_FUNCSIG); \
    } while(0)
