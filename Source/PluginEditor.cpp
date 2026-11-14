#include "PluginEditor.h"
namespace
{
juce::Colour col(juce::uint32 x) { return juce::Colour(x); }
void well(juce::Graphics &g, juce::Rectangle<float> r, bool accent = true)
{
    g.setColour(col(0xff090b13));
    g.fillRoundedRectangle(r, 1.5f);
    g.setColour(col(0xff02030a));
    g.drawRoundedRectangle(r.expanded(.7f), 1.8f, 1.f);
    g.setColour(col(accent ? 0xff5293bd : 0xff48516d));
    g.drawRoundedRectangle(r, 1.5f, .6f);
    g.setColour(juce::Colours::white.withAlpha(.13f));
    g.drawLine(r.getX() + 1, r.getBottom() + .8f, r.getRight() - 1, r.getBottom() + .8f, .6f);
}
void text(juce::Graphics &g, const juce::String &s, juce::Rectangle<float> r, float size,
          juce::Colour c = col(0xffe3e7f2), float spacing = 0)
{
    auto font = juce::Font(size);
    font.setExtraKerningFactor(spacing);
    g.setFont(font);
    g.setColour(juce::Colours::black.withAlpha(.8f));
    g.drawText(s, r.translated(0, .7f), juce::Justification::centred);
    g.setColour(c);
    g.drawText(s, r, juce::Justification::centred);
}
} // namespace
double HardwareFader::valueToProportionOfLength(double value)
{
    return isRelease ? (std::log10(std::max(.01, value)) + 2.) / 5.
                     : juce::Slider::valueToProportionOfLength(value);
}
double HardwareFader::proportionOfLengthToValue(double proportion)
{
    return isRelease ? std::pow(10., juce::jlimit(0., 1., proportion) * 5. - 2.)
                     : juce::Slider::proportionOfLengthToValue(proportion);
}
void HardwareFader::mouseDoubleClick(const juce::MouseEvent &)
{
    addAndMakeVisible(entry);
    entry.setBounds(2, getHeight() / 2 - 10, getWidth() - 4, 20);
    entry.setColour(juce::TextEditor::backgroundColourId, col(0xff080d17));
    entry.setColour(juce::TextEditor::textColourId, col(0xffe8ecf0));
    entry.setColour(juce::TextEditor::outlineColourId, col(0xff4eb8ff));
    entry.setInputRestrictions(12, "0123456789.-");
    entry.setFont(juce::Font(10));
    entry.setText(juce::String(getValue(), 2));
    entry.selectAll();
    entry.grabKeyboardFocus();
    entry.onReturnKey = [this]
    {
        setValue(juce::jlimit(getMinimum(), getMaximum(), entry.getText().getDoubleValue()),
                 juce::sendNotificationSync);
        entry.setVisible(false);
    };
    entry.onEscapeKey = [this] { entry.setVisible(false); };
    entry.onFocusLost = [this] { entry.setVisible(false); };
}
ScalerL2LookAndFeel::ScalerL2LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, col(0xff181b30));
    setColour(juce::TextButton::textColourOffId, col(0xffe8ecf0));
    setColour(juce::ComboBox::textColourId, col(0xffe8ecf0));
    setColour(juce::PopupMenu::backgroundColourId, col(0xff151a2c));
    setColour(juce::PopupMenu::textColourId, col(0xffe8ecf0));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col(0xff2d577c));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::Label::textColourId, col(0xffe8ecf0));
}
juce::Slider::SliderLayout ScalerL2LookAndFeel::getSliderLayout(juce::Slider &s)
{
    juce::Slider::SliderLayout l;
    l.sliderBounds = s.getLocalBounds();
    return l;
}
void ScalerL2LookAndFeel::drawLinearSlider(juce::Graphics &g, int, int, int, int, float pos, float, float,
                                           const juce::Slider::SliderStyle, juce::Slider &slider)
{
    auto &s = static_cast<HardwareFader &>(slider);
    const float w = float(s.getWidth()), h = float(s.getHeight()), cx = w * .5f;
    if (s.isRelease)
    {
        g.setColour(col(0xff03040b));
        g.fillRect(cx - 2, 7.f, 4.f, h - 14);
        g.setColour(col(0xffb5bed0));
        g.fillRect(cx - .5f, 8.f, 1.f, h - 16);
        juce::Rectangle<float> r(cx - 10.5f, pos - 12, 21, 25);
        juce::DropShadow(juce::Colours::black.withAlpha(.9f), 5, {3, 3})
            .drawForRectangle(g, r.toNearestInt());
        g.setGradientFill(juce::ColourGradient(col(0xff556179), r.getX(), r.getY(), col(0xff131929),
                                               r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 1.4f);
        g.setColour(col(0xff68aad2));
        g.drawRoundedRectangle(r, 1.4f, .6f);
        g.setColour(juce::Colours::white.withAlpha(.38f));
        g.drawLine(r.getX() + 1, r.getY() + 1, r.getRight() - 1, r.getY() + 1, .7f);
        for (int y = 3; y < 23; y += 3)
        {
            g.setColour(juce::Colours::black.withAlpha(.55f));
            g.drawLine(r.getX() + 2, r.getY() + y, r.getRight() - 2, r.getY() + y, .6f);
            g.setColour(juce::Colours::white.withAlpha(.1f));
            g.drawLine(r.getX() + 2, r.getY() + y + 1, r.getRight() - 2, r.getY() + y + 1, .5f);
        }
        g.setColour(col(0xffafdafa));
        g.drawLine(r.getX() + 2, pos, r.getRight() - 2, pos, .8f);
    }
    else
    {
        for (int i = 0; i <= 10; ++i)
        {
            float y = 8 + (h - 16) * i / 10.f;
            g.setColour(col(0xff737d98));
            g.drawLine(1, y, 8, y, .6f);
            g.drawLine(w - 8, y, w - 1, y, .6f);
            g.setColour(juce::Colours::black.withAlpha(.5f));
            g.drawLine(1, y + 1, 8, y + 1, .6f);
            g.drawLine(w - 8, y + 1, w - 1, y + 1, .6f);
        }
        juce::Rectangle<float> r(8, pos - 7, w - 16, 14);
        juce::DropShadow(juce::Colours::black.withAlpha(.95f), 5, {2, 3})
            .drawForRectangle(g, r.expanded(2).toNearestInt());
        g.setGradientFill(
            juce::ColourGradient(col(0xff6485a1), 0, pos - 9, col(0xff0b1524), 0, pos + 9, false));
        g.fillRoundedRectangle(r.expanded(2, 1.5f), 1);
        well(g, r);
        juce::Path arrows;
        arrows.addTriangle(1, pos, 6, pos - 4, 6, pos + 4);
        arrows.addTriangle(w - 1, pos, w - 6, pos - 4, w - 6, pos + 4);
        g.setColour(col(0xff6ec6ff));
        g.fillPath(arrows);
        text(g, juce::String(s.getValue(), 1), r, 10);
    }
}
void ScalerL2LookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &,
                                               bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(.6f);
    const bool active = b.getToggleState();
    juce::DropShadow(juce::Colours::black.withAlpha(.9f), 2, {1, 2}).drawForRectangle(g, r.toNearestInt());
    const bool isArc = b.getButtonText() == "ARC";
    auto top = active ? (isArc ? col(0xff95ffdc) : col(0xff4c99ce)) : col(over ? 0xff52617c : 0xff3c4963);
    auto bottom = active ? (isArc ? col(0xff21bfae) : col(0xff254865)) : col(0xff0b1220);
    if (down)
        std::swap(top, bottom);
    g.setGradientFill(juce::ColourGradient(top, 0, 0, bottom, 0, r.getHeight(), false));
    g.fillRoundedRectangle(r, 1.5f);
    g.setColour(col(active ? 0xff85eddc : 0xff6f97bd));
    g.drawRoundedRectangle(r, 1.5f, .65f);
    g.setColour(juce::Colours::white.withAlpha(.3f));
    g.drawLine(1.5f, 1.5f, r.getRight() - 1, 1.5f, .5f);
}
void ScalerL2LookAndFeel::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool, bool)
{
    text(g, b.getButtonText(), b.getLocalBounds().toFloat(), b.getButtonText() == "ARC" ? 11.f : 9.2f,
         b.getToggleState() && b.getButtonText() == "ARC" ? col(0xff07262a) : col(0xffe3e8f5));
}
void ScalerL2LookAndFeel::drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int,
                                       juce::ComboBox &box)
{
    auto r = juce::Rectangle<float>(.5f, .5f, float(w) - 1, float(h) - 1);
    g.setGradientFill(juce::ColourGradient(col(0xff29374c), 0, 0, col(0xff070d17), 0, float(h), false));
    g.fillRoundedRectangle(r, 1.5f);
    g.setColour(col(0xff5f99c2));
    g.drawRoundedRectangle(r, 1.5f, .7f);
    g.setColour(juce::Colours::white.withAlpha(.2f));
    g.drawLine(1.5f, 1.5f, float(w) - 2, 1.5f, .5f);
    if (box.getComponentID() == "presets")
    {
        juce::Path arrow;
        arrow.addTriangle(float(w) - 9, 6, float(w) - 4, 6, float(w) - 6.5f, 9);
        g.setColour(col(0xff8aa2be));
        g.fillPath(arrow);
    }
}
juce::Font ScalerL2LookAndFeel::getComboBoxFont(juce::ComboBox &box)
{
    return juce::Font(box.getComponentID() == "presets" ? 9.2f : 10.f);
}
void ScalerL2LookAndFeel::positionComboBoxText(juce::ComboBox &box, juce::Label &label)
{
    label.setBounds(2, 0, box.getWidth() - (box.getComponentID() == "presets" ? 12 : 4), box.getHeight());
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centred);
}
L2Editor::L2Editor(L2Processor &p) : AudioProcessorEditor(p), processor(p)
{
    setLookAndFeel(&look);
    addAndMakeVisible(controls);
    controls.setInterceptsMouseClicks(false, true);
    auto slider = [&](HardwareFader &s, const char *id)
    {
        controls.addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setPopupMenuEnabled(true);
        s.onDragStart = [this] { processor.undo.beginNewTransaction(); };
        s.setTooltip(juce::String(id) == "release" ? "Release time in ms. Double-click to enter a value."
                                                   : "Drag to adjust. Double-click to enter a value.");
        sliders.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, id, s));
    };
    slider(threshold, "threshold");
    slider(ceiling, "ceiling");
    slider(release, "release");
    auto combo = [&](juce::ComboBox &c, const char *id, juce::StringArray options)
    {
        controls.addAndMakeVisible(c);
        c.addItemList(options, 1);
        combos.push_back(
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, id, c));
    };
    combo(bits, "bit_depth", {"24 Bits", "22 Bits", "20 Bits", "18 Bits", "16 Bits"});
    combo(dither, "dither_type", {"None", "Type1", "Type2"});
    combo(shaping, "noise_shaping", {"None", "Moderate", "Normal", "Ultra"});
    for (auto *b : {&undo, &ab, &load, &save, &help, &arc, &link})
        controls.addAndMakeVisible(b);
    arc.setClickingTogglesState(true);
    arcAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state, "arc", arc);
    arc.setTooltip("Automatic release control");
    link.setClickingTogglesState(true);
    link.setTooltip("Link Threshold and Out Ceiling while dragging");
    undo.setTooltip("Undo. Shift-click to redo. Ctrl/Cmd+Z and Ctrl/Cmd+Y are also available.");
    undo.onClick = [this]
    {
        if (juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown())
            processor.undo.redo();
        else
            processor.undo.undo();
    };
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
        ab.setButtonText(slotB ? "B->A" : "A->B");
        syncPreset();
    };
    load.onClick = [this] { file(false); };
    save.onClick = [this] { file(true); };
    help.onClick = []
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon, "L2 Limiter",
            "Drag the Threshold and Ceiling readouts to adjust gain. Double-click for numeric entry. Link "
            "moves both controls together. ARC selects automatic release; turn it off for manual Release. "
            "Click a meter to reset its peak hold. Presets are starting points: adjust Threshold for your "
            "source level.");
    };
    controls.addAndMakeVisible(preset);
    preset.setComponentID("presets");
    for (size_t i = 0; i < factoryPresets.size(); ++i)
        preset.addItem(factoryPresets[i].name, int(i) + 1);
    preset.onChange = [this]
    {
        if (preset.getSelectedId() > 0)
        {
            processor.setCurrentProgram(preset.getSelectedId() - 1);
            syncPreset();
        }
    };
    preset.setTooltip("Factory presets for vocals, bass, acoustic instruments, piano, drums and mastering");
    syncPreset();
    controls.addAndMakeVisible(releaseValue);
    releaseValue.setEditable(false, true);
    releaseValue.setJustificationType(juce::Justification::centred);
    releaseValue.setFont(juce::Font(10));
    releaseValue.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    releaseValue.onTextChange = [this]
    {
        release.setValue(juce::jlimit(.01, 1000., releaseValue.getText().getDoubleValue()),
                         juce::sendNotificationSync);
    };
    createTexture();
    setWantsKeyboardFocus(true);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(528. / 294.);
    setResizeLimits(528, 294, 1584, 882);
    setSize(1056, 588);
    timerCallback();
    startTimerHz(60);
}
L2Editor::~L2Editor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}
void L2Editor::resized()
{
    controls.setBounds(0, 0, 528, 294);
    controls.setTransform(juce::AffineTransform::scale(getWidth() / 528.f, getHeight() / 294.f));
    undo.setBounds(73, 8, 42, 18);
    preset.setBounds(122, 8, 180, 18);
    ab.setBounds(307, 8, 44, 18);
    load.setBounds(357, 8, 45, 18);
    save.setBounds(407, 8, 45, 18);
    help.setBounds(457, 8, 18, 18);
    threshold.setBounds(39, 76, 51, 155);
    ceiling.setBounds(173, 76, 50, 155);
    release.setBounds(269, 99, 30, 129);
    releaseValue.setBounds(274, 77, 45, 15);
    link.setBounds(121, 146, 22, 17);
    arc.setBounds(279, 237, 38, 19);
    bits.setBounds(438, 127, 68, 19);
    dither.setBounds(438, 178, 68, 19);
    shaping.setBounds(438, 229, 68, 19);
}
void L2Editor::createTexture()
{
    texture = juce::Image(juce::Image::RGB, 1056, 588, false);
    juce::Random random(0x512ab);
    juce::Image::BitmapData data(texture, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < texture.getHeight(); ++y)
    {
        float brush = random.nextFloat() * 1.6f;
        for (int x = 0; x < texture.getWidth(); ++x)
        {
            float n = (random.nextFloat() - .5f) * 4.f + brush;
            float light = 3.5f * std::exp(-std::pow((x - 390.f) / 420.f, 2));
            data.setPixelColour(x, y,
                                juce::Colour(juce::uint8(juce::jlimit(0, 255, int(24 + n + light))),
                                             juce::uint8(juce::jlimit(0, 255, int(28 + n + light))),
                                             juce::uint8(juce::jlimit(0, 255, int(48 + n + light)))));
        }
    }
}
void L2Editor::paint(juce::Graphics &original)
{
    juce::Graphics::ScopedSaveState saveState(original);
    original.addTransform(juce::AffineTransform::scale(getWidth() / 528.f, getHeight() / 294.f));
    auto &g = original;
    g.drawImage(texture, juce::Rectangle<float>(0, 0, 528, 294));
    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, 264, 70,
                                           juce::Colours::black.withAlpha(.35f), 264, 294, false));
    g.fillAll();
    g.setColour(col(0xff050711));
    g.drawRoundedRectangle({.8f, .8f, 526.4f, 292.4f}, 4, 2);
    g.setColour(col(0xff61728b));
    g.drawLine(5, 2, 523, 2, .8f);
    g.drawLine(2, 5, 2, 288, .6f);
    g.setColour(col(0xff3b425b));
    g.drawLine(6, 51, 522, 51, .6f);
    well(g, {10, 8, 58, 18});
    text(g, "AULAMIX", {12, 8, 54, 18}, 9, col(0xffe8ecf0), .04f);
    text(g, "L2 ULTRAMAXIMIZER", {251, 29, 209, 18}, 13.5f, col(0xffc7cedd), .08f);
    juce::Path emblem;
    emblem.startNewSubPath(487, 10);
    emblem.lineTo(492, 39);
    emblem.lineTo(497, 10);
    emblem.lineTo(502, 39);
    emblem.lineTo(507, 10);
    emblem.lineTo(512, 39);
    emblem.lineTo(517, 10);
    g.setColour(col(0xff5e637f));
    g.strokePath(emblem, juce::PathStrokeType(1.6f));
    auto label = [&](juce::String s, float x, float y, float w, float h, float size = 10.f)
    { text(g, s, {x, y, w, h}, size, col(0xffd8deed), .13f); };
    label("THRESHOLD", 25, 54, 84, 14);
    label("OUT CEILING", 155, 54, 85, 14);
    label("RELEASE", 268, 54, 67, 14);
    label("ATTEN", 370, 54, 42, 14);
    auto meter = [&](float x, float level, bool attenuation = false, size_t channel = 0)
    {
        juce::Rectangle<float> r(x, 78, 15, 153);
        well(g, r, false);
        auto inside = r.reduced(2);
        g.setColour(col(0xff090b11));
        g.fillRect(inside);
        float fraction =
            attenuation ? juce::jlimit(0.f, 1.f, level / 30.f)
                        : juce::jlimit(0.f, 1.f, (juce::Decibels::gainToDecibels(level, -60.f) + 60) / 60);
        auto fill = attenuation ? inside.withHeight(inside.getHeight() * fraction)
                                : inside.removeFromBottom(inside.getHeight() * fraction);
        if (attenuation)
        {
            g.setGradientFill(juce::ColourGradient(col(0xff68d5ff), fill.getX(), fill.getY(), col(0xff2592d3),
                                                   fill.getRight(), fill.getY(), false));
        }
        else
        {
            juce::ColourGradient gradient(col(0xfff03c52), x, 79, col(0xff53c879), x, 230, false);
            gradient.addColour(.35, col(0xffffb237));
            gradient.addColour(.68, col(0xffe7e14f));
            g.setGradientFill(gradient);
        }
        g.fillRect(fill);
        g.setColour(juce::Colours::white.withAlpha(.12f));
        g.fillRect(x + 2, 80.f, 1.5f, 149.f);
        if (!attenuation)
        {
            float rms = displayRms[channel];
            float y =
                229 - 149 * juce::jlimit(0.f, 1.f, (juce::Decibels::gainToDecibels(rms, -60.f) + 60) / 60);
            if (rms > 0)
            {
                g.setColour(juce::Colours::white.withAlpha(.85f));
                g.drawLine(x + 2, y, x + 13, y, .6f);
            }
        }
    };
    meter(24, displayPeaks[0], false, 0);
    meter(91, displayPeaks[1], false, 1);
    meter(158, displayPeaks[2], false, 2);
    meter(225, displayPeaks[3], false, 3);
    meter(384, reduction, true);
    for (int k = 0; k < 4; ++k)
    {
        float x = k == 0 ? 23 : k == 1 ? 89 : k == 2 ? 156 : 223;
        well(g, {x, 240, 32, 15}, false);
        text(g, juce::String(juce::Decibels::gainToDecibels(holds[size_t(k)], -90.f), 1), {x, 240, 32, 15},
             9.5f, col(0xffd6dceb));
    }
    for (int i = 0; i <= 10; ++i)
    {
        float y = 78 + 15.1f * i;
        text(g, juce::String(i * 3), {358, y - 6, 20, 12}, 9, col(0xffb1b8cc));
        g.setColour(col(0xff8792a8));
        g.drawLine(378, y, 382, y, .5f);
    }
    well(g, {380, 240, 24, 15}, false);
    text(g, "-" + juce::String(reduction, 1), {379, 240, 27, 15}, 9, col(0xff65cbf6));
    const std::array<const char *, 6> ticks{"1000", "100", "10.0", "1.0", "0.1", "0.01"};
    for (int i = 0; i < 6; ++i)
    {
        float y = 107 + i * 22.2f;
        g.setColour(col(0xff8b95ab));
        g.drawLine(299, y, 305, y, .6f);
        text(g, ticks[size_t(i)], {305, y - 6, 25, 12}, 9, col(0xffc0c6d7));
    }
    well(g, {274, 77, 45, 15});
    juce::Rectangle<float> idr(437, 71, 69, 27);
    juce::DropShadow(juce::Colours::black.withAlpha(.8f), 3, {1, 2}).drawForRectangle(g, idr.toNearestInt());
    g.setColour(col(0xff080811));
    g.fillRoundedRectangle(idr, 14);
    g.setColour(col(0xff656780));
    g.drawRoundedRectangle(idr, 14, .8f);
    text(g, "IDR", idr, 21, col(0xff9972f9), .06f);
    label("QUANTIZE", 435, 104, 73, 15, 9.3f);
    label("DITHER", 436, 155, 72, 15, 9.3f);
    label("SHAPING", 436, 206, 72, 15, 9.3f);
    g.setColour(col(clipped ? 0xffe63946 : 0xff151a2a));
    g.fillEllipse(404, 79, 4, 4);
}
void L2Editor::syncPreset()
{
    const auto name = processor.getPresetName();
    int found = 0;
    for (size_t i = 0; i < factoryPresets.size(); ++i)
        if (name == factoryPresets[i].name)
            found = int(i) + 1;
    bool modified = false;
    if (found > 0)
    {
        const auto &original = factoryPresets[size_t(found - 1)];
        auto differs = [this](const char *id, float expected)
        { return std::abs(processor.state.getRawParameterValue(id)->load() - expected) > .011f; };
        modified = differs("threshold", original.threshold) || differs("ceiling", original.ceiling) ||
                   differs("release", original.release) || differs("arc", original.arc ? 1.f : 0.f) ||
                   differs("bit_depth", float(original.bits)) ||
                   differs("dither_type", float(original.dither)) ||
                   differs("noise_shaping", float(original.shaping));
    }
    if (found > 0 && modified)
        preset.setText(name + " *", juce::dontSendNotification);
    else if (found > 0)
        preset.setSelectedId(found, juce::dontSendNotification);
    else
        preset.setText(name, juce::dontSendNotification);
}
void L2Editor::mouseDown(const juce::MouseEvent &)
{
    holds = {};
    holdFrames = {};
    clipped = false;
    repaint();
}
bool L2Editor::keyPressed(const juce::KeyPress &key)
{
    if (key.getModifiers().isCommandDown() && (key.getKeyCode() == 'Z' || key.getKeyCode() == 'Y'))
    {
        if (key.getKeyCode() == 'Y' || key.getModifiers().isShiftDown())
            processor.undo.redo();
        else
            processor.undo.undo();
        return true;
    }
    return false;
}
void L2Editor::timerCallback()
{
    const auto block = processor.audioBlocks.load(std::memory_order_acquire);
    const bool fresh = block != lastAudioBlock;
    lastAudioBlock = block;
    for (size_t k = 0; k < 4; ++k)
    {
        displayPeaks[k] =
            fresh ? (k < 2 ? processor.inputLevels[k].load() : processor.outputLevels[k - 2].load())
                  : displayPeaks[k] * .92f;
        displayRms[k] = fresh ? (k < 2 ? processor.inputRms[k].load() : processor.outputRms[k - 2].load())
                              : displayRms[k] * .92f;
        float v = displayPeaks[k];
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
    const double quantum =
        std::ldexp(1., 1 - (24 - 2 * int(processor.state.getRawParameterValue("bit_depth")->load())));
    const double limit =
        std::floor(Limiter::db(processor.state.getRawParameterValue("ceiling")->load()) / quantum) * quantum;
    if (fresh && processor.outputPeak.load() >= std::nextafter(float(limit), 0.f))
        clipped = true;
    reduction = fresh ? processor.attenuation.load() : reduction * .92f;
    release.setEnabled(!arc.getToggleState());
    if (!releaseValue.isBeingEdited())
        releaseValue.setText(juce::String(release.getValue(), 2), juce::dontSendNotification);
    syncPreset();
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
                auto saved = safe->processor.state.copyState();
                saved.setProperty("presetName", f.getFileNameWithoutExtension(), nullptr);
                auto xml = saved.createXml();
                ok = xml->writeTo(f.withFileExtension("l2p"));
                if (ok)
                    safe->processor.setPresetName(f.getFileNameWithoutExtension());
            }
            else if (auto xml = juce::parseXML(f))
            {
                if (xml->hasTagName(safe->processor.state.state.getType()))
                {
                    safe->processor.state.replaceState(juce::ValueTree::fromXml(*xml));
                    safe->processor.setPresetName(f.getFileNameWithoutExtension());
                    ok = true;
                }
            }
            if (ok)
                safe->syncPreset();
            else
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Preset",
                                                       "Unable to read or write this preset.");
        });
}
