#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Original sample-peak limiter. ARC and shaping are not Waves' proprietary algorithms.
class Limiter
{
  public:
    void prepare(double sr)
    {
        rate = std::max(8000., sr);
        delay = std::max(1, int(std::round(rate * .0015)));
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
        adaptiveRelease = 100;
        errors = {};
        rng = 0x19642024u;
        threshold.reset();
        ceiling.reset();
        release.reset();
    }
    void set(double th, double ce, double ms, bool automatic, int bits, int dither, int shape)
    {
        threshold.set(db(std::clamp(th, -30., 0.)), int(rate * .01));
        ceiling.set(db(std::clamp(ce, -30., 0.)), int(rate * .01));
        release.set(std::clamp(ms, .01, 1000.), int(rate * .005));
        bits = std::clamp(bits, 16, 24);
        dither = std::clamp(dither, 0, 2);
        shape = std::clamp(shape, 0, 3);
        if (bitDepth != bits || ditherType != dither || shaping != shape)
            errors = {};
        arc = automatic;
        bitDepth = bits;
        ditherType = dither;
        shaping = shape;
    }
    std::array<double, 2> process(double left, double right)
    {
        if (ring.empty())
            return {};
        const double currentCeiling = ceiling.next();
        const double makeup = currentCeiling / std::max(threshold.next(), 1.e-12);
        auto &frame = ring[position];
        frame.audio = {std::isfinite(left) ? left * makeup : 0., std::isfinite(right) ? right * makeup : 0.};
        const double inputPeak = std::max(std::abs(frame.audio[0]), std::abs(frame.audio[1]));
        frame.inversePeak = inputPeak > 1.e-30 ? 1. / inputPeak : 1.e30;
        energy += (inputPeak * inputPeak - energy) * (1 - std::exp(-1 / (rate * .03)));
        position = (position + 1) % ring.size();
        auto out = ring[position].audio;

        // Each future peak imposes a linear attack ending at its playback deadline.
        // This avoids stepping gain when a transient first enters the lookahead buffer.
        double wanted = 1.;
        double windowPeak = inputPeak;
        for (size_t distance = 0, index = position; distance < ring.size(); ++distance)
        {
            const auto &upcoming = ring[index];
            const double allowed = std::min(1., currentCeiling * upcoming.inversePeak);
            wanted = std::min(wanted, allowed + (1. - allowed) * double(distance) / double(delay));
            windowPeak = std::max(windowPeak, 1. / std::max(upcoming.inversePeak, 1.e-30));
            index = (index + 1) % ring.size();
        }
        double ms = release.next();
        if (arc)
        {
            const double crest = windowPeak / std::max(std::sqrt(energy), 1.e-12);
            const double transient = std::clamp((crest - 1.4) / 4.6, 0., 1.);
            const double programRelease = 600. - transient * 580.;
            adaptiveRelease += (programRelease - adaptiveRelease) * (1 - std::exp(-1 / (rate * .02)));
            ms = adaptiveRelease;
        }
        gain = wanted < gain ? wanted : wanted + (gain - wanted) * std::exp(-1 / (rate * ms * .001));
        const double limit = std::min(currentCeiling, ceiling.target);
        const double step = std::ldexp(1., 1 - bitDepth);
        const double quantizedLimit = std::floor(limit / step) * step;
        for (size_t ch = 0; ch < 2; ++ch)
        {
            const double dry = std::clamp(out[ch] * gain, -limit, limit);
            const auto &e = errors[ch];
            // Own 1st/2nd/3rd-order high-pass error feedback, not the original IDR curves.
            const double feedback = shaping == 0   ? 0.
                                    : shaping == 1 ? e[0]
                                    : shaping == 2 ? 2 * e[0] - e[1]
                                                   : 3 * e[0] - 3 * e[1] + e[2];
            const double value = dry + feedback;
            const double noise = ditherType == 0   ? 0.
                                 : ditherType == 1 ? (random() - random()) * step
                                                   : (random() - .5) * step;
            const double quantized = std::round((value + noise) / step) * step;
            const double result = std::clamp(quantized, -quantizedLimit, quantizedLimit);
            errors[ch][2] = e[1];
            errors[ch][1] = e[0];
            errors[ch][0] = result != quantized ? 0. : std::clamp(value - quantized, -1.5 * step, 1.5 * step);
            out[ch] = result;
        }
        return out;
    }
    double reduction() const { return -20 * std::log10(std::max(gain, 1.e-12)); }
    int latency() const { return delay; }
    static double db(double v) { return std::pow(10., v / 20.); }

  private:
    struct Ramp
    {
        double current = 1, target = 1, increment = 0;
        int remaining = 0;
        void set(double next, int samples)
        {
            if (next == target)
                return;
            target = next;
            remaining = std::max(1, samples);
            increment = (target - current) / remaining;
        }
        double next()
        {
            if (remaining > 0 && --remaining > 0)
                current += increment;
            else
                current = target;
            return current;
        }
        void reset()
        {
            current = target;
            remaining = 0;
            increment = 0;
        }
    };
    struct Frame
    {
        std::array<double, 2> audio{};
        double inversePeak = 1.e30;
    };
    double random()
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return double(rng) / 4294967296.;
    }
    double rate = 48000, energy = 0, gain = 1, adaptiveRelease = 100;
    Ramp threshold, ceiling, release{100, 100, 0, 0};
    int delay = 72, bitDepth = 24, ditherType = 0, shaping = 0;
    bool arc = true;
    size_t position = 0;
    uint32_t rng = 0x19642024u;
    std::array<std::array<double, 3>, 2> errors{};
    std::vector<Frame> ring;
};
