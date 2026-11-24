#include "PluginEditor.h"

namespace
{
const juce::Colour background(0xff10151d);
const juce::Colour panel(0xff1c2532);
const juce::Colour accent(0xffd7ae61);
const juce::Colour text(0xffe8e9e9);
const juce::Colour muted(0xffaeb8c4);
}

AureaPianoEditor::AureaPianoEditor(AureaPianoProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setSize(1100, 760);
    setResizable(true, true);
    setResizeLimits(1100, 630, 1500, 1000);

    title.setText("AUREA PIANO", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);
    subtitle.setText("Piano de modelado modal • VST3 / independiente", juce::dontSendNotification);
    subtitle.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(subtitle);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setColour(juce::Viewport::backgroundColourId, background);
    addAndMakeVisible(viewport);
    content.setSize(1060, 1150);

    heading("PIANO PRINCIPAL", 8);
    const char* aIds[] = {"aLevel", "aBrightness", "aAttack", "aRelease", "aHardness", "aResonance", "aWidth", "aTranspose"};
    const char* aLabels[] = {"Volumen", "Brillo", "Ataque", "Liberación", "Martillo", "Resonancia", "Anchura", "Transponer"};
    for (int i = 0; i < 8; ++i) knob(aIds[i], aLabels[i], 16 + i * 129, 47,
                                      i == 2 || i == 3 ? " ms" : i == 7 ? " semitonos" : "");
    choice("aZone", "Zona principal", 16, 166, 190,
           {"Todo", "Izquierda", "Derecha", "Apagado"});
    mini("aVelocity", "Curva velo.", 730, 166);
    mini("aTune", "Afinación", 890, 166, " cents");

    heading("CAPA DUAL / BAJO", 270);
    const char* bIds[] = {"bLevel", "bBrightness", "bAttack", "bRelease", "bHardness", "bResonance", "bWidth", "bTranspose"};
    const char* bLabels[] = {"Volumen", "Brillo", "Ataque", "Liberación", "Martillo", "Resonancia", "Anchura", "Transponer"};
    for (int i = 0; i < 8; ++i) knob(bIds[i], bLabels[i], 16 + i * 129, 309,
                                      i == 2 || i == 3 ? " ms" : i == 7 ? " semitonos" : "");
    choice("bZone", "Zona dual", 16, 428, 190,
           {"Todo", "Izquierda", "Derecha", "Apagado"});
    choice("bTimbre", "Timbre dual", 220, 428, 190, {"Piano", "Bajo"});
    mini("split", "División MIDI", 425, 428, " nota");
    choice("sustainHold", "Sustain fijo", 590, 428, 120, {"Apagado", "Activado"});
    mini("bVelocity", "Curva velo.", 730, 428);
    mini("bTune", "Afinación", 890, 428, " cents");

    heading("ECUALIZADOR DE 4 BANDAS", 540);
    knob("eqLow", "90 Hz", 16, 579, " dB");
    knob("eqLowMid", "360 Hz", 145, 579, " dB");
    knob("eqHighMid", "2.7 kHz", 274, 579, " dB");
    knob("eqHigh", "8.3 kHz", 403, 579, " dB");
    auto& eqPresets = preset("Preset de EQ", 555, 590, 195,
                             {"Seleccionar", "Plano", "Cálido", "Brillante", "Presencia", "Suave"});
    eqPresets.onChange = [this, box = &eqPresets] { applyEqPreset(box->getSelectedId()); };

    heading("COMPRESOR", 710);
    knob("compThreshold", "Umbral", 16, 749, " dB");
    knob("compRatio", "Relación", 145, 749, ":1");
    knob("compAttack", "Ataque", 274, 749, " ms");
    knob("compRelease", "Liberación", 403, 749, " ms");
    knob("compMakeup", "Ganancia", 532, 749, " dB");
    auto& compPresets = preset("Preset de compresor", 680, 760, 205,
                               {"Seleccionar", "Sin compresión", "Natural", "Pop", "Escenario"});
    compPresets.onChange = [this, box = &compPresets] { applyCompPreset(box->getSelectedId()); };

    heading("ESPACIO Y SALIDA", 880);
    const char* fxIds[] = {"dry", "delayMix", "delayMs", "delayFeedback", "reverbMix", "reverbSize", "reverbDamping", "master"};
    const char* fxLabels[] = {"Seco", "Delay", "Tiempo", "Feedback", "Reverb", "Sala", "Amortiguación", "Volumen"};
    for (int i = 0; i < 8; ++i) knob(fxIds[i], fxLabels[i], 16 + i * 129, 919, i == 2 ? " ms" : "");
    auto& pianoPresets = preset("Piano inicial", 16, 1055, 210,
                                {"Seleccionar", "Concierto", "Cálido", "Pop", "Balada"});
    pianoPresets.onChange = [this, box = &pianoPresets] { applyPianoPreset(box->getSelectedId()); };
    choice("dryOnly", "Prueba seca / sin pedal", 240, 1055, 205,
           {"Normal", "Activada"});

    keyboard.setAvailableRange(21, 108);
    keyboard.setKeyWidth(18.0f);
    keyboard.setKeyPressBaseOctave(keyboardOctave);
    keyboard.clearKeyMappings();
    const char* keys = "awsedftgyhujkolp";
    for (int i = 0; keys[i] != 0; ++i)
        keyboard.setKeyPressForNote(juce::KeyPress(keys[i]), i);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, panel);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent);
    addAndMakeVisible(keyboard);
    keyboard.setWantsKeyboardFocus(true);
    keyboardHelp.setText("Ratón o A W S E D F T G Y H U J K O L P · MIDI: pista del DAW o Opciones > Audio/MIDI",
                         juce::dontSendNotification);
    keyboardHelp.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(keyboardHelp);
    for (auto* button : {&octaveDown, &octaveUp})
    {
        button->setColour(juce::TextButton::buttonColourId, panel);
        button->setColour(juce::TextButton::textColourOffId, text);
        addAndMakeVisible(*button);
    }
    octaveDown.onClick = [this]
    {
        keyboardOctave = juce::jmax(2, keyboardOctave - 1);
        keyboard.setKeyPressBaseOctave(keyboardOctave);
    };
    octaveUp.onClick = [this]
    {
        keyboardOctave = juce::jmin(7, keyboardOctave + 1);
        keyboard.setKeyPressBaseOctave(keyboardOctave);
    };
}

void AureaPianoEditor::heading(const juce::String& value, int y)
{
    auto label = std::make_unique<juce::Label>();
    label->setText(value, juce::dontSendNotification);
    label->setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    label->setColour(juce::Label::textColourId, accent);
    label->setBounds(16, y, 600, 31);
    content.addAndMakeVisible(*label);
    labels.push_back(std::move(label));
}

void AureaPianoEditor::knob(const char* id, const juce::String& caption,
                            int x, int y, const juce::String& suffix)
{
    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 102, 20);
    slider->setTextValueSuffix(suffix);
    slider->setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider->setColour(juce::Slider::rotarySliderOutlineColourId, muted.withAlpha(0.35f));
    slider->setColour(juce::Slider::textBoxTextColourId, text);
    slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider->setBounds(x, y, 112, 94);
    content.addAndMakeVisible(*slider);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(processor.parameters, id, *slider));
    sliders.push_back(std::move(slider));

    auto label = std::make_unique<juce::Label>();
    label->setText(caption, juce::dontSendNotification);
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, text);
    label->setBounds(x, y + 93, 112, 24);
    content.addAndMakeVisible(*label);
    labels.push_back(std::move(label));
}

void AureaPianoEditor::mini(const char* id, const juce::String& caption,
                            int x, int y, const juce::String& suffix)
{
    auto label = std::make_unique<juce::Label>();
    label->setText(caption, juce::dontSendNotification);
    label->setColour(juce::Label::textColourId, muted);
    label->setBounds(x, y, 150, 22);
    content.addAndMakeVisible(*label);
    labels.push_back(std::move(label));

    auto slider = std::make_unique<juce::Slider>();
    slider->setSliderStyle(juce::Slider::LinearHorizontal);
    slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 74, 23);
    slider->setTextValueSuffix(suffix);
    slider->setColour(juce::Slider::thumbColourId, accent);
    slider->setColour(juce::Slider::textBoxTextColourId, text);
    slider->setBounds(x, y + 22, 150, 29);
    content.addAndMakeVisible(*slider);
    sliderAttachments.push_back(std::make_unique<SliderAttachment>(processor.parameters, id, *slider));
    sliders.push_back(std::move(slider));
}

void AureaPianoEditor::choice(const char* id, const juce::String& caption,
                              int x, int y, int width, const juce::StringArray& items)
{
    auto combo = std::make_unique<juce::ComboBox>();
    combo->addItemList(items, 1);
    combo->setColour(juce::ComboBox::backgroundColourId, panel);
    combo->setColour(juce::ComboBox::textColourId, text);
    combo->setColour(juce::ComboBox::outlineColourId, muted.withAlpha(0.5f));
    combo->setBounds(x, y + 24, width, 30);
    content.addAndMakeVisible(*combo);
    comboAttachments.push_back(std::make_unique<ComboAttachment>(processor.parameters, id, *combo));
    combos.push_back(std::move(combo));

    auto label = std::make_unique<juce::Label>();
    label->setText(caption, juce::dontSendNotification);
    label->setColour(juce::Label::textColourId, muted);
    label->setBounds(x, y, width, 24);
    content.addAndMakeVisible(*label);
    labels.push_back(std::move(label));
}

juce::ComboBox& AureaPianoEditor::preset(const juce::String& caption, int x, int y,
                                         int width, const juce::StringArray& items)
{
    auto combo = std::make_unique<juce::ComboBox>();
    combo->addItemList(items, 1);
    combo->setSelectedId(1, juce::dontSendNotification);
    combo->setColour(juce::ComboBox::backgroundColourId, panel);
    combo->setColour(juce::ComboBox::textColourId, text);
    combo->setBounds(x, y + 26, width, 32);
    content.addAndMakeVisible(*combo);
    auto& ref = *combo;
    combos.push_back(std::move(combo));

    auto label = std::make_unique<juce::Label>();
    label->setText(caption, juce::dontSendNotification);
    label->setColour(juce::Label::textColourId, muted);
    label->setBounds(x, y, width, 24);
    content.addAndMakeVisible(*label);
    labels.push_back(std::move(label));
    return ref;
}

void AureaPianoEditor::applyEqPreset(int selected)
{
    static constexpr float gains[][4] = {
        {0, 0, 0, 0}, {2.5f, 1.5f, -1.5f, -1.0f},
        {-1, -1, 2.0f, 3.0f}, {-2, 0.5f, 3, 1.5f}, {1, -1, -2, -3}
    };
    if (selected < 2 || selected > 6) return;
    const auto& g = gains[selected - 2];
    const char* ids[] = {"eqLow", "eqLowMid", "eqHighMid", "eqHigh"};
    for (int i = 0; i < 4; ++i) processor.setParameterValue(ids[i], g[i]);
}

void AureaPianoEditor::applyCompPreset(int selected)
{
    static constexpr float settings[][5] = {
        {0, 1, 15, 180, 0}, {-22, 1.8f, 25, 250, 2},
        {-24, 3, 8, 120, 4}, {-28, 4, 4, 180, 5}
    };
    if (selected < 2 || selected > 5) return;
    const auto& s = settings[selected - 2];
    const char* ids[] = {"compThreshold", "compRatio", "compAttack", "compRelease", "compMakeup"};
    for (int i = 0; i < 5; ++i) processor.setParameterValue(ids[i], s[i]);
}

void AureaPianoEditor::applyPianoPreset(int selected)
{
    static constexpr float settings[][5] = {
        {0.55f, 0.5f, 0.50f, 0.16f, 2},
        {0.37f, 0.35f, 0.65f, 0.23f, 4},
        {0.75f, 0.65f, 0.35f, 0.10f, 1},
        {0.43f, 0.38f, 0.72f, 0.32f, 6}
    };
    if (selected < 2 || selected > 5) return;
    const auto& s = settings[selected - 2];
    const char* ids[] = {"aBrightness", "aHardness", "aResonance", "reverbMix", "aAttack"};
    for (int i = 0; i < 5; ++i) processor.setParameterValue(ids[i], s[i]);
}

void AureaPianoEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(accent);
    g.fillRect(0, 0, getWidth(), 3);
    g.setColour(panel);
    g.fillRect(0, getHeight() - 145, getWidth(), 145);
}

void AureaPianoEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    title.setBounds(24, 12, 330, 40);
    subtitle.setBounds(26, 52, w - 52, 23);
    viewport.setBounds(18, 86, w - 36, h - 245);
    keyboardHelp.setBounds(26, h - 148, w - 240, 26);
    octaveDown.setBounds(w - 182, h - 145, 75, 27);
    octaveUp.setBounds(w - 101, h - 145, 75, 27);
    keyboard.setBounds(18, h - 112, w - 36, 96);
}

void AureaPianoEditor::visibilityChanged()
{
    if (isShowing()) keyboard.grabKeyboardFocus();
}
