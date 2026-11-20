#pragma once
// #include <cmath>
#include "daisysp.h"
#include "Config.h"

enum {
    PD_MODE_1,
    PD_MODE_2,
    PD_MODE_3
};

/** Wavetable Oscillator that uses a referenced wavetable array
 * and renders an interpolated sample at a given frequencyhg
 */
class WavetableOsc 
{
public:
    WavetableOsc(float (&wavetable)[TABLE_SIZE]);
    WavetableOsc(const WavetableOsc&) = delete;
    WavetableOsc& operator=(const WavetableOsc&) = delete;
    WavetableOsc(WavetableOsc&&) = default;
    WavetableOsc& operator=(WavetableOsc&&) = delete;

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

        smoothCoef = 1.0f / (0.05f * sampleRate);
    }

    float Process();
    float ProcessWithoutPD();
    float GetFreq() const;

    void SetFreq(float freq_);
    void SetAmp(float amp_);
    void SetPhaseDistortion(float pd_);

    void SetPDMode(int mode_);

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
    float smoothN = 1.0f;
    float smoothCoef = 0.0f;

    int  mode = PD_MODE_1;
    int  pendingMode = PD_MODE_1;
    bool modePending = false;

    float k1;

    float InterpolateHermite4pt3oX(float index_) const;
    float LinearInterpolation(float index_) const;

    float KneeDistortX(float phase_);
    float KneeDistortY(float phase_);
    float KneeDistortXY(float phase_);
    float ResoDistort(float phase_);
    float TriDistort(float phase_);
    float StepDistort(float phase_);
    float PhaseFM(float phase_);
    float SCurveDistort(float phase_);
    float SineToSquare(float phase_);
};