/*
Adapted from DiaysSP Metro Class
*/

#pragma once
#include <stdint.h>
#include <array>

namespace daisysp
{
/** Creates a clock signal at a specific frequency.
*/
class TrillClock
{
  public:
    TrillClock() {}
    ~TrillClock() {}
    /** Initializes TrillClock module.
        Arguments:
        - freq: frequency at which new clock signal will be generated
            Input Range: 
        - sampleRate: sample rate of audio engine
            Input range: 
    */
    void Init(float freq, float sampleRate);

    /** checks current state of TrillClock object and updates state if necesary.
    */
    std::array<uint8_t, 2> Process();

    /** resets phase to 0
    */
    inline void Reset() { phs_ = 0.0f; }
    /** Sets frequency at which TrillClock module will run at.
    */
    void SetFreq(float freq);

    /** Sets the trill distance or phase of the second clock
    */
    void SetDistance(float distance);

    /** Returns current value for frequency.
    */
    inline float GetFreq() { return freq_; }

  private:
    float freq_;
    float phs_, sampleRate_, phsInc_;
    float distance_;
};
}
