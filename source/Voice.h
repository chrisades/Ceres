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
        bool IsReleased();
        void UpdateParams(const VoiceParams& params);

        // void UpdateParamsAudioCallback(const VoiceParams& params);

        void Start();
        void Release();

        bool IsInAttack();

        void Reset();

        float pitch = 1.0f;
        
        bool hold = false;

        TrillClock trillClk;

        int voiceOwner = -1;

        float touchEnvLevel = 0.0f;

    private:

        WavetableOsc wtOscA{TABLES.SIN_TABLE};
        WavetableOsc wtOscB{TABLES.SIN_TABLE};

        AdEnv trillEnvA;
        AdEnv trillEnvB;
        Adsr touchEnv;
        std::array<uint8_t, 2> lastTrillClkVal;

        bool gate = false;

        //values that need smoothing
        float pdAmtA = 0.5f;
        float pdAmtB = 0.5f;
        // float pitchMult = 1.0f;
        float smoothCoef;
};