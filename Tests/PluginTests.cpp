#include "PluginProcessor.h"
#include <iostream>
#include <stdexcept>
void check(bool b, const char *s)
{
    if (!b)
        throw std::runtime_error(s);
}
int main(int argc, char **argv)
{
    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> gui;
    if (argc > 1)
        gui = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    L2Processor p;
    auto set = [&](const char *id, float value)
    {
        auto *param = p.state.getParameter(id);
        check(param != nullptr, "missing parameter");
        param->setValueNotifyingHost(param->convertTo0to1(value));
    };
    check(p.state.getParameter("threshold")->isAutomatable(), "threshold automation disabled");
    check(!p.state.getParameter("dither_type")->isAutomatable(), "dither must not be automated");
    set("threshold", -12);
    set("ceiling", -1);
    set("release", 150);
    set("arc", 0);
    set("bit_depth", 4);
    set("dither_type", 1);
    set("noise_shaping", 2);
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    set("threshold", 0);
    p.setStateInformation(saved.getData(), int(saved.getSize()));
    check(std::abs(p.state.getRawParameterValue("threshold")->load() + 12) < .001f, "threshold not restored");
    check(p.state.getRawParameterValue("bit_depth")->load() == 4, "bits not restored");
    check(p.state.getRawParameterValue("noise_shaping")->load() == 2, "shaping not restored");
    p.prepareToPlay(48000, 512);
    check(p.getLatencySamples() == 72, "latency not reported");
    juce::AudioBuffer<float> b(2, 512);
    juce::MidiBuffer midi;
    float peak = 0;
    for (int block = 0; block < 100; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            float x = .8f * std::sin(float(block * 512 + i) * .2f);
            b.setSample(0, i, x);
            b.setSample(1, i, -x);
        }
        p.processBlock(b, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                float x = b.getSample(ch, i);
                check(std::isfinite(x), "nonfinite audio");
                check(std::abs(x) <= float(Limiter::db(-1)) + 1.e-6f, "ceiling exceeded");
                peak = std::max(peak, std::abs(x));
            }
    }
    check(peak > .1f, "silent plugin");
    check(p.attenuation.load() > 0, "missing meter data");
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add(juce::AudioChannelSet::mono());
    mono.outputBuses.add(juce::AudioChannelSet::mono());
    check(p.setBusesLayout(mono), "mono layout unsupported");
    p.prepareToPlay(44100, 256);
    b.setSize(1, 256);
    b.clear();
    p.processBlock(b, midi);
    if (argc > 1)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        juce::FileOutputStream stream(file);
        check(stream.openedOk(), "snapshot file");
        stream.setPosition(0);
        check(stream.truncate().wasOk(), "snapshot truncate");
        juce::PNGImageFormat png;
        check(png.writeImageToStream(image, stream), "snapshot render");
    }
    std::cout << "PASS: APVTS preset recall, stereo audio, ceiling, latency, metering and mono processing\n";
}
