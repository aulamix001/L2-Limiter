#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout L2Processor::parameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto f = [&](const char *id, const char *name, float lo, float hi, float def, float skew = 1.f)
    {
        p.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{lo, hi, .01f, skew}, def,
            juce::AudioParameterFloatAttributes().withLabel(juce::String(id) == "release" ? "ms" : "dB")));
    };
    f("threshold", "Threshold", -30, 0, 0);
    f("ceiling", "Out Ceiling", -30, 0, -.3f);
    f("release", "Release", .01f, 1000, 100, .3f);
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"arc", 1}, "ARC", true));
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"bit_depth", 1}, "Quantize",
        juce::StringArray{"24 Bits", "22 Bits", "20 Bits", "18 Bits", "16 Bits"}, 0,
        juce::AudioParameterChoiceAttributes().withAutomatable(false)));
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"dither_type", 1}, "Dither", juce::StringArray{"None", "Type1", "Type2"}, 0,
        juce::AudioParameterChoiceAttributes().withAutomatable(false)));
    p.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"noise_shaping", 1}, "Shaping",
        juce::StringArray{"None", "Moderate", "Normal", "Ultra"}, 0,
        juce::AudioParameterChoiceAttributes().withAutomatable(false)));
    return p;
}
L2Processor::L2Processor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, &undo, "L2State", parameters())
{
    state.state.setProperty("presetName", "Default", nullptr);
    state.state.addListener(this);
}
L2Processor::~L2Processor() { state.state.removeListener(this); }
void L2Processor::updateCurrentProgram()
{
    currentProgram =
        juce::jlimit(0, int(factoryPresets.size()) - 1, int(state.state.getProperty("programIndex", 0)));
}
void L2Processor::valueTreePropertyChanged(juce::ValueTree &tree, const juce::Identifier &property)
{
    if (tree == state.state && property == juce::Identifier("programIndex"))
        updateCurrentProgram();
}
void L2Processor::valueTreeRedirected(juce::ValueTree &) { updateCurrentProgram(); }
bool L2Processor::isBusesLayoutSupported(const BusesLayout &l) const
{
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet() &&
           (l.getMainOutputChannelSet() == juce::AudioChannelSet::mono() ||
            l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}
void L2Processor::prepareToPlay(double sr, int)
{
    limiter.set(*state.getRawParameterValue("threshold"), *state.getRawParameterValue("ceiling"),
                *state.getRawParameterValue("release"), *state.getRawParameterValue("arc") > .5f,
                24 - 2 * int(state.getRawParameterValue("bit_depth")->load()),
                int(state.getRawParameterValue("dither_type")->load()),
                int(state.getRawParameterValue("noise_shaping")->load()));
    limiter.prepare(sr);
    setLatencySamples(limiter.latency());
    inputPeak = 0;
    outputPeak = 0;
    attenuation = 0;
    for (size_t ch = 0; ch < 2; ++ch)
    {
        inputLevels[ch] = 0;
        outputLevels[ch] = 0;
        inputRms[ch] = 0;
        outputRms[ch] = 0;
    }
}
void L2Processor::processBlock(juce::AudioBuffer<float> &b, juce::MidiBuffer &)
{
    juce::ScopedNoDenormals guard;
    auto v = [this](const char *id) { return state.getRawParameterValue(id)->load(); };
    limiter.set(v("threshold"), v("ceiling"), v("release"), v("arc") > .5f, 24 - 2 * int(v("bit_depth")),
                int(v("dither_type")), int(v("noise_shaping")));
    std::array<double, 2> inSquares{}, outSquares{};
    std::array<float, 2> inPeaks{}, outPeaks{};
    const double outputLimit = Limiter::db(v("ceiling"));
    auto toFloat = [outputLimit](double value)
    {
        float result = float(value);
        if (std::abs(double(result)) > outputLimit)
            result = std::nextafter(result, 0.f);
        return result;
    };
    float ip = 0, op = 0, gr = 0;
    const int n = b.getNumChannels();
    if (n == 0)
        return;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        double l = b.getSample(0, i), r = n > 1 ? b.getSample(1, i) : l;
        if (!std::isfinite(l))
            l = 0;
        if (!std::isfinite(r))
            r = 0;
        ip = std::max(ip, float(std::max(std::abs(l), std::abs(r))));
        auto out = limiter.process(l, r);
        for (size_t ch = 0; ch < 2; ++ch)
        {
            double sample = ch == 0 ? l : r;
            inSquares[ch] += sample * sample;
            outSquares[ch] += out[ch] * out[ch];
            inPeaks[ch] = std::max(inPeaks[ch], float(std::abs(sample)));
            outPeaks[ch] = std::max(outPeaks[ch], float(std::abs(out[ch])));
        }
        b.setSample(0, i, toFloat(out[0]));
        if (n > 1)
            b.setSample(1, i, toFloat(out[1]));
        op = std::max(op, float(std::max(std::abs(out[0]), std::abs(out[1]))));
        gr = std::max(gr, float(limiter.reduction()));
    }
    const float meterDecay = float(std::exp(-b.getNumSamples() / (getSampleRate() * .3)));
    for (size_t ch = 0; ch < 2; ++ch)
    {
        inputLevels[ch] = std::max(inPeaks[ch], inputLevels[ch].load() * meterDecay);
        outputLevels[ch] = std::max(outPeaks[ch], outputLevels[ch].load() * meterDecay);
        inputRms[ch] = float(std::sqrt(inSquares[ch] / std::max(1, b.getNumSamples())));
        outputRms[ch] = float(std::sqrt(outSquares[ch] / std::max(1, b.getNumSamples())));
    }
    inputPeak = std::max(ip, inputPeak.load() * meterDecay);
    outputPeak = std::max(op, outputPeak.load() * meterDecay);
    attenuation = gr;
    audioBlocks.fetch_add(1, std::memory_order_release);
}
const juce::String L2Processor::getProgramName(int index)
{
    return index >= 0 && index < int(factoryPresets.size()) ? factoryPresets[size_t(index)].name
                                                            : juce::String{};
}
juce::String L2Processor::getPresetName() const
{
    return state.state.getProperty("presetName", "Custom").toString();
}
void L2Processor::setPresetName(const juce::String &name)
{
    state.state.setProperty("presetName", name, &undo);
}
void L2Processor::setCurrentProgram(int index)
{
    if (index < 0 || index >= int(factoryPresets.size()))
        return;
    const auto &preset = factoryPresets[size_t(index)];
    undo.beginNewTransaction("Load " + juce::String(preset.name));
    auto set = [this](const char *id, float value)
    {
        auto *parameter = state.getParameter(id);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    };
    set("threshold", preset.threshold);
    set("ceiling", preset.ceiling);
    set("release", preset.release);
    set("arc", preset.arc ? 1.f : 0.f);
    set("bit_depth", float(preset.bits));
    set("dither_type", float(preset.dither));
    set("noise_shaping", float(preset.shaping));
    currentProgram = index;
    state.state.setProperty("programIndex", index, &undo);
    setPresetName(preset.name);
}
void L2Processor::getStateInformation(juce::MemoryBlock &dest)
{
    auto xml = state.copyState().createXml();
    copyXmlToBinary(*xml, dest);
}
void L2Processor::setStateInformation(const void *data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType()))
        {
            state.replaceState(juce::ValueTree::fromXml(*xml));
            currentProgram = juce::jlimit(0, int(factoryPresets.size()) - 1,
                                          int(state.state.getProperty("programIndex", 0)));
        }
}
juce::AudioProcessorEditor *L2Processor::createEditor() { return new L2Editor(*this); }
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new L2Processor; }
