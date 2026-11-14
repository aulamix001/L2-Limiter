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
    for (double rate : {44100., 48000., 96000.})
        for (bool arc : {false, true})
            for (int type : {0, 1, 2})
                for (int shape : {0, 1, 2, 3})
                {
                    Limiter l;
                    l.set(-12, -1, 100, arc, 16, type, shape);
                    l.prepare(rate);
                    double max = 0;
                    for (int i = 0; i < 12000; ++i)
                    {
                        const double x = i % 300 == 0 ? 4. : .7 * std::sin(i * .13);
                        auto y = l.process(x, -x);
                        for (auto v : y)
                        {
                            check(std::isfinite(v), "finite output");
                            check(std::abs(v) <= Limiter::db(-1) + 1.e-12, "ceiling exceeded");
                            max = std::max(max, std::abs(v));
                        }
                    }
                    check(max > .1, "unexpected silence");
                    check(l.reduction() > 0, "gain reduction missing");
                    auto y = l.process(std::numeric_limits<double>::quiet_NaN(), 0);
                    check(std::isfinite(y[0]), "NaN handling");
                }
    Limiter l;
    l.set(0, 0, 100, false, 24, 0, 0);
    l.prepare(48000);
    for (int i = 0; i < l.latency(); ++i)
        check(l.process(i == 0 ? .5 : 0, 0)[0] == 0, "early output");
    check(std::abs(l.process(0, 0)[0] - .5) < 1.e-12, "latency wrong");
    l.set(0, -20, 100, true, 16, 2, 3);
    for (int i = 0; i < 200; ++i)
    {
        auto y = l.process(4, -4);
        for (auto v : y)
            check(std::abs(v) <= Limiter::db(-20), "automated ceiling exceeded");
    }
    l.set(0, 0, 100, false, 24, 0, 0);
    l.reset();
    check(l.process(0, 0)[0] == 0, "reset failed");
    std::cout
        << "PASS: 72 DSP configurations, finite output, ceiling, signal, reduction, latency and reset\n";
}
