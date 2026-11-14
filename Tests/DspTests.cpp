#include "Limiter.h"
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool x, const char *s)
{
    if (!x)
        throw std::runtime_error(s);
}
int main()
{
    int configurations = 0;
    for (double rate : {44100., 48000., 96000., 192000.})
        for (bool arc : {false, true})
            for (int bits : {16, 18, 20, 22, 24})
                for (int type : {0, 1, 2})
                    for (int shape : {0, 1, 2, 3})
                    {
                        Limiter l;
                        l.set(-12, -1, 100, arc, bits, type, shape);
                        l.prepare(rate);
                        double peak = 0;
                        const double step = std::ldexp(1., 1 - bits);
                        for (int i = 0; i < 3000; ++i)
                        {
                            const double x = i % 300 == 0 ? 4. : .7 * std::sin(i * .13);
                            auto y = l.process(x, -x);
                            for (auto v : y)
                            {
                                check(std::isfinite(v), "nonfinite output");
                                check(std::abs(v) <= Limiter::db(-1), "ceiling exceeded");
                                check(std::abs(v / step - std::round(v / step)) < 1.e-8,
                                      "output not quantized to selected bits");
                                peak = std::max(peak, std::abs(v));
                            }
                        }
                        check(peak > .1, "unexpected silence");
                        check(l.reduction() > 0, "gain reduction missing");
                        auto y = l.process(std::numeric_limits<double>::quiet_NaN(),
                                           std::numeric_limits<double>::infinity());
                        check(std::isfinite(y[0]) && std::isfinite(y[1]), "NaN/Inf handling");
                        ++configurations;
                    }
    Limiter l;
    l.set(0, 0, 100, false, 24, 0, 0);
    l.prepare(48000);
    for (int i = 0; i < l.latency(); ++i)
        check(l.process(i == 0 ? .5 : 0, 0)[0] == 0, "early output");
    check(std::abs(l.process(0, 0)[0] - .5) < 1.e-12, "latency wrong");
    // Unity gain, delayed sine: only the expected half-LSB rounding error is permitted.
    l.reset();
    for (int i = 0; i < 4000; ++i)
    {
        auto y = l.process(.2 * std::sin(i * .07), .1 * std::sin(i * .07));
        if (i >= l.latency())
        {
            const double source = .2 * std::sin((i - l.latency()) * .07);
            check(std::abs(y[0] - source) <= std::ldexp(1., -24) + 1.e-12,
                  "unity signal altered beyond quantization");
            check(std::abs(y[0] - 2 * y[1]) <= std::ldexp(1., -22), "stereo image changed");
        }
    }
    // Quantization must also work with dither disabled.
    l.set(0, 0, 100, false, 16, 0, 0);
    l.reset();
    double result = 0;
    for (int i = 0; i < 1000; ++i)
        result = l.process(.123456, 0)[0];
    const double step = std::ldexp(1., -15);
    check(result == std::round(.123456 / step) * step, "16-bit no-dither quantization bypassed");
    // A new transient must create an anticipated ramp, not an immediate gain step.
    l.set(0, 0, 100, false, 24, 0, 0);
    l.reset();
    for (int i = 0; i < 1000; ++i)
        l.process(.1, 0);
    double previous = l.process(4, 0)[0];
    check(std::abs(previous - .1) < 1.e-6, "gain stepped when transient entered lookahead");
    for (int i = 1; i < l.latency(); ++i)
    {
        double next = l.process(.1, 0)[0];
        check(std::abs(next - previous) < .002, "attack discontinuity");
        previous = next;
    }
    check(l.process(.1, 0)[0] <= 1., "transient exceeded ceiling");
    // Lowering the ceiling during audio must take effect even while its ramp runs.
    l.set(0, -20, 100, true, 16, 2, 3);
    for (int i = 0; i < 1000; ++i)
    {
        auto y = l.process(4, -4);
        for (auto v : y)
            check(std::abs(v) <= Limiter::db(-20), "automated ceiling exceeded");
    }
    auto recovery = [](double ms)
    {
        Limiter x;
        x.set(0, 0, ms, false, 24, 0, 0);
        x.prepare(48000);
        x.process(4, 0);
        for (int i = 0; i < 10000; ++i)
            x.process(.01, 0);
        return x.reduction();
    };
    check(recovery(20) < recovery(400) * .1, "manual release does not affect recovery");
    // Dither must remove DC quantization bias at a fraction of one LSB.
    l.set(0, 0, 100, false, 16, 1, 0);
    l.reset();
    double sum = 0;
    int count = 0;
    for (int i = 0; i < 60000; ++i)
    {
        auto y = l.process(.31 * step, 0);
        if (i > l.latency())
        {
            sum += y[0];
            ++count;
        }
    }
    check(std::abs(sum / count - .31 * step) < .015 * step, "dither bias");
    uint32_t fuzz = 12345;
    auto random = [&]()
    {
        fuzz ^= fuzz << 13;
        fuzz ^= fuzz >> 17;
        fuzz ^= fuzz << 5;
        return double(fuzz) / 4294967296.;
    };
    double targetCeiling = 0;
    for (int i = 0; i < 100000; ++i)
    {
        if (i % 64 == 0)
        {
            targetCeiling = -30 * random();
            l.set(-30 * random(), targetCeiling, .01 + 999.99 * random(), random() > .5,
                  16 + 2 * int(random() * 5), int(random() * 3), int(random() * 4));
        }
        auto out = l.process(2 * std::sin(i * .17), .4 * std::sin(i * .07));
        for (auto sample : out)
            check(std::isfinite(sample) && std::abs(sample) <= Limiter::db(targetCeiling),
                  "random automation ceiling/finite failure");
    }
    l.set(0, 0, 100, false, 24, 0, 0);
    l.reset();
    check(l.process(0, 0)[0] == 0, "reset failed");
    std::cout << "PASS: " << configurations
              << " configurations; ceiling, bit grid, NaN/Inf, latency, unity signal, stereo, attack "
                 "continuity, automated ceiling, release, dither bias and reset\n";
}
