#pragma once

// Forge Engine - Platform Detection and Configuration

// ============================================================================
// Platform Detection
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #define FORGE_PLATFORM_WINDOWS 1
    #define FORGE_PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #define FORGE_PLATFORM_LINUX 1
    #define FORGE_PLATFORM_NAME "Linux"
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define FORGE_PLATFORM_MACOS 1
        #define FORGE_PLATFORM_NAME "macOS"
    #endif
#else
    #error "Unsupported platform"
#endif

// ============================================================================
// Compiler Detection
// ============================================================================

#if defined(_MSC_VER)
    #define FORGE_COMPILER_MSVC 1
    #define FORGE_COMPILER_VERSION _MSC_VER
    #define FORGE_COMPILER_NAME "MSVC"
#elif defined(__clang__)
    #define FORGE_COMPILER_CLANG 1
    #define FORGE_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
    #define FORGE_COMPILER_NAME "Clang"
#elif defined(__GNUC__)
    #define FORGE_COMPILER_GCC 1
    #define FORGE_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #define FORGE_COMPILER_NAME "GCC"
#else
    #error "Unsupported compiler"
#endif

// ============================================================================
// Architecture Detection
// ============================================================================

#if defined(_M_X64) || defined(__x86_64__)
    #define FORGE_ARCH_X64 1
    #define FORGE_ARCH_NAME "x64"
    #define FORGE_POINTER_SIZE 8
#elif defined(_M_IX86) || defined(__i386__)
    #define FORGE_ARCH_X86 1
    #define FORGE_ARCH_NAME "x86"
    #define FORGE_POINTER_SIZE 4
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define FORGE_ARCH_ARM64 1
    #define FORGE_ARCH_NAME "ARM64"
    #define FORGE_POINTER_SIZE 8
#else
    #error "Unsupported architecture"
#endif

// ============================================================================
// Build Configuration
// ============================================================================

#if !defined(FORGE_DEBUG) && !defined(FORGE_RELEASE)
    #if defined(_DEBUG) || defined(DEBUG)
        #define FORGE_DEBUG 1
    #else
        #define FORGE_RELEASE 1
    #endif
#endif

// ============================================================================
// Export/Import Macros
// ============================================================================

#if FORGE_PLATFORM_WINDOWS
    #define FORGE_EXPORT __declspec(dllexport)
    #define FORGE_IMPORT __declspec(dllimport)
#else
    #define FORGE_EXPORT __attribute__((visibility("default")))
    #define FORGE_IMPORT
#endif

// For static library builds
#define FORGE_API

// ============================================================================
// Compiler Hints
// ============================================================================

// Force inline
#if FORGE_COMPILER_MSVC
    #define FORGE_FORCEINLINE __forceinline
    #define FORGE_NOINLINE __declspec(noinline)
#else
    #define FORGE_FORCEINLINE __attribute__((always_inline)) inline
    #define FORGE_NOINLINE __attribute__((noinline))
#endif

// Restrict pointer aliasing
#if FORGE_COMPILER_MSVC
    #define FORGE_RESTRICT __restrict
#else
    #define FORGE_RESTRICT __restrict__
#endif

// Branch prediction hints
#if FORGE_COMPILER_MSVC
    #define FORGE_LIKELY(x) (x)
    #define FORGE_UNLIKELY(x) (x)
#else
    #define FORGE_LIKELY(x) __builtin_expect(!!(x), 1)
    #define FORGE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

// Unreachable code hint
#if FORGE_COMPILER_MSVC
    #define FORGE_UNREACHABLE() __assume(0)
#else
    #define FORGE_UNREACHABLE() __builtin_unreachable()
#endif

// Assume condition is true (optimization hint)
#if FORGE_COMPILER_MSVC
    #define FORGE_ASSUME(cond) __assume(cond)
#elif FORGE_COMPILER_CLANG
    #define FORGE_ASSUME(cond) __builtin_assume(cond)
#else
    #define FORGE_ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while(0)
#endif

// Prefetch
#if FORGE_COMPILER_MSVC
    #include <intrin.h>
    #define FORGE_PREFETCH(ptr) _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0)
#else
    #define FORGE_PREFETCH(ptr) __builtin_prefetch(ptr)
#endif

// ============================================================================
// Alignment
// ============================================================================

#define FORGE_ALIGNAS(x) alignas(x)
#define FORGE_CACHE_LINE_SIZE 64
#define FORGE_CACHE_ALIGNED FORGE_ALIGNAS(FORGE_CACHE_LINE_SIZE)

// ============================================================================
// Function Signature
// ============================================================================

#if FORGE_COMPILER_MSVC
    #define FORGE_FUNCSIG __FUNCSIG__
#else
    #define FORGE_FUNCSIG __PRETTY_FUNCTION__
#endif

// ============================================================================
// Debug Break
// ============================================================================

#if FORGE_PLATFORM_WINDOWS
    #define FORGE_DEBUG_BREAK() __debugbreak()
#elif FORGE_COMPILER_CLANG || FORGE_COMPILER_GCC
    #define FORGE_DEBUG_BREAK() __builtin_trap()
#else
    #include <signal.h>
    #define FORGE_DEBUG_BREAK() raise(SIGTRAP)
#endif

// ============================================================================
// Deprecation
// ============================================================================

#if FORGE_COMPILER_MSVC
    #define FORGE_DEPRECATED(msg) __declspec(deprecated(msg))
#else
    #define FORGE_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

// ============================================================================
// No Discard
// ============================================================================

#define FORGE_NODISCARD [[nodiscard]]
#define FORGE_MAYBE_UNUSED [[maybe_unused]]

// ============================================================================
// Stringification
// ============================================================================

#define FORGE_STRINGIFY_IMPL(x) #x
#define FORGE_STRINGIFY(x) FORGE_STRINGIFY_IMPL(x)

#define FORGE_CONCAT_IMPL(a, b) a##b
#define FORGE_CONCAT(a, b) FORGE_CONCAT_IMPL(a, b)

// ============================================================================
// Unique Identifier Generation
// ============================================================================

#define FORGE_UNIQUE_NAME(prefix) FORGE_CONCAT(prefix, __LINE__)

// ============================================================================
// Platform Utilities
// ============================================================================

#include "Core/Types.h"

namespace Forge
{
    /// Platform-specific utility functions
    struct PlatformUtils
    {
        /// Trigger a breakpoint in the debugger
        static void DebugBreak();

        /// Check if a debugger is attached
        static bool IsDebuggerAttached();

        /// Output a string to the debug console
        static void OutputDebugString(const char* message);

        /// Get the current thread ID
        static u64 GetCurrentThreadId();

        /// High-resolution performance counter
        static u64 GetPerformanceCounter();

        /// Performance counter frequency (ticks per second)
        static u64 GetPerformanceFrequency();
    };

} // namespace Forge

namespace Forge::Threading
{
    /// Set the name of the current thread (for debugging)
    void SetThreadName(const char* name);

    /// Set thread CPU affinity
    void SetThreadAffinity(u64 mask);

    /// Yield the current thread
    void YieldThread();

    /// Sleep the current thread
    void SleepThread(u32 milliseconds);

    /// Get the number of hardware threads
    u32 GetHardwareConcurrency();

} // namespace Forge::Threading
