#pragma once

// Forge Engine - Shader Compiler
// Runtime HLSL compilation using DXC

#include "Core/CoreMinimal.h"
#include "RHI/RHI.h"
#include <unordered_map>

namespace Forge::Shaders
{
    // ========================================================================
    // Shader Compilation Types
    // ========================================================================

    /// Shader model target
    enum class ShaderModel : u8
    {
        SM_6_0,
        SM_6_1,
        SM_6_2,
        SM_6_3,
        SM_6_4,
        SM_6_5,
        SM_6_6,
    };

    /// Compilation flags
    enum class ShaderCompileFlags : u32
    {
        None = 0,
        Debug = 1 << 0,              // Include debug info
        SkipOptimization = 1 << 1,   // Skip optimizations (faster compile)
        OptimizationLevel0 = 1 << 2,
        OptimizationLevel1 = 1 << 3,
        OptimizationLevel2 = 1 << 4,
        OptimizationLevel3 = 1 << 5, // Maximum optimization
        WarningsAsErrors = 1 << 6,
        StrictMode = 1 << 7,         // Strict HLSL mode
        AllResourcesBound = 1 << 8,  // All resources must be bound
        PackMatrixRowMajor = 1 << 9,
        PackMatrixColumnMajor = 1 << 10,
    };
    FORGE_ENABLE_ENUM_FLAGS(ShaderCompileFlags)

    /// Shader macro definition
    struct ShaderMacro
    {
        String name;
        String value;

        ShaderMacro() = default;
        ShaderMacro(StringView n, StringView v = "1")
            : name(String(n)), value(String(v)) {}
    };

    /// Shader compile options
    struct ShaderCompileOptions
    {
        RHI::ShaderType type = RHI::ShaderType::Vertex;
        ShaderModel shaderModel = ShaderModel::SM_6_0;
        ShaderCompileFlags flags = ShaderCompileFlags::None;
        StringView entryPoint = "main";
        Vector<ShaderMacro> defines;
        Vector<String> includePaths;
    };

    /// Compilation result
    struct ShaderCompileResult
    {
        bool success = false;
        Vector<u8> bytecode;
        String errorMessage;
        String warningMessage;

        // Reflection data (populated if compilation succeeded)
        struct ResourceBinding
        {
            String name;
            u32 bindPoint = 0;
            u32 bindSpace = 0;
            u32 bindCount = 1;
            enum class Type : u8
            {
                ConstantBuffer,
                Texture,
                Sampler,
                UAV,
                StructuredBuffer,
            } type = Type::ConstantBuffer;
        };
        Vector<ResourceBinding> resourceBindings;

        struct ConstantBufferLayout
        {
            String name;
            u32 size = 0;
            u32 bindPoint = 0;
            u32 bindSpace = 0;

            struct Variable
            {
                String name;
                u32 offset = 0;
                u32 size = 0;
            };
            Vector<Variable> variables;
        };
        Vector<ConstantBufferLayout> constantBuffers;
    };

    // ========================================================================
    // Shader Compiler Interface
    // ========================================================================

    /// Shader compiler using DXC
    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        // Non-copyable
        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;

        /// Initialize the compiler
        bool Initialize();

        /// Shutdown and release resources
        void Shutdown();

        /// Compile shader from source string
        ShaderCompileResult CompileFromSource(
            StringView source,
            const ShaderCompileOptions& options);

        /// Compile shader from file
        ShaderCompileResult CompileFromFile(
            StringView filePath,
            const ShaderCompileOptions& options);

        /// Add global include path
        void AddIncludePath(StringView path);

        /// Set default shader model
        void SetDefaultShaderModel(ShaderModel model);

        /// Get last error message
        [[nodiscard]] StringView GetLastError() const { return m_lastError; }

    private:
        struct Impl;
        UniquePtr<Impl> m_impl;
        String m_lastError;
        ShaderModel m_defaultShaderModel = ShaderModel::SM_6_0;
        Vector<String> m_globalIncludePaths;
    };

    // ========================================================================
    // Shader Cache
    // ========================================================================

    /// Compiled shader cache key
    struct ShaderCacheKey
    {
        String sourcePath;
        String entryPoint;
        RHI::ShaderType type;
        ShaderModel shaderModel;
        u64 definesHash;

        bool operator==(const ShaderCacheKey& other) const
        {
            return sourcePath == other.sourcePath &&
                   entryPoint == other.entryPoint &&
                   type == other.type &&
                   shaderModel == other.shaderModel &&
                   definesHash == other.definesHash;
        }
    };

    struct ShaderCacheKeyHash
    {
        usize operator()(const ShaderCacheKey& key) const;
    };

    /// Shader cache for avoiding recompilation
    class ShaderCache
    {
    public:
        ShaderCache() = default;
        ~ShaderCache() = default;

        /// Initialize cache with directory path
        bool Initialize(StringView cacheDirectory);

        /// Check if shader is in cache and up-to-date
        [[nodiscard]] bool HasValidCache(const ShaderCacheKey& key, u64 sourceTimestamp) const;

        /// Load cached bytecode
        [[nodiscard]] Vector<u8> LoadCached(const ShaderCacheKey& key) const;

        /// Store compiled bytecode in cache
        void Store(const ShaderCacheKey& key, Span<const u8> bytecode, u64 sourceTimestamp);

        /// Clear all cached shaders
        void ClearCache();

        /// Get cache hit statistics
        [[nodiscard]] u64 GetCacheHits() const { return m_cacheHits; }
        [[nodiscard]] u64 GetCacheMisses() const { return m_cacheMisses; }

    private:
        struct CacheEntry
        {
            Vector<u8> bytecode;
            u64 sourceTimestamp = 0;
        };

        String m_cacheDirectory;
        std::unordered_map<ShaderCacheKey, CacheEntry, ShaderCacheKeyHash> m_memoryCache;
        mutable u64 m_cacheHits = 0;
        mutable u64 m_cacheMisses = 0;
    };

    // ========================================================================
    // Shader Library
    // ========================================================================

    /// Manages shader compilation and caching
    class ShaderLibrary
    {
    public:
        ShaderLibrary() = default;
        ~ShaderLibrary() = default;

        /// Initialize with device and cache directory
        bool Initialize(RHI::IDevice* device, StringView cacheDirectory = "");

        /// Shutdown and release resources
        void Shutdown();

        /// Load or compile a shader
        /// Returns nullptr on failure
        [[nodiscard]] RHI::IShader* GetShader(
            StringView path,
            const ShaderCompileOptions& options);

        /// Force recompile a shader
        [[nodiscard]] RHI::IShader* RecompileShader(
            StringView path,
            const ShaderCompileOptions& options);

        /// Add include directory for shader compilation
        void AddIncludePath(StringView path);

        /// Get all loaded shaders
        [[nodiscard]] const auto& GetLoadedShaders() const { return m_shaders; }

        /// Clear all loaded shaders (forces recompilation on next access)
        void ClearShaders();

        /// Get compilation statistics
        struct Stats
        {
            u64 totalCompilations = 0;
            u64 cacheHits = 0;
            u64 cacheMisses = 0;
            u64 compilationErrors = 0;
        };
        [[nodiscard]] Stats GetStats() const;

    private:
        RHI::IDevice* m_device = nullptr;
        ShaderCompiler m_compiler;
        ShaderCache m_cache;

        struct ShaderEntry
        {
            UniquePtr<RHI::IShader> shader;
            ShaderCacheKey cacheKey;
            u64 lastModified = 0;
        };
        std::unordered_map<String, ShaderEntry> m_shaders;

        Stats m_stats;
    };

    // ========================================================================
    // Utility Functions
    // ========================================================================

    /// Get shader type from file extension
    [[nodiscard]] RHI::ShaderType GetShaderTypeFromExtension(StringView path);

    /// Get default entry point for shader type
    [[nodiscard]] StringView GetDefaultEntryPoint(RHI::ShaderType type);

    /// Get shader model string for DXC
    [[nodiscard]] const char* GetShaderModelString(ShaderModel model, RHI::ShaderType type);

    /// Calculate hash for shader defines
    [[nodiscard]] u64 CalculateDefinesHash(Span<const ShaderMacro> defines);

} // namespace Forge::Shaders
