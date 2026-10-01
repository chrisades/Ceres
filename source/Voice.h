#include "daisysp.h"
#include "Config.h"
#include "TrillClock.h"
#include "WavetableOsc.h"

using namespace daisysp;

struct VoiceParams {
    float trillFreq;
    float distance;
    float oscAFreq;
    float oscBFreq;
    float widthA;
    float widthB;
    float shape;
    float morph;
    uint8_t switchAValue;
    uint8_t switchBValue;
};

class Voice {
    public:
        void Init(float sampleRate, size_t blockSize);
        float Process();
        bool IsActive();
        void UpdateParams(const VoiceParams& params);

        void Start();
        void Stop();

        float pitch = 1.0f;
        float octave = 1.0f;
        bool hold = false;

    private:

        WavetableOsc wtOscA{TABLES.SIN_TABLE};
        WavetableOsc wtOscB{TABLES.SIN_TABLE};

        AdEnv trillEnvA;
        AdEnv trillEnvB;
        Adsr touchEnv;
        TrillClock trillClk;
        std::array<uint8_t, 2> lastTrillClkVal;

        bool gate = false;
};