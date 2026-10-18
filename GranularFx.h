#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Original live-input design inspired by four-playhead granular instruments.
// Storage is supplied by the caller (SDRAM on Daisy). No allocation in Process.
class GranularFx
{
  public:
    static constexpr size_t kHistorySize = 131072;
    static constexpr size_t kDelaySize = 32768;
    static constexpr size_t kStorageSize = kHistorySize + 2 * kDelaySize;
    struct Stereo { float left, right; };

    void Init(float sampleRate, float* storage)
    {
        sr_ = sampleRate;
        history_ = storage;
        delayL_ = storage + kHistorySize;
        delayR_ = delayL_ + kDelaySize;
        std::fill(storage, storage + kStorageSize, 0.f);
        for(size_t i = 0; i <= kWindowSize; ++i)
            window_[i] = 0.5f - 0.5f * std::cos(6.28318530718f * i / kWindowSize);
        write_ = valid_ = 0;
        seed_ = 0x6d2b79f5u;
        target_ = mix_ = 0.f;
        limiterGain_ = 1.f;
        fadeStep_ = 1.f / (0.030f * sr_);
        limiterRelease_ = 1.f - std::exp(-1.f / (0.080f * sr_));
        toneCoef_ = 1.f - std::exp(-6.28318530718f * 3800.f / sr_);
        hpPole_ = std::exp(-6.28318530718f * 100.f / sr_);
        previous_ = highpass_ = 0.f;
        // Lowpass before resampling: protect the upward-octave head from aliasing.
        const float w = 6.28318530718f * 5200.f / sr_;
        const float alpha = std::sin(w) / 1.41421356237f;
        lpB0_ = (1.f - std::cos(w)) / (2.f * (1.f + alpha));
        lpB1_ = 2.f * lpB0_;
        lpA1_ = -2.f * std::cos(w) / (1.f + alpha);
        lpA2_ = (1.f - alpha) / (1.f + alpha);
        lpZ1_[0] = lpZ1_[1] = lpZ2_[0] = lpZ2_[1] = 0.f;
        delayLengthL_ = std::min(kDelaySize - 1, size_t(0.271f * sr_));
        delayLengthR_ = std::min(kDelaySize - 1, size_t(0.379f * sr_));
        ResetCloud();
    }

    // Called at block rate. Small endpoint deadband makes knob-left a true bypass.
    void SetControls(bool enabled, float amount)
    {
        amount = std::max(0.f, std::min(1.f, amount));
        target_ = enabled && amount > 0.015f ? (amount - 0.015f) / 0.985f : 0.f;
    }

    Stereo Process(float input)
    {
        // Capture continuously, including bypass, so enabling FX has recent speech.
        // The original signal does not go through these FX-only tone filters.
        highpass_ = input - previous_ + hpPole_ * highpass_;
        previous_ = input;
        float filtered = highpass_;
        for(int s = 0; s < 2; ++s)
        {
            const float y = lpB0_ * filtered + lpZ1_[s];
            lpZ1_[s] = lpB1_ * filtered - lpA1_ * y + lpZ2_[s];
            lpZ2_[s] = lpB0_ * filtered - lpA2_ * y;
            filtered = y;
        }
        history_[write_] = filtered;
        write_ = (write_ + 1) & (kHistorySize - 1);
        if(valid_ < kHistorySize) ++valid_;

        if(mix_ < target_) mix_ = std::min(target_, mix_ + fadeStep_);
        else if(mix_ > target_) mix_ = std::max(target_, mix_ - fadeStep_);
        if(mix_ == 0.f)
        {
            // No second limiter/coloration in bypass: bit-exact input passthrough.
            if(active_) ResetCloud();
            limiterGain_ = 1.f;
            return {input, input};
        }
        active_ = true;
        float left = 0.f, right = 0.f;
        for(int h = 0; h < 4; ++h)
        {
            Head& head = heads_[h];
            if(head.countdown == 0)
            {
                StartGrain(h, head.grains[head.next]);
                head.next ^= 1;
                head.countdown = head.length / 2;
            }
            --head.countdown;
            for(auto& grain : head.grains)
            {
                if(grain.age >= grain.length) continue;
                const float windowPosition = grain.age * grain.windowStep;
                const size_t wi = static_cast<size_t>(windowPosition);
                const float wf = windowPosition - wi;
                const float envelope = window_[wi] + wf * (window_[wi + 1] - window_[wi]);
                const size_t index = static_cast<size_t>(grain.position);
                const float frac = grain.position - index;
                const float sample = history_[index] + frac *
                    (history_[(index + 1) & (kHistorySize - 1)] - history_[index]);
                const float value = sample * envelope * grain.gain;
                left += value * grain.panL;
                right += value * grain.panR;
                grain.position += grain.rate;
                if(grain.position >= kHistorySize) grain.position -= kHistorySize;
                ++grain.age;
            }
        }
        // A short, damped stereo cross-feedback tail; never fed into the capture ring.
        const size_t readL = (delayWrite_ + kDelaySize - delayLengthL_) & (kDelaySize - 1);
        const size_t readR = (delayWrite_ + kDelaySize - delayLengthR_) & (kDelaySize - 1);
        const float echoL = delayValid_ >= delayLengthL_ ? delayL_[readL] : 0.f;
        const float echoR = delayValid_ >= delayLengthR_ ? delayR_[readR] : 0.f;
        echoToneL_ += toneCoef_ * (echoL - echoToneL_);
        echoToneR_ += toneCoef_ * (echoR - echoToneR_);
        delayL_[delayWrite_] = left + 0.32f * echoToneR_;
        delayR_[delayWrite_] = right + 0.32f * echoToneL_;
        delayWrite_ = (delayWrite_ + 1) & (kDelaySize - 1);
        if(delayValid_ < kDelaySize) ++delayValid_;
        left = input + mix_ * 0.85f * (left + 0.35f * echoToneL_);
        right = input + mix_ * 0.85f * (right + 0.35f * echoToneR_);
        // Stereo-linked peak control; the immediate vocoder remains present at all amounts.
        const float peak = std::max(std::fabs(left), std::fabs(right));
        const float required = peak > 0.9f ? 0.9f / peak : 1.f;
        limiterGain_ += limiterRelease_ * (1.f - limiterGain_);
        limiterGain_ = std::min(limiterGain_, required);
        return {left * limiterGain_, right * limiterGain_};
    }

    float Mix() const { return mix_; }

  private:
    static constexpr size_t kWindowSize = 1024;
    struct Grain
    {
        float position = 0, rate = 1, gain = 0, panL = 0, panR = 0, windowStep = 0;
        uint32_t age = 0, length = 0;
    };
    struct Head
    {
        Grain grains[2];
        uint32_t length = 0, countdown = 0, next = 0;
    };
    Head heads_[4];
    float window_[kWindowSize + 1];
    float* history_ = nullptr;
    float* delayL_ = nullptr;
    float* delayR_ = nullptr;
    float sr_ = 48000, target_ = 0, mix_ = 0, fadeStep_ = 0;
    float previous_ = 0, highpass_ = 0, hpPole_ = 0;
    float lpB0_ = 0, lpB1_ = 0, lpA1_ = 0, lpA2_ = 0;
    float lpZ1_[2] = {}, lpZ2_[2] = {};
    float limiterGain_ = 1, limiterRelease_ = 0, toneCoef_ = 0;
    float echoToneL_ = 0, echoToneR_ = 0;
    size_t write_ = 0, valid_ = 0, delayWrite_ = 0, delayValid_ = 0;
    size_t delayLengthL_ = 0, delayLengthR_ = 0;
    uint32_t seed_ = 0;
    bool active_ = false;

    float Random()
    {
        seed_ ^= seed_ << 13; seed_ ^= seed_ >> 17; seed_ ^= seed_ << 5;
        return float(seed_ >> 8) * (1.f / 16777216.f);
    }

    void ResetCloud()
    {
        const float duration[4] = {0.110f, 0.145f, 0.190f, 0.160f};
        for(int h = 0; h < 4; ++h)
        {
            heads_[h] = Head{};
            heads_[h].length = (uint32_t(duration[h] * sr_) / 2) * 2;
            heads_[h].countdown = uint32_t(h * 0.017f * sr_);
        }
        delayWrite_ = delayValid_ = 0;
        echoToneL_ = echoToneR_ = 0.f;
        active_ = false;
    }

    void StartGrain(int h, Grain& grain)
    {
        const float rates[4] = {1.f, 1.f, 0.5f, 2.f};
        const float lookback[4] = {0.100f, 0.235f, 0.380f, 0.510f};
        const float levels[4] = {0.42f, 0.42f, 0.20f, 0.16f};
        const float pan[4] = {-0.65f, 0.65f, -0.25f, 0.25f};
        const float length = float(heads_[h].length);
        // Keep every interpolated sample safely behind the writer, even at 2x rate,
        // and ahead of overwritten history when a grain runs at half speed.
        const float minimum = 4.f + std::max(0.f, rates[h] - 1.f) * length;
        const float maximum = float(kHistorySize) - 4.f -
                              std::max(0.f, 1.f - rates[h]) * length;
        const float delay = std::max(minimum, std::min(maximum,
            (lookback[h] + (Random() - 0.5f) * 0.095f) * sr_));
        // At startup, wait for genuine recorded history rather than stale SDRAM.
        if(float(valid_) < delay + 2.f) { grain.length = 0; return; }
        grain.position = float(write_) - 1.f - delay;
        if(grain.position < 0.f) grain.position += kHistorySize;
        grain.rate = rates[h];
        grain.gain = levels[h] * (0.80f + 0.20f * Random());
        const float p = std::max(-1.f, std::min(1.f, pan[h] + 0.5f * (Random() - 0.5f)));
        grain.panL = std::sqrt(0.5f * (1.f - p));
        grain.panR = std::sqrt(0.5f * (1.f + p));
        grain.age = 0;
        grain.length = heads_[h].length;
        grain.windowStep = float(kWindowSize) / float(grain.length);
    }
};
