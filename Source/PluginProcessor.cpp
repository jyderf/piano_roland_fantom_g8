#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;
using Range = juce::NormalisableRange<float>;
void addFloat(APVTS::ParameterLayout& p, const char* id, const char* name,
              float min, float max, float def, float step = 0.001f)
{
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                                                      Range(min, max, step), def));
}
void addChoice(APVTS::ParameterLayout& p, const char* id, const char* name,
               juce::StringArray choices, int def)
{
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id, 1}, name,
                                                       choices, def));
}
}

APVTS::ParameterLayout AureaPianoProcessor::makeLayout()
{
    APVTS::ParameterLayout p;
    for (auto prefix : { "a", "b" })
    {
        const juce::String x(prefix);
        const bool secondary = x == "b";
        addFloat(p, (x + "Level").toRawUTF8(), (x + " Level").toRawUTF8(), 0, 1, secondary ? 0.65f : 0.8f);
        addFloat(p, (x + "Brightness").toRawUTF8(), (x + " Brightness").toRawUTF8(), 0, 1, secondary ? 0.45f : 0.55f);
        addFloat(p, (x + "Attack").toRawUTF8(), (x + " Attack ms").toRawUTF8(), 0.2f, 120, 2);
        addFloat(p, (x + "Release").toRawUTF8(), (x + " Release ms").toRawUTF8(), 20, 1800, secondary ? 100 : 80, 1);
        addFloat(p, (x + "Hardness").toRawUTF8(), (x + " Hammer").toRawUTF8(), 0, 1, 0.5f);
        addFloat(p, (x + "Resonance").toRawUTF8(), (x + " Resonance").toRawUTF8(), 0, 1, 0.45f);
        addFloat(p, (x + "Width").toRawUTF8(), (x + " Stereo width").toRawUTF8(), 0, 1, 0.55f);
        addFloat(p, (x + "Velocity").toRawUTF8(), (x + " Velocity curve").toRawUTF8(), 0.5f, 2.0f, 1.0f);
        addFloat(p, (x + "Tune").toRawUTF8(), (x + " Fine tune cents").toRawUTF8(), -50, 50, 0, 0.1f);
        addFloat(p, (x + "Transpose").toRawUTF8(), (x + " Transpose").toRawUTF8(), -24, 24, secondary ? 0 : 0, 1);
        addChoice(p, (x + "Zone").toRawUTF8(), (x + " Zone").toRawUTF8(),
                  {"Todo", "Izquierda", "Derecha", "Apagado"}, secondary ? 3 : 0);
    }
    addChoice(p, "bTimbre", "Dual Timbre", {"Piano", "Bajo"}, 0);
    addFloat(p, "split", "Split MIDI note", 0, 127, 60, 1);
    addChoice(p, "sustainHold", "Sustain Hold", {"Apagado", "Activado"}, 0);
    addFloat(p, "master", "Master", 0, 1.5f, 0.8f);
    addFloat(p, "dry", "Dry", 0, 1, 1);
    addChoice(p, "dryOnly", "Dry no pedal audition", {"Normal", "Seco sin pedal"}, 0);
    addFloat(p, "eqLow", "EQ Bass", -18, 18, 0, 0.1f);
    addFloat(p, "eqLowMid", "EQ Low Mid", -18, 18, 0, 0.1f);
    addFloat(p, "eqHighMid", "EQ High Mid", -18, 18, 0, 0.1f);
    addFloat(p, "eqHigh", "EQ Treble", -18, 18, 0, 0.1f);
    addFloat(p, "compThreshold", "Compressor Threshold dB", -48, 0, -18, 0.1f);
    addFloat(p, "compRatio", "Compressor Ratio", 1, 12, 2.5f, 0.1f);
    addFloat(p, "compAttack", "Compressor Attack ms", 1, 100, 15, 0.1f);
    addFloat(p, "compRelease", "Compressor Release ms", 20, 800, 180, 1);
    addFloat(p, "compMakeup", "Compressor Makeup dB", 0, 18, 3, 0.1f);
    addFloat(p, "delayMix", "Delay Mix", 0, 1, 0);
    addFloat(p, "delayMs", "Delay Time ms", 50, 900, 320, 1);
    addFloat(p, "delayFeedback", "Delay Feedback", 0, 0.85f, 0.32f);
    addFloat(p, "reverbMix", "Reverb Mix", 0, 1, 0.16f);
    addFloat(p, "reverbSize", "Reverb Size", 0, 1, 0.55f);
    addFloat(p, "reverbDamping", "Reverb Damping", 0, 1, 0.5f);
    return p;
}

AureaPianoProcessor::AureaPianoProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "AureaState", makeLayout()) {}

void AureaPianoProcessor::prepareToPlay(double sampleRate, int)
{
    rate = sampleRate;
    model.prepare(sampleRate);
    for (auto& band : eq) band.reset();
    reverb.setSampleRate(sampleRate);
    reverb.reset();
    delayL.assign((size_t) std::ceil(sampleRate * 1.1), 0.0f);
    delayR.assign(delayL.size(), 0.0f);
    delayWrite = 0;
    compEnvelope = 0;
    midiPedalDown = false;
    for (auto* smoothed : {&masterSmooth, &drySmooth, &delayMixSmooth, &reverbMixSmooth})
        smoothed->reset(sampleRate, 0.025);
    masterSmooth.setCurrentAndTargetValue(value("master"));
    drySmooth.setCurrentAndTargetValue(value("dry"));
    delayMixSmooth.setCurrentAndTargetValue(value("delayMix"));
    reverbMixSmooth.setCurrentAndTargetValue(value("reverbMix"));
}

void AureaPianoProcessor::releaseResources() { model.allNotesOff(); }

bool AureaPianoProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float AureaPianoProcessor::value(const char* id) const noexcept
{
    return parameters.getRawParameterValue(id)->load(std::memory_order_relaxed);
}

piano::LayerSettings AureaPianoProcessor::readLayer(bool dual) const noexcept
{
    static constexpr const char* ids[2][10] = {
        {"aLevel", "aBrightness", "aAttack", "aRelease", "aHardness",
         "aResonance", "aWidth", "aVelocity", "aTune", "aTranspose"},
        {"bLevel", "bBrightness", "bAttack", "bRelease", "bHardness",
         "bResonance", "bWidth", "bVelocity", "bTune", "bTranspose"}
    };
    const auto get = [this, dual](int index)
    {
        return parameters.getRawParameterValue(ids[dual ? 1 : 0][index])->load(std::memory_order_relaxed);
    };
    piano::LayerSettings s;
    s.level = get(0);
    s.brightness = get(1);
    s.attackMs = get(2);
    s.releaseMs = get(3);
    s.hardness = get(4);
    s.resonance = get(5);
    s.width = get(6);
    s.velocityCurve = get(7);
    s.tuneCents = get(8);
    s.transpose = (int) get(9);
    s.zone = (int) value(dual ? "bZone" : "aZone");
    s.bass = dual && value("bTimbre") > 0.5f;
    return s;
}

void AureaPianoProcessor::handleMidi(const juce::MidiMessage& m,
                                     const piano::LayerSettings& a,
                                     const piano::LayerSettings& b, int split)
{
    if (m.isNoteOn()) model.noteOn(m.getNoteNumber(), m.getFloatVelocity(), split, a, b);
    else if (m.isNoteOff()) model.noteOff(m.getNoteNumber());
    else if (m.isController())
    {
        if (m.getControllerNumber() == 64)
        {
            midiPedalDown = m.getControllerValue() >= 64;
            model.setSustain(value("dryOnly") < 0.5f &&
                             (midiPedalDown || value("sustainHold") > 0.5f));
        }
        else if (m.getControllerNumber() == 67) model.setSoft(m.getControllerValue() >= 64);
        else if (m.getControllerNumber() == 120 || m.getControllerNumber() == 123)
        {
            model.allNotesOff();
            midiPedalDown = false;
        }
    }
}

void AureaPianoProcessor::Biquad::setPeak(double frequency, double gainDb,
                                           double q, double sampleRate) noexcept
{
    frequency = std::min(frequency, sampleRate * 0.45);
    const double A = std::pow(10.0, gainDb / 40.0);
    const double omega = 2.0 * piano::pi * frequency / sampleRate;
    const double alpha = std::sin(omega) / (2.0 * q);
    const double a0 = 1.0 + alpha / A;
    b0 = (1.0 + alpha * A) / a0;
    b1 = -2.0 * std::cos(omega) / a0;
    b2 = (1.0 - alpha * A) / a0;
    a1 = b1;
    a2 = (1.0 - alpha / A) / a0;
}

float AureaPianoProcessor::Biquad::tick(float input, int ch) noexcept
{
    const double out = b0 * input + z1[ch];
    z1[ch] = b1 * input - a1 * out + z2[ch];
    z2[ch] = b2 * input - a2 * out;
    return (float) out;
}

void AureaPianoProcessor::renderRange(juce::AudioBuffer<float>& buffer, int start, int end)
{
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    const bool dryOnly = value("dryOnly") > 0.5f;
    const float threshold = value("compThreshold");
    const float ratio = value("compRatio");
    const float makeup = std::pow(10.0f, value("compMakeup") / 20.0f);
    const float compAttack = std::exp(-1.0f / (float) (rate * value("compAttack") * 0.001f));
    const float compRelease = std::exp(-1.0f / (float) (rate * value("compRelease") * 0.001f));
    const float feedback = value("delayFeedback");
    const size_t delaySamples = (size_t) std::clamp(rate * value("delayMs") * 0.001,
                                                     1.0, (double) delayL.size() - 1.0);
    juce::Reverb::Parameters rp;
    rp.roomSize = value("reverbSize");
    rp.damping = value("reverbDamping");
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    reverb.setParameters(rp);
    masterSmooth.setTargetValue(value("master"));
    drySmooth.setTargetValue(value("dry"));
    delayMixSmooth.setTargetValue(value("delayMix"));
    reverbMixSmooth.setTargetValue(value("reverbMix"));

    for (int i = start; i < end; ++i)
    {
        float l = 0, r = 0;
        model.tick(l, r);
        if (! dryOnly)
        {
            for (auto& band : eq) { l = band.tick(l, 0); r = band.tick(r, 1); }
            const float peak = std::max(std::abs(l), std::abs(r));
            const float coefficient = peak > compEnvelope ? compAttack : compRelease;
            compEnvelope = coefficient * compEnvelope + (1.0f - coefficient) * peak;
            const float levelDb = 20.0f * std::log10(std::max(compEnvelope, 1.0e-8f));
            const float gainDb = levelDb > threshold ?
                (threshold - levelDb) * (1.0f - 1.0f / ratio) : 0.0f;
            const float gain = std::pow(10.0f, gainDb / 20.0f) * makeup;
            l *= gain;
            r *= gain;
        }

        const size_t read = (delayWrite + delayL.size() - delaySamples) % delayL.size();
        const float dl = delayL[read], dr = delayR[read];
        delayL[delayWrite] = dryOnly ? 0.0f : l + dr * feedback * 0.7f;
        delayR[delayWrite] = dryOnly ? 0.0f : r + dl * feedback * 0.7f;
        delayWrite = (delayWrite + 1) % delayL.size();

        float wetL = dryOnly ? 0.0f : l, wetR = dryOnly ? 0.0f : r;
        reverb.processStereo(&wetL, &wetR, 1);
        const float master = masterSmooth.getNextValue();
        const float dry = dryOnly ? 1.0f : drySmooth.getNextValue();
        const float delayMix = dryOnly ? 0.0f : delayMixSmooth.getNextValue();
        const float reverbMix = dryOnly ? 0.0f : reverbMixSmooth.getNextValue();
        left[i] = std::tanh((l * dry + dl * delayMix + wetL * reverbMix) * master);
        right[i] = std::tanh((r * dry + dr * delayMix + wetR * reverbMix) * master);
    }
}

void AureaPianoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumChannels() < 2 || delayL.empty()) return;
    keyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    auto a = readLayer(false), b = readLayer(true);
    const int split = (int) value("split");
    const bool dryOnly = value("dryOnly") > 0.5f;
    if (dryOnly)
    {
        a.releaseMs = std::min(a.releaseMs, 70.0f);
        b.releaseMs = std::min(b.releaseMs, 70.0f);
    }
    model.setSustain(! dryOnly && (midiPedalDown || value("sustainHold") > 0.5f));
    const double frequencies[] = {90.0, 360.0, 2700.0, 8300.0};
    const char* ids[] = {"eqLow", "eqLowMid", "eqHighMid", "eqHigh"};
    for (int i = 0; i < 4; ++i) eq[(size_t) i].setPeak(frequencies[i], value(ids[i]), 0.8, rate);
    int rendered = 0;
    for (const auto metadata : midi)
    {
        const int at = std::clamp(metadata.samplePosition, 0, buffer.getNumSamples());
        if (at > rendered) renderRange(buffer, rendered, at);
        handleMidi(metadata.getMessage(), a, b, split);
        rendered = at;
    }
    if (rendered < buffer.getNumSamples()) renderRange(buffer, rendered, buffer.getNumSamples());
}

void AureaPianoProcessor::setParameterValue(const juce::String& id, float newValue)
{
    if (auto* p = parameters.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(newValue));
}

void AureaPianoProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void AureaPianoProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType().toString()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* AureaPianoProcessor::createEditor()
{
    return new AureaPianoEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AureaPianoProcessor();
}
