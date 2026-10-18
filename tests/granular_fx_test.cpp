#include "../GranularFx.h"
#include "../VocoderDsp.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

static float Signal(int i, float sr)
{
    return 0.3f * std::sin(6.28318530718f * 220.f * i / sr)
         + 0.12f * std::sin(6.28318530718f * 880.f * i / sr);
}

static void Test(float sr)
{
    std::vector<float> memory(GranularFx::kStorageSize + 2, 123.f);
    GranularFx fx;
    fx.Init(sr, memory.data() + 1);
    // Baseline equivalence, including control changes, with FX disabled.
    VocoderDsp reference, expanded;
    reference.Init(sr); expanded.Init(sr);
    for(int i = 0; i < int(2 * sr); ++i)
    {
        float input = Signal(i, sr) * 0.1f;
        float carrier = Signal(i * 2, sr);
        float dry = i < sr ? 0.f : 0.55f;
        float wet = i % int(sr) < sr / 2 ? 0.55f : 0.f;
        float baseline = reference.Process(input, carrier, dry, wet);
        float source = expanded.Process(input, carrier, dry, wet);
        fx.SetControls(false, (i % 101) / 100.f);
        auto y = fx.Process(source);
        assert(y.left == baseline && y.right == baseline);
    }
    // Knob-left is exact bypass even when the FX button is enabled.
    fx.SetControls(true, 0.f);
    for(int i = 0; i < 1000; ++i)
    {
        float x = Signal(i, sr); auto y = fx.Process(x);
        assert(y.left == x && y.right == x);
    }
    fx.SetControls(true, 1.f);
    double wetEnergy = 0, widthEnergy = 0, tailEnergy = 0, lateEnergy = 0;
    float peak = 0;
    // Long enough to wrap the history and delay buffers repeatedly.
    for(int i = 0; i < int(12 * sr); ++i)
    {
        float input = i < int(4 * sr) ? Signal(i, sr) : 0.f;
        auto y = fx.Process(input);
        assert(std::isfinite(y.left) && std::isfinite(y.right));
        assert(std::fabs(y.left) <= 0.900001f && std::fabs(y.right) <= 0.900001f);
        peak = std::max(peak, std::max(std::fabs(y.left), std::fabs(y.right)));
        if(i > sr && i < 4 * sr)
        {
            wetEnergy += (y.left - input) * (y.left - input);
            widthEnergy += (y.left - y.right) * (y.left - y.right);
        }
        if(i > 4.05f * sr && i < 4.45f * sr) tailEnergy += y.left*y.left+y.right*y.right;
        if(i > 11.f * sr) lateEnergy += y.left*y.left+y.right*y.right;
    }
    assert(wetEnergy > 1 && widthEnergy > 1 && tailEnergy > .01);
    assert(lateEnergy < 1e-8);
    // Wet gain at every knob position remains finite at worst-case baseline levels.
    for(int i = 0; i < int(4 * sr); ++i)
    {
        if(i % 48 == 0) fx.SetControls(true, float((i / 48) % 101) / 100.f);
        const float x = (i / 211) % 2 ? 0.9f : -0.9f;
        auto y = fx.Process(x);
        assert(std::isfinite(y.left) && std::isfinite(y.right));
        assert(std::fabs(y.left) <= .900001f && std::fabs(y.right) <= .900001f);
    }
    // Disabling ramps back to exact bypass within 30 ms, without stale delay bursts.
    fx.SetControls(false, 1.f);
    for(int i = 0; i < int(sr * .04f); ++i) fx.Process(0.f);
    assert(fx.Mix() == 0.f);
    for(int i = 0; i < int(sr * 3); ++i) fx.Process(0.f);
    fx.SetControls(true,1.f);
    for(int i = 0; i < int(sr * 2); ++i)
    {
        auto y = fx.Process(0.f);
        assert(std::fabs(y.left) < 1e-10f && std::fabs(y.right) < 1e-10f);
    }
    assert(memory.front() == 123.f && memory.back() == 123.f);
    std::printf("PASS %.0f Hz: bit-exact bypass; wet RMS %.4f; stereo difference RMS %.4f; peak %.4f; tails decay; wrap/toggle/overload safe\n",
                sr, std::sqrt(wetEnergy/(3*sr)), std::sqrt(widthEnergy/(3*sr)), peak);
}
int main()
{
    Test(44100); Test(48000); Test(96000);
}
