#include <math.h>
#include "TrillClock.h"
#include "daisysp.h"

using namespace daisysp;

void TrillClock::Init(float freq, float sampleRate)
{
    freq_        = freq;
    phs_         = 0.0f;
    distance_     = 0.0f;
    sampleRate_ = sampleRate;
    phsInc_     = (TWOPI_F * freq_) / sampleRate_;
}

std::array<uint8_t, 2> TrillClock::Process()
{
    phs_ += phsInc_;

    uint8_t clk2 = 0;
    if(phs_ >= distance_)
    {
        clk2 = 1;
    }

    if(phs_ >= TWOPI_F)
    {
        phs_ -= TWOPI_F;
        return {1, clk2};
    }
    return {0, clk2};
}

void TrillClock::SetFreq(float freq)
{
    freq_    = freq;
    phsInc_ = (TWOPI_F * freq_) / sampleRate_;
}

void TrillClock::SetDistance(float distance){
    distance_ = TWOPI_F * fclamp(distance, 0.0f, 1.0f);
}
