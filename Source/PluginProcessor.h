#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "PianoModel.h"
#include <array>
#include <vector>

class AureaPianoProcessor final : public juce::AudioProcessor
{
public:
    AureaPianoProcessor();
    ~AureaPianoProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Aurea Piano"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void setParameterValue(const juce::String& id, float value);

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] {}, z2[2] {};
        void setPeak(double frequency, double gainDb, double q, double sampleRate) noexcept;
        float tick(float input, int channel) noexcept;
        void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    };

    float value(const char* id) const noexcept;
    piano::LayerSettings readLayer(bool dual) const noexcept;
    void handleMidi(const juce::MidiMessage&, const piano::LayerSettings&,
                    const piano::LayerSettings&, int split);
    void renderRange(juce::AudioBuffer<float>&, int start, int end);

    piano::Model model;
    std::array<Biquad, 4> eq {};
    juce::Reverb reverb;
    std::vector<float> delayL, delayR;
    size_t delayWrite = 0;
    double rate = 48000.0;
    float compEnvelope = 0.0f;
    bool midiPedalDown = false;
    juce::SmoothedValue<float> masterSmooth, drySmooth, delayMixSmooth, reverbMixSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AureaPianoProcessor)
};
