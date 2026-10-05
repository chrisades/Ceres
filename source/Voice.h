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
        bool IsInRelease();
        void UpdateParams(const VoiceParams& params);
        void SetPitch(float p);

        void Start();
        void Release();
        bool IsInAttack();
        void Reset();

        bool hold = false;

        int voiceOwner = -1;
        int voiceOwnerPrev = -1;
        float touchEnvLevel = 0.0f;

        // voice steal smoother params
        bool steal = false;


    private:

        WavetableOsc wtOscA{TABLES.SIN_TABLE};
        WavetableOsc wtOscB{TABLES.SIN_TABLE};

        AdEnv trillEnvA;
        AdEnv trillEnvB;
        Adsr touchEnv;
        TrillClock trillClk;
        std::array<uint8_t, 2> lastTrillClkVal;

        VoiceParams params_;

        bool resetTrillClk;
        bool gate = false;
        float pitch = 1.0f;


        //values that need smoothing
        float pdAmtA = 0.5f;
        float pdAmtB = 0.5f;
        float smoothCoef;
};