#include "WavetableOsc.h"

/** Constructs the oscillator object with the referenced wavetable
 */
WavetableOsc::WavetableOsc(float (&wavetable)[TABLE_SIZE])
    : wavetable{ wavetable } {}
 

float WavetableOsc::Process()
{
    index = std::fmod(index, fTABLE_SIZE);

    phase = index * inverseTABLE_SIZE;; 
    distortedPhase = Distort(phase);
    savedIndex = index;
    index = distortedPhase * fTABLE_SIZE; //calculate distorted index before interpolation
    // const float sample = InterpolateHermite4pt3oX(index);
    const float sample = LinearInterpolation(index);
    index = savedIndex + indexIncrement; // use un-distorted index to continue oscillating

    return amp * sample;
}
    

/** Sets the frequency of the oscillator by increasing
 * or decreasing the increment of the index
 */
void WavetableOsc::SetFreq(float freq_)
{
    indexIncrement = k1 * freq;
    freq = freq_;
}

/** Sets the amplitude of the oscillator
 */
void WavetableOsc::SetAmp(float amp_)
{
    amp = amp_ * amp_;
}

/** Sets phase distortion amount
 * accepts values between 0 and 1 (change to -1 to 1)
 */
void WavetableOsc::SetPhaseDistortion(float pd_)
{
    pd = 0.05f + pd_ * 0.90f;
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
float WavetableOsc::Distort(float phase_) const //knee
{
    if (phase_ < pd) {
        return (0.5f / pd) * phase_; 
    }
    else {
        return 1.0f - 0.5f * (phase_ - 1.0f) / (pd - 1.0f);
    }
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