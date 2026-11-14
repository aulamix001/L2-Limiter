#pragma once
#include "FactoryPresets.h"
#include "Limiter.h"
#include <juce_audio_utils/juce_audio_utils.h>
class L2Processor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
  public:
    L2Processor();
    ~L2Processor() override;
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
    int getNumPrograms() override { return int(factoryPresets.size()); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    juce::String getPresetName() const;
    void setPresetName(const juce::String &name);
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    juce::UndoManager undo{30000};
    juce::AudioProcessorValueTreeState state;
    std::atomic<uint64_t> audioBlocks{0};
    std::atomic<float> inputPeak{0}, outputPeak{0}, attenuation{0};
    std::array<std::atomic<float>, 2> inputLevels{}, outputLevels{}, inputRms{}, outputRms{};

  private:
    void valueTreePropertyChanged(juce::ValueTree &, const juce::Identifier &) override;
    void valueTreeRedirected(juce::ValueTree &) override;
    void updateCurrentProgram();
    Limiter limiter;
    std::atomic<int> currentProgram{0};
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
};
