#include "Delay.h"

void Delay::Init(float sampleRate, Line* line)
{
    sampleRate_ = sampleRate;
    del_        = line;
    del_->Init();
    feedback_ = .2f;
    SetDelayS(1.0f);
}

float Delay::Process(float in)
{
    del_->SetDelay(delay_);

    float out = del_->Read();
    del_->Write(in + out * feedback_);

    return (in + out) * .5f;
}

void Delay::SetDelayMs(float ms)
{
    ms = fmax(.1f, ms);
    SetDelaySamples(ms * .001f * sampleRate_);
}

void Delay::SetDelayS(float s)
{
    s = fmax(0.0001f, s);
    SetDelaySamples(s * sampleRate_);
}

void Delay::SetFeedback(float feedback)
{
    feedback_ = feedback;
}

void Delay::SetDelaySamples(float samples)
{
    delay_ = fmin(samples, (float)(DELAY_LENGTH - 1));
}