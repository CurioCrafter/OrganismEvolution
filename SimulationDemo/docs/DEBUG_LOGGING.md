# Debug Logging System

The OrganismEvolution project uses a compile-time debug logging system that allows conditional output based on build configuration. This ensures debug builds provide detailed logs while release builds remain silent (with zero runtime overhead).

## Overview

Debug logging is controlled via the `ENABLE_DEBUG_LOGGING` compile flag and implemented through macros in `src/utils/DebugLog.h`.

## Build Configuration

### Debug Builds (logs enabled)
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Builds (logs disabled)
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Macro Reference

### Core Macros

| Macro | Description | Output in Release |
|-------|-------------|-------------------|
| `DEBUG_LOG(fmt, ...)` | General debug information | Silent |
| `DEBUG_INFO(fmt, ...)` | Informational messages (startup, milestones) | Silent |
| `DEBUG_WARN(fmt, ...)` | Warnings that may indicate issues | Silent |
| `DEBUG_ERROR(fmt, ...)` | Errors - **always logged even in release** | Active |

### Utility Macros

| Macro | Description |
|-------|-------------|
| `DEBUG_LOG_ONCE(fmt, ...)` | Logs only once (useful in loops) |
| `DEBUG_LOG_IF(condition, fmt, ...)` | Conditional logging |
| `DEBUG_LOG_EVERY_N(n, fmt, ...)` | Logs every N calls |
| `DEBUG_ASSERT(condition, fmt, ...)` | Assert with message (debug only) |

### Category-Specific Macros

These can be individually enabled/disabled via compile flags:

| Macro | Compile Flag | Default |
|-------|--------------|---------|
| `SPAWN_LOG(fmt, ...)` | `DEBUG_SPAWN` | ON in Debug |
| `RENDER_LOG(fmt, ...)` | `DEBUG_RENDER` | ON in Debug |
| `AI_LOG(fmt, ...)` | `DEBUG_AI` | ON in Debug |
| `ECOSYSTEM_LOG(fmt, ...)` | `DEBUG_ECOSYSTEM` | ON in Debug |
| `SHADER_LOG(fmt, ...)` | `DEBUG_SHADER` | ON in Debug |

## Usage Examples

### Basic Logging
```cpp
#include "utils/DebugLog.h"

// Simple message
DEBUG_LOG("Creature spawned at position");

// With variables (printf-style)
DEBUG_LOG("Creature %d at (%.2f, %.2f, %.2f)", id, x, y, z);

// Warnings
DEBUG_WARN("Population low: %d creatures remaining", count);

// Errors (always output)
DEBUG_ERROR("Failed to load shader: %s", filename);
```

### Category-Specific Logging
```cpp
// Only outputs if DEBUG_SPAWN is enabled
SPAWN_LOG("Spawned %d herbivores", count);

// Only outputs if DEBUG_RENDER is enabled
RENDER_LOG("Shadow map initialized (%dx%d)", width, height);

// Only outputs if DEBUG_ECOSYSTEM is enabled
ECOSYSTEM_LOG("Herbivore population low (%d)! Spawning more.", count);
```

### Conditional and Throttled Logging
```cpp
// Log only once (first call only)
DEBUG_LOG_ONCE("Initialization complete");

// Log only if condition is true
DEBUG_LOG_IF(population < 10, "Critical population level!");

// Log every 60 calls (e.g., once per second at 60fps)
DEBUG_LOG_EVERY_N(60, "FPS: %.1f", fps);
```

## CMake Configuration Options

Control debug categories at build time:

```bash
# Enable/disable verbose logging
cmake -DDEBUG_VERBOSE=ON ..

# Disable specific categories
cmake -DDEBUG_SPAWN=OFF -DDEBUG_RENDER=OFF ..

# Enable only AI logging
cmake -DDEBUG_AI=ON -DDEBUG_SPAWN=OFF -DDEBUG_RENDER=OFF -DDEBUG_ECOSYSTEM=OFF ..
```

## Best Practices

1. **Use appropriate levels**:
   - `DEBUG_LOG` for general debugging
   - `DEBUG_INFO` for startup/milestone messages
   - `DEBUG_WARN` for recoverable issues
   - `DEBUG_ERROR` for critical failures (always shown)

2. **Use category macros** for high-frequency logs (SPAWN, RENDER, AI, ECOSYSTEM, SHADER)

3. **Use `DEBUG_LOG_EVERY_N`** for per-frame information to avoid log spam

4. **Use `DEBUG_LOG_ONCE`** for initialization messages in loops

5. **Never log sensitive data** (even in debug builds)

## Output Format

```
[DEBUG] Message here
[INFO]  Startup message
[WARN]  Warning message
[ERROR] Error message
[DEBUG] [SPAWN] Spawn-specific message
[DEBUG] [RENDER] Render-specific message
```

## Migration from printf/cout

Replace existing output statements:

```cpp
// Before:
std::cout << "Generating terrain..." << std::endl;
std::cerr << "[ERROR] Failed to load shader" << std::endl;
printf("Position: (%f, %f, %f)\n", x, y, z);

// After:
DEBUG_INFO("Generating terrain...");
DEBUG_ERROR("Failed to load shader");
DEBUG_LOG("Position: (%.2f, %.2f, %.2f)", x, y, z);
```

## Technical Details

- Macros compile to `((void)0)` in release builds (zero overhead)
- Uses `std::printf` internally for consistent formatting
- All output is flushed immediately for real-time debugging
- Thread-safe static counters for `DEBUG_LOG_ONCE` and `DEBUG_LOG_EVERY_N`
- `DEBUG_ERROR` always outputs to stderr regardless of build type
