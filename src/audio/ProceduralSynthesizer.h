#pragma once

// ProceduralSynthesizer - Generates musically-constrained audio waveforms
// All output is designed to be pleasant and non-fatiguing for extended listening
//
// Key design principles:
// 1. Musical constraints: All pitches snap to pentatonic scale (no dissonant intervals)
// 2. Smooth envelopes: 100ms minimum fade-in, 200ms fade-out prevents clicks
// 3. Harmonic richness: Sine + overtones at musical ratios, never harsh
// 4. Volume safety: Hard limiter prevents clipping

#include <vector>
#include <array>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <random>

namespace Forge {

// ============================================================================
// Audio Constants (reuse existing project constants where possible)
// ============================================================================

namespace AudioConstants {
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNELS = 2;  // Stereo
    constexpr int BITS_PER_SAMPLE = 16;
    constexpr float MAX_AMPLITUDE = 0.8f;  // Leave headroom to prevent clipping

    // Voice budget limits (hard caps)
    constexpr int MAX_CREATURE_VOICES = 16;
    constexpr int MAX_AMBIENT_LAYERS = 4;
    constexpr int MAX_WEATHER_SOUNDS = 2;
    constexpr int MAX_UI_SOUNDS = 2;
    constexpr int TOTAL_VOICE_POOL = 32;

    // Audio distance limits (matches PerformanceManager LOD thresholds)
    constexpr float MAX_AUDIO_DISTANCE = 200.0f;  // Match lowToBillboard
    constexpr float FULL_VOLUME_DISTANCE = 30.0f; // Match everyFrameDistance
}

// ============================================================================
// Pentatonic Scale - Musical constraint to ensure pleasant sounds
// ============================================================================

class PentatonicScale {
public:
    // Pentatonic scale ratios: C, D, E, G, A (1, 9/8, 5/4, 3/2, 5/3)
    static constexpr float RATIOS[5] = {
        1.0f,           // Root (C)
        9.0f / 8.0f,    // Major second (D)
        5.0f / 4.0f,    // Major third (E)
        3.0f / 2.0f,    // Perfect fifth (G)
        5.0f / 3.0f     // Major sixth (A)
    };

    // Snap any frequency to the nearest pentatonic note
    static float snapToScale(float frequency, float rootFrequency = 110.0f) {
        // Find which octave the frequency is in
        float octaveMultiplier = 1.0f;
        float tempFreq = frequency;

        while (tempFreq > rootFrequency * 2.0f) {
            tempFreq /= 2.0f;
            octaveMultiplier *= 2.0f;
        }
        while (tempFreq < rootFrequency) {
            tempFreq *= 2.0f;
            octaveMultiplier /= 2.0f;
        }

        // Find the nearest scale degree
        float ratio = tempFreq / rootFrequency;
        float nearestRatio = RATIOS[0];
        float minDiff = std::abs(ratio - RATIOS[0]);

        for (int i = 1; i < 5; ++i) {
            float diff = std::abs(ratio - RATIOS[i]);
            if (diff < minDiff) {
                minDiff = diff;
                nearestRatio = RATIOS[i];
            }
        }

        return rootFrequency * nearestRatio * octaveMultiplier;
    }

    // Get a random note from the scale within an octave range
    static float getRandomNote(float baseFreq, int octaveRange, std::mt19937& rng) {
        std::uniform_int_distribution<int> noteDist(0, 4);
        std::uniform_int_distribution<int> octaveDist(0, octaveRange);

        int noteIdx = noteDist(rng);
        int octave = octaveDist(rng);

        return baseFreq * RATIOS[noteIdx] * std::pow(2.0f, static_cast<float>(octave));
    }
};

// ============================================================================
// Envelope Generator - ADSR with guaranteed smooth transitions
// ============================================================================

struct Envelope {
    float attack = 0.1f;   // Seconds (minimum 100ms)
    float decay = 0.1f;    // Seconds
    float sustain = 0.7f;  // Level (0-1)
    float release = 0.2f;  // Seconds (minimum 200ms)

    // Ensure minimum values for smooth transitions
    void validate() {
        attack = std::max(attack, 0.1f);
        release = std::max(release, 0.2f);
        sustain = std::clamp(sustain, 0.0f, 1.0f);
    }

    // Get envelope value at time t for a note of given duration
    float getValue(float t, float noteDuration) const {
        if (t < 0.0f) return 0.0f;

        // Attack phase
        if (t < attack) {
            return t / attack;
        }

        // Decay phase
        float afterAttack = t - attack;
        if (afterAttack < decay) {
            return 1.0f - (1.0f - sustain) * (afterAttack / decay);
        }

        // Sustain phase
        float releaseStart = noteDuration - release;
        if (t < releaseStart) {
            return sustain;
        }

        // Release phase
        float releaseProgress = (t - releaseStart) / release;
        return sustain * (1.0f - std::min(releaseProgress, 1.0f));
    }

    // Quick envelope for short sounds
    static Envelope quick() {
        return {0.1f, 0.05f, 0.8f, 0.2f};
    }

    // Soft envelope for ambient sounds
    static Envelope soft() {
        return {0.3f, 0.2f, 0.6f, 0.5f};
    }

    // Percussive envelope
    static Envelope percussive() {
        return {0.1f, 0.2f, 0.3f, 0.2f};
    }
};

// ============================================================================
// Voice Types - Different synthesis algorithms for creature sounds
// ============================================================================

enum class VoiceType {
    SINE,           // Pure tone (underwater sounds)
    TRIANGLE,       // Soft, mellow (herbivore coos)
    SAWTOOTH,       // Bright, buzzy (insects)
    PULSE,          // Hollow, reedy (birds)
    NOISE_FILTERED, // Noise through resonant filter (wind, growls)
    FM_BELL,        // FM synthesis for bell-like tones
    ADDITIVE        // Multiple harmonics (rich tones)
};

// ============================================================================
// Synthesizer Parameters
// ============================================================================

struct SynthParams {
    VoiceType voiceType = VoiceType::SINE;
    float baseFrequency = 440.0f;  // Hz (will be snapped to pentatonic)
    float duration = 1.0f;         // Seconds
    float volume = 0.5f;           // 0-1
    Envelope envelope;

    // Modulation
    float vibratoRate = 5.0f;      // Hz
    float vibratoDepth = 0.02f;    // Frequency deviation (0-0.1)
    float tremoloRate = 0.0f;      // Hz (0 = disabled)
    float tremoloDepth = 0.0f;     // Volume deviation

    // Harmonics (for additive synthesis)
    float harmonic2 = 0.3f;        // 2nd harmonic amplitude
    float harmonic3 = 0.1f;        // 3rd harmonic amplitude
    float harmonic4 = 0.05f;       // 4th harmonic amplitude

    // FM parameters
    float fmModulatorRatio = 2.0f; // Frequency ratio of modulator
    float fmIndex = 1.0f;          // Modulation index

    // Noise filter
    float filterCutoff = 1000.0f;  // Hz
    float filterResonance = 0.5f;  // 0-1

    // Apply musical constraint to frequency
    void snapFrequencyToPentatonic() {
        baseFrequency = PentatonicScale::snapToScale(baseFrequency);
    }
};

// ============================================================================
// Procedural Synthesizer
// ============================================================================

class ProceduralSynthesizer {
public:
    ProceduralSynthesizer();
    ~ProceduralSynthesizer() = default;

    // ========================================================================
    // Sound Generation
    // ========================================================================

    // Generate a sound buffer from parameters
    // Returns PCM audio data (16-bit signed, stereo interleaved)
    std::vector<int16_t> generate(const SynthParams& params);

    // Generate mono float buffer (for internal processing)
    std::vector<float> generateMono(const SynthParams& params);

    // ========================================================================
    // Creature Voice Presets
    // ========================================================================

    // Herbivore sounds (gentle, non-threatening)
    SynthParams createHerbivoreCoo(float creatureSize);
    SynthParams createHerbivoreBleat(float creatureSize);
    SynthParams createGrazingSound();

    // Carnivore sounds (low, powerful but not scary)
    SynthParams createCarnivoreGrowl(float creatureSize);
    SynthParams createCarnivoreHunt(float creatureSize);

    // Bird sounds (melodic whistles)
    SynthParams createBirdChirp(float wingspan);
    SynthParams createBirdSong(float wingspan);

    // Fish/aquatic sounds (bubbles, underwater)
    SynthParams createFishBubble(float fishSize);
    SynthParams createUnderwaterAmbient();

    // Insect sounds (buzzing)
    SynthParams createInsectBuzz(float wingFrequency);

    // Alert/alarm sounds
    SynthParams createAlarmCall(float creatureSize);

    // Mating calls (melodic, attractive)
    SynthParams createMatingCall(float creatureSize, bool isBird);

    // Pain/death (brief, not disturbing)
    SynthParams createPainSound(float creatureSize);

    // ========================================================================
    // Ambient Sound Presets
    // ========================================================================

    // Wind (pink noise, not harsh)
    SynthParams createWind(float intensity);

    // Rain drops (filtered noise bursts)
    SynthParams createRainDrop();
    SynthParams createRainAmbient(float intensity);

    // Thunder (low rumble)
    SynthParams createThunder(float distance);

    // Crickets (rhythmic chirping)
    SynthParams createCrickets();

    // Frogs (rhythmic croaking)
    SynthParams createFrogChorus();

    // Water flow (continuous noise)
    SynthParams createWaterFlow(float speed);

    // ========================================================================
    // Audio Processing
    // ========================================================================

    // Apply lowpass filter (for underwater effect)
    void applyLowpassFilter(std::vector<float>& buffer, float cutoff);

    // Apply reverb (simple delay-based)
    void applyReverb(std::vector<float>& buffer, float roomSize, float wetLevel);

    // Soft limiter to prevent clipping
    void applySoftLimiter(std::vector<float>& buffer);

    // Convert mono float to stereo int16
    std::vector<int16_t> convertToStereo16(const std::vector<float>& mono, float pan = 0.0f);

    // ========================================================================
    // Utility
    // ========================================================================

    // Map creature size (0.5-2.0) to frequency range
    static float sizeToFrequency(float size);

    // Map creature speed to rhythm
    static float speedToTempo(float speed);

private:
    std::mt19937 m_rng;

    // Oscillator functions
    float sineOsc(float phase);
    float triangleOsc(float phase);
    float sawtoothOsc(float phase);
    float pulseOsc(float phase, float width = 0.5f);
    float noiseOsc();

    // Simple state variable filter
    struct SVFState {
        float low = 0.0f;
        float band = 0.0f;
        float high = 0.0f;
    };
    float runSVF(SVFState& state, float input, float cutoff, float resonance);

    // Pink noise generator (Voss-McCartney algorithm)
    float generatePinkNoise();
    std::array<float, 16> m_pinkNoiseOctaves{};
    int m_pinkNoiseIndex = 0;
};

// ============================================================================
// Inline Implementation - Oscillators
// ============================================================================

inline float ProceduralSynthesizer::sineOsc(float phase) {
    return std::sin(phase * 2.0f * 3.14159265f);
}

inline float ProceduralSynthesizer::triangleOsc(float phase) {
    phase = std::fmod(phase, 1.0f);
    if (phase < 0.25f) return phase * 4.0f;
    if (phase < 0.75f) return 2.0f - phase * 4.0f;
    return phase * 4.0f - 4.0f;
}

inline float ProceduralSynthesizer::sawtoothOsc(float phase) {
    phase = std::fmod(phase, 1.0f);
    return 2.0f * phase - 1.0f;
}

inline float ProceduralSynthesizer::pulseOsc(float phase, float width) {
    phase = std::fmod(phase, 1.0f);
    return phase < width ? 1.0f : -1.0f;
}

inline float ProceduralSynthesizer::noiseOsc() {
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    return dist(m_rng);
}

// ============================================================================
// Inline Implementation - Size/Speed Mappings
// ============================================================================

inline float ProceduralSynthesizer::sizeToFrequency(float size) {
    // Size 0.5 (tiny) -> 800 Hz (high pitch)
    // Size 2.0 (huge) -> 80 Hz (low pitch)
    // Logarithmic mapping for natural sound
    float normalizedSize = std::clamp(size, 0.5f, 2.0f);
    float logSize = std::log2(normalizedSize / 0.5f) / std::log2(4.0f);  // 0 to 1
    float logFreq = std::log2(800.0f) * (1.0f - logSize) + std::log2(80.0f) * logSize;
    return std::pow(2.0f, logFreq);
}

inline float ProceduralSynthesizer::speedToTempo(float speed) {
    // Speed 5 (slow) -> 0.5 Hz rhythm
    // Speed 20 (fast) -> 2.0 Hz rhythm
    float normalizedSpeed = std::clamp(speed, 5.0f, 20.0f);
    return 0.1f * normalizedSpeed;
}

} // namespace Forge
