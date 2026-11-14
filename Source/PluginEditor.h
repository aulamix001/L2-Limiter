#pragma once
#include "PluginProcessor.h"
class HardwareFader : public juce::Slider
{
  public:
    explicit HardwareFader(bool release = false) : isRelease(release) {}
    double valueToProportionOfLength(double) override;
    double proportionOfLengthToValue(double) override;
    void mouseDoubleClick(const juce::MouseEvent &) override;
    bool isRelease;

  private:
    juce::TextEditor entry;
};
class ScalerL2LookAndFeel : public juce::LookAndFeel_V4
{
  public:
    ScalerL2LookAndFeel();
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          const juce::Slider::SliderStyle, juce::Slider &) override;
    juce::Slider::SliderLayout getSliderLayout(juce::Slider &) override;
    int getSliderThumbRadius(juce::Slider &) override { return 8; }
    void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool, bool) override;
    void drawButtonText(juce::Graphics &, juce::TextButton &, bool, bool) override;
    void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int, juce::ComboBox &) override;
    juce::Font getComboBoxFont(juce::ComboBox &) override;
    void positionComboBoxText(juce::ComboBox &, juce::Label &) override;
};
class L2Editor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit L2Editor(L2Processor &);
    ~L2Editor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;
    bool keyPressed(const juce::KeyPress &) override;

  private:
    void timerCallback() override;
    void file(bool save);
    void createTexture();
    void syncPreset();
    L2Processor &processor;
    ScalerL2LookAndFeel look;
    juce::Component controls;
    HardwareFader threshold, ceiling, release{true};
    juce::TextButton arc{"ARC"}, link{"<>"};
    juce::ComboBox bits, dither, shaping, preset;
    juce::TextButton undo{"Undo"}, ab{"A->B"}, load{"Load"}, save{"Save"}, help{"?"};
    juce::Label releaseValue;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arcAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree other;
    juce::Image texture;
    juce::TooltipWindow tooltips{this, 650};
    bool slotB = false, clipped = false, linkUpdating = false;
    float reduction = 0;
    std::array<float, 4> holds{}, displayPeaks{}, displayRms{};
    uint64_t lastAudioBlock = 0;
    std::array<int, 4> holdFrames{};
    double lastThreshold = 0, lastCeiling = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(L2Editor)
};
