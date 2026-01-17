#include "AudioManager.h"
#include "ProceduralSynthesizer.h"
#include "SoundscapeBudget.h"

// Windows and XAudio2 headers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#pragma comment(lib, "xaudio2.lib")
#endif

#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor / Destructor
// ============================================================================

AudioManager::AudioManager() {
    // Initialize category volumes
    m_categoryVolumes[SoundCategory::MASTER] = 1.0f;
    m_categoryVolumes[SoundCategory::MUSIC] = 0.7f;
    m_categoryVolumes[SoundCategory::AMBIENT] = 0.5f;
    m_categoryVolumes[SoundCategory::CREATURES] = 0.8f;
    m_categoryVolumes[SoundCategory::UI] = 1.0f;
    m_categoryVolumes[SoundCategory::WEATHER] = 0.6f;

    // Create procedural synthesizer
    m_synthesizer = std::make_unique<ProceduralSynthesizer>();

    // Create soundscape budget manager
    m_soundscapeBudget = std::make_unique<SoundscapeBudget>();
}

AudioManager::~AudioManager() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool AudioManager::init() {
    if (m_initialized) return true;

#ifdef _WIN32
    // Initialize COM for XAudio2
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        // Failed to initialize COM
        return false;
    }

    // Create XAudio2 engine
    hr = XAudio2Create(&m_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return false;
    }

    // Create mastering voice
    hr = m_xaudio->CreateMasteringVoice(&m_masterVoice);
    if (FAILED(hr)) {
        m_xaudio->Release();
        m_xaudio = nullptr;
        return false;
    }

    // Create submix voice for underwater effect
    XAUDIO2_VOICE_DETAILS masterDetails;
    m_masterVoice->GetVoiceDetails(&masterDetails);

    hr = m_xaudio->CreateSubmixVoice(&m_underwaterSubmix, masterDetails.InputChannels,
                                      masterDetails.InputSampleRate);
    if (FAILED(hr)) {
        // Non-fatal, underwater effect just won't work
        m_underwaterSubmix = nullptr;
    }

    // Pre-allocate voice pool
    m_voicePool.resize(VOICE_POOL_SIZE);

    m_initialized = true;
    return true;

#else
    // Stub for non-Windows platforms
    m_initialized = true;
    return true;
#endif
}

void AudioManager::shutdown() {
    if (!m_initialized) return;

    stopAll();
    unloadAllSounds();

#ifdef _WIN32
    // Destroy all source voices
    for (auto& voice : m_voicePool) {
        if (voice.voice) {
            voice.voice->DestroyVoice();
            voice.voice = nullptr;
        }
    }

    // Destroy submix voice
    if (m_underwaterSubmix) {
        m_underwaterSubmix->DestroyVoice();
        m_underwaterSubmix = nullptr;
    }

    // Destroy mastering voice
    if (m_masterVoice) {
        m_masterVoice->DestroyVoice();
        m_masterVoice = nullptr;
    }

    // Release XAudio2
    if (m_xaudio) {
        m_xaudio->Release();
        m_xaudio = nullptr;
    }
#endif

    m_initialized = false;
}

// ============================================================================
// Sound Loading
// ============================================================================

bool AudioManager::loadSound(SoundEffect effect, const std::string& filepath) {
    // For now, we generate all sounds procedurally
    // This is a placeholder for loading WAV files if needed
    return true;
}

bool AudioManager::loadDefaultSounds(const std::string& soundDirectory) {
    // Procedural generation means we don't need to load files
    return true;
}

void AudioManager::unloadSound(SoundEffect effect) {
    m_loadedSounds.erase(effect);
}

void AudioManager::unloadAllSounds() {
    m_loadedSounds.clear();
}

// ============================================================================
// Sound Playback
// ============================================================================

SoundHandle AudioManager::play(SoundEffect effect, float volume, float pitch, bool loop) {
    if (!m_initialized || m_muted) return SoundHandle::invalid();

    VoiceInfo* voiceInfo = findAvailableVoice();
    if (!voiceInfo) return SoundHandle::invalid();

    // Generate procedural sound
    ProceduralSoundParams params;
    params.creatureSize = 1.0f;
    std::vector<int16_t> buffer = generateProceduralSound(effect, params);

    if (buffer.empty()) return SoundHandle::invalid();

    // Create handle
    SoundHandle handle;
    handle.id = m_nextSoundId++;
    handle.valid = true;

    // Set up voice info
    voiceInfo->handle = handle;
    voiceInfo->category = getCategoryForEffect(effect);
    voiceInfo->baseVolume = volume;
    voiceInfo->currentVolume = volume;
    voiceInfo->is3D = false;
    voiceInfo->inUse = true;
    voiceInfo->paused = false;
    voiceInfo->fadeTarget = 1.0f;
    voiceInfo->fadeSpeed = 0.0f;
    voiceInfo->bufferData = std::move(buffer);

#ifdef _WIN32
    if (m_xaudio && !voiceInfo->bufferData.empty()) {
        // Create format
        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = 44100;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        // Create source voice if needed
        if (!voiceInfo->voice) {
            HRESULT hr = m_xaudio->CreateSourceVoice(&voiceInfo->voice, &wfx);
            if (FAILED(hr)) {
                voiceInfo->inUse = false;
                return SoundHandle::invalid();
            }
        }

        // Set pitch
        voiceInfo->voice->SetFrequencyRatio(pitch);

        // Submit buffer
        XAUDIO2_BUFFER xbuf = {};
        xbuf.AudioBytes = static_cast<UINT32>(voiceInfo->bufferData.size() * sizeof(int16_t));
        xbuf.pAudioData = reinterpret_cast<const BYTE*>(voiceInfo->bufferData.data());
        xbuf.Flags = XAUDIO2_END_OF_STREAM;
        if (loop) {
            xbuf.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        voiceInfo->voice->Stop();
        voiceInfo->voice->FlushSourceBuffers();
        voiceInfo->voice->SubmitSourceBuffer(&xbuf);
        applyVolume(*voiceInfo);
        voiceInfo->voice->Start();
    }
#endif

    return handle;
}

SoundHandle AudioManager::play3D(SoundEffect effect, const Sound3DParams& params) {
    if (!m_initialized || m_muted) return SoundHandle::invalid();

    // Check distance - don't play if too far
    float distance = glm::length(params.position - m_listenerPosition);
    if (distance > params.maxDistance) {
        return SoundHandle::invalid();
    }

    VoiceInfo* voiceInfo = findAvailableVoice();
    if (!voiceInfo) return SoundHandle::invalid();

    // Generate procedural sound based on effect type
    ProceduralSoundParams procParams;
    procParams.creatureSize = 1.0f;
    std::vector<int16_t> buffer = generateProceduralSound(effect, procParams);

    if (buffer.empty()) return SoundHandle::invalid();

    // Create handle
    SoundHandle handle;
    handle.id = m_nextSoundId++;
    handle.valid = true;

    // Set up voice info
    voiceInfo->handle = handle;
    voiceInfo->category = getCategoryForEffect(effect);
    voiceInfo->position = params.position;
    voiceInfo->velocity = params.velocity;
    voiceInfo->baseVolume = params.volume;
    voiceInfo->is3D = true;
    voiceInfo->inUse = true;
    voiceInfo->paused = false;
    voiceInfo->fadeTarget = 1.0f;
    voiceInfo->fadeSpeed = 0.0f;
    voiceInfo->bufferData = std::move(buffer);

    // Calculate 3D attenuation
    float attenuation = calculateDistanceAttenuation(params.position, params.minDistance, params.maxDistance);
    voiceInfo->currentVolume = params.volume * attenuation;

#ifdef _WIN32
    if (m_xaudio && !voiceInfo->bufferData.empty()) {
        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = 44100;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        if (!voiceInfo->voice) {
            HRESULT hr = m_xaudio->CreateSourceVoice(&voiceInfo->voice, &wfx);
            if (FAILED(hr)) {
                voiceInfo->inUse = false;
                return SoundHandle::invalid();
            }
        }

        voiceInfo->voice->SetFrequencyRatio(params.pitch);

        XAUDIO2_BUFFER xbuf = {};
        xbuf.AudioBytes = static_cast<UINT32>(voiceInfo->bufferData.size() * sizeof(int16_t));
        xbuf.pAudioData = reinterpret_cast<const BYTE*>(voiceInfo->bufferData.data());
        xbuf.Flags = XAUDIO2_END_OF_STREAM;
        if (params.loop) {
            xbuf.LoopCount = XAUDIO2_LOOP_INFINITE;
        }

        voiceInfo->voice->Stop();
        voiceInfo->voice->FlushSourceBuffers();
        voiceInfo->voice->SubmitSourceBuffer(&xbuf);
        applyVolume(*voiceInfo);
        voiceInfo->voice->Start();
    }
#endif

    return handle;
}

SoundHandle AudioManager::playCreatureSound(SoundEffect effect, const glm::vec3& position,
                                            const ProceduralSoundParams& params) {
    if (!m_initialized || m_muted) return SoundHandle::invalid();

    // Generate procedural sound with creature-specific parameters
    std::vector<int16_t> buffer = generateProceduralSound(effect, params);
    if (buffer.empty()) return SoundHandle::invalid();

    return playBuffer(buffer, position, 1.0f);
}

SoundHandle AudioManager::playBuffer(const std::vector<int16_t>& buffer,
                                     const glm::vec3& position, float volume) {
    if (!m_initialized || m_muted || buffer.empty()) return SoundHandle::invalid();

    VoiceInfo* voiceInfo = findAvailableVoice();
    if (!voiceInfo) return SoundHandle::invalid();

    // Create handle
    SoundHandle handle;
    handle.id = m_nextSoundId++;
    handle.valid = true;

    // Set up voice info
    voiceInfo->handle = handle;
    voiceInfo->category = SoundCategory::CREATURES;
    voiceInfo->position = position;
    voiceInfo->baseVolume = volume;
    voiceInfo->is3D = true;
    voiceInfo->inUse = true;
    voiceInfo->paused = false;
    voiceInfo->bufferData = buffer;

    // Calculate 3D attenuation
    float attenuation = calculateDistanceAttenuation(position, 1.0f, 200.0f);
    voiceInfo->currentVolume = volume * attenuation;

#ifdef _WIN32
    if (m_xaudio && !voiceInfo->bufferData.empty()) {
        WAVEFORMATEX wfx = {};
        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = 44100;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        if (!voiceInfo->voice) {
            HRESULT hr = m_xaudio->CreateSourceVoice(&voiceInfo->voice, &wfx);
            if (FAILED(hr)) {
                voiceInfo->inUse = false;
                return SoundHandle::invalid();
            }
        }

        XAUDIO2_BUFFER xbuf = {};
        xbuf.AudioBytes = static_cast<UINT32>(voiceInfo->bufferData.size() * sizeof(int16_t));
        xbuf.pAudioData = reinterpret_cast<const BYTE*>(voiceInfo->bufferData.data());
        xbuf.Flags = XAUDIO2_END_OF_STREAM;

        voiceInfo->voice->Stop();
        voiceInfo->voice->FlushSourceBuffers();
        voiceInfo->voice->SubmitSourceBuffer(&xbuf);
        applyVolume(*voiceInfo);
        voiceInfo->voice->Start();
    }
#endif

    return handle;
}

void AudioManager::stop(SoundHandle handle) {
    VoiceInfo* voice = findVoiceByHandle(handle);
    if (voice) {
#ifdef _WIN32
        if (voice->voice) {
            voice->voice->Stop();
            voice->voice->FlushSourceBuffers();
        }
#endif
        voice->inUse = false;
        voice->handle = SoundHandle::invalid();
    }
}

void AudioManager::stopAll() {
    for (auto& voice : m_voicePool) {
        if (voice.inUse) {
#ifdef _WIN32
            if (voice.voice) {
                voice.voice->Stop();
                voice.voice->FlushSourceBuffers();
            }
#endif
            voice.inUse = false;
            voice.handle = SoundHandle::invalid();
        }
    }

    m_currentMusic = SoundHandle::invalid();
    m_ambientLayers.clear();
}

void AudioManager::pause(SoundHandle handle) {
    VoiceInfo* voice = findVoiceByHandle(handle);
    if (voice && !voice->paused) {
#ifdef _WIN32
        if (voice->voice) {
            voice->voice->Stop();
        }
#endif
        voice->paused = true;
    }
}

void AudioManager::resume(SoundHandle handle) {
    VoiceInfo* voice = findVoiceByHandle(handle);
    if (voice && voice->paused) {
#ifdef _WIN32
        if (voice->voice) {
            voice->voice->Start();
        }
#endif
        voice->paused = false;
    }
}

bool AudioManager::isPlaying(SoundHandle handle) const {
    for (const auto& voice : m_voicePool) {
        if (voice.handle.valid && voice.handle.id == handle.id) {
            if (voice.inUse && !voice.paused) {
#ifdef _WIN32
                if (voice.voice) {
                    XAUDIO2_VOICE_STATE state;
                    voice.voice->GetState(&state);
                    return state.BuffersQueued > 0;
                }
#endif
            }
            return false;
        }
    }
    return false;
}

// ============================================================================
// Volume Control
// ============================================================================

void AudioManager::setMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);

#ifdef _WIN32
    if (m_masterVoice) {
        m_masterVoice->SetVolume(m_masterVolume);
    }
#endif
}

void AudioManager::setCategoryVolume(SoundCategory category, float volume) {
    m_categoryVolumes[category] = std::clamp(volume, 0.0f, 1.0f);

    // Update all active sounds in this category
    for (auto& voice : m_voicePool) {
        if (voice.inUse && voice.category == category) {
            applyVolume(voice);
        }
    }
}

float AudioManager::getCategoryVolume(SoundCategory category) const {
    auto it = m_categoryVolumes.find(category);
    return it != m_categoryVolumes.end() ? it->second : 1.0f;
}

void AudioManager::setMuted(bool muted) {
    m_muted = muted;

#ifdef _WIN32
    if (m_masterVoice) {
        m_masterVoice->SetVolume(muted ? 0.0f : m_masterVolume);
    }
#endif
}

// ============================================================================
// 3D Audio
// ============================================================================

void AudioManager::setListenerPosition(const glm::vec3& position,
                                        const glm::vec3& forward,
                                        const glm::vec3& up) {
    m_listenerPosition = position;
    m_listenerForward = forward;
    m_listenerUp = up;

    // Update soundscape budget with listener position
    if (m_soundscapeBudget) {
        m_soundscapeBudget->setListenerPosition(position);
    }
}

void AudioManager::updateSound3D(SoundHandle handle, const glm::vec3& position,
                                  const glm::vec3& velocity) {
    VoiceInfo* voice = findVoiceByHandle(handle);
    if (voice && voice->is3D) {
        voice->position = position;
        voice->velocity = velocity;
        apply3DAudio(*voice);
    }
}

void AudioManager::setUnderwaterMode(bool underwater) {
    if (m_underwaterMode == underwater) return;
    m_underwaterMode = underwater;

#ifdef _WIN32
    // Apply lowpass filter effect for underwater
    // This would use XAUDIO2_FILTER_PARAMETERS on the submix voice
    if (m_underwaterSubmix) {
        XAUDIO2_FILTER_PARAMETERS filterParams = {};

        if (underwater) {
            // Lowpass filter at 800 Hz
            filterParams.Type = LowPassFilter;
            filterParams.Frequency = 800.0f / 22050.0f;  // Normalized
            filterParams.OneOverQ = 1.0f;
        } else {
            // Bypass filter
            filterParams.Type = LowPassFilter;
            filterParams.Frequency = 1.0f;  // Max frequency
            filterParams.OneOverQ = 1.0f;
        }

        m_underwaterSubmix->SetFilterParameters(&filterParams);
    }
#endif
}

// ============================================================================
// Music
// ============================================================================

void AudioManager::playMusic(SoundEffect track, float fadeTime) {
    // Fade out current music
    if (m_currentMusic.valid) {
        stopMusic(fadeTime);
    }

    m_currentMusicTrack = track;
    m_currentMusic = play(track, 0.0f, 1.0f, true);  // Start at 0 volume

    // Set up fade in
    if (m_currentMusic.valid) {
        VoiceInfo* voice = findVoiceByHandle(m_currentMusic);
        if (voice) {
            voice->fadeTarget = 1.0f;
            voice->fadeSpeed = fadeTime > 0.0f ? 1.0f / fadeTime : 10.0f;
        }
    }
}

void AudioManager::stopMusic(float fadeTime) {
    if (!m_currentMusic.valid) return;

    VoiceInfo* voice = findVoiceByHandle(m_currentMusic);
    if (voice) {
        voice->fadeTarget = 0.0f;
        voice->fadeSpeed = fadeTime > 0.0f ? 1.0f / fadeTime : 10.0f;
    }

    // Will be stopped when fade completes
}

void AudioManager::setMusicVolume(float volume) {
    setCategoryVolume(SoundCategory::MUSIC, volume);
}

// ============================================================================
// Ambient Sound System
// ============================================================================

void AudioManager::startAmbient(SoundEffect ambient, float volume) {
    SoundHandle handle = play(ambient, volume, 1.0f, true);
    if (handle.valid) {
        m_ambientLayers.push_back({ambient, handle});
    }
}

void AudioManager::stopAmbient(SoundEffect ambient, float fadeTime) {
    for (auto it = m_ambientLayers.begin(); it != m_ambientLayers.end(); ) {
        if (it->first == ambient) {
            VoiceInfo* voice = findVoiceByHandle(it->second);
            if (voice) {
                voice->fadeTarget = 0.0f;
                voice->fadeSpeed = fadeTime > 0.0f ? 1.0f / fadeTime : 10.0f;
            }
            it = m_ambientLayers.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioManager::updateAmbientForConditions(float timeOfDay, float weatherIntensity) {
    // This would be called by AmbientSoundscape to adjust ambient layers
    // based on time of day and weather
}

// ============================================================================
// Update
// ============================================================================

void AudioManager::update(float deltaTime) {
    if (!m_initialized) return;

    // Update all active voices
    for (auto& voice : m_voicePool) {
        if (!voice.inUse) continue;

        // Update fades
        if (voice.fadeSpeed > 0.0f) {
            if (voice.currentVolume < voice.fadeTarget) {
                voice.currentVolume = std::min(voice.currentVolume + voice.fadeSpeed * deltaTime,
                                                voice.fadeTarget);
            } else if (voice.currentVolume > voice.fadeTarget) {
                voice.currentVolume = std::max(voice.currentVolume - voice.fadeSpeed * deltaTime,
                                                voice.fadeTarget);

                // Stop when faded out
                if (voice.currentVolume <= 0.0f) {
                    stop(voice.handle);
                    continue;
                }
            }
            applyVolume(voice);
        }

        // Update 3D audio
        if (voice.is3D) {
            apply3DAudio(voice);
        }

        // Check if sound has finished playing
#ifdef _WIN32
        if (voice.voice) {
            XAUDIO2_VOICE_STATE state;
            voice.voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (state.BuffersQueued == 0) {
                voice.inUse = false;
                voice.handle = SoundHandle::invalid();
            }
        }
#endif
    }

    // Update soundscape budget
    if (m_soundscapeBudget) {
        m_soundscapeBudget->update(deltaTime, this);
    }
}

// ============================================================================
// Debug/Stats
// ============================================================================

int AudioManager::getActiveVoiceCount() const {
    int count = 0;
    for (const auto& voice : m_voicePool) {
        if (voice.inUse) count++;
    }
    return count;
}

// ============================================================================
// Internal Helpers
// ============================================================================

SoundCategory AudioManager::getCategoryForEffect(SoundEffect effect) const {
    switch (effect) {
        case SoundEffect::MUSIC_PEACEFUL:
        case SoundEffect::MUSIC_TENSE:
        case SoundEffect::MUSIC_DRAMATIC:
            return SoundCategory::MUSIC;

        case SoundEffect::WIND:
        case SoundEffect::WATER_FLOW:
        case SoundEffect::GRASS_RUSTLE:
        case SoundEffect::TREE_CREAK:
        case SoundEffect::UNDERWATER_AMBIENT:
        case SoundEffect::CRICKETS:
        case SoundEffect::FROGS:
            return SoundCategory::AMBIENT;

        case SoundEffect::RAIN_LIGHT:
        case SoundEffect::RAIN_HEAVY:
        case SoundEffect::THUNDER:
            return SoundCategory::WEATHER;

        case SoundEffect::UI_CLICK:
        case SoundEffect::UI_HOVER:
        case SoundEffect::UI_CONFIRM:
        case SoundEffect::UI_CANCEL:
            return SoundCategory::UI;

        default:
            return SoundCategory::CREATURES;
    }
}

AudioManager::VoiceInfo* AudioManager::findAvailableVoice() {
    for (auto& voice : m_voicePool) {
        if (!voice.inUse) {
            return &voice;
        }
    }

    // No available voice - could implement priority stealing here
    return nullptr;
}

AudioManager::VoiceInfo* AudioManager::findVoiceByHandle(SoundHandle handle) {
    if (!handle.valid) return nullptr;

    for (auto& voice : m_voicePool) {
        if (voice.handle.valid && voice.handle.id == handle.id) {
            return &voice;
        }
    }
    return nullptr;
}

float AudioManager::calculateDistanceAttenuation(const glm::vec3& position,
                                                  float minDist, float maxDist) const {
    float distance = glm::length(position - m_listenerPosition);

    if (distance <= minDist) return 1.0f;
    if (distance >= maxDist) return 0.0f;

    // Linear falloff (could use inverse square for more realism)
    return 1.0f - (distance - minDist) / (maxDist - minDist);
}

void AudioManager::applyVolume(VoiceInfo& voice) {
    float categoryVol = getCategoryVolume(voice.category);
    float finalVolume = voice.currentVolume * voice.baseVolume * categoryVol * m_masterVolume;

    // Apply underwater attenuation for non-underwater sounds
    if (m_underwaterMode && voice.category != SoundCategory::UI) {
        finalVolume *= 0.3f;  // Muffled
    }

#ifdef _WIN32
    if (voice.voice) {
        voice.voice->SetVolume(finalVolume);
    }
#endif
}

void AudioManager::apply3DAudio(VoiceInfo& voice) {
    // Recalculate distance attenuation
    float attenuation = calculateDistanceAttenuation(voice.position, 1.0f, 200.0f);
    voice.currentVolume = voice.baseVolume * attenuation;

    // Calculate stereo panning based on position relative to listener
    glm::vec3 toSound = voice.position - m_listenerPosition;
    glm::vec3 right = glm::normalize(glm::cross(m_listenerForward, m_listenerUp));

    float pan = glm::dot(glm::normalize(toSound), right);
    pan = std::clamp(pan, -1.0f, 1.0f);

    // Apply panning (simple stereo panning)
#ifdef _WIN32
    if (voice.voice) {
        float leftVol = std::sqrt(0.5f * (1.0f - pan));
        float rightVol = std::sqrt(0.5f * (1.0f + pan));

        float channelVolumes[2] = {leftVol * voice.currentVolume, rightVol * voice.currentVolume};
        // XAudio2 stereo panning would need output matrix, simplified here
        applyVolume(voice);
    }
#endif
}

std::vector<int16_t> AudioManager::generateProceduralSound(SoundEffect effect,
                                                           const ProceduralSoundParams& params) {
    if (!m_synthesizer) return {};

    SynthParams synthParams;

    switch (effect) {
        case SoundEffect::CREATURE_IDLE:
            synthParams = m_synthesizer->createHerbivoreCoo(params.creatureSize);
            break;

        case SoundEffect::CREATURE_EAT:
            synthParams = m_synthesizer->createGrazingSound();
            break;

        case SoundEffect::CREATURE_ATTACK:
        case SoundEffect::CREATURE_HURT:
            if (params.isPredator) {
                synthParams = m_synthesizer->createCarnivoreGrowl(params.creatureSize);
            } else {
                synthParams = m_synthesizer->createPainSound(params.creatureSize);
            }
            break;

        case SoundEffect::CREATURE_DEATH:
            synthParams = m_synthesizer->createPainSound(params.creatureSize);
            break;

        case SoundEffect::CREATURE_MATING:
            synthParams = m_synthesizer->createMatingCall(params.creatureSize, params.isBird);
            break;

        case SoundEffect::CREATURE_ALERT:
            synthParams = m_synthesizer->createAlarmCall(params.creatureSize);
            break;

        case SoundEffect::BIRD_CHIRP:
            synthParams = m_synthesizer->createBirdChirp(params.wingSpan);
            break;

        case SoundEffect::BIRD_SONG:
            synthParams = m_synthesizer->createBirdSong(params.wingSpan);
            break;

        case SoundEffect::INSECT_BUZZ:
            synthParams = m_synthesizer->createInsectBuzz(params.wingFrequency);
            break;

        case SoundEffect::FISH_BUBBLE:
            synthParams = m_synthesizer->createFishBubble(params.creatureSize);
            break;

        case SoundEffect::UNDERWATER_AMBIENT:
            synthParams = m_synthesizer->createUnderwaterAmbient();
            break;

        case SoundEffect::WIND:
            synthParams = m_synthesizer->createWind(0.5f);
            break;

        case SoundEffect::RAIN_LIGHT:
            synthParams = m_synthesizer->createRainAmbient(0.3f);
            break;

        case SoundEffect::RAIN_HEAVY:
            synthParams = m_synthesizer->createRainAmbient(0.8f);
            break;

        case SoundEffect::THUNDER:
            synthParams = m_synthesizer->createThunder(1.0f);
            break;

        case SoundEffect::WATER_FLOW:
            synthParams = m_synthesizer->createWaterFlow(0.5f);
            break;

        case SoundEffect::CRICKETS:
            synthParams = m_synthesizer->createCrickets();
            break;

        case SoundEffect::FROGS:
            synthParams = m_synthesizer->createFrogChorus();
            break;

        default:
            // Default to a simple tone
            synthParams = m_synthesizer->createHerbivoreCoo(1.0f);
            break;
    }

    return m_synthesizer->generate(synthParams);
}

} // namespace Forge
