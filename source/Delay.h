#pragma once

#include <stdint.h>
#include "daisysp.h"
#include "daisy_seed.h"

class Delay
{
  public:
    static constexpr int32_t DELAY_LENGTH = 240000; // 5 s at 48kHz = 5 * 48000
    using Line = daisysp::DelayLine<float, DELAY_LENGTH>;

    Delay() {}
    ~Delay() {}

    /** Initialize the module
        \param sampleRate Audio engine sample rate
        \param line Delay line buffer, allocated with DSY_SDRAM_BSS
    */
    void Init(float sampleRate, Line* line);

    /** Get the next floating point sample.
        \param in Sample to process
    */
    float Process(float in);

    /** Set delay in ms.
        \param ms Delay amount in ms.
    */
    void SetDelayMs(float ms);

    /** Set delay in seconds.
        \param s Delay amount in seconds.
    */
    void SetDelayS(float s);

    /** Set feedback.
        \param feedback Feedback. Works 0-1.
    */
    void SetFeedback(float feedback);

  private:
    /** Set delay in samples, clamped to the buffer size. */
    void SetDelaySamples(float samples);

    float sampleRate_ = 48000.f;
    float feedback_   = 0.f;
    float delay_      = 0.f;

    Line* del_ = nullptr; // points at a buffer owned by the caller
};