#include "Voice.h"

void Voice::Init(float sampleRate, size_t blockSize)
{
    wtOscA.Init(sampleRate);
    wtOscB.Init(sampleRate);

    trillClk.Init(2.0f, sampleRate);

    trillEnvA.Init(sampleRate);
    trillEnvA.SetMin(0.0);
    trillEnvA.SetMax(1.f);
    trillEnvA.SetCurve(0.0f);

    trillEnvB.Init(sampleRate);
    trillEnvB.SetMin(0.0);
    trillEnvB.SetMax(1.f);
    trillEnvB.SetCurve(0.0f);

    touchEnv.Init(sampleRate, blockSize);
    touchEnv.SetAttackTime(0.5f);
    touchEnv.SetDecayTime(0.2f);
    touchEnv.SetSustainLevel(1.0f);
    touchEnv.SetReleaseTime(1.0f);

    //voice steal de-clicker
}

void Voice::UpdateParams(const VoiceParams& params)
{
    params_ = params;
    
    float pitchMult = pitch;
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
    wtOscA.SetFreq(baseOscAFreq);
    wtOscB.SetFreq(baseOscBFreq);
    trillClk.SetFreq(baseTrillFreq);

    trillClk.SetDistance(params.distance);
    float trillPeriod = 1.0f / baseTrillFreq;
    float minWidth = 0.2f * trillPeriod;
    float maxWidth = 1.0f * trillPeriod;
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

    float pdAmtATarget = 0.5f + params.morph * 0.5f * touchEnvLevel;
    float pdAmtBTarget = 0.5f + params.morph * 0.5f * touchEnvLevel;
    wtOscA.SetPhaseDistortion(pdAmtATarget); //needs smoothing
    wtOscB.SetPhaseDistortion(pdAmtBTarget); //needs smoothing

    // if previous pitch is not equal to current pitch 
    // AND if the touch env is greater than threshold
    // then activate 
}

float Voice::Process()
{
    // Before you process, update params with new pitch
    // after stealfade is over


    if (resetTrillClk) { trillClk.Reset(); resetTrillClk = false; }

    auto trillClkVal = trillClk.Process();
    if ((trillClkVal[0] == 1) && (lastTrillClkVal[0] == 0)) {
        trillEnvA.Trigger();
    }
    if ((trillClkVal[1] == 1) && (lastTrillClkVal[1] == 0)) {
        trillEnvB.Trigger();
    }
    lastTrillClkVal = trillClkVal;

    touchEnvLevel = touchEnv.Process(gate);

    float envALevel = trillEnvA.Process();
    float envBLevel = trillEnvB.Process();

    float outA = envALevel * wtOscA.Process();
    float outB = envBLevel * wtOscB.Process();
    // float outA = wtOscA.Process();
    // float outB = wtOscB.Process();

    float out = touchEnvLevel * (outA + outB);

    return out;
}



void Voice::Reset() { resetTrillClk = true; }
void Voice::SetPitch(float p) { pitch = p; }

void Voice::Start() { gate = true; }

void Voice::Release() { gate = false; }
bool Voice::IsActive() { return touchEnv.IsRunning(); }
bool Voice::IsInRelease() { return touchEnv.GetCurrentSegment() == ADSR_SEG_RELEASE; }
bool Voice::IsInAttack() { return touchEnv.GetCurrentSegment() == ADSR_SEG_ATTACK; }