#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Original DSP implementation. Not Waves' proprietary ARC or IDR algorithm.
class Limiter
{
  public:
    void prepare(double sr)
    {
        rate = sr;
        delay = std::max(1, int(std::round(sr * 0.0015)));
        ring.assign(size_t(delay + 1), {});
        reset();
    }
    void reset()
    {
        for (auto &f : ring)
            f = {};
        position = 0;
        gain = 1;
        energy = 0;
        error = {};
        rng = 0x19642024u;
        threshold = targetThreshold;
        ceiling = targetCeiling;
    }
    void set(double th, double ce, double ms, bool automatic, int bits, int dither, int shape)
    {
        targetThreshold = db(th);
        targetCeiling = db(ce);
        release = ms;
        arc = automatic;
        bitDepth = bits;
        ditherType = dither;
        shaping = shape;
    }
    std::array<double, 2> process(double left, double right)
    {
        // Linear parameter ramps lasting 10 ms; bounded step avoids parameter zippering.
        auto smooth = [this](double &v, double target)
        { v += std::clamp(target - v, -1.0 / (rate * .01), 1.0 / (rate * .01)); };
        smooth(threshold, targetThreshold);
        smooth(ceiling, targetCeiling);
        const double makeup = ceiling / std::max(threshold, 1.e-12);
        std::array<double, 2> in{std::isfinite(left) ? left * makeup : 0.,
                                 std::isfinite(right) ? right * makeup : 0.};
        ring[position] = in;
        double peak = 0;
        for (const auto &f : ring)
            peak = std::max({peak, std::abs(f[0]), std::abs(f[1])});
        energy += (std::max(in[0] * in[0], in[1] * in[1]) - energy) * (1 - std::exp(-1 / (rate * .03)));
        double ms = release;
        if (arc)
        {
            const double crest = peak / std::max(std::sqrt(energy), 1.e-9);
            ms = std::clamp(600. / std::max(crest, 1.), 20., 600.);
        }
        const double wanted = peak > ceiling ? ceiling / peak : 1.;
        gain = wanted < gain ? wanted
                             : wanted + (gain - wanted) * std::exp(-1 / (rate * std::max(ms, .01) * .001));
        position = (position + 1) % ring.size();
        auto out = ring[position];
        for (size_t ch = 0; ch < 2; ++ch)
        {
            double value = std::clamp(out[ch] * gain, -ceiling, ceiling);
            if (ditherType != 0)
            {
                const double step = std::ldexp(1., 1 - bitDepth);
                const double feedback = shaping == 0 ? 0. : shaping == 1 ? .25 : shaping == 2 ? .5 : .75;
                value += error[ch] * feedback;
                const double noise = ditherType == 1 ? (random() - random()) * step : (random() - .5) * step;
                const double quantized = std::round((value + noise) / step) * step;
                error[ch] = std::clamp(value - quantized, -step, step);
                value = quantized;
            }
            else
                error[ch] = 0;
            // Safety ceiling also after quantisation; sample-peak guarantee, not true peak.
            const double safetyCeiling = std::min(ceiling, targetCeiling);
            out[ch] = std::clamp(value, -safetyCeiling, safetyCeiling);
        }
        return out;
    }
    double reduction() const { return -20 * std::log10(std::max(gain, 1.e-12)); }
    int latency() const { return delay; }
    static double db(double v) { return std::pow(10., v / 20.); }

  private:
    double random()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return double(rng) / 4294967296.;
    }
    double rate = 48000, threshold = 1, targetThreshold = 1, ceiling = 1, targetCeiling = 1, release = 100,
           energy = 0, gain = 1;
    int delay = 72, bitDepth = 24, ditherType = 0, shaping = 0;
    bool arc = true;
    size_t position = 0;
    uint32_t rng = 0x19642024u;
    std::array<double, 2> error{};
    std::vector<std::array<double, 2>> ring;
};
