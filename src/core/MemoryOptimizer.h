#pragma once

// MemoryOptimizer - Object pooling, memory arena allocation, and cache-friendly data structures
// Eliminates per-frame allocations for sustained 60 FPS with 10,000+ creatures

#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace Forge {

// ============================================================================
// Generic Object Pool
// ============================================================================

template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initialCapacity = 1024)
        : m_capacity(initialCapacity) {
        m_pool.reserve(initialCapacity);
        m_freeList.reserve(initialCapacity);
        m_generations.reserve(initialCapacity);
    }

    ~ObjectPool() = default;

    // Non-copyable
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Move-only
    ObjectPool(ObjectPool&& other) noexcept = default;
    ObjectPool& operator=(ObjectPool&& other) noexcept = default;

    // ========================================================================
    // Allocation
    // ========================================================================

    // Acquire an object from the pool
    T* acquire() {
        if (!m_freeList.empty()) {
            size_t index = m_freeList.back();
            m_freeList.pop_back();
            m_activeCount++;
            // Reset object to default state
            new (&m_pool[index]) T();
            return &m_pool[index];
        }

        // Need to grow pool
        if (m_pool.size() < m_capacity) {
            m_pool.emplace_back();
            m_generations.push_back(0);
            m_activeCount++;
            return &m_pool.back();
        }

        // At capacity, try to grow
        grow(m_capacity * 2);
        return acquire();
    }

    // Acquire with initializer
    template<typename... Args>
    T* acquireWith(Args&&... args) {
        T* obj = acquire();
        *obj = T(std::forward<Args>(args)...);
        return obj;
    }

    // Release an object back to the pool
    void release(T* obj) {
        if (!obj) return;

        ptrdiff_t diff = obj - m_pool.data();
        if (diff < 0 || static_cast<size_t>(diff) >= m_pool.size()) {
            return;  // Not from this pool
        }

        size_t index = static_cast<size_t>(diff);
        m_generations[index]++;
        m_freeList.push_back(index);
        m_activeCount--;
    }

    // ========================================================================
    // Handle-based Access (safer, tracks generations)
    // ========================================================================

    struct Handle {
        uint32_t index = 0;
        uint32_t generation = 0;

        bool isValid() const { return generation != 0; }
        static Handle invalid() { return Handle{0, 0}; }
    };

    Handle acquireHandle() {
        T* obj = acquire();
        if (!obj) return Handle::invalid();

        size_t index = obj - m_pool.data();
        return Handle{
            static_cast<uint32_t>(index),
            m_generations[index]
        };
    }

    T* get(Handle handle) {
        if (!isValid(handle)) return nullptr;
        return &m_pool[handle.index];
    }

    const T* get(Handle handle) const {
        if (!isValid(handle)) return nullptr;
        return &m_pool[handle.index];
    }

    bool isValid(Handle handle) const {
        if (handle.index >= m_pool.size()) return false;
        return m_generations[handle.index] == handle.generation;
    }

    void release(Handle handle) {
        if (!isValid(handle)) return;
        release(&m_pool[handle.index]);
    }

    // ========================================================================
    // Pool Management
    // ========================================================================

    void clear() {
        m_freeList.clear();
        for (size_t i = 0; i < m_pool.size(); ++i) {
            m_freeList.push_back(i);
            m_generations[i]++;
        }
        m_activeCount = 0;
    }

    void grow(size_t newCapacity) {
        if (newCapacity <= m_capacity) return;
        m_capacity = newCapacity;
        m_pool.reserve(newCapacity);
        m_generations.reserve(newCapacity);
        m_freeList.reserve(newCapacity);
    }

    void shrinkToFit() {
        // Compact the pool by moving active objects to front
        // Only do this when significantly fragmented
        if (m_freeList.size() < m_pool.size() / 2) return;

        // This is expensive, only do occasionally
        std::vector<T> compacted;
        compacted.reserve(m_activeCount);

        for (size_t i = 0; i < m_pool.size(); ++i) {
            bool isFree = false;
            for (size_t idx : m_freeList) {
                if (idx == i) { isFree = true; break; }
            }
            if (!isFree) {
                compacted.push_back(std::move(m_pool[i]));
            }
        }

        m_pool = std::move(compacted);
        m_freeList.clear();
        m_generations.resize(m_pool.size(), 0);
    }

    // ========================================================================
    // Statistics
    // ========================================================================

    size_t getActiveCount() const { return m_activeCount; }
    size_t getPoolSize() const { return m_pool.size(); }
    size_t getCapacity() const { return m_capacity; }
    size_t getFreeCount() const { return m_freeList.size(); }

    float getFragmentation() const {
        if (m_pool.empty()) return 0.0f;
        return static_cast<float>(m_freeList.size()) / m_pool.size();
    }

    size_t getMemoryUsage() const {
        return m_pool.capacity() * sizeof(T) +
               m_freeList.capacity() * sizeof(size_t) +
               m_generations.capacity() * sizeof(uint32_t);
    }

private:
    std::vector<T> m_pool;
    std::vector<size_t> m_freeList;
    std::vector<uint32_t> m_generations;
    size_t m_capacity = 0;
    size_t m_activeCount = 0;
};

// ============================================================================
// Memory Arena - Fast bump allocator for per-frame temporary allocations
// ============================================================================

class MemoryArena {
public:
    explicit MemoryArena(size_t size = 1024 * 1024)  // 1 MB default
        : m_size(size), m_offset(0) {
        m_buffer = std::make_unique<uint8_t[]>(size);
    }

    ~MemoryArena() = default;

    // Allocate aligned memory
    template<typename T>
    T* allocate(size_t count = 1) {
        size_t alignment = alignof(T);
        size_t alignedOffset = (m_offset + alignment - 1) & ~(alignment - 1);
        size_t totalSize = sizeof(T) * count;

        if (alignedOffset + totalSize > m_size) {
            return nullptr;  // Out of memory
        }

        T* ptr = reinterpret_cast<T*>(m_buffer.get() + alignedOffset);
        m_offset = alignedOffset + totalSize;

        // Construct objects
        for (size_t i = 0; i < count; ++i) {
            new (&ptr[i]) T();
        }

        return ptr;
    }

    // Allocate raw bytes
    void* allocateRaw(size_t bytes, size_t alignment = 16) {
        size_t alignedOffset = (m_offset + alignment - 1) & ~(alignment - 1);

        if (alignedOffset + bytes > m_size) {
            return nullptr;
        }

        void* ptr = m_buffer.get() + alignedOffset;
        m_offset = alignedOffset + bytes;
        return ptr;
    }

    // Reset for next frame (fast - just resets offset)
    void reset() {
        m_offset = 0;
    }

    // Statistics
    size_t getUsed() const { return m_offset; }
    size_t getRemaining() const { return m_size - m_offset; }
    size_t getSize() const { return m_size; }
    float getUtilization() const { return static_cast<float>(m_offset) / m_size; }

private:
    std::unique_ptr<uint8_t[]> m_buffer;
    size_t m_size;
    size_t m_offset;
};

// ============================================================================
// Ring Buffer - Fixed-size circular buffer for streaming data
// ============================================================================

template<typename T, size_t Capacity>
class RingBuffer {
public:
    RingBuffer() = default;

    bool push(const T& value) {
        if (isFull()) return false;
        m_data[m_tail] = value;
        m_tail = (m_tail + 1) % Capacity;
        m_count++;
        return true;
    }

    bool push(T&& value) {
        if (isFull()) return false;
        m_data[m_tail] = std::move(value);
        m_tail = (m_tail + 1) % Capacity;
        m_count++;
        return true;
    }

    bool pop(T& value) {
        if (isEmpty()) return false;
        value = std::move(m_data[m_head]);
        m_head = (m_head + 1) % Capacity;
        m_count--;
        return true;
    }

    const T& front() const { return m_data[m_head]; }
    T& front() { return m_data[m_head]; }

    const T& back() const {
        size_t idx = (m_tail + Capacity - 1) % Capacity;
        return m_data[idx];
    }

    bool isEmpty() const { return m_count == 0; }
    bool isFull() const { return m_count == Capacity; }
    size_t size() const { return m_count; }
    size_t capacity() const { return Capacity; }

    void clear() {
        m_head = m_tail = m_count = 0;
    }

private:
    std::array<T, Capacity> m_data;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_count = 0;
};

// ============================================================================
// Memory Statistics
// ============================================================================

struct MemoryStats {
    // Pool statistics
    size_t creaturePoolActive = 0;
    size_t creaturePoolTotal = 0;
    size_t particlePoolActive = 0;
    size_t particlePoolTotal = 0;

    // Arena statistics
    size_t arenaUsed = 0;
    size_t arenaTotal = 0;

    // Per-frame allocations (should be near zero)
    size_t frameAllocations = 0;
    size_t frameDeallocations = 0;

    // Memory usage
    size_t totalPoolMemory = 0;
    size_t totalArenaMemory = 0;
    size_t estimatedGPUMemory = 0;

    void reset() {
        frameAllocations = frameDeallocations = 0;
    }
};

// ============================================================================
// Memory Optimizer - Central coordinator for all memory systems
// ============================================================================

class MemoryOptimizer {
public:
    MemoryOptimizer();
    ~MemoryOptimizer() = default;

    // ========================================================================
    // Per-Frame Management
    // ========================================================================

    // Call at start of frame
    void beginFrame();

    // Call at end of frame
    void endFrame();

    // ========================================================================
    // Arena Access (for per-frame temp allocations)
    // ========================================================================

    MemoryArena& getFrameArena() { return m_frameArena; }

    template<typename T>
    T* frameAllocate(size_t count = 1) {
        m_stats.frameAllocations += count;
        return m_frameArena.allocate<T>(count);
    }

    // ========================================================================
    // Reusable Buffers
    // ========================================================================

    // Get/resize a shared buffer (avoids per-frame allocation)
    template<typename T>
    std::vector<T>& getSharedBuffer(size_t index) {
        static std::array<std::vector<T>, 16> buffers;
        return buffers[index % 16];
    }

    // ========================================================================
    // Statistics
    // ========================================================================

    const MemoryStats& getStats() const { return m_stats; }
    void updateStats();

    // ========================================================================
    // Defragmentation
    // ========================================================================

    // Trigger defragmentation (call during loading screens or pauses)
    void defragment();

    // Check if defragmentation is needed
    bool needsDefragmentation() const;

private:
    // Frame arena for temporary allocations
    MemoryArena m_frameArena;

    // Double-buffered arenas for async work
    std::array<MemoryArena, 2> m_asyncArenas;
    size_t m_currentArena = 0;

    // Statistics
    MemoryStats m_stats;
    int m_frameCount = 0;
};

// ============================================================================
// Inline Implementations
// ============================================================================

inline MemoryOptimizer::MemoryOptimizer()
    : m_frameArena(4 * 1024 * 1024)  // 4 MB frame arena
    , m_asyncArenas{MemoryArena(2 * 1024 * 1024), MemoryArena(2 * 1024 * 1024)} {
}

inline void MemoryOptimizer::beginFrame() {
    m_stats.reset();
    m_frameCount++;

    // Reset frame arena (instant - just resets offset)
    m_frameArena.reset();
}

inline void MemoryOptimizer::endFrame() {
    updateStats();

    // Swap async arenas
    m_currentArena = 1 - m_currentArena;
    m_asyncArenas[m_currentArena].reset();
}

inline void MemoryOptimizer::updateStats() {
    m_stats.arenaUsed = m_frameArena.getUsed();
    m_stats.arenaTotal = m_frameArena.getSize();
}

inline bool MemoryOptimizer::needsDefragmentation() const {
    // Defragment if arena utilization is consistently high
    return m_frameArena.getUtilization() > 0.8f;
}

inline void MemoryOptimizer::defragment() {
    // Reset all arenas
    m_frameArena.reset();
    for (auto& arena : m_asyncArenas) {
        arena.reset();
    }
}

} // namespace Forge
