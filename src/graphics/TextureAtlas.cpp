#include "TextureAtlas.h"
#include <algorithm>
#include <cstring>

namespace graphics {

// TextureAtlasPage implementation
TextureAtlasPage::TextureAtlasPage(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    m_root = std::make_unique<AtlasNode>(0, 0, width, height);
}

void TextureAtlasPage::clear() {
    m_root = std::make_unique<AtlasNode>(0, 0, m_width, m_height);
    m_allocatedCount = 0;
    m_allocatedPixels = 0;
}

bool TextureAtlasPage::canFit(uint32_t width, uint32_t height) const {
    // Quick check if there's any possibility of fitting
    uint32_t freePixels = m_width * m_height - m_allocatedPixels;
    return freePixels >= width * height;
}

float TextureAtlasPage::getUsagePercent() const {
    return static_cast<float>(m_allocatedPixels) / static_cast<float>(m_width * m_height) * 100.0f;
}

AtlasNode* TextureAtlasPage::findNode(AtlasNode* node, uint32_t width, uint32_t height) {
    if (!node) return nullptr;

    if (node->used) {
        // Try right child first, then down
        AtlasNode* found = findNode(node->right.get(), width, height);
        if (found) return found;
        return findNode(node->left.get(), width, height);
    }

    // Check if this node can fit the texture
    if (width <= node->width && height <= node->height) {
        return node;
    }

    return nullptr;
}

AtlasNode* TextureAtlasPage::splitNode(AtlasNode* node, uint32_t width, uint32_t height) {
    node->used = true;

    // Split remaining space
    uint32_t remainingWidth = node->width - width;
    uint32_t remainingHeight = node->height - height;

    if (remainingWidth > remainingHeight) {
        // Split vertically
        node->left = std::make_unique<AtlasNode>(node->x, node->y + height, width, remainingHeight);
        node->right = std::make_unique<AtlasNode>(node->x + width, node->y, remainingWidth, node->height);
    } else {
        // Split horizontally
        node->left = std::make_unique<AtlasNode>(node->x + width, node->y, remainingWidth, height);
        node->right = std::make_unique<AtlasNode>(node->x, node->y + height, node->width, remainingHeight);
    }

    // Resize this node to the allocated size
    node->width = width;
    node->height = height;

    return node;
}

bool TextureAtlasPage::allocate(uint32_t width, uint32_t height, uint32_t& outX, uint32_t& outY) {
    AtlasNode* node = findNode(m_root.get(), width, height);
    if (!node) return false;

    AtlasNode* allocated = splitNode(node, width, height);
    outX = allocated->x;
    outY = allocated->y;

    m_allocatedCount++;
    m_allocatedPixels += width * height;

    return true;
}

// TextureAtlasManager implementation
TextureAtlasManager::TextureAtlasManager() {
    m_generator = std::make_unique<CreatureTextureGenerator>();
}

bool TextureAtlasManager::initialize(ID3D12Device* device, const TextureAtlasConfig& config) {
    m_device = device;
    m_config = config;

    // Create initial atlas page
    m_atlasPages.push_back(std::make_unique<TextureAtlasPage>(config.atlasWidth, config.atlasHeight));

    // Create texture array
    if (!createTextureArray()) {
        return false;
    }

    // Create SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap));
    if (FAILED(hr)) return false;

    m_srvCpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
    m_srvGpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

    // Create SRV for texture array
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MipLevels = m_config.generateMipmaps ? m_config.mipLevels : 1;
    srvDesc.Texture2DArray.ArraySize = m_config.maxAtlases;

    m_device->CreateShaderResourceView(m_textureArray.Get(), &srvDesc, m_srvCpuHandle);

    // Allocate staging buffer
    size_t maxTextureSize = m_config.textureWidth * m_config.textureHeight * 4;
    m_stagingBuffer.resize(maxTextureSize);

    m_initialized = true;
    return true;
}

void TextureAtlasManager::shutdown() {
    m_textureEntries.clear();
    m_atlasPages.clear();
    m_textureArray.Reset();
    m_uploadBuffer.Reset();
    m_srvHeap.Reset();
    m_device = nullptr;
    m_initialized = false;
}

bool TextureAtlasManager::createTextureArray() {
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = m_config.atlasWidth;
    texDesc.Height = m_config.atlasHeight;
    texDesc.DepthOrArraySize = static_cast<UINT16>(m_config.maxAtlases);
    texDesc.MipLevels = static_cast<UINT16>(m_config.generateMipmaps ? m_config.mipLevels : 1);
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&m_textureArray));

    if (FAILED(hr)) return false;

    // Create upload buffer (large enough for one full atlas layer)
    UINT64 uploadBufferSize = 0;
    m_device->GetCopyableFootprints(&texDesc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc = {};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadBufferSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_uploadBuffer));

    return SUCCEEDED(hr);
}

AtlasRegion TextureAtlasManager::allocateRegion(uint32_t width, uint32_t height) {
    AtlasRegion region;
    region.width = width;
    region.height = height;

    uint32_t paddedWidth = width + m_config.padding * 2;
    uint32_t paddedHeight = height + m_config.padding * 2;

    // Try to allocate in existing pages
    for (size_t i = 0; i < m_atlasPages.size(); i++) {
        uint32_t x, y;
        if (m_atlasPages[i]->allocate(paddedWidth, paddedHeight, x, y)) {
            region.atlasIndex = static_cast<uint32_t>(i);
            region.uvBounds = glm::vec4(
                (x + m_config.padding) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding) / static_cast<float>(m_config.atlasHeight),
                (x + m_config.padding + width) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding + height) / static_cast<float>(m_config.atlasHeight)
            );
            region.isValid = true;
            return region;
        }
    }

    // Create new atlas page if possible
    if (m_atlasPages.size() < m_config.maxAtlases) {
        m_atlasPages.push_back(std::make_unique<TextureAtlasPage>(
            m_config.atlasWidth, m_config.atlasHeight));

        uint32_t x, y;
        if (m_atlasPages.back()->allocate(paddedWidth, paddedHeight, x, y)) {
            region.atlasIndex = static_cast<uint32_t>(m_atlasPages.size() - 1);
            region.uvBounds = glm::vec4(
                (x + m_config.padding) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding) / static_cast<float>(m_config.atlasHeight),
                (x + m_config.padding + width) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding + height) / static_cast<float>(m_config.atlasHeight)
            );
            region.isValid = true;
            return region;
        }
    }

    // Failed to allocate - evict some textures
    evictLRUTextures(10);

    // Try again after eviction
    for (size_t i = 0; i < m_atlasPages.size(); i++) {
        uint32_t x, y;
        if (m_atlasPages[i]->allocate(paddedWidth, paddedHeight, x, y)) {
            region.atlasIndex = static_cast<uint32_t>(i);
            region.uvBounds = glm::vec4(
                (x + m_config.padding) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding) / static_cast<float>(m_config.atlasHeight),
                (x + m_config.padding + width) / static_cast<float>(m_config.atlasWidth),
                (y + m_config.padding + height) / static_cast<float>(m_config.atlasHeight)
            );
            region.isValid = true;
            return region;
        }
    }

    return region;  // isValid = false
}

const AtlasRegion& TextureAtlasManager::getCreatureTexture(
    uint32_t creatureId,
    uint32_t speciesId,
    const ColorGenes& genes,
    ID3D12GraphicsCommandList* commandList)
{
    static AtlasRegion invalidRegion;

    // Check if we already have this creature's texture
    auto it = m_textureEntries.find(creatureId);
    if (it != m_textureEntries.end()) {
        it->second.lastUsedTime = m_currentTime;
        return it->second.region;
    }

    // Allocate new region
    AtlasRegion region = allocateRegion(m_config.textureWidth, m_config.textureHeight);
    if (!region.isValid) {
        return invalidRegion;
    }

    // Create new entry
    CreatureTextureEntry entry;
    entry.creatureId = creatureId;
    entry.speciesId = speciesId;
    entry.colorGenes = genes;
    entry.region = region;
    entry.lastUsedTime = m_currentTime;

    // Generate and upload texture
    generateAndUploadTexture(entry, commandList);

    // Store entry
    auto result = m_textureEntries.emplace(creatureId, entry);
    return result.first->second.region;
}

void TextureAtlasManager::generateAndUploadTexture(
    CreatureTextureEntry& entry,
    ID3D12GraphicsCommandList* commandList)
{
    // Generate texture
    TextureGenParams params;
    params.width = m_config.textureWidth;
    params.height = m_config.textureHeight;
    params.seed = entry.creatureId;

    GeneratedTexture texture = m_generator->generate(entry.colorGenes, params);

    // Copy to atlas
    copyToAtlas(texture, entry.region, commandList);
}

void TextureAtlasManager::copyToAtlas(
    const GeneratedTexture& texture,
    const AtlasRegion& region,
    ID3D12GraphicsCommandList* commandList)
{
    if (!m_uploadBuffer || !m_textureArray || texture.albedoData.empty()) return;

    // Calculate destination position in atlas
    uint32_t destX = static_cast<uint32_t>(region.uvBounds.x * m_config.atlasWidth);
    uint32_t destY = static_cast<uint32_t>(region.uvBounds.y * m_config.atlasHeight);

    // Map upload buffer
    void* mappedData = nullptr;
    D3D12_RANGE readRange = {0, 0};
    HRESULT hr = m_uploadBuffer->Map(0, &readRange, &mappedData);
    if (FAILED(hr)) return;

    // Copy texture data to upload buffer
    uint8_t* destPtr = static_cast<uint8_t*>(mappedData);
    const uint8_t* srcPtr = texture.albedoData.data();

    // Get row pitch for the atlas
    UINT rowPitch = m_config.atlasWidth * 4;
    rowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);

    for (uint32_t y = 0; y < texture.height; y++) {
        memcpy(destPtr + y * rowPitch, srcPtr + y * texture.width * 4, texture.width * 4);
    }

    m_uploadBuffer->Unmap(0, nullptr);

    // Transition texture array to copy destination
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = m_textureArray.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = region.atlasIndex;
    commandList->ResourceBarrier(1, &barrier);

    // Copy from upload buffer to texture
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = m_textureArray.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = region.atlasIndex;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = 0;
    footprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    footprint.Footprint.Width = texture.width;
    footprint.Footprint.Height = texture.height;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = m_uploadBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = footprint;

    D3D12_BOX srcBox = {0, 0, 0, texture.width, texture.height, 1};
    commandList->CopyTextureRegion(&dst, destX, destY, 0, &src, &srcBox);

    // Transition back to shader resource
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    commandList->ResourceBarrier(1, &barrier);
}

void TextureAtlasManager::regenerateTexture(
    uint32_t creatureId,
    const ColorGenes& newGenes,
    ID3D12GraphicsCommandList* commandList)
{
    auto it = m_textureEntries.find(creatureId);
    if (it == m_textureEntries.end()) return;

    it->second.colorGenes = newGenes;
    it->second.needsUpdate = true;

    generateAndUploadTexture(it->second, commandList);
    it->second.needsUpdate = false;
}

void TextureAtlasManager::removeCreatureTexture(uint32_t creatureId) {
    m_textureEntries.erase(creatureId);
    // Note: We don't free the atlas region - it will be reclaimed on next clear
}

void TextureAtlasManager::update(float deltaTime, ID3D12GraphicsCommandList* commandList) {
    m_currentTime += deltaTime;

    // Process pending regenerations (limit per frame for performance)
    int processed = 0;
    for (auto& [id, entry] : m_textureEntries) {
        if (entry.needsUpdate && processed < 5) {
            generateAndUploadTexture(entry, commandList);
            entry.needsUpdate = false;
            processed++;
        }
    }
}

void TextureAtlasManager::evictLRUTextures(size_t count) {
    if (m_textureEntries.size() <= count) return;

    // Find oldest textures
    std::vector<std::pair<float, uint32_t>> timeToId;
    timeToId.reserve(m_textureEntries.size());

    for (const auto& [id, entry] : m_textureEntries) {
        timeToId.emplace_back(entry.lastUsedTime, id);
    }

    std::sort(timeToId.begin(), timeToId.end());

    // Remove oldest
    for (size_t i = 0; i < count && i < timeToId.size(); i++) {
        m_textureEntries.erase(timeToId[i].second);
    }
}

float TextureAtlasManager::getTotalUsagePercent() const {
    if (m_atlasPages.empty()) return 0.0f;

    float totalUsage = 0.0f;
    for (const auto& page : m_atlasPages) {
        totalUsage += page->getUsagePercent();
    }
    return totalUsage / m_atlasPages.size();
}

void TextureAtlasManager::clear() {
    m_textureEntries.clear();
    for (auto& page : m_atlasPages) {
        page->clear();
    }
}

// CreatureTextureBatcher implementation
CreatureTextureBatcher::CreatureTextureBatcher(TextureAtlasManager& atlasManager)
    : m_atlasManager(atlasManager) {}

void CreatureTextureBatcher::begin(ID3D12GraphicsCommandList* commandList) {
    m_commandList = commandList;
    m_requestedRegions.clear();
}

const AtlasRegion& CreatureTextureBatcher::requestTexture(
    uint32_t creatureId,
    uint32_t speciesId,
    const ColorGenes& genes)
{
    const AtlasRegion& region = m_atlasManager.getCreatureTexture(
        creatureId, speciesId, genes, m_commandList);

    m_requestedRegions.emplace_back(creatureId, region);
    return region;
}

void CreatureTextureBatcher::end() {
    m_commandList = nullptr;
}

} // namespace graphics
