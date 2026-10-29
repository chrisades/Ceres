#include "Synth.h"

Delay::Line DSY_SDRAM_BSS delayLines[4];

void Synth::Init(float sampleRate, size_t blockSize)
{
    for(int v = 0; v < NUM_VOICES; ++v) {
        voice[v].Init(sampleRate, blockSize);
    }

    delay1L.Init(sampleRate, &delayLines[0]);
    delay2L.Init(sampleRate, &delayLines[1]);
    delay1R.Init(sampleRate, &delayLines[2]);
    delay2R.Init(sampleRate, &delayLines[3]);
    delSmoothCoef = 1.0f / (0.07f * sampleRate);
    delXFade.Init(CROSSFADE_EXP);

}

void Synth::Process(daisy::AudioHandle::OutputBuffer &out, size_t size)
{
    float mixL;
    float mixR;
    float level;

    for(int v = 0; v < NUM_VOICES; ++v) {
        voice[v].UpdateParams(params);
    }
    
    for(size_t i = 0; i < size; ++i) {
        mixL = 0.0f;
        mixR = 0.0f;

        for(int v = 0; v < NUM_VOICES; ++v) {
            float sample = voice[v].Process();

            mixL += voice[v].GetPanLeft() * sample;
            mixR += voice[v].GetPanRight() * sample;
        }

        level = 0.15f;

        // float width = 2.0f * potValue[6];
        // float mid  = 0.5f * (mixL + mixR);
        // float side = 0.5f * (mixL - mixR) * width;
        // mixL = mid + side;
        // mixR = mid - side;

        // fonepole(delTime1, 0.001f + 0.1f * potValue[1], delSmoothCoef); 
        // fonepole(delTime2, 0.001f + 0.1f * (1.0f - potValue[4]), delSmoothCoef); 
        // fonepole(delTime1, 1.0f / (fmap(potValue[0], MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG)), delSmoothCoef); 
        // fonepole(delTime2, 1.0f / (fmap(potValue[5], MIN_OSC_FREQ, MAX_OSC_FREQ, Mapping::LOG)), delSmoothCoef); 
        fonepole(delTime1, 1.0f / (fmap(potValue[3], 0.4f, 20.0f, Mapping::LOG)), delSmoothCoef); 
        fonepole(delTime2, 1.0f / (fmap(1.0f - potValue[3], 0.4f, 20.0f, Mapping::LOG)), delSmoothCoef); 

        delay1L.SetDelayS(delTime1);
        delay1R.SetDelayS(delTime1);
        delay2L.SetDelayS(delTime2);
        delay2R.SetDelayS(delTime2);

        // float feed1 = 0.5f;
        // float feed2 = 0.5f;
        float feed1 = 0.8f * (1.0f - potValue[2]);
        float feed2 = 0.8f * potValue[2];
        delay1L.SetFeedback(feed1);
        delay1R.SetFeedback(feed1);
        delay2L.SetFeedback(feed2);
        delay2R.SetFeedback(feed2);
        float delMixL = 2.0f * delay2L.Process(delay1L.Process(mixL));
        float delMixR = 2.0f * delay2R.Process(delay1R.Process(mixR));

        fonepole(delXFadeAmt, fclamp(5.0f * potValue[6] - 4.0f, 0.0f, 1.0f), delSmoothCoef);
        delXFade.SetPos(delXFadeAmt);

        out[0][i] = SoftClip(level * delXFade.Process(mixL, delMixL));
        out[1][i] = SoftClip(level * delXFade.Process(mixR, delMixR));
    }
}


void Synth::UpdateParams()
{
    params.oscAFreq = potValue[0];
    params.widthA = potValue[1];
    params.distance = potValue[2];
    params.modDepth = potValue[6];
    params.widthB = potValue[4];
    params.oscBFreq = potValue[5];
    params.trillFreq = potValue[3];
    params.morph = potValue[7];

    switch (switchAValue) {
        case (2):
            params.modMode = MOD_MODE_1;
            // params.fxAmt = 0.0f;
            break;
        case (0):
            params.modMode = MOD_MODE_2;
            // params.fxAmt = 0.5f;
            break;
        case (1):
            params.modMode = MOD_MODE_3;
            // params.fxAmt = 1.0f;
            break;
    }

    switch (switchBValue) {
        case (2):
            params.morphMode = PD_MODE_1;
            break;
        case (0):
            params.morphMode = PD_MODE_2;
            break;
        case (1):
            params.morphMode = PD_MODE_3;
            break;
    }

    switch (params.trillEnvMode) {
        case (TRILL_ENV_SHAPE_1):
            params.shape = 0.0f;
            break;
        case (TRILL_ENV_SHAPE_2):
            params.shape = 0.5f;
            break;
        case (TRILL_ENV_SHAPE_3):
            params.shape = 1.0f;
            break;
    }

    lastHold = hold;
    switch (params.touchEnvMode) { // decide attack and release for touch env
        case (TOUCH_ENV_SLOW):
            params.touchEnvAtk = 15.0f;
            params.touchEnvRel = 15.0f;
            hold = false;
            break;
        case (TOUCH_ENV_FAST):
            params.touchEnvAtk = 0.5f;
            params.touchEnvRel = 1.0f;
            hold = false;
            break;
        case (TOUCH_ENV_HOLD):
            params.touchEnvAtk = 10.0f;
            params.touchEnvRel = 1.0f;
            hold = true;
            break;
    }
    HW::hw().setLed(hold);
    if (lastHold == true && hold == false) {
        ReleaseHeldVoices();
    }

}

void Synth::PadTouch(int pad)
{
    if (pad == 10) { 
        //params.trillEnvMode = ((params.trillEnvMode + 1) % NUM_TRILL_ENV_MODES); 
        params.trillTrigMode = ((params.trillTrigMode + 1) % NUM_TRILL_TRIG_MODES);
    }

    if (pad == 11) { params.touchEnvMode = ((params.touchEnvMode + 1) % NUM_TOUCH_ENV_MODES); }

    if (pad == 0) { 
        octaveIndex = (octaveIndex + 1) % 4;
        octave = exp2f(static_cast<float>(octaveIndex)); 
    }
    //if (pad == 0) { octave = fmax(octave * 0.5, 0.125); }
    //if (pad == 2) { octave = fmin(octave * 2.0, 8.0); }  

    if (pad == 1) { padScaleIndex = ((padScaleIndex + 1) % NUM_PAD_SCALES); }

    if (pad == 2) { params.trackingMode = ((params.trackingMode + 1) % NUM_TRACKING_MODES); }

    if (PAD_SCALE_MAP[pad] != NULL_PLACE) {
        VoiceStart(pad);
    }
}

void Synth::PadRelease(int pad)
{
    if (PAD_SCALE_MAP[pad] != NULL_PLACE) {
        VoiceRelease(pad);
    } 
}

void Synth::VoiceStart(int owner)
{   
    // If called by a pad
    float pitch = octave * PAD_SCALES[padScaleIndex][PAD_SCALE_MAP[owner]];
    
    // If called by midi note
    // float pitch = ....

    int v = FindFreeVoice(pitch);
    
    if (hold && (voice[v].voiceOwner != -1)) {
        voice[v].Release();
        voice[v].voiceOwner = -1;
    }
    
    else {
        if (voice[v].GetPitch() != pitch) {
            voice[v].StealFadeStart(pitch);
        }
        else { 
            voice[v].Start(pitch); 
        }

        lastPlayedVoice = v;
        voice[v].voiceOwner = owner;
    }
}

void Synth::VoiceRelease(int owner)
{ 
    if(!hold) {
        for(int v = 0; v < NUM_VOICES; ++v) {
            if (voice[v].voiceOwner == owner) {
                voice[v].Release();
                voice[v].voiceOwner = -1;
            }
        }
    }
}

void Synth::ReleaseHeldVoices()
{
    for(int v = 0; v < NUM_VOICES; ++v) {
        voice[v].Release();
        voice[v].voiceOwner = -1;
    }
}

int Synth::FindFreeVoice(float pitch)
{
    int v = 0;
    float l = 100.0f;
    int numSustained = 0;
    for(int i = 0; i < NUM_VOICES; ++i) {

        if(voice[i].voiceOwner != -1) { ++numSustained; }

        if(voice[i].GetPitch() == pitch) { return i; }

        float level = voice[i].GetTouchEnvLvl();
        if(level < l && !voice[i].IsInAttack()) {
            l = level;
            v = i;
        }
    }

    if(numSustained == NUM_VOICES) { 
        if(hold) {
            voice[lastPlayedVoice].Release();
            voice[lastPlayedVoice].voiceOwner = -1;
        }
        return lastPlayedVoice; }

    return v;
}