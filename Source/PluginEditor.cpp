#include "PluginEditor.h"
namespace
{
juce::Colour col(juce::uint32 x) { return juce::Colour(x); }
} // namespace
ScalerL2LookAndFeel::ScalerL2LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, col(0xff1a1b2e));
    setColour(juce::Slider::textBoxBackgroundColourId, col(0xff0f1020));
    setColour(juce::Slider::textBoxTextColourId, col(0xffe8ecf0));
    setColour(juce::Slider::textBoxOutlineColourId, col(0xff4eb8ff));
    setColour(juce::TextButton::buttonColourId, col(0xff252640));
    setColour(juce::TextButton::textColourOffId, col(0xffe8ecf0));
    setColour(juce::ComboBox::backgroundColourId, col(0xff0f1020));
    setColour(juce::ComboBox::textColourId, col(0xffe8ecf0));
    setColour(juce::ComboBox::outlineColourId, col(0xff4eb8ff));
    setColour(juce::PopupMenu::backgroundColourId, col(0xff252640));
    setColour(juce::PopupMenu::textColourId, col(0xffe8ecf0));
    setColour(juce::ToggleButton::textColourId, col(0xff38d9c0));
    setColour(juce::ToggleButton::tickColourId, col(0xff38d9c0));
}
void ScalerL2LookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float,
                                           float, const juce::Slider::SliderStyle, juce::Slider &)
{
    const float cx = float(x) + float(w) * .5f;
    g.setColour(col(0xff0f1020));
    g.fillRect(cx - 3, float(y), 6.f, float(h));
    g.setColour(col(0xff7a82a0));
    g.fillRect(cx - 1, float(y), 2.f, float(h));
    g.setColour(col(0xff2e3050));
    g.fillRoundedRectangle(cx - 15, pos - 11, 30, 22, 3);
    g.setColour(col(0xff4eb8ff));
    g.drawRoundedRectangle(cx - 15, pos - 11, 30, 22, 3, 1);
    g.setColour(col(0xffe8ecf0));
    g.drawLine(cx - 10, pos, cx + 10, pos, 1);
}
L2Editor::L2Editor(L2Processor &p) : AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&look);
    auto addSlider = [&](juce::Slider &s, const char *id)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 76, 24);
        s.onDragStart = [this] { processor.undo.beginNewTransaction(); };
        s.setNumDecimalPlacesToDisplay(2);
        s.setDoubleClickReturnValue(true, juce::String(id) == "release" ? 100. : 0.);
        sliders.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, s));
    };
    addSlider(threshold, "threshold");
    addSlider(ceiling, "ceiling");
    addSlider(release, "release");
    auto combo = [&](juce::ComboBox &c, const char *id, juce::StringArray names)
    {
        addAndMakeVisible(c);
        c.addItemList(names, 1);
        combos.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, id, c));
    };
    combo(bits, "bit_depth", {"24 Bits", "22 Bits", "20 Bits", "18 Bits", "16 Bits"});
    combo(dither, "dither_type", {"None", "Type1", "Type2"});
    combo(shaping, "noise_shaping", {"None", "Moderate", "Normal", "Ultra"});
    addAndMakeVisible(arc);
    arcAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state, "arc", arc);
    for (auto *b : {&undo, &redo, &ab, &load, &save, &link})
        addAndMakeVisible(b);
    undo.onClick = [this] { processor.undo.undo(); };
    redo.onClick = [this] { processor.undo.redo(); };
    link.setClickingTogglesState(true);
    lastThreshold = threshold.getValue();
    lastCeiling = ceiling.getValue();
    threshold.onValueChange = [this]
    {
        double now = threshold.getValue();
        if (link.getToggleState() && threshold.isMouseButtonDown() && !linkUpdating)
        {
            juce::ScopedValueSetter<bool> guard(linkUpdating, true);
            ceiling.setValue(ceiling.getValue() + now - lastThreshold, juce::sendNotificationSync);
        }
        lastThreshold = now;
    };
    ceiling.onValueChange = [this]
    {
        double now = ceiling.getValue();
        if (link.getToggleState() && ceiling.isMouseButtonDown() && !linkUpdating)
        {
            juce::ScopedValueSetter<bool> guard(linkUpdating, true);
            threshold.setValue(threshold.getValue() + now - lastCeiling, juce::sendNotificationSync);
        }
        lastCeiling = now;
    };
    other = p.state.copyState();
    ab.onClick = [this]
    {
        auto current = processor.state.copyState();
        processor.state.replaceState(other);
        other = current;
        slotB = !slotB;
        ab.setButtonText(slotB ? "B > A" : "A > B");
    };
    load.onClick = [this] { file(false); };
    save.onClick = [this] { file(true); };
    addAndMakeVisible(preset);
    preset.setText("Default", juce::dontSendNotification);
    preset.setJustificationType(juce::Justification::centred);
    preset.setColour(juce::Label::textColourId, col(0xffe8ecf0));
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(1.8);
    setResizeLimits(630, 350, 1440, 800);
    setSize(900, 500);
    startTimerHz(60);
}
L2Editor::~L2Editor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}
void L2Editor::resized()
{
    const float sx = getWidth() / 900.f, sy = getHeight() / 500.f;
    auto at = [&](juce::Component &c, int x, int y, int w, int h)
    {
        c.setBounds(juce::roundToInt(x * sx), juce::roundToInt(y * sy), juce::roundToInt(w * sx),
                    juce::roundToInt(h * sy));
    };
    at(undo, 16, 14, 62, 28);
    at(redo, 83, 14, 62, 28);
    at(preset, 152, 14, 340, 28);
    at(ab, 500, 14, 90, 28);
    at(load, 598, 14, 80, 28);
    at(save, 686, 14, 80, 28);
    at(threshold, 63, 132, 112, 295);
    at(ceiling, 279, 132, 112, 295);
    at(release, 456, 132, 100, 295);
    at(arc, 474, 441, 80, 28);
    at(link, 209, 285, 41, 28);
    at(bits, 744, 215, 130, 32);
    at(dither, 744, 314, 130, 32);
    at(shaping, 744, 413, 130, 32);
}
void L2Editor::paint(juce::Graphics &original)
{
    juce::Graphics::ScopedSaveState guard(original);
    original.addTransform(juce::AffineTransform::scale(getWidth() / 900.f, getHeight() / 500.f));
    auto &g = original;
    g.fillAll(col(0xff1a1b2e));
    g.setColour(col(0xff2a2d4a));
    g.drawRect(3, 3, 894, 494, 2);
    g.setColour(col(0xff252640));
    g.fillRoundedRectangle(12, 93, 876, 389, 4);
    auto label = [&](juce::String s, int x, int y, int w, int h, float font, juce::uint32 color = 0xffe8ecf0)
    {
        g.setColour(col(color));
        g.setFont(font);
        g.drawText(s, x, y, w, h, juce::Justification::centred);
    };
    label("L2 LIMITER", 477, 52, 397, 34, 27);
    label("AULAMIX", 778, 14, 101, 28, 15, 0xff4eb8ff);
    label("THRESHOLD", 27, 100, 183, 25, 17);
    label("OUT CEILING", 243, 100, 183, 25, 17);
    label("RELEASE", 451, 100, 117, 25, 17);
    label("ATTEN", 600, 100, 108, 25, 17);
    auto meter = [&](int x, float level, bool gr = false)
    {
        g.setColour(col(0xff0f1020));
        g.fillRect(x, 143, 19, 279);
        for (int i = 0; i < 60; ++i)
        {
            float db = -60.f + float(i);
            bool on = gr ? float(i) < level * 2 : db < juce::Decibels::gainToDecibels(level, -60.f);
            g.setColour(on ? col(gr          ? 0xff4eb8ff
                                 : db < -6   ? 0xff4caf7d
                                 : db < -3   ? 0xfff9c74f
                                 : db < -.3f ? 0xfff4845f
                                             : 0xffe63946)
                           : col(0xff252640));
            g.fillRect(x + 3, gr ? 146 + i * 4 : 416 - i * 4, 13, 3);
        }
    };
    meter(38, processor.inputLevels[0].load());
    meter(180, processor.inputLevels[1].load());
    meter(254, processor.outputLevels[0].load());
    meter(396, processor.outputLevels[1].load());
    meter(663, reduction, true);
    for (int i = 0; i <= 10; ++i)
    {
        label(juce::String(i * 3), 620, 135 + i * 27, 34, 20, 13, 0xff7a82a0);
    }
    label(juce::String(juce::Decibels::gainToDecibels(input, -90.f), 1), 63, 441, 112, 24, 16, 0xff4eb8ff);
    label(juce::String(juce::Decibels::gainToDecibels(output, -90.f), 1), 279, 441, 112, 24, 16, 0xff4eb8ff);
    label("-" + juce::String(reduction, 1), 637, 441, 66, 24, 16, 0xff4eb8ff);
    for (int k = 0; k < 4; ++k)
    {
        int x = k == 0 ? 38 : k == 1 ? 180 : k == 2 ? 254 : 396;
        float rms = k < 2 ? processor.inputRms[size_t(k)].load() : processor.outputRms[size_t(k - 2)].load();
        int y = juce::roundToInt(416.f - (juce::Decibels::gainToDecibels(rms, -60.f) + 60.f) * 4.f);
        g.setColour(col(0xffe8ecf0));
        g.fillRect(x + 2, juce::jlimit(143, 420, y), 15, 2);
        label(juce::String(juce::Decibels::gainToDecibels(holds[size_t(k)], -90.f), 1), x - 16, 423, 52, 17,
              11);
    }
    g.setColour(col(clipped ? 0xffe63946 : 0xff0f1020));
    g.fillEllipse(692, 143, 12, 12);
    label("IDR", 741, 130, 136, 50, 38, 0xff7b5feb);
    label("QUANTIZE", 740, 187, 138, 22, 16);
    label("DITHER", 740, 286, 138, 22, 16);
    label("SHAPING", 740, 385, 138, 22, 16);
}
void L2Editor::mouseDown(const juce::MouseEvent &)
{
    holds = {};
    holdFrames = {};
    clipped = false;
    repaint();
}
void L2Editor::timerCallback()
{
    for (size_t k = 0; k < 4; ++k)
    {
        float v = k < 2 ? processor.inputLevels[k].load() : processor.outputLevels[k - 2].load();
        if (v >= holds[k])
        {
            holds[k] = v;
            holdFrames[k] = 180;
        }
        else if (--holdFrames[k] <= 0)
        {
            holds[k] = v;
            holdFrames[k] = 180;
        }
    }
    if (processor.outputPeak.load() >=
        float(Limiter::db(processor.state.getRawParameterValue("ceiling")->load())) * .99999f)
        clipped = true;
    input = processor.inputPeak.load();
    output = processor.outputPeak.load();
    reduction = processor.attenuation.load();
    release.setEnabled(!arc.getToggleState());
    repaint();
}
void L2Editor::file(bool writing)
{
    chooser = std::make_unique<juce::FileChooser>(
        writing ? "Save preset" : "Load preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.l2p");
    auto safe = juce::Component::SafePointer<L2Editor>(this);
    chooser->launchAsync(
        juce::FileBrowserComponent::canSelectFiles |
            (writing ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                     : juce::FileBrowserComponent::openMode),
        [safe, writing](const juce::FileChooser &fc)
        {
            if (!safe)
                return;
            auto f = fc.getResult();
            if (f == juce::File{})
                return;
            bool ok = false;
            if (writing)
            {
                auto xml = safe->processor.state.copyState().createXml();
                ok = xml->writeTo(f.withFileExtension("l2p"));
            }
            else if (auto xml = juce::parseXML(f))
            {
                if (xml->hasTagName(safe->processor.state.state.getType()))
                {
                    safe->processor.state.replaceState(juce::ValueTree::fromXml(*xml));
                    ok = true;
                }
            }
            if (ok)
                safe->preset.setText(f.getFileNameWithoutExtension(), juce::dontSendNotification);
            else
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Preset",
                                                       "Unable to read or write this preset.");
        });
}
