#include "PluginProcessor.h"
#include <iostream>
#include <limits>
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
    check(p.getNumPrograms() == int(factoryPresets.size()), "factory program count");
    for (size_t index = 0; index < factoryPresets.size(); ++index)
    {
        p.setCurrentProgram(int(index));
        const auto &factory = factoryPresets[index];
        auto value = [&](const char *id) { return p.state.getRawParameterValue(id)->load(); };
        check(std::abs(value("threshold") - factory.threshold) < .001f, "preset threshold");
        check(std::abs(value("ceiling") - factory.ceiling) < .001f, "preset ceiling");
        check(std::abs(value("release") - factory.release) < .01f, "preset release");
        check((value("arc") > .5f) == factory.arc, "preset ARC");
        check(int(value("bit_depth")) == factory.bits && int(value("dither_type")) == factory.dither &&
                  int(value("noise_shaping")) == factory.shaping,
              "preset IDR");
        juce::MemoryBlock recalled;
        p.getStateInformation(recalled);
        p.setCurrentProgram(0);
        p.setStateInformation(recalled.getData(), int(recalled.getSize()));
        check(p.getPresetName() == factory.name && p.getCurrentProgram() == int(index),
              "preset name/program recall");
        p.prepareToPlay(48000, 256);
        b.setSize(1, 256);
        for (int block = 0; block < 12; ++block)
        {
            for (int i = 0; i < 256; ++i)
                b.setSample(0, i, .9f * std::sin(float(i + block * 256) * .12f));
            p.processBlock(b, midi);
            for (int i = 0; i < 256; ++i)
                check(std::isfinite(b.getSample(0, i)) &&
                          std::abs(double(b.getSample(0, i))) <= Limiter::db(factory.ceiling),
                      "preset audio ceiling/finite");
        }
    }
    p.setCurrentProgram(1);
    p.state.copyState();
    p.undo.clearUndoHistory();
    p.setCurrentProgram(8);
    p.state.copyState();
    check(p.undo.undo(), "preset undo unavailable");
    check(std::abs(p.state.getRawParameterValue("threshold")->load() - factoryPresets[1].threshold) < .001f,
          "undo did not restore parameters");
    check(p.getCurrentProgram() == 1 && p.getPresetName() == factoryPresets[1].name,
          "undo did not restore preset identity");
    check(p.undo.redo() && p.getCurrentProgram() == 8, "preset redo failed");
    for (int size : {0, 1, 17, 64, 511, 1024})
    {
        b.setSize(1, size);
        b.clear();
        if (size > 0)
            b.setSample(0, 0, std::numeric_limits<float>::infinity());
        if (size > 1)
            b.setSample(0, 1, std::numeric_limits<float>::quiet_NaN());
        p.processBlock(b, midi);
        for (int i = 0; i < size; ++i)
            check(std::isfinite(b.getSample(0, i)), "nonfinite input escaped plugin");
        check(std::isfinite(p.inputRms[0].load()) && std::isfinite(p.outputRms[0].load()), "meter nonfinite");
    }
    if (argc > 1)
    {
        p.setCurrentProgram(argc > 2 && juce::String(argv[2]) == "master" ? 25 : 0);
        if (argc > 2 && juce::String(argv[2]) == "limits")
        {
            set("threshold", -30);
            set("ceiling", -30);
            set("release", .01f);
            set("arc", 0);
        }
        else if (argc <= 2)
        {
            set("threshold", 0);
            set("ceiling", 0);
        }
        p.prepareToPlay(48000, 512);
        b.setSize(1, 512);
        for (int block = 0; block < 30; ++block)
        {
            for (int i = 0; i < 512; ++i)
                b.setSample(0, i, .95f * std::sin(float(i + block * 512) * .12f));
            p.processBlock(b, midi);
        }
        std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
        // Let JUCE's normal startup splash and component animations finish before capturing.
        editor->createComponentSnapshot(editor->getLocalBounds());
        for (int frame = 0; frame < 310; ++frame)
        {
            for (int i = 0; i < 512; ++i)
                b.setSample(0, i, .95f * std::sin(float(i + frame * 512) * .12f));
            p.processBlock(b, midi);
            juce::Thread::sleep(16);
            juce::Timer::callPendingTimersSynchronously();
        }
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        juce::FileOutputStream stream(file);
        check(stream.openedOk(), "snapshot file");
        stream.setPosition(0);
        check(stream.truncate().wasOk(), "snapshot truncate");
        juce::PNGImageFormat png;
        check(png.writeImageToStream(image, stream), "snapshot render");
    }
    std::cout << "PASS: APVTS recall, all factory presets and audio ceilings, stereo audio, latency, "
                 "metering and mono processing\n";
}
