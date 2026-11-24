#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Lightweight modal piano model. Every note excites inharmonic, damped string
// modes; the three slightly detuned strings produce natural beating without
// sample playback or disk access on the audio thread.
namespace piano
{
constexpr int maxVoices = 40;
constexpr int stringsPerVoice = 3;
constexpr int modesPerString = 14;
constexpr double pi = 3.14159265358979323846;

struct LayerSettings
{
    float level = 0.8f;
    float brightness = 0.55f;
    float attackMs = 2.0f;
    float releaseMs = 80.0f;
    float hardness = 0.5f;
    float resonance = 0.45f;
    float width = 0.55f;
    float velocityCurve = 1.0f;
    float tuneCents = 0.0f;
    int transpose = 0;
    int zone = 0; // 0 full, 1 left, 2 right, 3 off
    bool bass = false;
};

struct Modal
{
    float sine = 0.0f, cosine = 1.0f;
    float stepSine = 0.0f, stepCosine = 1.0f;
    float gain = 0.0f, decay = 1.0f;

    inline float tick() noexcept
    {
        const float result = sine * gain;
        const float nextSine = sine * stepCosine + cosine * stepSine;
        cosine = cosine * stepCosine - sine * stepSine;
        sine = nextSine;
        gain *= decay;
        return result;
    }
};

struct Voice
{
    std::array<std::array<Modal, modesPerString>, stringsPerVoice> strings {};
    int sourceNote = -1, layer = 0;
    uint64_t serial = 0;
    bool active = false, keyDown = false, heldByPedal = false, bass = false;
    float envelope = 0.0f, attackStep = 0.01f, releaseStep = 0.999f;
    float level = 1.0f, width = 0.5f, noiseEnvelope = 0.0f;
    uint32_t noiseState = 1;

    void start(int midiNote, int source, int layerIndex, float velocity,
               const LayerSettings& settings, double sampleRate, uint64_t generation,
               bool softPedal)
    {
        sourceNote = source;
        layer = layerIndex;
        serial = generation;
        active = keyDown = true;
        heldByPedal = false;
        bass = settings.bass;
        envelope = 0.0f;
        attackStep = 1.0f - std::exp(-1.0f / (float) (sampleRate *
                         std::max(0.2f, settings.attackMs) * 0.001f));
        releaseStep = std::exp(-1.0f / (float) (sampleRate *
                          std::max(20.0f, settings.releaseMs) * 0.001f));
        level = settings.level * (softPedal ? 0.68f : 1.0f);
        width = settings.width;
        noiseEnvelope = (settings.hardness * 0.006f + 0.001f) * velocity;
        noiseState = 0x9e3779b9u ^ (uint32_t) (generation * 747796405u);

        const double base = 440.0 * std::pow(2.0,
            (midiNote - 69) / 12.0 + settings.tuneCents / 1200.0);
        const float touch = std::pow(std::max(0.005f, velocity),
                                     settings.velocityCurve * (softPedal ? 1.5f : 1.0f));
        const float bright = std::clamp(settings.brightness + velocity * 0.17f, 0.0f, 1.0f);
        const double stiffness = bass ? 0.00002 : 0.000035 +
                                 0.00027 * std::pow(std::max(0, midiNote - 40) / 88.0, 2.0);
        const int modeCount = bass ? 10 : modesPerString;

        for (int s = 0; s < stringsPerVoice; ++s)
        {
            const double detune = bass ? 0.0 : (s - 1) * (0.35 + settings.resonance * 0.65);
            const double stringFrequency = base * std::pow(2.0, detune / 1200.0);
            for (int n = 1; n <= modesPerString; ++n)
            {
                auto& mode = strings[(size_t) s][(size_t) (n - 1)];
                const double frequency = stringFrequency * n *
                                         std::sqrt(1.0 + stiffness * n * n);
                const bool enabled = n <= modeCount && frequency < sampleRate * 0.45;
                const double angle = 2.0 * pi * frequency / sampleRate;
                mode.stepSine = enabled ? (float) std::sin(angle) : 0.0f;
                mode.stepCosine = enabled ? (float) std::cos(angle) : 1.0f;
                const double initialPhase = s * 0.12 + n * 0.017;
                mode.sine = (float) std::sin(initialPhase);
                mode.cosine = (float) std::cos(initialPhase);
                // Hammer contact position creates spectral notches. The
                // contact width and hardness change with velocity.
                const double strike = std::abs(std::sin(pi * n * (bass ? 0.14 : 0.125)));
                const double spectral = std::exp(-(n - 1) *
                    (bass ? 0.42 : (0.16 + (1.0 - bright) * 0.34)));
                mode.gain = enabled ? (float) (touch * strike * spectral /
                            (n * (bass ? 2.8 : 2.2))) : 0.0f;
                const double seconds = (bass ? 7.0 : 11.0) *
                    (0.35 + settings.resonance * 1.15) /
                    (1.0 + (n - 1) * (bass ? 0.40 : 0.52));
                mode.decay = (float) std::exp(-1.0 / (sampleRate * seconds));
            }
        }
    }

    void release(bool pedalDown) noexcept
    {
        keyDown = false;
        heldByPedal = pedalDown;
    }

    void pedalUp() noexcept { heldByPedal = false; }

    inline void tick(float& left, float& right) noexcept
    {
        if (! active) return;
        if (keyDown || heldByPedal)
            envelope += (1.0f - envelope) * attackStep;
        else
            envelope *= releaseStep;

        float stringOutput[3] = {};
        for (int s = 0; s < stringsPerVoice; ++s)
            for (auto& mode : strings[(size_t) s])
                stringOutput[s] += mode.tick();

        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        const float noise = ((noiseState & 0xffffu) / 32768.0f - 1.0f) * noiseEnvelope;
        noiseEnvelope *= 0.995f;
        const float scale = envelope * level * 0.22f;
        const float side = (stringOutput[2] - stringOutput[0]) * width * 0.35f;
        const float centre = (stringOutput[0] + stringOutput[1] + stringOutput[2]) / 3.0f + noise;
        left += (centre - side) * scale;
        right += (centre + side) * scale;
        if (envelope < 0.00005f && ! keyDown && ! heldByPedal)
            active = false;
    }
};

class Model
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
        allNotesOff();
        constexpr double bodyFrequencies[] = {112.0, 224.0, 454.0};
        for (int i = 0; i < 3; ++i)
            body[(size_t) i].prepare(bodyFrequencies[i], sampleRate);
    }

    void noteOn(int note, float velocity, int splitNote,
                const LayerSettings& main, const LayerSettings& dual)
    {
        if (note < 0 || note > 127 || velocity <= 0.0f) return;
        startLayer(note, velocity, splitNote, main, 0);
        startLayer(note, velocity, splitNote, dual, 1);
    }

    void noteOff(int note) noexcept
    {
        for (auto& voice : voices)
            if (voice.active && voice.sourceNote == note && voice.keyDown)
                voice.release(pedalDown);
    }

    void setSustain(bool down) noexcept
    {
        pedalDown = down;
        if (! down)
            for (auto& voice : voices)
                if (! voice.keyDown) voice.pedalUp();
    }

    void setSoft(bool down) noexcept { softDown = down; }

    void allNotesOff() noexcept
    {
        for (auto& voice : voices) voice.active = false;
        pedalDown = softDown = false;
    }

    inline void tick(float& left, float& right) noexcept
    {
        for (auto& voice : voices) voice.tick(left, right);
        const float mono = (left + right) * 0.5f;
        float soundboard = 0.0f;
        for (auto& mode : body) soundboard += mode.tick(mono);
        left += soundboard * 0.055f;
        right += soundboard * 0.055f;
    }

private:
    struct BodyMode
    {
        float a1 = 0, a2 = 0, drive = 0;
        float y1 = 0, y2 = 0;
        void prepare(double frequency, double sampleRate) noexcept
        {
            const double radius = std::exp(-1.0 / (sampleRate * 0.65));
            a1 = (float) (2.0 * radius * std::cos(2.0 * pi * frequency / sampleRate));
            a2 = (float) (-radius * radius);
            drive = (float) (1.0 - radius);
            y1 = y2 = 0;
        }
        inline float tick(float input) noexcept
        {
            const float y = input * drive + a1 * y1 + a2 * y2;
            y2 = y1;
            y1 = y;
            return y;
        }
    };
    static bool inZone(int note, int split, int zone) noexcept
    {
        return zone == 0 || (zone == 1 && note < split) ||
               (zone == 2 && note >= split);
    }

    void startLayer(int source, float velocity, int split,
                    const LayerSettings& settings, int index)
    {
        if (settings.zone == 3 || ! inZone(source, split, settings.zone)) return;
        const int sounding = std::clamp(source + settings.transpose, 21, 108);
        Voice* selected = nullptr;
        for (auto& voice : voices)
            if (! voice.active) { selected = &voice; break; }
        if (selected == nullptr)
        {
            selected = &voices[0];
            for (auto& voice : voices)
                if (voice.serial < selected->serial) selected = &voice;
        }
        selected->start(sounding, source, index, velocity, settings,
                        sampleRate, ++serial, softDown);
    }

    std::array<Voice, maxVoices> voices {};
    std::array<BodyMode, 3> body {};
    double sampleRate = 48000.0;
    uint64_t serial = 0;
    bool pedalDown = false, softDown = false;
};
} // namespace piano
