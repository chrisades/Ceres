#include "Synth.h"

void Synth::Init(float sampleRate, size_t blockSize)
{
    float onePoleFreq = 100.0f;
    levelSmoother.Init();
    levelSmoother.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    levelSmoother.SetFrequency(onePoleFreq / sampleRate);

    for(int v = 0; v < NUM_VOICES; v++) {
        voice[v].Init(sampleRate, blockSize);
    }
}

void Synth::Process(daisy::AudioHandle::OutputBuffer &out, size_t size)
{
    float mixL;
    float mixR;
    float level;
    
    for(size_t i = 0; i < size; i++) {
        mixL = 0.0f;
        mixR = 0.0f;

        for(int v = 0; v < NUM_VOICES; v++) {
            mixL += voice[v].Process();
        }

        mixR = mixL;

        level = levelSmoother.Process(levelTarget);

        out[0][i] = SoftClip(0.3f * level * mixL);
        out[1][i] = SoftClip(0.3f * level * mixR);
    }
}

void Synth::UpdateParams()
{
    params.oscAFreq = potValue[0];
    params.widthA = potValue[1];
    params.shape = potValue[2];
    params.distance = potValue[3];
    params.widthB = potValue[4];
    params.oscBFreq = potValue[5];
    params.trillFreq = potValue[6];
    params.morph = potValue[7];
    params.switchAValue = switchAValue;
    params.switchBValue = switchBValue;

    //TESTING
    voice[0].pitch = 1.0f;
    // voice[1].pitch = 1.5f;
    // voice[2].pitch = 0.5f;

    for(int v = 0; v < NUM_VOICES; v++) {
        // if (voice[v].IsActive()) 
            voice[v].UpdateParams(params);
    }
}

void Synth::PadTouch(int pad)
{
    if (pad == 10) { levelTarget = fmax(levelTarget - 0.1, 0.1); }
    if (pad == 11) { levelTarget = fmin(levelTarget + 0.1, 1.0); }

    if (pad == 3) {   
        voice[0].pitch = 1.0f;
        voice[0].Start();
    }  

    if (pad == 4) {   
        voice[1].pitch = 1.5f;
        voice[1].Start();
    }  
}

void Synth::PadRelease(int pad)
{
    if (pad == 3) {   
        voice[0].Stop();
    }  

    if (pad == 4) {   
        voice[1].Stop();
    }  
}
