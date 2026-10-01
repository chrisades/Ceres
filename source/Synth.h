#include "daisysp.h"
#include "daisy.h"
#include "Config.h"
#include "Voice.h"

class Synth {
public:
    void Init(float sampleRate, size_t blockSize);

    void PadTouch(int pad);

    void PadRelease(int pad);

    void Process(daisy::AudioHandle::OutputBuffer &out, size_t size);

    void UpdateParams();

    bool  padPressed[12];
    float padPressure[12];
    int   switchAValue;
    int   switchBValue;
    float potValue[8];

private:
    Voice voice[NUM_VOICES];
    VoiceParams params;

    daisysp::OnePole levelSmoother;
    float levelTarget = 0.6f;
    
    int scaleIndex = 0;
};