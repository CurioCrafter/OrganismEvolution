/*
 * DebugLog.h - Conditional Debug Logging Macros
 *
 * This header provides macros for debug output that can be
 * enabled/disabled at compile time using the ENABLE_DEBUG_LOGGING flag.
 *
 * Usage:
 *   DEBUG_LOG("Creature at position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
 *   DEBUG_WARN("Population low: %d creatures remaining", count);
 *   DEBUG_ERROR("Failed to load shader: %s", filename);
 *
 * Build Configuration:
 *   - Debug builds: Pass -DENABLE_DEBUG_LOGGING to enable all debug output
 *   - Release builds: All debug macros compile to no-ops (zero overhead)
 *
 * Logging Levels:
 *   DEBUG_LOG   - General debug information (verbose)
 *   DEBUG_WARN  - Warnings that may indicate issues
 *   DEBUG_ERROR - Errors that should always be logged (even in release)
 *   DEBUG_INFO  - Informational messages (startup, milestones)
 */

#pragma once

#include <cstdio>
#include <cstdarg>

// Allow override via compile flag: -DENABLE_DEBUG_LOGGING
// By default, debug logging is enabled for Debug builds and disabled for Release

#ifndef ENABLE_DEBUG_LOGGING
    #ifdef _DEBUG
        #define ENABLE_DEBUG_LOGGING 1
    #elif defined(DEBUG)
        #define ENABLE_DEBUG_LOGGING 1
    #elif !defined(NDEBUG)
        #define ENABLE_DEBUG_LOGGING 1
    #else
        #define ENABLE_DEBUG_LOGGING 0
    #endif
#endif

// Optional: Verbose mode for extra detailed logging
#ifndef DEBUG_VERBOSE
    #define DEBUG_VERBOSE 0
#endif

// ============================================================================
// MAIN DEBUG MACROS
// ============================================================================

#if ENABLE_DEBUG_LOGGING

    // Standard debug log - general information
    #define DEBUG_LOG(fmt, ...) \
        do { \
            std::printf("[DEBUG] " fmt "\n", ##__VA_ARGS__); \
            std::fflush(stdout); \
        } while(0)

    // Warning log - potential issues
    #define DEBUG_WARN(fmt, ...) \
        do { \
            std::printf("[WARN]  " fmt "\n", ##__VA_ARGS__); \
            std::fflush(stdout); \
        } while(0)

    // Info log - milestone/status information
    #define DEBUG_INFO(fmt, ...) \
        do { \
            std::printf("[INFO]  " fmt "\n", ##__VA_ARGS__); \
            std::fflush(stdout); \
        } while(0)

    // Verbose log - extra detailed (only if DEBUG_VERBOSE is enabled)
    #if DEBUG_VERBOSE
        #define DEBUG_VERBOSE_LOG(fmt, ...) \
            do { \
                std::printf("[VERB]  " fmt "\n", ##__VA_ARGS__); \
                std::fflush(stdout); \
            } while(0)
    #else
        #define DEBUG_VERBOSE_LOG(fmt, ...) ((void)0)
    #endif

#else
    // Release mode - all debug macros compile to nothing (zero overhead)
    #define DEBUG_LOG(fmt, ...)         ((void)0)
    #define DEBUG_WARN(fmt, ...)        ((void)0)
    #define DEBUG_INFO(fmt, ...)        ((void)0)
    #define DEBUG_VERBOSE_LOG(fmt, ...) ((void)0)
#endif

// ERROR always logs (even in release) - these indicate serious issues
#define DEBUG_ERROR(fmt, ...) \
    do { \
        std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
        std::fflush(stderr); \
    } while(0)

// ============================================================================
// UTILITY MACROS
// ============================================================================

// One-time log - only prints once (useful in loops)
#define DEBUG_LOG_ONCE(fmt, ...) \
    do { \
        static bool _logged_once_ = false; \
        if (!_logged_once_) { \
            DEBUG_LOG(fmt, ##__VA_ARGS__); \
            _logged_once_ = true; \
        } \
    } while(0)

// Conditional log - only logs if condition is true
#define DEBUG_LOG_IF(condition, fmt, ...) \
    do { \
        if (condition) { \
            DEBUG_LOG(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

// Throttled log - only logs every N calls
#define DEBUG_LOG_EVERY_N(n, fmt, ...) \
    do { \
        static int _log_counter_ = 0; \
        if (++_log_counter_ >= (n)) { \
            DEBUG_LOG(fmt, ##__VA_ARGS__); \
            _log_counter_ = 0; \
        } \
    } while(0)

// ============================================================================
// CATEGORY-SPECIFIC MACROS (for filtering output)
// ============================================================================

// Enable/disable specific categories with compile flags
// e.g., -DDEBUG_SPAWN=1 -DDEBUG_RENDER=0

#ifndef DEBUG_SPAWN
    #define DEBUG_SPAWN ENABLE_DEBUG_LOGGING
#endif

#ifndef DEBUG_RENDER
    #define DEBUG_RENDER ENABLE_DEBUG_LOGGING
#endif

#ifndef DEBUG_AI
    #define DEBUG_AI ENABLE_DEBUG_LOGGING
#endif

#ifndef DEBUG_ECOSYSTEM
    #define DEBUG_ECOSYSTEM ENABLE_DEBUG_LOGGING
#endif

#ifndef DEBUG_SHADER
    #define DEBUG_SHADER ENABLE_DEBUG_LOGGING
#endif

#if DEBUG_SPAWN
    #define SPAWN_LOG(fmt, ...) DEBUG_LOG("[SPAWN] " fmt, ##__VA_ARGS__)
#else
    #define SPAWN_LOG(fmt, ...) ((void)0)
#endif

#if DEBUG_RENDER
    #define RENDER_LOG(fmt, ...) DEBUG_LOG("[RENDER] " fmt, ##__VA_ARGS__)
#else
    #define RENDER_LOG(fmt, ...) ((void)0)
#endif

#if DEBUG_AI
    #define AI_LOG(fmt, ...) DEBUG_LOG("[AI] " fmt, ##__VA_ARGS__)
#else
    #define AI_LOG(fmt, ...) ((void)0)
#endif

#if DEBUG_ECOSYSTEM
    #define ECOSYSTEM_LOG(fmt, ...) DEBUG_LOG("[ECO] " fmt, ##__VA_ARGS__)
#else
    #define ECOSYSTEM_LOG(fmt, ...) ((void)0)
#endif

#if DEBUG_SHADER
    #define SHADER_LOG(fmt, ...) DEBUG_LOG("[SHADER] " fmt, ##__VA_ARGS__)
#else
    #define SHADER_LOG(fmt, ...) ((void)0)
#endif

// ============================================================================
// ASSERT MACRO WITH MESSAGE
// ============================================================================

#if ENABLE_DEBUG_LOGGING
    #define DEBUG_ASSERT(condition, fmt, ...) \
        do { \
            if (!(condition)) { \
                DEBUG_ERROR("ASSERT FAILED: " #condition " - " fmt, ##__VA_ARGS__); \
            } \
        } while(0)
#else
    #define DEBUG_ASSERT(condition, fmt, ...) ((void)0)
#endif
