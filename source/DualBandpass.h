#pragma once

#include <cmath>
#include <algorithm>

class DualBandpass
{
public:
    void Init(float sampleRate)
    {
        sampleRate_ = sampleRate;
        band1_.Reset();
        band2_.Reset();
        
        SetBands(500.0f, 1.0f, 2000.0f, 2.0f);
    }

    // Set center frequency and width for a band in one call (one coefficient update)
    void SetBand1(float centerFreqHz, float widthHz)
    {
        centerFreq1_ = centerFreqHz;
        width1_      = widthHz;
        Update(band1_, centerFreq1_, width1_);
    }

    void SetBand2(float centerFreqHz, float widthHz)
    {
        centerFreq2_ = centerFreqHz;
        width2_      = widthHz;
        Update(band2_, centerFreq2_, width2_);
    }

    // Set both bands at once
    void SetBands(float centerFreq1Hz, float width1Hz, float centerFreq2Hz, float width2Hz)
    {
        SetBand1(centerFreq1Hz, width1Hz);
        SetBand2(centerFreq2Hz, width2Hz);
    }

    void SetGains(float gain1, float gain2)
    {
        gain1_ = gain1;
        gain2_ = gain2;
    }


    // Sum of both bandpass outputs
    float Process(float in)
    {
        return (gain1_ * band1_.Process(in)) + (gain2_ * band2_.Process(in));
    }

private:
    struct Biquad
    {
        float b0 = 0.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
        float z1 = 0.f, z2 = 0.f;

        void Reset() { z1 = z2 = 0.f; }

        float Process(float x)
        {
            float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    void Update(Biquad& f, float freq, float width)
    {
        const float nyquist = 0.5f * sampleRate_;
        freq  = std::clamp(freq, 10.0f, 0.49f * sampleRate_);
        width = std::clamp(width, 1.0f, nyquist);
        // if (!std::isfinite(freq) || !std::isfinite(width)) return;
        const float q = std::clamp(freq / width, 0.1f, 100.0f);

        const float w0    = 2.0f * 3.14159265358979f * freq / sampleRate_;
        const float alpha = std::sin(w0) / (2.0f * q);
        const float a0inv = 1.0f / (1.0f + alpha);

        f.b0 =  alpha * a0inv;
        f.b1 =  0.0f;
        f.b2 = -alpha * a0inv;
        f.a1 = -2.0f * std::cos(w0) * a0inv;
        f.a2 = (1.0f - alpha) * a0inv;
    }

    float sampleRate_  = 48000.0f;
    float centerFreq1_ = 500.0f,  width1_ = 100.0f;
    float centerFreq2_ = 2000.0f, width2_ = 400.0f;
    float gain1_ = 1.0f, gain2_ = 1.0f;
    Biquad band1_, band2_;
};