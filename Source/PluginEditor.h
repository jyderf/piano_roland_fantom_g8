#pragma once

#include "PluginProcessor.h"
#include <memory>
#include <vector>

class AureaPianoEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AureaPianoEditor(AureaPianoProcessor&);
    ~AureaPianoEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void heading(const juce::String& text, int y);
    void knob(const char* id, const juce::String& title, int x, int y,
              const juce::String& suffix = {});
    void mini(const char* id, const juce::String& title, int x, int y,
              const juce::String& suffix = {});
    void choice(const char* id, const juce::String& title, int x, int y, int width,
                const juce::StringArray& items);
    juce::ComboBox& preset(const juce::String& title, int x, int y, int width,
                           const juce::StringArray& items);
    void applyEqPreset(int index);
    void applyCompPreset(int index);
    void applyPianoPreset(int index);

    AureaPianoProcessor& processor;
    juce::Component content;
    juce::Viewport viewport;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton octaveDown {"− oct"}, octaveUp {"+ oct"};
    juce::Label title, subtitle, keyboardHelp;
    int keyboardOctave = 5;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AureaPianoEditor)
};
