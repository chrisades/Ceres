#include "WavetableOsc.h"

/** Constructs the oscillator object with the referenced wavetable
 */
WavetableOsc::WavetableOsc(float (&wavetable)[TABLE_SIZE])
    : wavetable{ wavetable } {}
 

float WavetableOsc::Process()
{
    index = std::fmod(index, fTABLE_SIZE);

    phase = index * inverseTABLE_SIZE;

    switch(mode) {
        case (PD_MODE_1): 
            distortedPhase = ResoDistort(phase);
            break;
        
        case (PD_MODE_2): 
            distortedPhase = StepDistort(phase);
            break;
    
        case (PD_MODE_3): 
            distortedPhase = TriDistort(phase);
            break;

        default:
            distortedPhase = StepDistort(phase);
    }

    savedIndex = index;
    index = distortedPhase * fTABLE_SIZE;
    // const float sample = InterpolateHermite4pt3oX(index);
    const float sample = LinearInterpolation(index);
    index = savedIndex + indexIncrement;

    return amp * sample;
}

/** Sets the frequency of the oscillator by increasing
 * or decreasing the increment of the index
 */
void WavetableOsc::SetFreq(float freq_)
{
    freq = freq_;
    indexIncrement = k1 * freq;
}

/** Sets the amplitude of the oscillator
 */
void WavetableOsc::SetAmp(float amp_)
{
    amp = amp_;
}

/** Sets phase distortion amount
 * accepts values between 0 and 1
 */
void WavetableOsc::SetPhaseDistortion(float pd_)
{
    pd = pd_;
}

/** Stops the oscillator by setting
 * index and index increment to 0
 */
void WavetableOsc::Stop()
{
    index = 0.0f;
    indexIncrement = 0.0f;
}

float WavetableOsc::GetFreq() const
{
    return freq;
}

/** Returns true if oscillator is stopped
 */
bool WavetableOsc::IsPlaying() const
{
    return indexIncrement != 0;
}

// /** Computes the distorted phase
//  */
float WavetableOsc::KneeDistortX(float phase_)
{
    // pd goes from 0 to 1
    float a = 0.05f + pd * 0.90f;
    //float a = 0.5f + pd * 0.45f;

    if (phase_ < a) {
        return (0.5f / a) * phase_; 
    }
    else {
        return 1.0f - 0.5f * (phase_ - 1.0f) / (a - 1.0f);
    }
}

float WavetableOsc::KneeDistortY(float phase_)
{
    // pd goes from 0 to 1
    float b = 0.5f + pd * 0.5f;

    if (phase_ < 0.5f) {
        return (b / 0.5f) * phase_; 
    }
    else {
        return 2.0f * (1.0f - b) * (phase_ - 1.0f) + 1.0f;
    }
}

float WavetableOsc::KneeDistortXY(float phase_)
{
    // pd goes from 0 to 1
    float c = 0.05f + pd * 0.90f;

    if (c < 0.5f) {
        if (phase_ < 0.5f) {
            return (c / 0.5f) * phase_; 
        }
        else {
            return 2.0f * (1.0f - c) * (phase_ - 1.0f) + 1.0f;
        }
    }
    else {
        if (phase_ < c) {
            return (0.5f / c) * phase_; 
        }
        else {
            return 1.0f - 0.5f * (phase_ - 1.0f) / (c - 1.0f);
        }
    }
}

float WavetableOsc::ResoDistort(float phase_)
{
    // pd goes from 0 to 1
    float N = 1.0f + pd * 30.0f;
    float x = N * phase_;

    SetAmp(1.0f - phase_);

    return x - floorf(x);
}

float WavetableOsc::TriDistort(float phase_)
{
    SetAmp(1.0f);

    float c = 0.5f + pd * 0.45f;
    float a = 1.0f - c;

    float up   = phase_ * a;           
    float down = (1.0f - phase_) * c; 

    return fminf(up, down) / (c * a);
}

float WavetableOsc::StepDistort(float phase_)
{
    SetAmp(1.0f);
    // float a = 0.5f - pd * 0.45f;
    // if (phase_ >= 0.0f && phase_ < a * 0.4f) {
    //     return phase_ * 0.5f / a;
    // }
    // else if (phase_ >= a * 0.4f && phase_ < 0.2f) {
    //     return 0.2f;
    // }
    // else if (phase_ >= 0.2f && phase_ < (a * 0.4f + 0.2f)) {
    //     return (5.0f * phase_ + 2.0f * a - 1.0f) / (10.0f * a);
    // }
    // else if (phase_ >= (a * 0.4f + 0.2f) && phase_ < 0.4f) {
    //     return 0.4f;
    // }
    // else if (phase_ >= 0.4f && phase_ < (a * 0.4f + 0.4f)) {
    //     return (5.0f * phase_ + 4.0f * a - 2.0f) / (10.0f * a);
    // }
    // else if (phase_ >= (a * 0.4f + 0.4f) && phase_ < 0.6f) {
    //     return 0.6f;
    // }
    // else if (phase_ >= 0.6f && phase_ < (a * 0.4f + 0.6f)) {
    //     return (5.0f * phase_ + 6.0f * a - 3.0f) / (10.0f * a);
    // }
    // else if (phase_ >= (a * 0.4f + 0.6f) && phase_ < 0.8f) {
    //     return 0.8f;
    // }
    // else if (phase_ >= 0.8f && phase_ < (a * 0.4f + 0.8f)) {
    //     return (5.0f * phase_ + 8.0f * a - 4.0f) / (10.0f * a);
    // }
    // else {
    //     return 1.0f;
    // }

    const float numSteps = 6.0f; //
    const float invNumSteps = 1.0f / numSteps;

    const float a = 0.5f - pd * 0.45f;
    const float x = phase_ * numSteps;
    const float k = std::floor(x);         
    const float frac = x - k;             
    const float ramp = std::fmin(frac / (2.0f * a), 1.0f);

    return invNumSteps * (k + ramp);
}

void WavetableOsc::SetPDMode(int mode_)
{
    mode = mode_;
}

//Hermite Interpolation
float WavetableOsc::InterpolateHermite4pt3oX(float index_) const
{
    const auto i0  = static_cast<size_t>(index_);
    const auto im1 = (i0 - 1) & (TABLE_SIZE - 1);
    const auto i1  = (i0 + 1) & (TABLE_SIZE - 1);
    const auto i2  = (i0 + 2) & (TABLE_SIZE - 1);
    const auto x   = index_ - static_cast<float>(i0);

    float ym1 = wavetable[im1];
    float y0  = wavetable[i0];
    float y1  = wavetable[i1];
    float y2  = wavetable[i2];

    float c0  = y0;
    float c1  = 0.5f * (y1 - ym1);
    float c2  = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c3  = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
    return ((c3 * x + c2) * x + c1) * x + c0;
}

float WavetableOsc::LinearInterpolation(float index_) const
{
    const auto i0  = static_cast<size_t>(index_);
    const auto i1  = (i0 + 1) & (TABLE_SIZE - 1);
    float y0  = wavetable[i0];
    float y1  = wavetable[i1];
    const auto x   = index_ - static_cast<float>(i0);

    return y0 + x * (y1 - y0);
}