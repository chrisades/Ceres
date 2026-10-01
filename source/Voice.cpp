#include "Voice.h"

void Voice::Init(float sampleRate, size_t blockSize)
{
    wtOscA.Init(sampleRate);
    wtOscB.Init(sampleRate);

    trillClk.Init(2.0f, sampleRate);

    trillEnvA.Init(sampleRate);
    trillEnvA.SetMin(0.0);
    trillEnvA.SetMax(1.f);
    trillEnvA.SetCurve(0);

    trillEnvB.Init(sampleRate);
    trillEnvB.SetMin(0.0);
    trillEnvB.SetMax(1.f);
    trillEnvB.SetCurve(0.0f);

    touchEnv.Init(sampleRate, blockSize);
    touchEnv.SetAttackTime(0.5f);
    touchEnv.SetDecayTime(0.2f);
    touchEnv.SetSustainLevel(1.0f);
    touchEnv.SetReleaseTime(1.0f);
}

void Voice::UpdateParams(const VoiceParams& params)
{
    float pitchMult = octave * pitch;
    float pitchMultTrill = 1.0f;
    float pitchMultOscA = 1.0f;
    float pitchMultOscB = 1.0f;

    switch (params.switchAValue) {
        case (2):
            pitchMultTrill = pitchMult;
            pitchMultOscA = pitchMult;
            break;
        case (0):
            pitchMultTrill = pitchMult;
            pitchMultOscA = pitchMult;
            pitchMultOscB = pitchMult;
            break;
        case (1):
            pitchMultOscA = pitchMult;
            pitchMultOscB = pitchMult;
            break;
    }

    float baseTrillFreq = pitchMultTrill * fmap(params.trillFreq, MIN_TRILL_FREQ, MAX_TRILL_FREQ, Mapping::LOG);
    float baseOscAFreq = pitchMultOscA * fmap(params.oscAFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG);
    float baseOscBFreq = pitchMultOscB * fmap(params.oscBFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG);

    // float baseTrillFreq = MIN_TRILL_FREQ + trillFreq * (MAX_TRILL_FREQ - MIN_TRILL_FREQ);
    // float baseOscAFreq = MIN_OSC_FREQ + oscAFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);
    // float baseOscBFreq = MIN_OSC_FREQ + oscBFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);

    wtOscA.SetFreq(baseOscAFreq);
    wtOscB.SetFreq(baseOscBFreq);

    trillClk.SetFreq(baseTrillFreq);
    trillClk.SetDistance(params.distance);

    float trillPeriod = 1.0f / baseTrillFreq;

    float minWidth = 0.2f * trillPeriod;
    float maxWidth = 1.2f * trillPeriod;

    float baseWidthA = minWidth + params.widthA * (maxWidth - minWidth);
    float baseWidthB = minWidth + params.widthB * (maxWidth - minWidth);

    float skew = 0.1 + params.shape * 0.8f;
    float skewAtk = skew;
    float skewDec = 1.0f - skew;

    float trillEnvAtkA = skewAtk * baseWidthA;
    float trillEnvDecA = skewDec * baseWidthA;
    float trillEnvAtkB = skewAtk * baseWidthB;
    float trillEnvDecB = skewDec * baseWidthB;

    trillEnvA.SetTime(ADENV_SEG_ATTACK, trillEnvAtkA);
    trillEnvA.SetTime(ADENV_SEG_DECAY, trillEnvDecA);
    trillEnvB.SetTime(ADENV_SEG_ATTACK, trillEnvAtkB);
    trillEnvB.SetTime(ADENV_SEG_DECAY, trillEnvDecB);

    hold = false;
    switch (params.switchBValue) {
        case (2):
            touchEnv.SetAttackTime(10.0f);
            touchEnv.SetReleaseTime(10.0f);
            break;
        case (0):
            touchEnv.SetAttackTime(0.5f);
            touchEnv.SetReleaseTime(1.0f);
            break;
        case (1):
            hold = true;
            break;
    }

    // float pdAmtA = 0.5f + params.morph * env * 0.5f; 
    // float pdAmtB = pdAmtA; 
    float pdAmtA = 0.5f + params.morph * 0.5f;//envA * 0.5f; 
    float pdAmtB = 0.5f + params.morph * 0.5f;//envB * 0.5f; 
    wtOscA.SetPhaseDistortion(pdAmtA);
    wtOscB.SetPhaseDistortion(pdAmtB);
}

float Voice::Process()
{
    auto trillClkVal = trillClk.Process();
    if ((trillClkVal[0] == 1) && (lastTrillClkVal[0] == 0)) {
        trillEnvA.Trigger();
    }
    if ((trillClkVal[1] == 1) && (lastTrillClkVal[1] == 0)) {
        trillEnvB.Trigger();
    }
    lastTrillClkVal = trillClkVal;

    float env = touchEnv.Process(true);
    float envA = trillEnvA.Process();
    float envB = trillEnvB.Process();

    float outA = envA * wtOscA.Process();
    float outB = envB * wtOscB.Process();

    return env * (outA + outB);
}

void Voice::Start()
{
    gate = true;
}

void Voice::Stop()
{
    gate = false;
}

bool Voice::IsActive()
{
    return touchEnv.IsRunning();
}