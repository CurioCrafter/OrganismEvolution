#pragma once

// AudioManager - XAudio2-based audio system with 3D spatial audio
//
// Features:
// - XAudio2 for Windows-native audio (matches DX12 graphics choice)
// - Voice pooling with hard limits (32 total voices)
// - 3D spatial audio using X3DAudio
// - Distance-based volume falloff
// - Underwater lowpass filter
// - Category-based volume control
// - Procedural sound synthesis integration

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

// Forward declare XAudio2 types
struct IXAudio2;
struct IXAudio2MasteringVoice;
struct IXAudio2SourceVoice;
struct IXAudio2SubmixVoice;

namespace Forge {

// Forward declarations
class ProceduralSynthesizer;
class SoundscapeBudget;

// ============================================================================
// Sound Categories
// ============================================================================

enum class SoundCategory {
    MASTER,
    MUSIC,
    AMBIENT,
    CREATURES,
    UI,
    WEATHER
};

// ============================================================================
// Sound Effect IDs
// ============================================================================

enum class SoundEffect {
    // Creature sounds
    CREATURE_IDLE,
    CREATURE_MOVE,
    CREATURE_EAT,
    CREATURE_ATTACK,
    CREATURE_HURT,
    CREATURE_DEATH,
    CREATURE_BIRTH,
    CREATURE_MATING,
    CREATURE_ALERT,

    // Flying creatures
    WING_FLAP,
    BIRD_CHIRP,
    BIRD_SONG,
    INSECT_BUZZ,

    // Aquatic creatures
    SPLASH,
    UNDERWATER_AMBIENT,
    FISH_SWIM,
    FISH_BUBBLE,

    // Environment
    WIND,
    RAIN_LIGHT,
    RAIN_HEAVY,
    THUNDER,
    WATER_FLOW,
    GRASS_RUSTLE,
    TREE_CREAK,
    CRICKETS,
    FROGS,

    // UI
    UI_CLICK,
    UI_HOVER,
    UI_CONFIRM,
    UI_CANCEL,

    // Music
    MUSIC_PEACEFUL,
    MUSIC_TENSE,
    MUSIC_DRAMATIC,

    COUNT
};

// ============================================================================
// Sound Handle
// ============================================================================

struct SoundHandle {
    uint32_t id = 0;
    bool valid = false;

    static SoundHandle invalid() { return {0, false}; }
};

// ============================================================================
// 3D Sound Parameters
// ============================================================================

struct Sound3DParams {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
};

// ============================================================================
// Procedural Sound Parameters
// ============================================================================

struct ProceduralSoundParams {
    float creatureSize = 1.0f;
    float creatureSpeed = 10.0f;
    float wingSpan = 1.0f;          // For flying creatures
    float wingFrequency = 5.0f;     // Hz
    bool isBird = false;
    bool isAquatic = false;
    bool isInsect = false;
    bool isPredator = false;
};

// ============================================================================
// Audio Manager
// ============================================================================

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool init();
    void shutdown();
    bool isAvailable() const { return m_initialized; }

    // ========================================================================
    // Sound Loading (for pre-recorded sounds, optional)
    // ========================================================================

    bool loadSound(SoundEffect effect, const std::string& filepath);
    bool loadDefaultSounds(const std::string& soundDirectory);
    void unloadSound(SoundEffect effect);
    void unloadAllSounds();

    // ========================================================================
    // Sound Playback
    // ========================================================================

    // Play a 2D sound (UI, music)
    SoundHandle play(SoundEffect effect, float volume = 1.0f, float pitch = 1.0f,
                     bool loop = false);

    // Play a 3D sound at position
    SoundHandle play3D(SoundEffect effect, const Sound3DParams& params);

    // Play procedurally generated creature sound
    SoundHandle playCreatureSound(SoundEffect effect, const glm::vec3& position,
                                  const ProceduralSoundParams& params);

    // Play custom audio buffer (from ProceduralSynthesizer)
    SoundHandle playBuffer(const std::vector<int16_t>& buffer,
                          const glm::vec3& position, float volume = 1.0f);

    // Stop/pause/resume
    void stop(SoundHandle handle);
    void stopAll();
    void pause(SoundHandle handle);
    void resume(SoundHandle handle);
    bool isPlaying(SoundHandle handle) const;

    // ========================================================================
    // Volume Control
    // ========================================================================

    void setMasterVolume(float volume);
    float getMasterVolume() const { return m_masterVolume; }

    void setCategoryVolume(SoundCategory category, float volume);
    float getCategoryVolume(SoundCategory category) const;

    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    // ========================================================================
    // 3D Audio
    // ========================================================================

    // Set listener position (use CameraController position)
    void setListenerPosition(const glm::vec3& position, const glm::vec3& forward,
                             const glm::vec3& up);

    // Update a 3D sound's position
    void updateSound3D(SoundHandle handle, const glm::vec3& position,
                       const glm::vec3& velocity = glm::vec3(0.0f));

    // Set underwater mode (enables lowpass filter)
    void setUnderwaterMode(bool underwater);
    bool isUnderwaterMode() const { return m_underwaterMode; }

    // ========================================================================
    // Music
    // ========================================================================

    void playMusic(SoundEffect track, float fadeTime = 1.0f);
    void stopMusic(float fadeTime = 1.0f);
    void setMusicVolume(float volume);

    // ========================================================================
    // Ambient Sound System
    // ========================================================================

    void startAmbient(SoundEffect ambient, float volume = 0.5f);
    void stopAmbient(SoundEffect ambient, float fadeTime = 2.0f);
    void updateAmbientForConditions(float timeOfDay, float weatherIntensity);

    // ========================================================================
    // Update
    // ========================================================================

    // Call every frame to update 3D sounds, fades, etc.
    void update(float deltaTime);

    // ========================================================================
    // Accessors
    // ========================================================================

    ProceduralSynthesizer* getSynthesizer() { return m_synthesizer.get(); }
    SoundscapeBudget* getSoundscapeBudget() { return m_soundscapeBudget.get(); }

    // ========================================================================
    // Debug/Stats
    // ========================================================================

    int getActiveVoiceCount() const;
    int getTotalVoiceCount() const { return VOICE_POOL_SIZE; }
    int getMaxVoices() const { return VOICE_POOL_SIZE; }

private:
    static constexpr int VOICE_POOL_SIZE = 32;
    static constexpr int CREATURE_VOICES = 16;
    static constexpr int AMBIENT_VOICES = 4;
    static constexpr int WEATHER_VOICES = 2;
    static constexpr int UI_VOICES = 2;

    bool m_initialized = false;
    bool m_muted = false;
    bool m_underwaterMode = false;
    float m_masterVolume = 1.0f;

    // Category volumes
    std::unordered_map<SoundCategory, float> m_categoryVolumes;

    // Listener state
    glm::vec3 m_listenerPosition = glm::vec3(0.0f);
    glm::vec3 m_listenerForward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_listenerUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // XAudio2 resources
    IXAudio2* m_xaudio = nullptr;
    IXAudio2MasteringVoice* m_masterVoice = nullptr;
    IXAudio2SubmixVoice* m_underwaterSubmix = nullptr;  // For lowpass filter

    // Voice pool
    struct VoiceInfo {
        IXAudio2SourceVoice* voice = nullptr;
        SoundHandle handle;
        SoundCategory category = SoundCategory::CREATURES;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        float baseVolume = 1.0f;
        float currentVolume = 1.0f;
        bool is3D = false;
        bool paused = false;
        bool inUse = false;
        float fadeTarget = 1.0f;
        float fadeSpeed = 0.0f;

        // Buffer data (for procedural sounds, we own the buffer)
        std::vector<int16_t> bufferData;
    };
    std::vector<VoiceInfo> m_voicePool;

    // Sound ID counter
    uint32_t m_nextSoundId = 1;

    // Currently playing music
    SoundHandle m_currentMusic;
    SoundEffect m_currentMusicTrack = SoundEffect::COUNT;
    float m_musicFadeProgress = 1.0f;
    float m_musicFadeTarget = 1.0f;

    // Ambient layers
    std::vector<std::pair<SoundEffect, SoundHandle>> m_ambientLayers;

    // Procedural synthesizer
    std::unique_ptr<ProceduralSynthesizer> m_synthesizer;

    // Soundscape budget manager
    std::unique_ptr<SoundscapeBudget> m_soundscapeBudget;

    // Loaded sounds (PCM data)
    struct SoundData {
        std::vector<int16_t> pcmData;
        int sampleRate = 44100;
        int channels = 2;
        float duration = 0.0f;
    };
    std::unordered_map<SoundEffect, SoundData> m_loadedSounds;

    // Internal helpers
    SoundCategory getCategoryForEffect(SoundEffect effect) const;
    VoiceInfo* findAvailableVoice();
    VoiceInfo* findVoiceByHandle(SoundHandle handle);
    float calculateDistanceAttenuation(const glm::vec3& position, float minDist, float maxDist) const;
    void applyVolume(VoiceInfo& voice);
    void apply3DAudio(VoiceInfo& voice);

    // Generate procedural sound for effect type
    std::vector<int16_t> generateProceduralSound(SoundEffect effect,
                                                  const ProceduralSoundParams& params);
};

} // namespace Forge
