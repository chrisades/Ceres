#include "Voice.h"

inline float quantizeFreq12TET(float hz, float a4 = 440.0f) {
    hz = std::max(hz, 1e-3f);                           
    const float semis = 12.0f * std::log2(hz / a4);     
    return a4 * std::exp2(std::round(semis) / 12.0f);
}

void Voice::Init(float sampleRate, size_t blockSize)
{
    oscA.Init(sampleRate);
    oscB.Init(sampleRate);

    trillClk.Init(2.0f, sampleRate);
    panClk.Init(1.0f, sampleRate);

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

    xFade.Init(CROSSFADE_CPOW);

    xFadeFX.Init(CROSSFADE_EXP);

    //voice steal de-clicker
    STEAL_STEP = 1.0f / (0.03f * sampleRate);

    //Smooth Coeffs
    smoothCoef = 1.0f / (0.05f * sampleRate);
    slowSmoothCoef = 1.0f / (0.3f * sampleRate);
}

void Voice::UpdateParams(const VoiceParams& params)
{
    params_ = params;

    float pitchMult = pitch;
    float pitchMultTrill = 1.0f;
    float pitchMultOscA = 1.0f;
    float pitchMultOscB = 1.0f;

    switch (params_.trackingMode) {
        case (TRACKING_BOTH):
            pitchMultTrill = pitchMult;
            pitchMultOscA = pitchMult;
            pitchMultOscB = pitchMult;
            break;
        case (TRACKING_OSC):
            pitchMultOscA = pitchMult;
            pitchMultOscB = pitchMult;
            break;
    }

    switch (params_.modMode) {
        case (MOD_MODE_1):
            rungModA = 1.0f; 
            rungModB = 1.0f; 
            fmodA = 1.0f; 
            fmodB = 1.0f;
            fmodTrill = 1.0f;
            widthAMod = 0.0f;
            widthBMod = 0.0f;
            break;
        case (MOD_MODE_2):
            rungModA = exp2f((rungOutA) * 2.0f * modDepth);
            rungModB = exp2f((rungOutB) * 2.0f * modDepth);
            fmodA = 1.0f;
            fmodB = 1.0f;
            fmodTrill = 1.0f;
            widthAMod = 0.0f;
            widthBMod = 0.0f;
            break;
        case (MOD_MODE_3):
            rungModA = exp2f((rungOutA) * 4.0f * modDepth);
            rungModB = exp2f((rungOutB) * -4.0f * modDepth);
            fmodA = exp2f((oscBValue * params_.widthB) * 4.0f * modDepth);
            fmodB = exp2f((oscAValue * params_.widthA) * -4.0f * modDepth);
            fmodTrill = exp2f((rungOutA + rungOutB) * 2.0f * modDepth * modDepth);
            widthAMod = 0.0f;
            widthBMod = 0.0f;
            break;
    }


    baseTrillFreq =  pitchMultTrill * fmap(params_.trillFreq, MIN_TRILL_FREQ, MAX_TRILL_FREQ, Mapping::LOG);

    // baseOscAFreq = pitchMultOscA * fmap(params_.oscAFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG);
    // baseOscBFreq = pitchMultOscB * fmap(params_.oscBFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG);

    if (params_.pitchSwap) {
        fonepole(baseOscAFreq, 0.5f * pitchMultOscB * fmap(params_.oscBFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG), 8.0f * smoothCoef);
        fonepole(baseOscBFreq, 0.5f * pitchMultOscA * fmap(params_.oscAFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG), 8.0f * smoothCoef);
    }
    else {
        fonepole(baseOscAFreq, pitchMultOscA * fmap(params_.oscAFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG), 32.0f * smoothCoef);
        fonepole(baseOscBFreq, pitchMultOscB * fmap(params_.oscBFreq, MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG), 32.0f * smoothCoef);
    }

    oscA.SetFreq(fmodA * rungModA * baseOscAFreq); 
    oscA.SetPDMode(params_.morphMode);
    oscB.SetFreq(fmodB * rungModB * baseOscBFreq);
    oscB.SetPDMode(params_.morphMode);
    trillClk.SetFreq(fmodTrill * baseTrillFreq);

    trillClk.SetDistance(params_.distance);
    float trillPeriod = 1.0f / baseTrillFreq;
    float minWidth = 0.2f * trillPeriod;
    float maxWidth = 1.0f * trillPeriod;

    panClk.SetFreq(pitchMult * 1.0f);

    float widthA = fclamp(params_.widthA + widthAMod, 0.0f, 1.0f);
    float widthB = fclamp(params_.widthB + widthBMod, 0.0f, 1.0f);

    float baseWidthA = minWidth + widthA * (maxWidth - minWidth);
    float baseWidthB = minWidth + widthB * (maxWidth - minWidth);
    float skew = 0.1 + params_.shape * 0.8f;
    float skewAtk = skew;
    float skewDec = 1.0f - skew;

    float trillEnvAtkA = fmaxf(skewAtk * baseWidthA, 0.00003f);
    float trillEnvDecA = fmaxf(skewDec * baseWidthA, 0.00003f);
    float trillEnvAtkB = fmaxf(skewAtk * baseWidthB, 0.00003f);
    float trillEnvDecB = fmaxf(skewDec * baseWidthB, 0.00003f);

    trillEnvA.SetTime(ADENV_SEG_ATTACK, trillEnvAtkA);
    trillEnvA.SetTime(ADENV_SEG_DECAY, trillEnvDecA);
    trillEnvB.SetTime(ADENV_SEG_ATTACK, trillEnvAtkB);
    trillEnvB.SetTime(ADENV_SEG_DECAY, trillEnvDecB);

    touchEnv.SetAttackTime(params_.touchEnvAtk);
    touchEnv.SetReleaseTime(params_.touchEnvRel);

    fonepole(xFadeAmt, params_.distance, smoothCoef);
    xFade.SetPos(xFadeAmt);

}


float Voice::Process()
{

    if (resetTrillClk) { trillClk.Reset(); }

    auto trillClkVal = trillClk.Process();
    if ((trillClkVal[0] == 1) && (lastTrillClkVal[0] == 0)) {

        runglerB.setThreshold(-1.0f + params_.widthB * 2.0f);
        runglerA.shift(oscAValue);        

        if (runglerA.getFirstBit()) { rungOutA = runglerA.getOutput(); }  
        switch (params_.trillTrigMode) {
            case (TRILL_TRIG_MODE_1):
                trillEnvA.Trigger();
                break;
            case (TRILL_TRIG_MODE_2):
                if (runglerA.getFirstBit() || resetTrillClk) { trillEnvA.Trigger(); } 
                break;
        }
    }
    if ((trillClkVal[1] == 1) && (lastTrillClkVal[1] == 0)) {

        runglerB.setThreshold(-1.0f + params_.widthA * 2.0f);

        runglerB.shift(oscBValue);        
        
        if (runglerB.getFirstBit()) { rungOutB = runglerB.getOutput(); }

        switch (params_.trillTrigMode) {
            case (TRILL_TRIG_MODE_1):
                trillEnvB.Trigger();
                break;
            case (TRILL_TRIG_MODE_2):
                if (runglerB.getFirstBit()) { trillEnvB.Trigger(); }
                break;
        }
    }
    lastTrillClkVal = trillClkVal;
    if (resetTrillClk) { resetTrillClk = false; }

    auto panClkVal = panClk.Process();
    if ((panClkVal == 1) && lastPanClkVal == 0) {
        runglerPan.setThreshold(0.0f);
        runglerPan.shift(0.5f * (oscAValue + oscBValue));
        // if (runglerPan.getFirstBit()) { 
        //     rungOutPan = runglerPan.getOutput();
        //     panLTarget = QuarterSine(1.0f - rungOutPan);
        //     panRTarget = QuarterSine(rungOutPan);
        // }

        rungOutPan = runglerPan.getOutput();
        panLTarget = QuarterSine(1.0f - rungOutPan);
        panRTarget = QuarterSine(rungOutPan);

    }
    lastPanClkVal = panClkVal;
    fonepole(panL, panLTarget, slowSmoothCoef);
    fonepole(panR, panRTarget, slowSmoothCoef);

    fonepole(modDepth, fclamp(1.25f * params_.modDepth, 0.0f, 1.0f) * params_.modMult, smoothCoef);
    fonepole(morph, params_.morph, smoothCoef);

    //fonepole(vowelMorph, rungOutPan, smoothCoef);

    touchEnvLevel = touchEnv.Process(gate);
    trillEnvALevel = trillEnvA.Process();
    trillEnvBLevel = trillEnvB.Process();

    switch (params_.modMode) {
        case (MOD_MODE_1):
            morphModA = modDepth * trillEnvALevel;
            morphModB = modDepth * trillEnvBLevel;
            break;
        case (MOD_MODE_2):
            morphModA = modDepth * 0.0f;
            morphModB = modDepth * 0.0f;
            break;
        case (MOD_MODE_3):
            morphModA = modDepth * 0.0f; 
            morphModB = modDepth * 0.0f;
            break;
    }

    mA = fclamp(morph + morphModA, 0.0f, 1.0f);
    mB = fclamp(morph + morphModB, 0.0f, 1.0f);
    oscA.SetPhaseDistortion(mA);
    oscB.SetPhaseDistortion(mB);

    oscAValue = oscA.Process();
    oscBValue = oscB.Process();

    float outA = trillEnvALevel * oscAValue;
    float outB = trillEnvBLevel * oscBValue;

    float mix = xFade.Process(outA, outB);

    float out =  touchEnvLevel * mix;

    if (stealFade) {

        out *= stealGain;

        if (!fadeUp) {
            stealGain -= STEAL_STEP;
            if (stealGain <= 0.0f) {
                Start(pendingPitch);
                gate = pendingGate;
                UpdateParams(params_);
                fadeUp = true;
            }
        }

        if (fadeUp) {
            stealGain += STEAL_STEP;
            if (stealGain >= 1.0f) {
                stealGain = 1.0f;
                fadeUp = false;
                stealFade = false;
            }
        }
    }

    return out;
}

float Voice::QuarterSine(float x)
{
    float x2 = x * x;
    return x * (1.570f + x2 * (-0.643f + x2 * 0.0727f));
}

float Voice::GetPanLeft()
{
    return panL;
}

float Voice::GetPanRight()
{
    return panR;
}

float Voice::XOR16f(float f1, float f2)
{
    int16_t a = static_cast<int16_t>(f1 * 32767.0f);
    int16_t b = static_cast<int16_t>(f2 * 32767.0f);

    int16_t result = a ^ b;

    return static_cast<float>(result / 32768.0f);
}

void Voice::Start(float p) { 
    pitch = p;
    resetTrillClk = true;
    gate = true;
}


void Voice::StealFadeStart(float p) {
    stealFade = true;
    fadeUp = false;
    pendingPitch = p;
    pendingGate = true;
}

void Voice::Release() {
    if (stealFade && !fadeUp) pendingGate = false; // release arrived before the pending start
    gate = false;
}


float Voice::GetPitch() {
    return pitch;
}

bool Voice::IsGateOn() { return gate == true; }
bool Voice::IsActive() { return touchEnv.IsRunning(); }
bool Voice::IsInRelease() { return touchEnv.GetCurrentSegment() == ADSR_SEG_RELEASE; }
bool Voice::IsInAttack() { return touchEnv.GetCurrentSegment() == ADSR_SEG_ATTACK; }
float Voice::GetTouchEnvLvl() { return touchEnvLevel; }