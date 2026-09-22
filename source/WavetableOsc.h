#pragma once
// #include <cmath>
#include "daisysp.h"
#include "config.h"


/** Wavetable Oscillator that uses a referenced wavetable array
 * and renders an interpolated sample at a given frequency
 */
class WavetableOsc 
{
public:
    WavetableOsc(float (&wavetable)[TABLE_SIZE]);
    WavetableOsc(const WavetableOsc&) = delete;
    WavetableOsc& operator=(const WavetableOsc&) = delete;
    WavetableOsc(WavetableOsc&&) = default;
    WavetableOsc& operator=(WavetableOsc&&) = default;

    void Init(float sampleRate)
    {
        fTABLE_SIZE = static_cast<float>(TABLE_SIZE);
        index = 0.0f;
        indexIncrement = 0.0f;
        inverseSampleRate = 1.0f / sampleRate;
        amp = 1.0f;

        k1 = fTABLE_SIZE * inverseSampleRate;
        inverseTABLE_SIZE = 1.0f / TABLE_SIZE;

        distortedPhase = 0.0f;
        savedIndex = 0.0f;;
        phase = 0.0f;
        pd = 0.5f;
    }

    float Process();
    float GetFreq() const;

    void SetFreq(float freq_);
    void SetAmp(float amp_);
    void SetPhaseDistortion(float pd_);

    void Stop();
    bool IsPlaying() const;

private:
    float (&wavetable)[TABLE_SIZE];

    float fTABLE_SIZE;

    float freq;
    float index;
    float indexIncrement;
    float inverseSampleRate;
    float amp;

    // distortion variables;
    float inverseTABLE_SIZE;
    float distortedPhase;
    float savedIndex;
    float phase;
    float pd;

    float k1;

    float InterpolateHermite4pt3oX(float index_) const;
    float LinearInterpolation(float index_) const;
    float Distort(float phase_) const;
};