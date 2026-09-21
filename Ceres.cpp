#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/mpr121.h"

#include "TrillClock.h"

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

Oscillator oscA;
Oscillator oscB;
AdEnv trillEnvA;
AdEnv trillEnvB;

TrillClock trillClk;
std::array<uint8_t, 2> lastTrillClkVals;

bool  padPressed[12];
float padPressure[12];
int   switchAValue;
int   switchBValue;
float potValue[8];

constexpr float MIN_TRILL_FREQ  = 0.1f;   
constexpr float MAX_TRILL_FREQ  = 200.0f;  

constexpr float MIN_OSC_FREQ  = 40.0f;   
constexpr float MAX_OSC_FREQ  = 6000.0f; 

// Touch sensor thresholds
constexpr uint8_t kTouchThreshold = 12;
constexpr uint8_t kReleaseThreshold = 6;

// Pressure tuning
constexpr float kPressureMaxDelta    = 100.0f; // counts past baseline that equal full pressure (1.0), tune for your pads
constexpr float kPressureSmoothing   = 0.4f;  // 0 to 1, higher = faster response, lower = smoother
constexpr bool  kPressureExponential = true; // true squares the curve for finer control at light touch
static_assert(kPressureMaxDelta > kTouchThreshold, "kPressureMaxDelta must be larger than kTouchThreshold");

// Estimate how hard a pad is pressed, 0 to 1.
float ReadPadPressure(int pad)
{
    int32_t filtered = mpr.FilteredData(pad) & 0x03FF; 
    int32_t baseline = mpr.BaselineData(pad);          
    int32_t delta    = baseline - filtered;            

    // Map touch threshold .. max delta onto 0 .. 1
    float norm = (float)(delta - kTouchThreshold) / (kPressureMaxDelta - kTouchThreshold);
    if(norm < 0.0f) norm = 0.0f;
    if(norm > 1.0f) norm = 1.0f;

    return kPressureExponential ? (norm * norm) : norm;
}

void OnPadTouch(int pad)   { (void)pad; }
void OnPadRelease(int pad) { (void)pad; }


void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    float trillFreq = potValue[6];
    float distance = potValue[7];

    float oscAFreq = potValue[0];
    float oscBFreq = potValue[5];

    float widthA = potValue[1];
    float widthB = potValue[4];


    float baseTrillFreq = MIN_TRILL_FREQ + trillFreq * (MAX_TRILL_FREQ - MIN_TRILL_FREQ);
    float baseOscAFreq = MIN_OSC_FREQ + oscAFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);
    float baseOscBFreq = MIN_OSC_FREQ + oscBFreq * (MAX_OSC_FREQ - MIN_OSC_FREQ);

    oscA.SetFreq(baseOscAFreq);
    oscB.SetFreq(baseOscBFreq);

    trillClk.SetFreq(baseTrillFreq);
    trillClk.SetDistance(distance);
    float trillPeriod = 1.0f / baseTrillFreq;

    // // float skew = 
    // float skewAtk = 0.5f;
    // float skewDec = 0.5f;
    // float envAtkA = trillPeriod * skewAtk * widthA;
    // float envDecA = trillPeriod * skewDec * widthA;
    // float envAtkB = trillPeriod * skewAtk * widthB;
    // float envDecB = trillPeriod * skewDec * widthB;
    // trillEnvA.SetTime(ADENV_SEG_ATTACK, envAtkA);
    // trillEnvA.SetTime(ADENV_SEG_DECAY, envDecA);
    // trillEnvB.SetTime(ADENV_SEG_ATTACK, envAtkB);
    // trillEnvB.SetTime(ADENV_SEG_DECAY, envDecB);
    float max = 1.5f;
    float multA = max * 0.5f;
    float multB = max * 0.5f;

    trillEnvA.SetTime(ADENV_SEG_ATTACK, trillPeriod * multA);
    trillEnvA.SetTime(ADENV_SEG_DECAY, trillPeriod * multA);
    trillEnvB.SetTime(ADENV_SEG_ATTACK, trillPeriod * multB);
    trillEnvB.SetTime(ADENV_SEG_DECAY, trillPeriod * multB);

    auto trillClkVals = trillClk.Process();
    if ((trillClkVals[0] > 0) && (lastTrillClkVals[0] == 0)) {
        trillEnvA.Trigger();
    }
    if ((trillClkVals[1] > 0) && (lastTrillClkVals[1] == 0)) {
        trillEnvB.Trigger();
    }
    lastTrillClkVals = trillClkVals;

    for(size_t i = 0; i < size; i++)
    {
        float out1 = trillEnvA.Process() * 0.2f * oscA.Process();
        float out2 = trillEnvB.Process() * 0.2f * oscB.Process();

        float mix = out1 + out2;

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

    // hardware.StartLog();

    // Initialize touch sensor
    Mpr121I2C::Config mprConfig;
    mpr.Init(mprConfig);
    mpr.SetThresholds(kTouchThreshold, kReleaseThreshold);
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
    oscA.Init(sampleRate);
    oscB.Init(sampleRate);

    trillEnvA.Init(sampleRate);
    trillEnvA.SetMin(0.0);
    trillEnvA.SetMax(1.f);
    trillEnvA.SetCurve(0);

    trillEnvB.Init(sampleRate);
    trillEnvB.SetMin(0.0);
    trillEnvB.SetMax(1.f);
    trillEnvB.SetCurve(0.0f);

    trillClk.Init(2.0f, sampleRate);


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
                    padPressure[i] += (target - padPressure[i]) * kPressureSmoothing;
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