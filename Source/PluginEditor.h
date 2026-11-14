#pragma once
#include "PluginProcessor.h"
class ScalerL2LookAndFeel : public juce::LookAndFeel_V4
{
  public:
    ScalerL2LookAndFeel();
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          const juce::Slider::SliderStyle, juce::Slider &) override;
};
class L2Editor : public juce::AudioProcessorEditor, private juce::Timer
{
  public:
    explicit L2Editor(L2Processor &);
    ~L2Editor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent &) override;

  private:
    void timerCallback() override;
    void file(bool save);
    L2Processor &processor;
    ScalerL2LookAndFeel look;
    juce::Slider threshold, ceiling, release;
    juce::ToggleButton arc{"ARC"};
    juce::TextButton link{"Link"};
    juce::ComboBox bits, dither, shaping;
    juce::TextButton undo{"Undo"}, redo{"Redo"}, ab{"A > B"}, load{"Load"}, save{"Save"};
    juce::Label preset;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> arcAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree other;
    bool slotB = false;
    float input = 0, output = 0, reduction = 0;
    std::array<float, 4> holds{};
    std::array<int, 4> holdFrames{};
    bool clipped = false, linkUpdating = false;
    double lastThreshold = 0, lastCeiling = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(L2Editor)
};
