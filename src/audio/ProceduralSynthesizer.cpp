#include "ProceduralSynthesizer.h"
#include <random>
#include <cstring>

namespace Forge {

// Initialize static pentatonic ratios
constexpr float PentatonicScale::RATIOS[5];

// ============================================================================
// Constructor
// ============================================================================

ProceduralSynthesizer::ProceduralSynthesizer() {
    std::random_device rd;
    m_rng.seed(rd());
    m_pinkNoiseOctaves.fill(0.0f);
}

// ============================================================================
// Sound Generation
// ============================================================================

std::vector<int16_t> ProceduralSynthesizer::generate(const SynthParams& params) {
    auto mono = generateMono(params);
    return convertToStereo16(mono);
}

std::vector<float> ProceduralSynthesizer::generateMono(const SynthParams& params) {
    int numSamples = static_cast<int>(params.duration * AudioConstants::SAMPLE_RATE);
    std::vector<float> buffer(numSamples);

    // Snap frequency to pentatonic scale for musical output
    float frequency = PentatonicScale::snapToScale(params.baseFrequency);

    float phase = 0.0f;
    float phaseIncrement = frequency / AudioConstants::SAMPLE_RATE;

    // FM modulator phase
    float fmPhase = 0.0f;
    float fmPhaseIncrement = (frequency * params.fmModulatorRatio) / AudioConstants::SAMPLE_RATE;

    // Filter state
    SVFState filterState;

    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / AudioConstants::SAMPLE_RATE;

        // Apply vibrato (frequency modulation)
        float vibratoMod = 1.0f + params.vibratoDepth * std::sin(2.0f * 3.14159265f * params.vibratoRate * t);
        float currentFreq = frequency * vibratoMod;
        phaseIncrement = currentFreq / AudioConstants::SAMPLE_RATE;

        // Generate sample based on voice type
        float sample = 0.0f;

        switch (params.voiceType) {
            case VoiceType::SINE:
                sample = sineOsc(phase);
                break;

            case VoiceType::TRIANGLE:
                sample = triangleOsc(phase);
                break;

            case VoiceType::SAWTOOTH:
                sample = sawtoothOsc(phase);
                break;

            case VoiceType::PULSE:
                sample = pulseOsc(phase, 0.5f);
                break;

            case VoiceType::NOISE_FILTERED:
                sample = generatePinkNoise();
                sample = runSVF(filterState, sample, params.filterCutoff, params.filterResonance);
                break;

            case VoiceType::FM_BELL: {
                // Simple 2-operator FM synthesis
                float modulator = std::sin(fmPhase * 2.0f * 3.14159265f);
                float modulatedPhase = phase + params.fmIndex * modulator;
                sample = std::sin(modulatedPhase * 2.0f * 3.14159265f);
                fmPhase += fmPhaseIncrement;
                if (fmPhase > 1.0f) fmPhase -= 1.0f;
                break;
            }

            case VoiceType::ADDITIVE: {
                // Fundamental + harmonics
                sample = sineOsc(phase);
                sample += params.harmonic2 * sineOsc(phase * 2.0f);
                sample += params.harmonic3 * sineOsc(phase * 3.0f);
                sample += params.harmonic4 * sineOsc(phase * 4.0f);
                // Normalize
                sample /= (1.0f + params.harmonic2 + params.harmonic3 + params.harmonic4);
                break;
            }
        }

        // Apply tremolo (amplitude modulation)
        if (params.tremoloRate > 0.0f) {
            float tremoloMod = 1.0f - params.tremoloDepth * 0.5f * (1.0f + std::sin(2.0f * 3.14159265f * params.tremoloRate * t));
            sample *= tremoloMod;
        }

        // Apply envelope
        float envelope = params.envelope.getValue(t, params.duration);
        sample *= envelope;

        // Apply volume
        sample *= params.volume;

        buffer[i] = sample;

        // Advance phase
        phase += phaseIncrement;
        if (phase > 1.0f) phase -= 1.0f;
    }

    // Apply soft limiter to prevent clipping
    applySoftLimiter(buffer);

    return buffer;
}

// ============================================================================
// Creature Voice Presets
// ============================================================================

SynthParams ProceduralSynthesizer::createHerbivoreCoo(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::TRIANGLE;
    params.baseFrequency = sizeToFrequency(creatureSize);
    params.duration = 0.3f + 0.2f * (2.0f - creatureSize);  // Smaller = shorter
    params.volume = 0.25f;  // Idle sounds are quiet
    params.envelope = Envelope::soft();
    params.vibratoRate = 4.0f;
    params.vibratoDepth = 0.015f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createHerbivoreBleat(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::ADDITIVE;
    params.baseFrequency = sizeToFrequency(creatureSize) * 1.2f;  // Slightly higher
    params.duration = 0.4f;
    params.volume = 0.35f;
    params.envelope = {0.1f, 0.15f, 0.5f, 0.25f};
    params.vibratoRate = 6.0f;
    params.vibratoDepth = 0.03f;
    params.harmonic2 = 0.4f;
    params.harmonic3 = 0.15f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createGrazingSound() {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 0.15f;
    params.volume = 0.1f;  // Very quiet
    params.envelope = Envelope::percussive();
    params.filterCutoff = 2000.0f;
    params.filterResonance = 0.3f;
    return params;
}

SynthParams ProceduralSynthesizer::createCarnivoreGrowl(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::ADDITIVE;
    params.baseFrequency = sizeToFrequency(creatureSize) * 0.7f;  // Lower
    params.duration = 0.6f;
    params.volume = 0.4f;
    params.envelope = {0.15f, 0.1f, 0.7f, 0.25f};
    params.vibratoRate = 3.0f;
    params.vibratoDepth = 0.02f;
    params.harmonic2 = 0.5f;
    params.harmonic3 = 0.3f;
    params.harmonic4 = 0.15f;
    params.tremoloRate = 8.0f;
    params.tremoloDepth = 0.2f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createCarnivoreHunt(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.baseFrequency = sizeToFrequency(creatureSize);
    params.duration = 0.25f;
    params.volume = 0.3f;
    params.envelope = Envelope::percussive();
    params.filterCutoff = 800.0f;
    params.filterResonance = 0.6f;
    return params;
}

SynthParams ProceduralSynthesizer::createBirdChirp(float wingspan) {
    SynthParams params;
    params.voiceType = VoiceType::FM_BELL;
    params.baseFrequency = 800.0f + (2.0f - wingspan) * 600.0f;  // Smaller = higher
    params.duration = 0.1f + 0.05f * wingspan;
    params.volume = 0.3f;
    params.envelope = Envelope::quick();
    params.fmModulatorRatio = 3.0f;
    params.fmIndex = 2.0f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createBirdSong(float wingspan) {
    SynthParams params;
    params.voiceType = VoiceType::SINE;
    params.baseFrequency = 600.0f + (2.0f - wingspan) * 400.0f;
    params.duration = 0.5f;
    params.volume = 0.35f;
    params.envelope = {0.1f, 0.1f, 0.6f, 0.3f};
    params.vibratoRate = 8.0f;
    params.vibratoDepth = 0.04f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createFishBubble(float fishSize) {
    SynthParams params;
    params.voiceType = VoiceType::SINE;
    params.baseFrequency = 200.0f + (1.0f / fishSize) * 100.0f;  // Smaller = higher bubbles
    params.duration = 0.08f + fishSize * 0.02f;
    params.volume = 0.2f;
    params.envelope = {0.1f, 0.02f, 0.1f, 0.2f};  // Quick attack, long release
    params.vibratoRate = 20.0f;
    params.vibratoDepth = 0.1f;
    return params;
}

SynthParams ProceduralSynthesizer::createUnderwaterAmbient() {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 2.0f;
    params.volume = 0.15f;
    params.envelope = Envelope::soft();
    params.filterCutoff = 400.0f;
    params.filterResonance = 0.4f;
    return params;
}

SynthParams ProceduralSynthesizer::createInsectBuzz(float wingFrequency) {
    SynthParams params;
    params.voiceType = VoiceType::PULSE;
    params.baseFrequency = wingFrequency;  // 20-200 Hz typically
    params.duration = 0.5f;
    params.volume = 0.2f;
    params.envelope = {0.1f, 0.05f, 0.9f, 0.2f};
    params.tremoloRate = wingFrequency * 0.5f;
    params.tremoloDepth = 0.3f;
    return params;
}

SynthParams ProceduralSynthesizer::createAlarmCall(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::ADDITIVE;
    params.baseFrequency = sizeToFrequency(creatureSize) * 1.5f;  // Higher pitch = urgent
    params.duration = 0.35f;
    params.volume = 0.5f;  // Louder than idle
    params.envelope = Envelope::quick();
    params.vibratoRate = 10.0f;
    params.vibratoDepth = 0.05f;
    params.harmonic2 = 0.6f;
    params.harmonic3 = 0.3f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createMatingCall(float creatureSize, bool isBird) {
    SynthParams params;

    if (isBird) {
        params.voiceType = VoiceType::FM_BELL;
        params.baseFrequency = 600.0f + (2.0f - creatureSize) * 300.0f;
        params.duration = 0.8f;
        params.fmModulatorRatio = 2.5f;
        params.fmIndex = 1.5f;
    } else {
        params.voiceType = VoiceType::ADDITIVE;
        params.baseFrequency = sizeToFrequency(creatureSize);
        params.duration = 0.6f;
        params.harmonic2 = 0.4f;
        params.harmonic3 = 0.2f;
    }

    params.volume = 0.45f;
    params.envelope = {0.15f, 0.1f, 0.7f, 0.3f};
    params.vibratoRate = 5.0f;
    params.vibratoDepth = 0.03f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createPainSound(float creatureSize) {
    SynthParams params;
    params.voiceType = VoiceType::TRIANGLE;
    params.baseFrequency = sizeToFrequency(creatureSize) * 0.9f;
    params.duration = 0.25f;  // Brief
    params.volume = 0.35f;
    params.envelope = {0.1f, 0.05f, 0.2f, 0.2f};
    params.vibratoRate = 15.0f;
    params.vibratoDepth = 0.08f;
    params.snapFrequencyToPentatonic();
    return params;
}

// ============================================================================
// Ambient Sound Presets
// ============================================================================

SynthParams ProceduralSynthesizer::createWind(float intensity) {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 3.0f;
    params.volume = 0.15f + intensity * 0.2f;
    params.envelope = Envelope::soft();
    params.filterCutoff = 500.0f + intensity * 1000.0f;
    params.filterResonance = 0.2f;
    return params;
}

SynthParams ProceduralSynthesizer::createRainDrop() {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 0.05f;
    params.volume = 0.15f;
    params.envelope = {0.1f, 0.01f, 0.0f, 0.2f};
    params.filterCutoff = 3000.0f + (m_rng() % 2000);
    params.filterResonance = 0.7f;
    return params;
}

SynthParams ProceduralSynthesizer::createRainAmbient(float intensity) {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 2.0f;
    params.volume = 0.2f + intensity * 0.15f;
    params.envelope = Envelope::soft();
    params.filterCutoff = 4000.0f;
    params.filterResonance = 0.3f;
    return params;
}

SynthParams ProceduralSynthesizer::createThunder(float distance) {
    SynthParams params;
    params.voiceType = VoiceType::ADDITIVE;
    params.baseFrequency = 40.0f;  // Very low rumble
    params.duration = 1.5f + distance * 0.5f;  // Farther = longer roll
    params.volume = 0.6f / (1.0f + distance * 0.1f);  // Farther = quieter
    params.envelope = {0.1f, 0.3f, 0.4f, 0.8f};
    params.harmonic2 = 0.7f;
    params.harmonic3 = 0.4f;
    params.harmonic4 = 0.2f;
    params.tremoloRate = 3.0f;
    params.tremoloDepth = 0.5f;
    return params;
}

SynthParams ProceduralSynthesizer::createCrickets() {
    SynthParams params;
    params.voiceType = VoiceType::PULSE;
    params.baseFrequency = 4000.0f;  // High pitch
    params.duration = 0.08f;
    params.volume = 0.15f;
    params.envelope = Envelope::quick();
    params.tremoloRate = 40.0f;  // Rapid modulation
    params.tremoloDepth = 0.8f;
    return params;
}

SynthParams ProceduralSynthesizer::createFrogChorus() {
    SynthParams params;
    params.voiceType = VoiceType::ADDITIVE;
    params.baseFrequency = 150.0f + (m_rng() % 100);
    params.duration = 0.3f;
    params.volume = 0.25f;
    params.envelope = {0.1f, 0.05f, 0.6f, 0.2f};
    params.harmonic2 = 0.5f;
    params.harmonic3 = 0.2f;
    params.tremoloRate = 15.0f;
    params.tremoloDepth = 0.4f;
    params.snapFrequencyToPentatonic();
    return params;
}

SynthParams ProceduralSynthesizer::createWaterFlow(float speed) {
    SynthParams params;
    params.voiceType = VoiceType::NOISE_FILTERED;
    params.duration = 2.0f;
    params.volume = 0.2f + speed * 0.1f;
    params.envelope = Envelope::soft();
    params.filterCutoff = 800.0f + speed * 200.0f;
    params.filterResonance = 0.4f;
    return params;
}

// ============================================================================
// Audio Processing
// ============================================================================

void ProceduralSynthesizer::applyLowpassFilter(std::vector<float>& buffer, float cutoff) {
    if (buffer.empty()) return;

    float rc = 1.0f / (2.0f * 3.14159265f * cutoff);
    float dt = 1.0f / AudioConstants::SAMPLE_RATE;
    float alpha = dt / (rc + dt);

    float prev = buffer[0];
    for (size_t i = 1; i < buffer.size(); ++i) {
        buffer[i] = prev + alpha * (buffer[i] - prev);
        prev = buffer[i];
    }
}

void ProceduralSynthesizer::applyReverb(std::vector<float>& buffer, float roomSize, float wetLevel) {
    if (buffer.empty()) return;

    // Simple delay-based reverb with multiple tap delays
    std::vector<float> output = buffer;

    // Delay times in samples for different room reflections
    int delays[] = {
        static_cast<int>(AudioConstants::SAMPLE_RATE * 0.023f * roomSize),
        static_cast<int>(AudioConstants::SAMPLE_RATE * 0.037f * roomSize),
        static_cast<int>(AudioConstants::SAMPLE_RATE * 0.053f * roomSize),
        static_cast<int>(AudioConstants::SAMPLE_RATE * 0.079f * roomSize)
    };

    float decays[] = {0.6f, 0.5f, 0.4f, 0.3f};

    for (int tap = 0; tap < 4; ++tap) {
        int delay = delays[tap];
        float decay = decays[tap] * wetLevel;

        for (size_t i = delay; i < buffer.size(); ++i) {
            output[i] += buffer[i - delay] * decay;
        }
    }

    // Mix dry and wet
    float dryLevel = 1.0f - wetLevel * 0.5f;
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = buffer[i] * dryLevel + output[i] * wetLevel;
    }
}

void ProceduralSynthesizer::applySoftLimiter(std::vector<float>& buffer) {
    // Soft clipping using tanh-like curve
    for (float& sample : buffer) {
        // Scale to prevent hard clipping
        sample *= AudioConstants::MAX_AMPLITUDE;

        // Soft clip at +/- 1.0
        if (sample > 0.8f) {
            sample = 0.8f + 0.2f * std::tanh((sample - 0.8f) / 0.2f);
        } else if (sample < -0.8f) {
            sample = -0.8f + 0.2f * std::tanh((sample + 0.8f) / 0.2f);
        }
    }
}

std::vector<int16_t> ProceduralSynthesizer::convertToStereo16(const std::vector<float>& mono, float pan) {
    std::vector<int16_t> stereo(mono.size() * 2);

    // Calculate left/right gains from pan (-1 to +1)
    float leftGain = std::sqrt(0.5f * (1.0f - pan));
    float rightGain = std::sqrt(0.5f * (1.0f + pan));

    for (size_t i = 0; i < mono.size(); ++i) {
        float sample = mono[i];

        // Convert to 16-bit with proper scaling
        int16_t left = static_cast<int16_t>(std::clamp(sample * leftGain * 32767.0f, -32767.0f, 32767.0f));
        int16_t right = static_cast<int16_t>(std::clamp(sample * rightGain * 32767.0f, -32767.0f, 32767.0f));

        stereo[i * 2] = left;
        stereo[i * 2 + 1] = right;
    }

    return stereo;
}

// ============================================================================
// Internal Helpers
// ============================================================================

float ProceduralSynthesizer::runSVF(SVFState& state, float input, float cutoff, float resonance) {
    // State variable filter (2-pole)
    float f = 2.0f * std::sin(3.14159265f * cutoff / AudioConstants::SAMPLE_RATE);
    float q = 1.0f - resonance;

    state.low += f * state.band;
    state.high = input - state.low - q * state.band;
    state.band += f * state.high;

    return state.low;
}

float ProceduralSynthesizer::generatePinkNoise() {
    // Voss-McCartney pink noise algorithm
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    m_pinkNoiseIndex = (m_pinkNoiseIndex + 1) & 15;

    // Update one octave each sample, cycling through
    int numOctavesToUpdate = 0;
    int temp = m_pinkNoiseIndex;
    while ((temp & 1) == 0 && numOctavesToUpdate < 16) {
        numOctavesToUpdate++;
        temp >>= 1;
    }

    if (numOctavesToUpdate < 16) {
        m_pinkNoiseOctaves[numOctavesToUpdate] = dist(m_rng);
    }

    // Sum all octaves
    float sum = 0.0f;
    for (int i = 0; i < 16; ++i) {
        sum += m_pinkNoiseOctaves[i];
    }

    return sum / 16.0f;
}

} // namespace Forge
