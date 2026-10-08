#include "daisysp.h"
#include "daisy.h"
#include "Config.h"
#include "Voice.h"
#include "Log.h"

class Synth {
public:
    void Init(float sampleRate, size_t blockSize);

    void PadTouch(int pad);
    void PadRelease(int pad);

    void VoiceStart(int owner);
    void VoiceRelease(int owner);

    void Process(daisy::AudioHandle::OutputBuffer &out, size_t size);

    void UpdateParams();

    bool  padPressed[12];
    float padPressure[12];
    int   switchAValue;
    int   switchBValue;
    float potValue[8];
    bool hold = false;

private:
    Voice voice[NUM_VOICES];
    VoiceParams params;

    daisysp::OnePole levelSmoother;
    float levelTarget = 0.4f;
    float octave= 2.0f;
    int scaleIndex = 0;
    int lastPlayedVoice = 0;

    bool lastHold = false;

    void ReleaseHeldVoices();

    int FindFreeVoice(float pitch);
};