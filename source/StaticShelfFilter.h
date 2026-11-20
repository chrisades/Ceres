#pragma once

#include <cmath>
#include <cstddef>

// Two RBJ cookbook shelving biquads in series, transposed Direct Form II.
template <std::size_t NumChannels = 2>
class StaticShelfFilter
{
  public:
    struct Params
    {
        float lowFreqHz  = 150.0f;   
        float lowGainDB  = 3.0f;     
        float highFreqHz = 6000.0f; 
        float highGainDB = -6.0f;   
        float slope        = 0.707f;   
    };

    StaticShelfFilter() { Reset(); }

    void Init(float sample_rate) { Init(sample_rate, Params()); }

    void Init(float sample_rate, const Params& p)
    {
        fs_   = static_cast<double>(sample_rate);
        low_  = MakeShelf(true, p.lowFreqHz, p.lowGainDB, p.slope);
        high_ = MakeShelf(false, p.highFreqHz, p.highGainDB, p.slope);
        Reset();
    }

    void Reset()
    {
        for(std::size_t ch = 0; ch < NumChannels; ch++)
        {
            z_[ch][0] = z_[ch][1] = z_[ch][2] = z_[ch][3] = 0.0f;
        }
    }

    inline float Process(std::size_t ch, float in)
    {
        float* z = z_[ch];

        // low shelf (TDF2)
        float y = low_.b0 * in + z[0];
        z[0]    = low_.b1 * in - low_.a1 * y + z[1];
        z[1]    = low_.b2 * in - low_.a2 * y;

        // high shelf (TDF2)
        const float x2 = y;
        y              = high_.b0 * x2 + z[2];
        z[2]           = high_.b1 * x2 - high_.a1 * y + z[3];
        z[3]           = high_.b2 * x2 - high_.a2 * y;

        return y;
    }

    void ProcessBlock(std::size_t ch, const float* in, float* out, std::size_t size)
    {
        for(std::size_t i = 0; i < size; i++)
            out[i] = Process(ch, in[i]);
    }

    void ProcessBuffer(const float* const* in,
                       float**             out,
                       std::size_t         size,
                       std::size_t         num_channels = NumChannels)
    {
        for(std::size_t ch = 0; ch < num_channels && ch < NumChannels; ch++)
            ProcessBlock(ch, in[ch], out[ch], size);
    }

  private:
    struct Coeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f; 
    };

    Coeffs MakeShelf(bool is_low, double freq, double gain_db, double slope) const
    {
        constexpr double kPi = 3.14159265358979323846;

        // Keep corner in a safe range
        if(freq < 10.0)
            freq = 10.0;
        if(freq > 0.45 * fs_)
            freq = 0.45 * fs_;
        if(slope < 0.05)
            slope = 0.05;

        const double A    = std::pow(10.0, gain_db / 40.0);
        const double w0   = 2.0 * kPi * freq / fs_;
        const double cosw = std::cos(w0);
        const double sinw = std::sin(w0);

        double root = (A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0;
        if(root < 0.0)
            root = 0.0;
        const double alpha = sinw / 2.0 * std::sqrt(root);
        const double tsa   = 2.0 * std::sqrt(A) * alpha;

        double b0, b1, b2, a0, a1, a2;
        if(is_low)
        {
            b0 = A * ((A + 1) - (A - 1) * cosw + tsa);
            b1 = 2.0 * A * ((A - 1) - (A + 1) * cosw);
            b2 = A * ((A + 1) - (A - 1) * cosw - tsa);
            a0 = (A + 1) + (A - 1) * cosw + tsa;
            a1 = -2.0 * ((A - 1) + (A + 1) * cosw);
            a2 = (A + 1) + (A - 1) * cosw - tsa;
        }
        else
        {
            b0 = A * ((A + 1) + (A - 1) * cosw + tsa);
            b1 = -2.0 * A * ((A - 1) + (A + 1) * cosw);
            b2 = A * ((A + 1) + (A - 1) * cosw - tsa);
            a0 = (A + 1) - (A - 1) * cosw + tsa;
            a1 = 2.0 * ((A - 1) - (A + 1) * cosw);
            a2 = (A + 1) - (A - 1) * cosw - tsa;
        }

        Coeffs c;
        c.b0 = static_cast<float>(b0 / a0);
        c.b1 = static_cast<float>(b1 / a0);
        c.b2 = static_cast<float>(b2 / a0);
        c.a1 = static_cast<float>(a1 / a0);
        c.a2 = static_cast<float>(a2 / a0);
        return c;
    }

    double fs_ = 48000.0;
    Coeffs low_, high_;
    float  z_[NumChannels][4];
};