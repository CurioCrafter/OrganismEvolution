# DirectX 12 Memory Patterns: Advanced Implementation Guide

> **Target Project**: ModularGameEngine - High-performance simulation with dynamic resources

This document focuses specifically on memory management patterns, allocation strategies, and optimization techniques for DirectX 12.

---

## Table of Contents

1. [Memory Architecture Overview](#1-memory-architecture-overview)
2. [Ring Buffer Patterns](#2-ring-buffer-patterns)
3. [Pool Allocators](#3-pool-allocators)
4. [Frame-Based Resource Management](#4-frame-based-resource-management)
5. [Memory Defragmentation](#5-memory-defragmentation)
6. [GPU Memory Budgeting](#6-gpu-memory-budgeting)
7. [Streaming Resource Patterns](#7-streaming-resource-patterns)
8. [Debug and Profiling](#8-debug-and-profiling)
9. [Anti-Patterns to Avoid](#9-anti-patterns-to-avoid)
10. [Production-Ready Examples](#10-production-ready-examples)

---

## 1. Memory Architecture Overview

### 1.1 NUMA vs UMA Memory Model

Understanding your target hardware's memory architecture is critical for optimization.

```cpp
// Detect memory architecture
D3D12_FEATURE_DATA_ARCHITECTURE archData = {};
device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &archData, sizeof(archData));

bool isUMA = archData.UMA;           // Unified Memory Architecture (integrated GPU)
bool isCacheCoherent = archData.CacheCoherentUMA;  // CPU cache coherent with GPU

struct MemoryConfig {
    D3D12_HEAP_TYPE uploadHeap;
    D3D12_HEAP_TYPE defaultHeap;
    bool useGPUUpload;
    size_t uploadBufferSize;
    size_t defaultPoolSize;
};

MemoryConfig ConfigureForArchitecture(bool isUMA, bool hasReBAR) {
    MemoryConfig config;

    if (isUMA) {
        // Integrated GPU: All memory is shared
        // Prefer keeping data in DEFAULT with CPU access via custom heaps
        config.uploadHeap = D3D12_HEAP_TYPE_UPLOAD;
        config.defaultHeap = D3D12_HEAP_TYPE_DEFAULT;
        config.useGPUUpload = false;  // No benefit on UMA
        config.uploadBufferSize = 32 * 1024 * 1024;   // Smaller staging
        config.defaultPoolSize = 256 * 1024 * 1024;
    } else {
        // Discrete GPU: VRAM vs System RAM
        config.uploadHeap = D3D12_HEAP_TYPE_UPLOAD;
        config.defaultHeap = D3D12_HEAP_TYPE_DEFAULT;
        config.useGPUUpload = hasReBAR;  // Use ReBAR if available
        config.uploadBufferSize = 64 * 1024 * 1024;
        config.defaultPoolSize = 512 * 1024 * 1024;
    }

    return config;
}
```

### 1.2 Memory Pools Taxonomy

| Pool Type | Location | CPU Access | GPU Access | Use Case |
|-----------|----------|------------|------------|----------|
| L0 (System RAM) | PCIe reachable | Fast R/W | Slow via PCIe | Upload, Readback |
| L1 (VRAM) | GPU local | None (or ReBAR) | Full bandwidth | Static resources |
| GPU_UPLOAD | VRAM via ReBAR | Write-combined | Full bandwidth | Dynamic resources |

### 1.3 Allocation Size Considerations

D3D12 has specific alignment and granularity requirements:

```cpp
struct AllocationRequirements {
    // Minimum heap size alignment
    static constexpr size_t HEAP_ALIGNMENT = 64 * 1024;  // 64 KB

    // Constant buffer view alignment
    static constexpr size_t CBV_ALIGNMENT = 256;  // 256 bytes

    // Texture placement alignment (varies by format)
    static constexpr size_t TEXTURE_ALIGNMENT = 64 * 1024;  // 64 KB for most

    // Small buffer alignment
    static constexpr size_t BUFFER_ALIGNMENT = 64 * 1024;  // 64 KB

    // MSAA alignment
    static constexpr size_t MSAA_ALIGNMENT = 4 * 1024 * 1024;  // 4 MB
};

size_t AlignAllocation(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
```

---

## 2. Ring Buffer Patterns

Ring buffers are the foundation of efficient per-frame resource management in D3D12.

### 2.1 Basic Ring Buffer Implementation

```cpp
template<typename T>
class RingBuffer {
private:
    std::vector<T> m_buffer;
    size_t m_head = 0;      // Write position
    size_t m_tail = 0;      // Read position (oldest valid data)
    size_t m_capacity;

public:
    explicit RingBuffer(size_t capacity) : m_buffer(capacity), m_capacity(capacity) {}

    bool CanAllocate(size_t count) const {
        size_t used = (m_head >= m_tail) ?
            (m_head - m_tail) :
            (m_capacity - m_tail + m_head);
        return (m_capacity - used) >= count;
    }

    T* Allocate(size_t count) {
        if (!CanAllocate(count)) return nullptr;

        // Handle wrap-around
        if (m_head + count > m_capacity) {
            // If we can't fit at end, wrap to start
            m_head = 0;
        }

        T* ptr = &m_buffer[m_head];
        m_head = (m_head + count) % m_capacity;
        return ptr;
    }

    void Free(size_t count) {
        m_tail = (m_tail + count) % m_capacity;
    }

    size_t GetHead() const { return m_head; }
    size_t GetTail() const { return m_tail; }
};
```

### 2.2 GPU Ring Buffer for Constant Data

```cpp
class GPUConstantRingBuffer {
private:
    ComPtr<ID3D12Resource> m_buffer;
    uint8_t* m_cpuPtr = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuBase;

    size_t m_capacity;
    size_t m_head = 0;

    // Track frame boundaries for safe reuse
    struct FrameMarker {
        uint64_t fenceValue;
        size_t offset;
    };
    std::queue<FrameMarker> m_frameMarkers;

    ComPtr<ID3D12Fence> m_fence;

    static constexpr size_t ALIGNMENT = 256;  // D3D12 CBV alignment

public:
    void Initialize(ID3D12Device* device, size_t capacityBytes) {
        m_capacity = AlignUp(capacityBytes, 64 * 1024);  // Heap alignment

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = m_capacity;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_UNKNOWN;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_buffer)
        ));

        // Persistent map
        D3D12_RANGE readRange = { 0, 0 };  // We won't read
        ThrowIfFailed(m_buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cpuPtr)));

        m_gpuBase = m_buffer->GetGPUVirtualAddress();

        // Create fence for tracking
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    }

    struct Allocation {
        void* cpuAddress;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        size_t size;
    };

    Allocation Allocate(size_t size) {
        size_t alignedSize = AlignUp(size, ALIGNMENT);

        // Reclaim completed frames
        ReclaimCompletedFrames();

        // Handle wrap-around
        if (m_head + alignedSize > m_capacity) {
            // Check if we can wrap
            if (alignedSize > GetTailOffset()) {
                // Not enough space even after wrapping
                // This is a serious error - buffer too small
                throw std::runtime_error("Ring buffer exhausted");
            }
            m_head = 0;
        }

        Allocation alloc;
        alloc.cpuAddress = m_cpuPtr + m_head;
        alloc.gpuAddress = m_gpuBase + m_head;
        alloc.size = alignedSize;

        m_head += alignedSize;

        return alloc;
    }

    // Call at end of frame with the fence value that will signal when GPU completes
    void EndFrame(uint64_t completionFenceValue) {
        m_frameMarkers.push({ completionFenceValue, m_head });
    }

private:
    void ReclaimCompletedFrames() {
        uint64_t completedValue = m_fence->GetCompletedValue();

        while (!m_frameMarkers.empty() &&
               m_frameMarkers.front().fenceValue <= completedValue) {
            m_frameMarkers.pop();
        }
    }

    size_t GetTailOffset() const {
        return m_frameMarkers.empty() ? 0 : m_frameMarkers.front().offset;
    }

    static size_t AlignUp(size_t value, size_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
};
```

### 2.3 Thread-Safe Ring Buffer

For multi-threaded command recording:

```cpp
class ThreadSafeRingBuffer {
private:
    ComPtr<ID3D12Resource> m_buffer;
    uint8_t* m_cpuPtr;
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuBase;
    size_t m_capacity;

    std::atomic<size_t> m_head{ 0 };
    std::atomic<size_t> m_tail{ 0 };

    static constexpr size_t ALIGNMENT = 256;

public:
    struct Allocation {
        void* cpu;
        D3D12_GPU_VIRTUAL_ADDRESS gpu;
    };

    Allocation Allocate(size_t size) {
        size_t alignedSize = (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

        size_t currentHead;
        size_t newHead;

        do {
            currentHead = m_head.load(std::memory_order_acquire);
            newHead = currentHead + alignedSize;

            // Handle wrap
            if (newHead > m_capacity) {
                // Try to reset to beginning
                if (m_head.compare_exchange_weak(currentHead, alignedSize,
                    std::memory_order_release, std::memory_order_relaxed)) {
                    currentHead = 0;
                    newHead = alignedSize;
                    break;
                }
                continue;  // Retry
            }
        } while (!m_head.compare_exchange_weak(currentHead, newHead,
            std::memory_order_release, std::memory_order_relaxed));

        return {
            m_cpuPtr + currentHead,
            m_gpuBase + currentHead
        };
    }

    void ResetTail(size_t newTail) {
        m_tail.store(newTail, std::memory_order_release);
    }
};
```

### 2.4 Chunk-Based Ring Allocation

For variable-sized allocations with less fragmentation:

```cpp
class ChunkedRingAllocator {
private:
    static constexpr size_t CHUNK_SIZE = 64 * 1024;  // 64 KB chunks

    struct Chunk {
        ComPtr<ID3D12Resource> resource;
        uint8_t* cpuPtr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuBase;
        size_t used = 0;
    };

    std::vector<Chunk> m_chunks;
    size_t m_currentChunk = 0;
    size_t m_totalChunks;

    struct FrameMarker {
        uint64_t fenceValue;
        size_t chunkIndex;
        size_t offsetInChunk;
    };
    std::queue<FrameMarker> m_frameMarkers;

public:
    void Initialize(ID3D12Device* device, size_t numChunks) {
        m_totalChunks = numChunks;
        m_chunks.resize(numChunks);

        for (auto& chunk : m_chunks) {
            CreateUploadBuffer(device, CHUNK_SIZE, &chunk.resource);
            chunk.resource->Map(0, nullptr, reinterpret_cast<void**>(&chunk.cpuPtr));
            chunk.gpuBase = chunk.resource->GetGPUVirtualAddress();
            chunk.used = 0;
        }
    }

    struct Allocation {
        void* cpu;
        D3D12_GPU_VIRTUAL_ADDRESS gpu;
    };

    Allocation Allocate(size_t size) {
        assert(size <= CHUNK_SIZE);

        Chunk& current = m_chunks[m_currentChunk];

        // Check if current chunk has space
        if (current.used + size > CHUNK_SIZE) {
            // Move to next chunk
            m_currentChunk = (m_currentChunk + 1) % m_totalChunks;
            Chunk& next = m_chunks[m_currentChunk];

            // Wait if chunk is still in use
            WaitForChunkAvailable(m_currentChunk);

            next.used = 0;
            return AllocateFromChunk(next, size);
        }

        return AllocateFromChunk(current, size);
    }

private:
    Allocation AllocateFromChunk(Chunk& chunk, size_t size) {
        size_t offset = chunk.used;
        chunk.used += (size + 255) & ~255;  // Align to 256

        return {
            chunk.cpuPtr + offset,
            chunk.gpuBase + offset
        };
    }
};
```

---

## 3. Pool Allocators

### 3.1 Fixed-Size Block Pool

Optimal for uniform resources (same-sized textures, buffers):

```cpp
template<size_t BlockSize>
class FixedBlockPool {
private:
    ComPtr<ID3D12Heap> m_heap;
    size_t m_heapSize;
    size_t m_blockCount;

    std::vector<bool> m_allocated;
    std::queue<size_t> m_freeList;
    std::mutex m_mutex;

public:
    void Initialize(ID3D12Device* device, size_t blockCount) {
        m_blockCount = blockCount;
        m_heapSize = BlockSize * blockCount;

        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = m_heapSize;
        heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

        ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

        m_allocated.resize(blockCount, false);
        for (size_t i = 0; i < blockCount; ++i) {
            m_freeList.push(i);
        }
    }

    struct BlockAllocation {
        ID3D12Heap* heap;
        size_t offset;
        size_t size;
        size_t blockIndex;
    };

    std::optional<BlockAllocation> Allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_freeList.empty()) {
            return std::nullopt;
        }

        size_t blockIndex = m_freeList.front();
        m_freeList.pop();
        m_allocated[blockIndex] = true;

        return BlockAllocation{
            m_heap.Get(),
            blockIndex * BlockSize,
            BlockSize,
            blockIndex
        };
    }

    void Free(size_t blockIndex) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (blockIndex < m_blockCount && m_allocated[blockIndex]) {
            m_allocated[blockIndex] = false;
            m_freeList.push(blockIndex);
        }
    }

    size_t GetFreeBlockCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_freeList.size();
    }
};

// Usage: Pool for 4KB constant buffers
using ConstantBufferPool = FixedBlockPool<4096>;
```

### 3.2 Buddy Allocator

For power-of-two sized allocations with efficient coalescing:

```cpp
class BuddyAllocator {
private:
    static constexpr size_t MIN_BLOCK_SIZE = 256;     // 256 bytes minimum
    static constexpr size_t MAX_BLOCK_SIZE = 64 * 1024 * 1024;  // 64 MB max

    ComPtr<ID3D12Heap> m_heap;
    size_t m_heapSize;

    // Free lists for each power-of-two size
    // Index 0 = MIN_BLOCK_SIZE, Index N = MIN_BLOCK_SIZE * 2^N
    static constexpr size_t NUM_LEVELS = 18;  // log2(64MB/256B) + 1
    std::vector<size_t> m_freeLists[NUM_LEVELS];

    std::vector<uint8_t> m_blockStates;  // 0=free, 1=allocated, 2=split

    std::mutex m_mutex;

public:
    void Initialize(ID3D12Device* device, size_t heapSize) {
        m_heapSize = RoundUpToPowerOf2(heapSize);

        D3D12_HEAP_DESC heapDesc = {};
        heapDesc.SizeInBytes = m_heapSize;
        heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

        // Initialize with one big free block
        size_t topLevel = GetLevel(m_heapSize);
        m_freeLists[topLevel].push_back(0);

        // Allocate state tracking
        size_t totalBlocks = (m_heapSize / MIN_BLOCK_SIZE) * 2;  // Binary tree nodes
        m_blockStates.resize(totalBlocks, 0);
    }

    struct Allocation {
        ID3D12Heap* heap;
        size_t offset;
        size_t size;
    };

    std::optional<Allocation> Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t requiredSize = std::max(RoundUpToPowerOf2(size), MIN_BLOCK_SIZE);
        size_t level = GetLevel(requiredSize);

        // Find smallest available block >= required size
        size_t foundLevel = level;
        while (foundLevel < NUM_LEVELS && m_freeLists[foundLevel].empty()) {
            foundLevel++;
        }

        if (foundLevel >= NUM_LEVELS) {
            return std::nullopt;  // No space
        }

        // Get block from free list
        size_t offset = m_freeLists[foundLevel].back();
        m_freeLists[foundLevel].pop_back();

        // Split down to required level
        while (foundLevel > level) {
            foundLevel--;
            size_t buddySize = MIN_BLOCK_SIZE << foundLevel;
            size_t buddyOffset = offset + buddySize;

            // Add buddy to free list
            m_freeLists[foundLevel].push_back(buddyOffset);

            // Mark as split
            SetBlockState(offset, foundLevel + 1, 2);  // split
        }

        SetBlockState(offset, level, 1);  // allocated

        return Allocation{
            m_heap.Get(),
            offset,
            requiredSize
        };
    }

    void Free(size_t offset, size_t size) {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t level = GetLevel(size);
        SetBlockState(offset, level, 0);  // free

        // Coalesce with buddy if possible
        while (level < NUM_LEVELS - 1) {
            size_t blockSize = MIN_BLOCK_SIZE << level;
            size_t buddyOffset = (offset ^ blockSize);  // XOR gives buddy address

            // Check if buddy is free at same level
            if (GetBlockState(buddyOffset, level) == 0) {
                // Remove buddy from free list
                auto& freeList = m_freeLists[level];
                freeList.erase(std::remove(freeList.begin(), freeList.end(), buddyOffset),
                              freeList.end());

                // Merge
                offset = std::min(offset, buddyOffset);
                level++;

                SetBlockState(offset, level, 0);
            } else {
                break;
            }
        }

        m_freeLists[level].push_back(offset);
    }

private:
    size_t GetLevel(size_t size) const {
        size_t level = 0;
        size_t blockSize = MIN_BLOCK_SIZE;
        while (blockSize < size && level < NUM_LEVELS - 1) {
            blockSize *= 2;
            level++;
        }
        return level;
    }

    static size_t RoundUpToPowerOf2(size_t v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        v++;
        return v;
    }

    // Block state helpers
    void SetBlockState(size_t offset, size_t level, uint8_t state) {
        size_t index = GetBlockIndex(offset, level);
        if (index < m_blockStates.size()) {
            m_blockStates[index] = state;
        }
    }

    uint8_t GetBlockState(size_t offset, size_t level) const {
        size_t index = GetBlockIndex(offset, level);
        return (index < m_blockStates.size()) ? m_blockStates[index] : 0;
    }

    size_t GetBlockIndex(size_t offset, size_t level) const {
        size_t blockSize = MIN_BLOCK_SIZE << level;
        size_t levelStart = (1 << (NUM_LEVELS - level - 1)) - 1;
        return levelStart + (offset / blockSize);
    }
};
```

### 3.3 TLSF Allocator (Two-Level Segregated Fit)

Industry-standard O(1) allocator used in game engines:

```cpp
// Simplified TLSF concept - use actual TLSF library for production
// See: https://github.com/mattconte/tlsf

class TLSFAllocator {
private:
    // First level: log2 of size
    // Second level: linear subdivision within power-of-2 range
    static constexpr size_t FL_INDEX_COUNT = 32;  // Up to 4GB
    static constexpr size_t SL_INDEX_COUNT = 16;  // 16 sub-divisions per FL
    static constexpr size_t SL_INDEX_LOG2 = 4;

    ComPtr<ID3D12Heap> m_heap;

    struct BlockHeader {
        size_t size;           // Includes header
        bool isFree;
        bool isPrevFree;
        BlockHeader* prevPhysical;
        BlockHeader* nextFree;
        BlockHeader* prevFree;
    };

    // Segregated free lists
    BlockHeader* m_freeBlocks[FL_INDEX_COUNT][SL_INDEX_COUNT] = {};

    // Bitmaps for O(1) lookup
    uint32_t m_flBitmap = 0;
    uint32_t m_slBitmap[FL_INDEX_COUNT] = {};

public:
    // TLSF provides O(1) allocation/deallocation
    // with good fragmentation characteristics

    void* Allocate(size_t size) {
        // Adjust size and find suitable list
        size_t adjustedSize = AdjustRequestSize(size);

        int fl, sl;
        MappingSearch(adjustedSize, &fl, &sl);

        BlockHeader* block = FindSuitableBlock(&fl, &sl);
        if (!block) return nullptr;

        RemoveFreeBlock(block, fl, sl);

        // Split if block is significantly larger
        size_t remaining = block->size - adjustedSize;
        if (remaining >= sizeof(BlockHeader) + 16) {
            BlockHeader* remainingBlock = Split(block, adjustedSize);
            InsertFreeBlock(remainingBlock);
        }

        block->isFree = false;
        return GetPayload(block);
    }

    void Free(void* ptr) {
        if (!ptr) return;

        BlockHeader* block = GetHeader(ptr);
        block->isFree = true;

        // Merge with previous physical block if free
        if (block->isPrevFree) {
            block = Merge(block->prevPhysical, block);
        }

        // Merge with next physical block if free
        BlockHeader* next = GetNextPhysical(block);
        if (next && next->isFree) {
            RemoveFreeBlock(next, /* fl, sl */);
            block = Merge(block, next);
        }

        InsertFreeBlock(block);
    }

private:
    void MappingInsert(size_t size, int* fl, int* sl) {
        if (size < 256) {
            *fl = 0;
            *sl = static_cast<int>(size / 16);
        } else {
            *fl = FloorLog2(size);
            *sl = static_cast<int>((size >> (*fl - SL_INDEX_LOG2)) ^ (1 << SL_INDEX_LOG2));
        }
    }

    void MappingSearch(size_t size, int* fl, int* sl) {
        // Round up to next list
        size_t round = (1 << (FloorLog2(size) - SL_INDEX_LOG2)) - 1;
        size += round;
        MappingInsert(size, fl, sl);
    }

    static int FloorLog2(size_t n) {
        int pos = 0;
        if (n >= 1 << 16) { n >>= 16; pos += 16; }
        if (n >= 1 << 8) { n >>= 8; pos += 8; }
        if (n >= 1 << 4) { n >>= 4; pos += 4; }
        if (n >= 1 << 2) { n >>= 2; pos += 2; }
        if (n >= 1 << 1) { pos += 1; }
        return pos;
    }

    // ... additional helper methods
};
```

---

## 4. Frame-Based Resource Management

### 4.1 Frame Resource System

```cpp
class FrameResources {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

private:
    struct PerFrameData {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        uint64_t fenceValue = 0;

        // Ring buffer state for this frame
        size_t constantBufferOffset = 0;
        size_t uploadBufferOffset = 0;

        // Resources to delete after frame completes
        std::vector<ComPtr<ID3D12Resource>> deferredDeletions;
    };

    std::array<PerFrameData, MAX_FRAMES_IN_FLIGHT> m_frames;
    uint32_t m_currentFrame = 0;

    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_nextFenceValue = 1;
    HANDLE m_fenceEvent;

public:
    void Initialize(ID3D12Device* device) {
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        for (auto& frame : m_frames) {
            ThrowIfFailed(device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&frame.commandAllocator)
            ));
        }
    }

    void BeginFrame() {
        PerFrameData& frame = m_frames[m_currentFrame];

        // Wait for this frame slot to be available
        if (m_fence->GetCompletedValue() < frame.fenceValue) {
            ThrowIfFailed(m_fence->SetEventOnCompletion(frame.fenceValue, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        // Reset command allocator now that GPU is done with it
        ThrowIfFailed(frame.commandAllocator->Reset());

        // Clear deferred deletions - safe now that GPU finished
        frame.deferredDeletions.clear();

        // Reset per-frame allocator offsets
        frame.constantBufferOffset = 0;
        frame.uploadBufferOffset = 0;
    }

    void EndFrame(ID3D12CommandQueue* queue) {
        PerFrameData& frame = m_frames[m_currentFrame];

        // Signal fence with new value
        frame.fenceValue = m_nextFenceValue++;
        ThrowIfFailed(queue->Signal(m_fence.Get(), frame.fenceValue));

        // Move to next frame
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // Defer resource deletion until GPU is done
    void DeferDelete(ComPtr<ID3D12Resource> resource) {
        m_frames[m_currentFrame].deferredDeletions.push_back(std::move(resource));
    }

    ID3D12CommandAllocator* GetCurrentCommandAllocator() {
        return m_frames[m_currentFrame].commandAllocator.Get();
    }

    uint32_t GetCurrentFrameIndex() const { return m_currentFrame; }

    void WaitForAllFrames() {
        uint64_t valueToWait = m_nextFenceValue - 1;
        if (m_fence->GetCompletedValue() < valueToWait) {
            ThrowIfFailed(m_fence->SetEventOnCompletion(valueToWait, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }
};
```

### 4.2 Dynamic Buffer Manager

Manages all per-frame dynamic allocations:

```cpp
class DynamicBufferManager {
private:
    // Separate ring buffers for different purposes
    GPUConstantRingBuffer m_constantBuffer;     // CBV data
    GPUConstantRingBuffer m_vertexBuffer;       // Dynamic vertices
    GPUConstantRingBuffer m_indexBuffer;        // Dynamic indices
    GPUConstantRingBuffer m_uploadStaging;      // Texture upload staging

    uint32_t m_currentFrame = 0;

public:
    void Initialize(ID3D12Device* device) {
        // Size based on expected usage
        m_constantBuffer.Initialize(device, 16 * 1024 * 1024);   // 16 MB constants
        m_vertexBuffer.Initialize(device, 64 * 1024 * 1024);     // 64 MB vertices
        m_indexBuffer.Initialize(device, 16 * 1024 * 1024);      // 16 MB indices
        m_uploadStaging.Initialize(device, 128 * 1024 * 1024);   // 128 MB staging
    }

    struct ConstantAllocation {
        void* cpuPtr;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
    };

    ConstantAllocation AllocateConstants(size_t size) {
        auto alloc = m_constantBuffer.Allocate(size);
        return { alloc.cpuAddress, alloc.gpuAddress };
    }

    D3D12_VERTEX_BUFFER_VIEW AllocateVertices(
        size_t vertexCount,
        size_t stride,
        void** outCpuPtr
    ) {
        size_t size = vertexCount * stride;
        auto alloc = m_vertexBuffer.Allocate(size);
        *outCpuPtr = alloc.cpuAddress;

        D3D12_VERTEX_BUFFER_VIEW view;
        view.BufferLocation = alloc.gpuAddress;
        view.SizeInBytes = static_cast<UINT>(size);
        view.StrideInBytes = static_cast<UINT>(stride);
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW AllocateIndices(
        size_t indexCount,
        DXGI_FORMAT format,
        void** outCpuPtr
    ) {
        size_t indexSize = (format == DXGI_FORMAT_R16_UINT) ? 2 : 4;
        size_t size = indexCount * indexSize;
        auto alloc = m_indexBuffer.Allocate(size);
        *outCpuPtr = alloc.cpuAddress;

        D3D12_INDEX_BUFFER_VIEW view;
        view.BufferLocation = alloc.gpuAddress;
        view.SizeInBytes = static_cast<UINT>(size);
        view.Format = format;
        return view;
    }

    void* AllocateStagingMemory(size_t size, D3D12_GPU_VIRTUAL_ADDRESS* outGpuAddress) {
        auto alloc = m_uploadStaging.Allocate(size);
        *outGpuAddress = alloc.gpuAddress;
        return alloc.cpuAddress;
    }

    void BeginFrame(uint32_t frameIndex) {
        m_currentFrame = frameIndex;
    }

    void EndFrame(uint64_t fenceValue) {
        m_constantBuffer.EndFrame(fenceValue);
        m_vertexBuffer.EndFrame(fenceValue);
        m_indexBuffer.EndFrame(fenceValue);
        m_uploadStaging.EndFrame(fenceValue);
    }
};
```

---

## 5. Memory Defragmentation

### 5.1 Defragmentation Strategy

```cpp
class MemoryDefragmenter {
private:
    struct Allocation {
        ID3D12Resource* resource;
        size_t offset;
        size_t size;
        uint32_t priority;  // Lower = more important to keep in place
        bool canMove;
    };

    std::vector<Allocation> m_allocations;

public:
    struct DefragPlan {
        std::vector<std::pair<size_t, size_t>> moves;  // from, to
        size_t freedSpace;
    };

    DefragPlan ComputeDefragPlan() {
        DefragPlan plan;

        // Sort by offset
        std::vector<size_t> indices(m_allocations.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::sort(indices.begin(), indices.end(), [this](size_t a, size_t b) {
            return m_allocations[a].offset < m_allocations[b].offset;
        });

        // Find gaps and plan moves
        size_t currentOffset = 0;
        for (size_t idx : indices) {
            Allocation& alloc = m_allocations[idx];

            if (alloc.offset > currentOffset && alloc.canMove) {
                // There's a gap - plan move
                plan.moves.push_back({ alloc.offset, currentOffset });
                plan.freedSpace += alloc.offset - currentOffset;
            }

            currentOffset = alloc.offset + alloc.size;
        }

        return plan;
    }

    // Execute defrag over multiple frames to avoid stalls
    void ExecuteDefragStep(
        ID3D12GraphicsCommandList* commandList,
        const DefragPlan& plan,
        size_t stepIndex,
        size_t stepsPerFrame
    ) {
        size_t start = stepIndex * stepsPerFrame;
        size_t end = std::min(start + stepsPerFrame, plan.moves.size());

        for (size_t i = start; i < end; ++i) {
            auto [from, to] = plan.moves[i];
            // Copy resource data from 'from' to 'to'
            // This requires proper barrier handling
        }
    }
};
```

### 5.2 Incremental Defragmentation

```cpp
class IncrementalDefragmenter {
private:
    enum class State {
        Idle,
        Scanning,
        Copying,
        Barrier,
        Updating
    };

    State m_state = State::Idle;
    size_t m_currentMoveIndex = 0;
    std::vector<std::tuple<ID3D12Resource*, size_t, size_t>> m_pendingMoves;

    // Time budget per frame (microseconds)
    static constexpr uint64_t TIME_BUDGET_US = 500;

public:
    void BeginDefrag() {
        m_state = State::Scanning;
        m_currentMoveIndex = 0;
        ScanForFragmentation();
    }

    // Call each frame - does a small amount of work
    bool Update(ID3D12GraphicsCommandList* commandList) {
        auto startTime = std::chrono::high_resolution_clock::now();

        while (m_state != State::Idle) {
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - startTime
            ).count();

            if (elapsed >= TIME_BUDGET_US) {
                return false;  // Not done, continue next frame
            }

            switch (m_state) {
            case State::Scanning:
                if (ScanStep()) {
                    m_state = m_pendingMoves.empty() ? State::Idle : State::Copying;
                }
                break;

            case State::Copying:
                if (CopyStep(commandList)) {
                    m_state = State::Barrier;
                }
                break;

            case State::Barrier:
                InsertBarrier(commandList);
                m_state = State::Updating;
                break;

            case State::Updating:
                if (UpdateReferencesStep()) {
                    m_currentMoveIndex++;
                    if (m_currentMoveIndex >= m_pendingMoves.size()) {
                        m_state = State::Idle;
                    } else {
                        m_state = State::Copying;
                    }
                }
                break;
            }
        }

        return true;  // Defrag complete
    }

private:
    void ScanForFragmentation() { /* ... */ }
    bool ScanStep() { return true; }
    bool CopyStep(ID3D12GraphicsCommandList*) { return true; }
    void InsertBarrier(ID3D12GraphicsCommandList*) { }
    bool UpdateReferencesStep() { return true; }
};
```

---

## 6. GPU Memory Budgeting

### 6.1 Budget Monitor

```cpp
class MemoryBudgetMonitor {
private:
    ComPtr<IDXGIAdapter3> m_adapter;

    struct BudgetInfo {
        uint64_t budget;
        uint64_t currentUsage;
        uint64_t availableForReservation;
        float usageRatio;
    };

    BudgetInfo m_localMemory;   // VRAM
    BudgetInfo m_nonLocalMemory; // System RAM

    // Callbacks for budget changes
    std::function<void(float)> m_onBudgetPressure;

    // Historical tracking
    std::deque<float> m_usageHistory;
    static constexpr size_t HISTORY_SIZE = 60;  // 1 second at 60fps

public:
    void Initialize(IDXGIAdapter* adapter) {
        ThrowIfFailed(adapter->QueryInterface(IID_PPV_ARGS(&m_adapter)));
    }

    void Update() {
        DXGI_QUERY_VIDEO_MEMORY_INFO localInfo, nonLocalInfo;

        m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo);
        m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo);

        m_localMemory = {
            localInfo.Budget,
            localInfo.CurrentUsage,
            localInfo.AvailableForReservation,
            static_cast<float>(localInfo.CurrentUsage) / localInfo.Budget
        };

        m_nonLocalMemory = {
            nonLocalInfo.Budget,
            nonLocalInfo.CurrentUsage,
            nonLocalInfo.AvailableForReservation,
            static_cast<float>(nonLocalInfo.CurrentUsage) / nonLocalInfo.Budget
        };

        // Track history
        m_usageHistory.push_back(m_localMemory.usageRatio);
        if (m_usageHistory.size() > HISTORY_SIZE) {
            m_usageHistory.pop_front();
        }

        // Trigger callback if over budget
        if (m_localMemory.usageRatio > 1.0f && m_onBudgetPressure) {
            m_onBudgetPressure(m_localMemory.usageRatio);
        }
    }

    float GetVRAMUsageRatio() const { return m_localMemory.usageRatio; }
    uint64_t GetVRAMBudget() const { return m_localMemory.budget; }
    uint64_t GetVRAMUsage() const { return m_localMemory.currentUsage; }
    uint64_t GetVRAMAvailable() const { return m_localMemory.budget - m_localMemory.currentUsage; }

    bool IsOverBudget() const { return m_localMemory.usageRatio > 1.0f; }

    // Predict if allocation will exceed budget
    bool WillExceedBudget(uint64_t allocationSize) const {
        return (m_localMemory.currentUsage + allocationSize) > m_localMemory.budget;
    }

    void SetBudgetPressureCallback(std::function<void(float)> callback) {
        m_onBudgetPressure = std::move(callback);
    }

    // Get trend (positive = increasing usage)
    float GetUsageTrend() const {
        if (m_usageHistory.size() < 10) return 0.0f;

        float recent = 0, old = 0;
        for (size_t i = m_usageHistory.size() - 5; i < m_usageHistory.size(); ++i) {
            recent += m_usageHistory[i];
        }
        for (size_t i = 0; i < 5; ++i) {
            old += m_usageHistory[i];
        }
        return (recent - old) / 5.0f;
    }
};
```

### 6.2 Adaptive Quality Manager

Automatically adjusts quality based on memory pressure:

```cpp
class AdaptiveQualityManager {
private:
    MemoryBudgetMonitor* m_budgetMonitor;

    struct QualitySettings {
        int textureQuality;      // 0-4 (0 = highest)
        int shadowMapSize;       // 512, 1024, 2048, 4096
        bool enableVolumetrics;
        int particleCount;
        float lodBias;
    };

    QualitySettings m_current;
    QualitySettings m_target;

    float m_adaptationSpeed = 0.1f;

public:
    void Update() {
        float usage = m_budgetMonitor->GetVRAMUsageRatio();
        float trend = m_budgetMonitor->GetUsageTrend();

        // Determine target quality
        if (usage > 0.95f || trend > 0.02f) {
            // Critical - reduce quality immediately
            ReduceQuality(2);
        } else if (usage > 0.85f) {
            // Warning - start reducing
            ReduceQuality(1);
        } else if (usage < 0.7f && trend < 0) {
            // Headroom available - can increase quality
            IncreaseQuality(1);
        }

        // Smoothly interpolate to target
        InterpolateToTarget();
    }

private:
    void ReduceQuality(int levels) {
        m_target.textureQuality = std::min(4, m_target.textureQuality + levels);
        m_target.shadowMapSize = std::max(512, m_target.shadowMapSize / (1 << levels));
        m_target.lodBias += 0.5f * levels;
        m_target.particleCount = std::max(100, m_target.particleCount - 500 * levels);

        if (levels >= 2) {
            m_target.enableVolumetrics = false;
        }
    }

    void IncreaseQuality(int levels) {
        m_target.textureQuality = std::max(0, m_target.textureQuality - levels);
        m_target.shadowMapSize = std::min(4096, m_target.shadowMapSize * (1 << levels));
        m_target.lodBias = std::max(0.0f, m_target.lodBias - 0.25f * levels);
        m_target.particleCount += 250 * levels;
        m_target.enableVolumetrics = true;
    }

    void InterpolateToTarget() {
        // Instant changes for discrete settings
        m_current.textureQuality = m_target.textureQuality;
        m_current.shadowMapSize = m_target.shadowMapSize;
        m_current.enableVolumetrics = m_target.enableVolumetrics;

        // Smooth interpolation for continuous settings
        m_current.lodBias += (m_target.lodBias - m_current.lodBias) * m_adaptationSpeed;
        m_current.particleCount = static_cast<int>(
            m_current.particleCount +
            (m_target.particleCount - m_current.particleCount) * m_adaptationSpeed
        );
    }
};
```

---

## 7. Streaming Resource Patterns

### 7.1 Streaming Priority System

```cpp
class StreamingPriorityManager {
public:
    enum class Priority : uint8_t {
        Critical = 0,   // Must be resident (player, UI)
        High = 1,       // Nearby/visible objects
        Medium = 2,     // Distant visible objects
        Low = 3,        // Off-screen but may become visible
        Background = 4  // Prefetch
    };

private:
    struct StreamingResource {
        ID3D12Resource* resource;
        uint64_t lastUsedFrame;
        Priority priority;
        float screenCoverage;
        float distance;
        size_t memorySize;
        bool isResident;
    };

    std::vector<StreamingResource> m_resources;
    uint64_t m_currentFrame = 0;

public:
    void UpdatePriorities(const Camera& camera) {
        for (auto& res : m_resources) {
            // Calculate priority based on visibility and distance
            if (IsVisible(res, camera)) {
                res.screenCoverage = CalculateScreenCoverage(res, camera);
                res.distance = CalculateDistance(res, camera);

                if (res.screenCoverage > 0.1f) {
                    res.priority = Priority::Critical;
                } else if (res.distance < 50.0f) {
                    res.priority = Priority::High;
                } else if (res.distance < 200.0f) {
                    res.priority = Priority::Medium;
                } else {
                    res.priority = Priority::Low;
                }
            } else {
                // Check if it might become visible soon
                if (MightBecomeVisible(res, camera)) {
                    res.priority = Priority::Background;
                } else {
                    res.priority = Priority::Low;
                    // Mark for potential eviction
                }
            }
        }
    }

    std::vector<StreamingResource*> GetEvictionCandidates(size_t targetBytes) {
        std::vector<StreamingResource*> candidates;
        size_t totalBytes = 0;

        // Sort by eviction priority (higher priority number = more evictable)
        std::vector<StreamingResource*> sorted;
        for (auto& res : m_resources) {
            if (res.isResident) {
                sorted.push_back(&res);
            }
        }

        std::sort(sorted.begin(), sorted.end(),
            [this](StreamingResource* a, StreamingResource* b) {
                // Evict lowest priority first
                if (a->priority != b->priority) {
                    return static_cast<uint8_t>(a->priority) > static_cast<uint8_t>(b->priority);
                }
                // Then oldest usage
                return a->lastUsedFrame < b->lastUsedFrame;
            });

        for (auto* res : sorted) {
            if (totalBytes >= targetBytes) break;
            if (res->priority >= Priority::Low) {
                candidates.push_back(res);
                totalBytes += res->memorySize;
            }
        }

        return candidates;
    }

    std::vector<StreamingResource*> GetLoadCandidates(size_t budgetBytes) {
        std::vector<StreamingResource*> candidates;
        size_t totalBytes = 0;

        // Get non-resident resources sorted by priority
        std::vector<StreamingResource*> sorted;
        for (auto& res : m_resources) {
            if (!res.isResident) {
                sorted.push_back(&res);
            }
        }

        std::sort(sorted.begin(), sorted.end(),
            [](StreamingResource* a, StreamingResource* b) {
                return static_cast<uint8_t>(a->priority) < static_cast<uint8_t>(b->priority);
            });

        for (auto* res : sorted) {
            if (totalBytes + res->memorySize > budgetBytes) continue;
            candidates.push_back(res);
            totalBytes += res->memorySize;
        }

        return candidates;
    }

private:
    bool IsVisible(const StreamingResource& res, const Camera& cam) { return true; }
    float CalculateScreenCoverage(const StreamingResource& res, const Camera& cam) { return 0.1f; }
    float CalculateDistance(const StreamingResource& res, const Camera& cam) { return 100.0f; }
    bool MightBecomeVisible(const StreamingResource& res, const Camera& cam) { return false; }
};
```

### 7.2 Async Streaming Pipeline

```cpp
class AsyncStreamingPipeline {
private:
    struct StreamingRequest {
        ID3D12Resource* destination;
        std::wstring sourcePath;
        size_t offset;
        size_t size;
        uint8_t priority;
        std::function<void(bool)> callback;
    };

    // Request queues by priority
    std::array<std::queue<StreamingRequest>, 5> m_requestQueues;
    std::mutex m_queueMutex;

    // Dedicated copy queue and command list
    ComPtr<ID3D12CommandQueue> m_copyQueue;
    ComPtr<ID3D12CommandAllocator> m_copyAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_copyList;

    // Staging buffer pool
    std::vector<ComPtr<ID3D12Resource>> m_stagingBuffers;
    std::queue<ID3D12Resource*> m_availableStaging;

    // Thread for async loading
    std::thread m_loadThread;
    std::atomic<bool> m_running{ true };
    std::condition_variable m_workAvailable;

public:
    void Initialize(ID3D12Device* device) {
        // Create copy queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
        queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_copyQueue));

        device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_COPY,
            IID_PPV_ARGS(&m_copyAllocator)
        );

        device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_COPY,
            m_copyAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&m_copyList)
        );
        m_copyList->Close();

        // Create staging buffer pool
        CreateStagingBuffers(device, 16, 64 * 1024 * 1024);  // 16 x 64MB

        // Start worker thread
        m_loadThread = std::thread(&AsyncStreamingPipeline::WorkerThread, this);
    }

    void QueueRequest(
        ID3D12Resource* destination,
        const std::wstring& path,
        size_t offset,
        size_t size,
        uint8_t priority,
        std::function<void(bool)> callback
    ) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_requestQueues[priority].push({
            destination, path, offset, size, priority, std::move(callback)
        });
        m_workAvailable.notify_one();
    }

    void Shutdown() {
        m_running = false;
        m_workAvailable.notify_all();
        if (m_loadThread.joinable()) {
            m_loadThread.join();
        }
    }

private:
    void WorkerThread() {
        while (m_running) {
            StreamingRequest request;
            bool hasWork = false;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_workAvailable.wait(lock, [this] {
                    if (!m_running) return true;
                    for (auto& q : m_requestQueues) {
                        if (!q.empty()) return true;
                    }
                    return false;
                });

                if (!m_running) break;

                // Get highest priority request
                for (auto& q : m_requestQueues) {
                    if (!q.empty()) {
                        request = std::move(q.front());
                        q.pop();
                        hasWork = true;
                        break;
                    }
                }
            }

            if (hasWork) {
                ProcessRequest(request);
            }
        }
    }

    void ProcessRequest(StreamingRequest& request) {
        // 1. Get staging buffer
        ID3D12Resource* staging = AcquireStagingBuffer(request.size);
        if (!staging) {
            request.callback(false);
            return;
        }

        // 2. Load data from disk to staging
        bool loadSuccess = LoadFromDisk(request.sourcePath, request.offset, request.size, staging);
        if (!loadSuccess) {
            ReleaseStagingBuffer(staging);
            request.callback(false);
            return;
        }

        // 3. Copy from staging to destination
        m_copyAllocator->Reset();
        m_copyList->Reset(m_copyAllocator.Get(), nullptr);
        m_copyList->CopyResource(request.destination, staging);
        m_copyList->Close();

        ID3D12CommandList* lists[] = { m_copyList.Get() };
        m_copyQueue->ExecuteCommandLists(1, lists);

        // 4. Signal fence and wait
        // (In production, use async fence waiting)

        ReleaseStagingBuffer(staging);
        request.callback(true);
    }

    bool LoadFromDisk(const std::wstring& path, size_t offset, size_t size, ID3D12Resource* staging) {
        // Memory-map file and copy to staging
        void* mappedStaging;
        staging->Map(0, nullptr, &mappedStaging);

        // Use platform-specific file mapping...
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

        if (file == INVALID_HANDLE_VALUE) {
            staging->Unmap(0, nullptr);
            return false;
        }

        LARGE_INTEGER li;
        li.QuadPart = offset;
        SetFilePointerEx(file, li, nullptr, FILE_BEGIN);

        DWORD bytesRead;
        ReadFile(file, mappedStaging, static_cast<DWORD>(size), &bytesRead, nullptr);
        CloseHandle(file);

        staging->Unmap(0, nullptr);
        return bytesRead == size;
    }

    void CreateStagingBuffers(ID3D12Device* device, size_t count, size_t size) {
        for (size_t i = 0; i < count; ++i) {
            ComPtr<ID3D12Resource> buffer;
            D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };
            D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&buffer)
            );

            m_availableStaging.push(buffer.Get());
            m_stagingBuffers.push_back(std::move(buffer));
        }
    }

    ID3D12Resource* AcquireStagingBuffer(size_t size) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_availableStaging.empty()) return nullptr;
        auto* buffer = m_availableStaging.front();
        m_availableStaging.pop();
        return buffer;
    }

    void ReleaseStagingBuffer(ID3D12Resource* buffer) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_availableStaging.push(buffer);
    }
};
```

---

## 8. Debug and Profiling

### 8.1 Memory Usage Tracker

```cpp
class MemoryUsageTracker {
public:
    struct AllocationInfo {
        std::string name;
        std::string category;
        size_t size;
        D3D12_HEAP_TYPE heapType;
        uint64_t frameAllocated;
        std::string stackTrace;  // Debug only
    };

private:
    std::unordered_map<ID3D12Resource*, AllocationInfo> m_allocations;
    std::mutex m_mutex;

    // Category summaries
    std::unordered_map<std::string, size_t> m_categoryTotals;

    // Per-frame tracking
    size_t m_frameAllocations = 0;
    size_t m_frameDeallocations = 0;
    uint64_t m_currentFrame = 0;

public:
    void TrackAllocation(
        ID3D12Resource* resource,
        const std::string& name,
        const std::string& category,
        size_t size,
        D3D12_HEAP_TYPE heapType
    ) {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_allocations[resource] = {
            name,
            category,
            size,
            heapType,
            m_currentFrame,
#ifdef _DEBUG
            CaptureStackTrace()
#else
            ""
#endif
        };

        m_categoryTotals[category] += size;
        m_frameAllocations += size;
    }

    void TrackDeallocation(ID3D12Resource* resource) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_allocations.find(resource);
        if (it != m_allocations.end()) {
            m_categoryTotals[it->second.category] -= it->second.size;
            m_frameDeallocations += it->second.size;
            m_allocations.erase(it);
        }
    }

    void NewFrame() {
        m_currentFrame++;
        m_frameAllocations = 0;
        m_frameDeallocations = 0;
    }

    void PrintReport() const {
        std::lock_guard<std::mutex> lock(m_mutex);

        printf("=== Memory Usage Report ===\n");
        printf("Total allocations: %zu\n", m_allocations.size());

        printf("\nBy Category:\n");
        for (const auto& [category, total] : m_categoryTotals) {
            printf("  %s: %.2f MB\n", category.c_str(), total / (1024.0 * 1024.0));
        }

        printf("\nBy Heap Type:\n");
        size_t defaultTotal = 0, uploadTotal = 0, readbackTotal = 0;
        for (const auto& [ptr, info] : m_allocations) {
            switch (info.heapType) {
            case D3D12_HEAP_TYPE_DEFAULT: defaultTotal += info.size; break;
            case D3D12_HEAP_TYPE_UPLOAD: uploadTotal += info.size; break;
            case D3D12_HEAP_TYPE_READBACK: readbackTotal += info.size; break;
            }
        }
        printf("  DEFAULT: %.2f MB\n", defaultTotal / (1024.0 * 1024.0));
        printf("  UPLOAD: %.2f MB\n", uploadTotal / (1024.0 * 1024.0));
        printf("  READBACK: %.2f MB\n", readbackTotal / (1024.0 * 1024.0));

        printf("\nFrame Stats:\n");
        printf("  Allocated this frame: %.2f MB\n", m_frameAllocations / (1024.0 * 1024.0));
        printf("  Freed this frame: %.2f MB\n", m_frameDeallocations / (1024.0 * 1024.0));
    }

    std::vector<AllocationInfo> GetLargestAllocations(size_t count) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<AllocationInfo> sorted;
        for (const auto& [ptr, info] : m_allocations) {
            sorted.push_back(info);
        }

        std::sort(sorted.begin(), sorted.end(),
            [](const AllocationInfo& a, const AllocationInfo& b) {
                return a.size > b.size;
            });

        if (sorted.size() > count) {
            sorted.resize(count);
        }

        return sorted;
    }

private:
    std::string CaptureStackTrace() {
        // Platform-specific stack capture
        return "";
    }
};
```

### 8.2 PIX Integration

```cpp
class PixIntegration {
public:
    static void BeginEvent(ID3D12GraphicsCommandList* cmdList, const wchar_t* name, UINT64 color = 0) {
#ifdef USE_PIX
        PIXBeginEvent(cmdList, color, name);
#endif
    }

    static void EndEvent(ID3D12GraphicsCommandList* cmdList) {
#ifdef USE_PIX
        PIXEndEvent(cmdList);
#endif
    }

    static void SetMarker(ID3D12GraphicsCommandList* cmdList, const wchar_t* name) {
#ifdef USE_PIX
        PIXSetMarker(cmdList, 0, name);
#endif
    }

    // Memory-specific markers
    static void MarkAllocation(const wchar_t* name, size_t size) {
#ifdef USE_PIX
        wchar_t buffer[256];
        swprintf(buffer, 256, L"Alloc: %s (%.2f MB)", name, size / (1024.0 * 1024.0));
        PIXSetMarker(0, buffer);
#endif
    }

    // Scoped event helper
    class ScopedEvent {
        ID3D12GraphicsCommandList* m_cmdList;
    public:
        ScopedEvent(ID3D12GraphicsCommandList* cmdList, const wchar_t* name)
            : m_cmdList(cmdList) {
            BeginEvent(cmdList, name);
        }
        ~ScopedEvent() { EndEvent(m_cmdList); }
    };
};

#define PIX_SCOPED_EVENT(cmdList, name) \
    PixIntegration::ScopedEvent _pixEvent##__LINE__(cmdList, name)
```

---

## 9. Anti-Patterns to Avoid

### 9.1 Memory Leaks

```cpp
// ANTI-PATTERN: Forgetting to release resources
class BadResourceManager {
    std::vector<ID3D12Resource*> resources;  // Raw pointers!

    void CreateResource() {
        ID3D12Resource* res;
        device->CreateCommittedResource(..., &res);
        resources.push_back(res);  // Leak if not manually released
    }
};

// CORRECT: Use ComPtr or explicit tracking
class GoodResourceManager {
    std::vector<ComPtr<ID3D12Resource>> resources;  // Auto-release

    void CreateResource() {
        ComPtr<ID3D12Resource> res;
        device->CreateCommittedResource(..., &res);
        resources.push_back(std::move(res));  // Safe
    }
};
```

### 9.2 Use-After-Free

```cpp
// ANTI-PATTERN: Reusing memory too soon
void BadFrame() {
    auto alloc = ringBuffer.Allocate(1024);
    memcpy(alloc.cpu, data, 1024);
    commandList->DrawInstanced(...);

    // BUG: Resetting without waiting for GPU!
    ringBuffer.Reset();
}

// CORRECT: Fence-based synchronization
void GoodFrame() {
    auto alloc = ringBuffer.Allocate(1024);
    memcpy(alloc.cpu, data, 1024);
    commandList->DrawInstanced(...);

    // Record fence value for this frame's allocations
    ringBuffer.EndFrame(currentFenceValue);

    // Reset only reclaims memory from completed frames
    ringBuffer.BeginFrame();  // Checks fence before reclaiming
}
```

### 9.3 Fragmentation

```cpp
// ANTI-PATTERN: Random-sized allocations without pooling
void BadAllocations() {
    for (int i = 0; i < 1000; i++) {
        size_t size = rand() % 10000;  // Random sizes
        auto* res = AllocateBuffer(size);  // Creates fragmentation
    }
}

// CORRECT: Use size classes or pools
void GoodAllocations() {
    // Pre-defined size classes
    FixedBlockPool<256> smallPool;
    FixedBlockPool<4096> mediumPool;
    FixedBlockPool<65536> largePool;

    for (int i = 0; i < 1000; i++) {
        size_t size = rand() % 10000;
        if (size <= 256) smallPool.Allocate();
        else if (size <= 4096) mediumPool.Allocate();
        else largePool.Allocate();
    }
}
```

### 9.4 Excessive Barriers

```cpp
// ANTI-PATTERN: Barrier per resource in loop
void BadBarriers() {
    for (auto& texture : textures) {
        D3D12_RESOURCE_BARRIER barrier = ...;
        commandList->ResourceBarrier(1, &barrier);  // Expensive!
        RenderWithTexture(texture);
    }
}

// CORRECT: Batch all barriers
void GoodBarriers() {
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (auto& texture : textures) {
        barriers.push_back(CreateBarrier(texture));
    }
    commandList->ResourceBarrier(barriers.size(), barriers.data());  // One call

    for (auto& texture : textures) {
        RenderWithTexture(texture);
    }
}
```

---

## 10. Production-Ready Examples

### 10.1 Complete Resource Manager

```cpp
class ProductionResourceManager {
private:
    ComPtr<ID3D12Device> m_device;

    // Memory allocator (D3D12MA)
    D3D12MA::Allocator* m_allocator = nullptr;

    // Specialized pools
    GPUConstantRingBuffer m_constantRing;
    FixedBlockPool<64 * 1024> m_smallBufferPool;
    BuddyAllocator m_generalAllocator;

    // Tracking
    MemoryUsageTracker m_tracker;
    MemoryBudgetMonitor m_budgetMonitor;

    // Frame management
    FrameResources m_frameResources;

    // Streaming
    AsyncStreamingPipeline m_streamingPipeline;
    StreamingPriorityManager m_priorityManager;

public:
    void Initialize(ID3D12Device* device, IDXGIAdapter* adapter) {
        m_device = device;

        // Initialize D3D12MA
        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.pDevice = device;
        allocatorDesc.pAdapter = adapter;
        allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
        D3D12MA::CreateAllocator(&allocatorDesc, &m_allocator);

        // Initialize subsystems
        m_constantRing.Initialize(device, 32 * 1024 * 1024);
        m_smallBufferPool.Initialize(device, 1024);
        m_generalAllocator.Initialize(device, 256 * 1024 * 1024);

        m_budgetMonitor.Initialize(adapter);
        m_frameResources.Initialize(device);
        m_streamingPipeline.Initialize(device);

        // Set up budget pressure response
        m_budgetMonitor.SetBudgetPressureCallback([this](float ratio) {
            HandleBudgetPressure(ratio);
        });
    }

    void BeginFrame() {
        m_frameResources.BeginFrame();
        m_constantRing.BeginFrame(m_frameResources.GetCurrentFrameIndex());
        m_budgetMonitor.Update();
        m_tracker.NewFrame();
    }

    void EndFrame(ID3D12CommandQueue* queue) {
        m_frameResources.EndFrame(queue);
    }

    // Allocate constant buffer data
    D3D12_GPU_VIRTUAL_ADDRESS AllocateConstants(const void* data, size_t size) {
        auto alloc = m_constantRing.Allocate(size);
        memcpy(alloc.cpuAddress, data, size);
        return alloc.gpuAddress;
    }

    // Create static buffer
    ComPtr<ID3D12Resource> CreateStaticBuffer(
        const void* data,
        size_t size,
        const std::string& name
    ) {
        D3D12MA::ALLOCATION_DESC allocDesc = {};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        if (m_budgetMonitor.WillExceedBudget(size)) {
            allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
        }

        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);

        D3D12MA::Allocation* allocation;
        ComPtr<ID3D12Resource> resource;

        HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            &allocation,
            IID_PPV_ARGS(&resource)
        );

        if (FAILED(hr)) {
            // Try to free some memory
            HandleBudgetPressure(1.0f);

            // Retry
            hr = m_allocator->CreateResource(
                &allocDesc,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                &allocation,
                IID_PPV_ARGS(&resource)
            );

            if (FAILED(hr)) {
                return nullptr;
            }
        }

        m_tracker.TrackAllocation(resource.Get(), name, "StaticBuffer", size, D3D12_HEAP_TYPE_DEFAULT);

        // Upload initial data...

        return resource;
    }

    // Deferred deletion for safe GPU lifetime
    void DeferredDelete(ComPtr<ID3D12Resource> resource) {
        m_tracker.TrackDeallocation(resource.Get());
        m_frameResources.DeferDelete(std::move(resource));
    }

    void Shutdown() {
        m_frameResources.WaitForAllFrames();
        m_streamingPipeline.Shutdown();

        if (m_allocator) {
            m_allocator->Release();
            m_allocator = nullptr;
        }
    }

private:
    void HandleBudgetPressure(float ratio) {
        // 1. Evict streaming resources
        size_t targetFree = static_cast<size_t>(
            (ratio - 0.9f) * m_budgetMonitor.GetVRAMBudget()
        );

        auto candidates = m_priorityManager.GetEvictionCandidates(targetFree);
        for (auto* res : candidates) {
            // Evict resource...
        }

        // 2. If still over budget, reduce quality
        if (m_budgetMonitor.IsOverBudget()) {
            // Signal quality manager...
        }
    }
};
```

---

## References

- [D3D12 Memory Allocator (D3D12MA)](https://gpuopen.com/d3d12-memory-allocator/)
- [Microsoft D3D12 Memory Management](https://learn.microsoft.com/en-us/windows/win32/direct3d12/memory-management)
- [Diligent Engine Dynamic Resources](https://diligentgraphics.com/2016/04/20/implementing-dynamic-resources-with-direct3d12/)
- [TLSF Allocator](https://github.com/mattconte/tlsf)
- [PIX for Windows](https://devblogs.microsoft.com/pix/)

---

*Document Version: 1.0*
*Last Updated: January 2026*
*Target: DirectX 12 with Agility SDK 1.613.0+*
