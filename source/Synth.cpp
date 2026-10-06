#include "Synth.h"

void Synth::Init(float sampleRate, size_t blockSize)
{
    float onePoleFreq = 100.0f;
    levelSmoother.Init();
    levelSmoother.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    levelSmoother.SetFrequency(onePoleFreq / sampleRate);

    for(int v = 0; v < NUM_VOICES; ++v) {
        voice[v].Init(sampleRate, blockSize);
    }
}

void Synth::Process(daisy::AudioHandle::OutputBuffer &out, size_t size)
{
    float mixL;
    float mixR;
    float level;

    for(int v = 0; v < NUM_VOICES; ++v) {
        if (voice[v].IsActive())
            voice[v].UpdateParams(params);
    }
    
    for(size_t i = 0; i < size; ++i) {
        mixL = 0.0f;
        mixR = 0.0f;

        for(int v = 0; v < NUM_VOICES; ++v) {
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
}

void Synth::PadTouch(int pad)
{
    if (pad == 10) { levelTarget = fmax(levelTarget - 0.1, 0.1); }
    if (pad == 11) { levelTarget = fmin(levelTarget + 0.1, 1.0); }

    if (pad == 0) { octave = fmax(octave * 0.5, 0.125); }
    if (pad == 2) { octave = fmin(octave * 2.0, 8.0); }

    if (SCALE_MAP[pad] != NULL_PLACE) {
        VoiceStart(pad);
    }
}

void Synth::PadRelease(int pad)
{
    if (SCALE_MAP[pad] != NULL_PLACE) {
        VoiceRelease(pad);
    } 
}

void Synth::VoiceStart(int owner)
{   
    // If called by a pad
    float pitch = octave * SCALES[scaleIndex][SCALE_MAP[owner]];
    
    // If called by midi note
    // float pitch = ....

    int v = FindFreeVoice(owner);

    // if voice must be faded, call fade start
    // else start voice normally
    int numActive = 0;
    for(int v = 0; v < NUM_VOICES; ++v) { if (voice[v].IsActive()) { ++numActive; } }
    if ((numActive == NUM_VOICES) && (voice[v].voiceOwnerPrev != owner)) { 
        voice[v].StealFadeStart(pitch);
    }
    else { 
        voice[v].Start(pitch); 
    }

    // voice[v].Start(pitch);

    voice[v].voiceOwner = owner;
    voice[v].voiceOwnerPrev = -1;
    lastPlayedVoice = v;
}

void Synth::VoiceRelease(int owner)
{
    for(int v = 0; v < NUM_VOICES; ++v) {
        if (voice[v].voiceOwner == owner) {
            voice[v].Release();
            voice[v].voiceOwnerPrev = owner;
            voice[v].voiceOwner = -1;
            return;
        }
    }
}

int Synth::FindFreeVoice(int owner)
{
    int v = 0;
    float l = 100.0f;
    int numUnReleased = 0;
    for(int i = 0; i < NUM_VOICES; ++i) {

        if (voice[i].voiceOwnerPrev == owner && voice[i].voiceOwner == -1) { return i; } // diables unison like behavior

        if (!voice[i].IsReleased()) { ++numUnReleased; }

        if(voice[i].touchEnvLevel < l && !voice[i].IsInAttack()) {
            l = voice[i].touchEnvLevel;
            v = i;
        }
    }

    if (numUnReleased == NUM_VOICES) {
        return lastPlayedVoice;
    }

    return v;
}