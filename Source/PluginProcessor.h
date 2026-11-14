#pragma once
#include "Limiter.h"
#include <juce_audio_utils/juce_audio_utils.h>
class L2Processor : public juce::AudioProcessor
{
  public:
    L2Processor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "L2 Limiter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::UndoManager undo{30000};
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> inputPeak{0}, outputPeak{0}, attenuation{0};
    std::array<std::atomic<float>, 2> inputLevels{}, outputLevels{}, inputRms{}, outputRms{};

  private:
    Limiter limiter;
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
};
