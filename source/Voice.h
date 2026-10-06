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

        void Start(float p);
        void StealFadeStart(float p);

        void Release();
        bool IsInAttack();

        bool hold = false;

        int voiceOwner = -1;
        int voiceOwnerPrev = -1;
        float touchEnvLevel = 0.0f;


    private:
        WavetableOsc wtOscA{TABLES.SIN_TABLE};
        WavetableOsc wtOscB{TABLES.SIN_TABLE};

        AdEnv trillEnvA;
        AdEnv trillEnvB;
        Adsr touchEnv;
        TrillClock trillClk;
        std::array<uint8_t, 2> lastTrillClkVal;

        VoiceParams params_;

        bool resetTrillClk = false;
        bool gate = false;
        float pitch = 1.0f;

        
        // voice steal smoother params
        bool stealFade = false;
        bool fadeUp = false;
        float pendingPitch = 1.0f;
        float STEAL_STEP;
        float stealGain = 1.0f;


        //values that need smoothing
        float pdAmtA = 0.5f;
        float pdAmtB = 0.5f;
        float smoothCoef;
};