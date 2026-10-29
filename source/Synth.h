#include "daisysp.h"
#include "daisy.h"
#include "Config.h"
#include "Voice.h"
#include "Log.h"
#include "Delay.h"

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
    int   switchAValue;
    int   switchBValue;
    float potValue[8];

private:
    Voice voice[NUM_VOICES];
    VoiceParams params;

    Delay delay1L;
    Delay delay2L;
    Delay delay1R;
    Delay delay2R;
    float delTime1 = 1.0f;
    float delTime2 = 1.0f;
    float delSmoothCoef;

    CrossFade delXFade;
    float delXFadeAmt = 0.0f;


    float octave = 1.0f;
    uint8_t octaveIndex = 0;
    int padScaleIndex = 0;
    int lastPlayedVoice = 0;

    bool hold = false;
    bool lastHold = false;

    void ReleaseHeldVoices();

    int FindFreeVoice(float pitch);
};