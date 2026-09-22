#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/mpr121.h"

#include "Config.h"
#include "TrillClock.h"
#include "WavetableOsc.h"

const char* USBD_MANUFACTURER_STRING = "ChrisAdes";
const char* USBD_PRODUCT_STRING_HS = "Ceres";
const char* USBD_PRODUCT_STRING_FS = "Ceres";

using namespace daisy;
using namespace seed;
using namespace daisysp;

DaisySeed hardware;
Mpr121I2C mpr;
Switch3 switchA;
Switch3 switchB;
AnalogControl pots[8];

struct WaveTables {
    float sinTable[TABLE_SIZE];
    float sawTable[TABLE_SIZE];

    WaveTables() {
        SinTableFill(sinTable);
    }
};
WaveTables gTables;
WavetableOsc wtOscA(gTables.sinTable);
WavetableOsc wtOscB(gTables.sinTable);

AdEnv trillEnvA;
AdEnv trillEnvB;
Adsr touchEnv;

TrillClock trillClk;
std::array<uint8_t, 2> lastTrillClkVal;

Wavefolder wFold;

bool  padPressed[12];
float padPressure[12];
int   switchAValue;
int   switchBValue;
float potValue[8];

// Touch sensor thresholds
constexpr uint8_t PAD_TOUCH_THRESH = 12;
constexpr uint8_t PAD_RELEASE_THRESH = 6;

// Pressure tuning
constexpr float PRESS_MAX_DELTA    = 100.0f; // counts past baseline that equal full pressure (1.0), tune for your pads
constexpr float PRESS_SMOOTHING   = 0.4f;  // 0 to 1, higher = faster response, lower = smoother
constexpr bool  PRESS_MODE = true; // true squares the curve for finer control at light touch
static_assert(PRESS_MAX_DELTA > PAD_TOUCH_THRESH, "PRESS_MAX_DELTA must be larger than PAD_TOUCH_THRESH");

// Estimate how hard a pad is pressed, 0 to 1.
float ReadPadPressure(int pad)
{
    int32_t filtered = mpr.FilteredData(pad) & 0x03FF; 
    int32_t baseline = mpr.BaselineData(pad);          
    int32_t delta    = baseline - filtered;            

    // Map touch threshold .. max delta onto 0 .. 1
    float norm = (float)(delta - PAD_TOUCH_THRESH) / (PRESS_MAX_DELTA - PAD_TOUCH_THRESH);
    if(norm < 0.0f) norm = 0.0f;
    if(norm > 1.0f) norm = 1.0f;

    return PRESS_MODE ? (norm * norm) : norm;
}

OnePole levelSmoother;
float levelTarget = 0.6f;
OnePole pitchGlide;
float pitchTarget = 1.0f; 
float octave = 1.0f;

int scaleIndex = 0;
constexpr int NUM_SCALES = 4;
float SCALES[NUM_SCALES][7] = {{1.0f, 1.12246f, 1.25992, 1.49831f, 1.68179, 2.0f, 2.51984f}, //Pentatonic
                               {1.0f, 1.12246f, 1.25992f, 1.33484f, 1.49831f, 1.68179f, 2.0f}, //Major
                               {1.0f, 1.18921f, 1.33484f, 1.49831f, 1.78180f, 2.0f, 2.37841f}, // Pentatonic Minor
                               {1.0f, 1.18921f, 1.33484f, 1.41421f, 1.49831f, 1.78180f, 2.0f}}; //Blues
constexpr int NULL_PLACE = -1;
int SCALE_MAP[12] = {NULL_PLACE, NULL_PLACE, NULL_PLACE, 0, 4, 5, 6, 3, 1, 2, NULL_PLACE, NULL_PLACE};


void OnPadTouch(int pad) { 
     
    if (SCALE_MAP[pad] != NULL_PLACE) {
        pitchTarget = SCALES[scaleIndex][SCALE_MAP[pad]];
    }

    if (pad == 1) { 
        scaleIndex = ((scaleIndex + 1) % NUM_SCALES); 

        for(int i = 0; i < 12; i++) {
            if (padPressed[i] && SCALE_MAP[i] != NULL_PLACE) {
                pitchTarget = SCALES[scaleIndex][SCALE_MAP[i]];
            }   
        }
    }

    if (pad == 0) { octave = fmax(octave * 0.5, 0.125); }
    if (pad == 2) { octave = fmin(octave * 2.0, 8.0); }

    if (pad == 10) { levelTarget = fmax(levelTarget - 0.1, 0.1); }
    if (pad == 11) { levelTarget = fmin(levelTarget + 0.1, 1.0); }
 }

void OnPadRelease(int pad) { 
    if (SCALE_MAP[pad] != NULL_PLACE) {
        for(int i = 0; i < 12; i++) {
            if (padPressed[i] && SCALE_MAP[i] != NULL_PLACE) {
                pitchTarget = SCALES[scaleIndex][SCALE_MAP[i]];
            }   
        }
    }
 }

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float trillFreq = potValue[6];
    float distance = potValue[7];
    float oscAFreq = potValue[0];
    float oscBFreq = potValue[5];
    float widthA = potValue[1];
    float widthB = potValue[4];
    float shape = potValue[2];
    float fold = potValue[3];

    // float baseTrillFreq = MIN_TRILL_FREQ + trillFreq * (MAX_TRILL_FREQ - MIN_TRILL_FREQ);
    // float baseOscAFreq = MIN_OSC_FREQ + oscAFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);
    // float baseOscBFreq = MIN_OSC_FREQ + oscBFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);
    float pitchMult = octave * pitchGlide.Process(pitchTarget);

    float baseTrillFreq = pitchMult * fmap(trillFreq, MIN_TRILL_FREQ, MAX_TRILL_FREQ);
    float baseOscAFreq = pitchMult * fmap(oscAFreq, MIN_OSC_FREQ, MAX_OSC_FREQ);
    float baseOscBFreq = pitchMult * fmap(oscBFreq, MIN_OSC_FREQ, MAX_OSC_FREQ);

    wtOscA.SetFreq(baseOscAFreq);
    wtOscA.SetPhaseDistortion(fold);
    wtOscB.SetFreq(baseOscBFreq);
    wtOscB.SetPhaseDistortion(fold);

    trillClk.SetFreq(baseTrillFreq);
    trillClk.SetDistance(distance);
    float trillPeriod = 1.0f / baseTrillFreq;

    float minWidth = 0.1f * trillPeriod;
    float maxWidth = 4.0f * trillPeriod;

    float baseWidthA = minWidth + widthA * (maxWidth - minWidth);
    float baseWidthB = minWidth + widthB * (maxWidth - minWidth);

    float skew = 0.1 + shape * 0.8f;
    float skewAtk = skew;
    float skewDec = 1.0f - skew;

    float trillEnvAtkA = skewAtk * baseWidthA;
    float trillEnvDecA = skewDec * baseWidthA;
    float trillEnvAtkB = skewAtk * baseWidthB;
    float trillEnvDecB = skewDec * baseWidthB;

    trillEnvA.SetTime(ADENV_SEG_ATTACK, trillEnvAtkA);
    trillEnvA.SetTime(ADENV_SEG_DECAY, trillEnvDecA);
    trillEnvB.SetTime(ADENV_SEG_ATTACK, trillEnvAtkB);
    trillEnvB.SetTime(ADENV_SEG_DECAY, trillEnvDecB);

    auto trillClkVal = trillClk.Process();
    if ((trillClkVal[0] == 1) && (lastTrillClkVal[0] == 0)) {
        trillEnvA.Trigger();
    }
    if ((trillClkVal[1] == 1) && (lastTrillClkVal[1] == 0)) {
        trillEnvB.Trigger();
    }
    lastTrillClkVal = trillClkVal;

    float env = touchEnv.Process(padPressed[3]
            || padPressed[4] || padPressed[5] || padPressed[6]
            || padPressed[7] || padPressed[8] || padPressed[9]);

    for(size_t i = 0; i < size; i++)
    {
        float level = levelSmoother.Process(levelTarget);

        float out1 = trillEnvA.Process() * 0.3f * wtOscA.Process();
        float out2 = trillEnvB.Process() * 0.3f * wtOscB.Process();
        float mix = SoftClip(level * env * (out1 + out2));

        // float preFoldGain = 1.0f + fold * 9.0f;
        // float postFoldGain = 1.0f - 0.9f * fold;
        // wFold.SetGain(preFoldGain); //use one pole to smooth
        // // wFold.SetOffset();
        // float fold = postFoldGain * wFold.Process(mix);

        out[0][i] = mix;
        out[1][i] = mix;
    }
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hardware.SetAudioBlockSize(4);

    float sampleRate = hardware.AudioSampleRate();
    float blockSize = hardware.AudioBlockSize();

    //hardware.StartLog();

    // Initialize touch sensor
    Mpr121I2C::Config mprConfig;
    mpr.Init(mprConfig);
    mpr.SetThresholds(PAD_TOUCH_THRESH, PAD_RELEASE_THRESH);
    uint16_t prevPadState = 0;

    // Initialize switches
    switchA.Init(D9, D8); // S09/S10
    switchB.Init(D7, D6); // S07/S08

    // Initialize pots
    AdcChannelConfig adcConfig[8];
    adcConfig[0].InitSingle(A0); // S30
    adcConfig[1].InitSingle(A1); // S31
    adcConfig[2].InitSingle(A2); // S32
    adcConfig[3].InitSingle(A3); // S33
    adcConfig[4].InitSingle(A4); // S34
    adcConfig[5].InitSingle(A5); // S35
    adcConfig[6].InitSingle(A6); // S36
    adcConfig[7].InitSingle(A7); // S37
    hardware.adc.Init(adcConfig, 8);
    for(int i = 0; i < 8; i++) {
        pots[i].Init(hardware.adc.GetPtr(i), hardware.AudioCallbackRate());
    }
    hardware.adc.Start();

    // Initialize Audio Objects
    wtOscA.Init(sampleRate);
    wtOscB.Init(sampleRate);

    trillEnvA.Init(sampleRate);
    trillEnvA.SetMin(0.0);
    trillEnvA.SetMax(1.f);
    trillEnvA.SetCurve(0);

    trillEnvB.Init(sampleRate);
    trillEnvB.SetMin(0.0);
    trillEnvB.SetMax(1.f);
    trillEnvB.SetCurve(0.0f);

    touchEnv.Init(sampleRate, blockSize);
    touchEnv.SetAttackTime(0.5f);
    touchEnv.SetDecayTime(0.2f);
    touchEnv.SetSustainLevel(1.0f);
    touchEnv.SetReleaseTime(1.0f);

    trillClk.Init(2.0f, sampleRate);
    wFold.Init();

    float onePoleFreq = 100.0f; // lower = slower/longer, higher = snappier
    pitchGlide.Init();
    pitchGlide.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    pitchGlide.SetFrequency(onePoleFreq / sampleRate);

    levelSmoother.Init();
    levelSmoother.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    levelSmoother.SetFrequency(onePoleFreq / sampleRate);

    hardware.StartAudio(AudioCallback);

    for(;;)
    {
        // Set touch pad information
        uint16_t state = mpr.Touched();
        for(int i = 0; i < 12; i++) {
            bool isTouched  = state & (1 << i);
            bool wasTouched = prevPadState & (1 << i);

            // Update pressure first so it is already valid inside OnPadTouch / OnPadRelease.
            // Only touched pads are read, since each read is its own I2C transaction.
            if(isTouched) {
                float target = ReadPadPressure(i);
                if(wasTouched)
                    padPressure[i] += (target - padPressure[i]) * PRESS_SMOOTHING;
                else
                    padPressure[i] = target; // snap on the initial strike, no smoothing lag
            } else {
                padPressure[i] = 0.0f;
            }

            if(isTouched && !wasTouched)       OnPadTouch(i);
            else if(wasTouched && !isTouched)  OnPadRelease(i);
            padPressed[i] = isTouched;
        }
        prevPadState = state;

        // hardware.PrintLine("padPressure: " FLT_FMT3, FLT_VAR3(padPressure[3]));
        // System::Delay(30);

        // Set switch values
        switchAValue = switchA.Read(); // 2 == left, 0 == center, 1 == right
        switchBValue = switchB.Read(); // 2 == left, 0 == center, 1 == right

        // Set pot values
        for(int i = 0; i < 8; i++)
            potValue[i] = pots[i].Process(); // values go from 0 to 1

        System::Delay(4);
    }
}