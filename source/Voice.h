#include "daisysp.h"
#include "daisy.h"
#include "Config.h"
#include "TrillClock.h"
#include "WavetableOsc.h"
#include "Rungler.h"

enum
{
    TOUCH_ENV_FAST,
    TOUCH_ENV_SLOW,
    TOUCH_ENV_HOLD,
    NUM_TOUCH_ENV_MODES
};

enum
{
    TRILL_ENV_SHAPE_1,
    TRILL_ENV_SHAPE_2,
    TRILL_ENV_SHAPE_3,
    NUM_TRILL_ENV_MODES
};

enum
{
    TRACKING_BOTH,
    TRACKING_OSC,
    NUM_TRACKING_MODES
};

enum
{
    MOD_MODE_1,
    MOD_MODE_2,
    MOD_MODE_3,
    NUM_MOD_MODES
};

enum
{
    TRILL_TRIG_MODE_1,
    TRILL_TRIG_MODE_2,
    NUM_TRILL_TRIG_MODES
};

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
    float modDepth;
    float touchEnvAtk  = 10.0f;
    float touchEnvRel  = 10.0f;
    float fxAmt = 0.0f;
    uint8_t touchEnvMode = TOUCH_ENV_SLOW;
    uint8_t trillEnvMode = TRILL_ENV_SHAPE_2;
    uint8_t trackingMode = TRACKING_BOTH;
    uint8_t morphMode = PD_MODE_1;
    uint8_t modMode = MOD_MODE_1;
    uint8_t trillTrigMode = TRILL_TRIG_MODE_1;
    float modMult = 1.0f;
    bool pitchSwap = false;
};

class Voice {
    public:
        void Init(float sampleRate, size_t blockSize);
        float Process();
        bool IsActive();
        bool IsInRelease();
        void UpdateParams(const VoiceParams& params);

        void Start(float p);
        void StealFadeStart(float p);

        void Release();
        bool IsInAttack();
        bool IsGateOn();
        
        float GetPitch();
        float GetTouchEnvLvl();

        float GetPanLeft();
        float GetPanRight();

        int voiceOwner = -1;


    private:
        WavetableOsc oscA{TABLES.SIN_TABLE};
        WavetableOsc oscB{TABLES.SIN_TABLE};

        AdEnv trillEnvA;
        AdEnv trillEnvB;
        Adsr touchEnv;
        TrillClock trillClk;
        std::array<uint8_t, 2> lastTrillClkVal;

        Rungler runglerA;
        float rungOutA = 0.0f;
        Rungler runglerB;
        float rungOutB = 0.0f;

        Metro panClk;
        uint8_t lastPanClkVal;
        Rungler runglerPan;
        float rungOutPan = 0.0f;
        float panL = 0.707f;
        float panR = 0.707f;
        float panLTarget = 0.707f;
        float panRTarget = 0.707f;
        float slowSmoothCoef;

        // VowelFilter vFilt;
        // int phoneme1;
        // int phoneme2;
        // float vowelMorph;

        float widthAMod = 0.0f;
        float widthBMod = 0.0f;

        float fxAmt = 0.0f;

        VoiceParams params_;

        float rungModA = 1.0f;
        float rungModB = 1.0f;
        float fmodA = 1.0f;
        float fmodB = 1.0f;
        float fmodTrill = 1.0f;
        float morphModA = 0.0f;
        float morphModB = 0.0f;

        CrossFade xFade;
        CrossFade xFadeFX;

        float XOR16f(float f1, float f2);
        float QuarterSine(float x);

        bool resetTrillClk = false;
        bool gate = false;
        float pitch = 1.0f;
        float touchEnvLevel = 0.0f;
        
        // voice steal smoother params
        bool stealFade = false;
        bool fadeUp = false;
        float pendingPitch = 1.0f;
        bool pendingGate = true;
        float STEAL_STEP;
        float stealGain = 1.0f;

        float oscAValue = 0.0f;
        float oscBValue = 0.0f;    

        float mA = 0.0f;
        float mB = 0.0f;
        
        //values that need smoothing
        float morph = 0.5f;
        float xFadeAmt = 0.5f;
        float xFadeFXAmt = 0.5f;
        float modDepth = 0.0f;
        float smoothCoef;

        float baseOscAFreq = 1.0f;
        float baseOscBFreq = 1.0f;;
        float baseTrillFreq = 1.0f;;

        float trillEnvALevel = 1.0f;
        float trillEnvBLevel = 1.0f;
};