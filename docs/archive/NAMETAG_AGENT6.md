# Nametag System Fix - AGENT6

## Problem

Only 1 creature was showing a nametag, even though the rendering loop appeared to iterate through all creatures.

## Root Cause Analysis

The issue was a classic DirectX 12 constant buffer race condition:

### Original Code Flow
```cpp
for (const auto& creature : world.creatures)
{
    // Update constant buffer with this creature's data
    frameConstants.model = ShaderMat4(billboardModel);
    frameConstants.creatureID = (i32)creature->id;
    frameConstants.creatureType = (i32)creature->type;
    memcpy(cbData, &frameConstants, sizeof(frameConstants));  // OVERWRITES same memory!
    cmdList->BindConstantBuffer(0, constantsCB);
    cmdList->DrawIndexed(6, 0, 0);  // GPU doesn't execute immediately!
}
```

### Why Only 1 Nametag Showed

1. **Command List Batching**: In DirectX 12, draw calls are recorded into a command list but don't execute immediately. The GPU executes them asynchronously after `Submit()`.

2. **Single Constant Buffer**: All nametags shared the same constant buffer memory location. Each iteration overwrote the previous creature's data.

3. **Race Condition**: By the time the GPU executed ANY of the draw calls, the constant buffer contained ONLY the LAST creature's data (or sometimes intermediate values during the CPU's update loop).

4. **Result**: All nametag draw calls read the same data, so all nametags (if rendered) showed the same ID - appearing as "only 1 nametag" since they all overlapped at the last creature's position.

## The Fix

### Strategy: Offset-Based Constant Buffer Binding

DirectX 12's `SetGraphicsRootConstantBufferView` accepts a GPU virtual address. By creating a larger constant buffer with 256-byte aligned entries (DX12 CBV alignment requirement), each nametag can have its own constant data.

### Changes Made

#### 1. New NametagConstants Struct (src/main.cpp)

```cpp
// 512 bytes (256-byte aligned multiple for DX12 CBV)
struct NametagConstants
{
    Constants constants;  // 400 bytes
    u8 padding[112];      // 112 bytes padding
};
```

#### 2. Per-Frame Nametag Constant Buffer

Created dedicated constant buffers for nametags:
```cpp
UniquePtr<IBuffer> m_nametagCB[NUM_FRAMES_IN_FLIGHT];
std::vector<NametagConstants> m_nametagConstantsStaging;
```

Each buffer is sized for `MAX_NAMETAGS * 512 bytes = 2000 * 512 = 1MB`.

#### 3. Offset-Based Binding in RHI

Added offset parameter to `BindConstantBuffer`:

**ForgeEngine/RHI/Public/RHI/RHI.h**:
```cpp
virtual void BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset = 0) = 0;
```

**ForgeEngine/RHI/DX12/DX12Device.cpp**:
```cpp
void DX12CommandList::BindConstantBuffer(u32 slot, IBuffer* buffer, u32 offset)
{
    auto* dx12Buffer = static_cast<DX12Buffer*>(buffer);
    cmdList->SetGraphicsRootConstantBufferView(
        slot,
        dx12Buffer->resource->GetGPUVirtualAddress() + offset
    );
}
```

#### 4. New Rendering Flow

```cpp
// Phase 1: Build all nametag constants (CPU side)
for (creature : world.creatures)
{
    NametagConstants nc = {};
    nc.constants = frameConstants;  // Copy per-frame data
    nc.constants.model = billboardModel;
    nc.constants.creatureID = creature->id;
    nc.constants.creatureType = creature->type;
    m_nametagConstantsStaging.push_back(nc);
}

// Phase 2: Upload ALL constants in one memcpy
nametagCB->Map();
memcpy(data, staging.data(), staging.size() * 512);
nametagCB->Unmap();

// Phase 3: Draw each nametag with offset-based binding
for (u32 i = 0; i < count; ++i)
{
    u32 offset = i * 512;  // 256-byte aligned
    cmdList->BindConstantBuffer(0, nametagCB, offset);
    cmdList->DrawIndexed(6, 0, 0);
}
```

## Files Changed

1. **src/main.cpp**
   - Added `NametagConstants` struct (512 bytes, 256-byte aligned)
   - Added `MAX_NAMETAGS` constant
   - Added `m_nametagCB[]` per-frame buffers
   - Added `m_nametagConstantsStaging` vector
   - Rewrote nametag rendering loop with offset-based binding

2. **ForgeEngine/RHI/Public/RHI/RHI.h**
   - Added `offset` parameter to `BindConstantBuffer()`

3. **ForgeEngine/RHI/DX12/DX12Device.cpp**
   - Updated `DX12CommandList::BindConstantBuffer()` to use offset

## How Nametag System Now Works

```
Frame N:
  1. Build nametag data for all creatures into staging buffer
  2. Upload staging buffer to GPU constant buffer (single memcpy)
  3. For each nametag:
     - Bind constant buffer at offset i * 512
     - Draw 6 indices (quad)
  4. GPU executes with each draw having its own constant data

Memory Layout (per creature):
  Offset 0:    Creature 0's constants (512 bytes)
  Offset 512:  Creature 1's constants (512 bytes)
  Offset 1024: Creature 2's constants (512 bytes)
  ...
```

## Performance Notes

- **Memory**: ~1MB per frame for 2000 nametags (2 frames = 2MB)
- **Draw Calls**: Still 1 draw call per nametag (not instanced)
- **CPU Overhead**: Single memcpy per frame vs. N map/write/unmap cycles
- **GPU Efficiency**: Each draw has unique data, no race conditions

## Future Optimization

The nametag system could be further optimized with GPU instancing:
1. Pack nametag data into a structured buffer
2. Use `SV_InstanceID` in shader to index into the buffer
3. Single `DrawIndexedInstanced(6, nametagCount, ...)` call

This would reduce draw calls from N to 1, but requires shader modifications.

## Verification

Build output shows successful compilation with the fix:
```
OrganismEvolution.vcxproj -> Debug\OrganismEvolution.exe
```

Debug output when running will show:
```
=== NAMETAG RENDERING FIXED ===
Rendering N nametags with offset-based CBV binding
Each nametag uses 512 bytes (512-byte aligned for DX12)
Total nametag CB size: N*512 bytes
================================
```

## Additional Fix

During investigation, also fixed pre-existing `CreatureVertex` member name errors:
- Changed `v.tx` to `v.u`
- Changed `v.ty` to `v.v`
- Changed `tip.tx/ty` to `tip.u/v`
- Changed `baseCenter.tx/ty` to `baseCenter.u/v`

These were unrelated to the nametag issue but prevented compilation.
