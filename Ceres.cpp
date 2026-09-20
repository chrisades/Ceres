#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/mpr121.h"

using namespace daisy;
using namespace seed;
using namespace daisysp;

DaisySeed hardware;

Mpr121I2C mpr;
Switch3 switchA;
Switch3 switchB;
AnalogControl pots[8];

bool  padPressed[12];
float padPressure[12]; // values go from 0 to 1, 0 when the pad is not touched
int   switchAValue;
int   switchBValue;
float potValue[8];

// Touch sensor thresholds in raw capacitance counts (these are the libDaisy defaults)
constexpr uint8_t kTouchThreshold   = 12;
constexpr uint8_t kReleaseThreshold = 6;

// Pressure tuning
constexpr float kPressureMaxDelta    = 60.0f; // counts past baseline that equal full pressure (1.0), tune for your pads
constexpr float kPressureSmoothing   = 0.4f;  // 0 to 1, higher = faster response, lower = smoother
constexpr bool  kPressureExponential = false; // true squares the curve for finer control at light touch
static_assert(kPressureMaxDelta > kTouchThreshold, "kPressureMaxDelta must be larger than kTouchThreshold");

void OnPadTouch(int pad)   { (void)pad; }
void OnPadRelease(int pad) { (void)pad; }

// Estimate how hard a pad is pressed, 0 to 1.
// A harder press flattens the finger, which increases contact area and capacitance,
// so the filtered reading drops further below the baseline.
float ReadPadPressure(int pad)
{
    int32_t filtered = mpr.FilteredData(pad) & 0x03FF; // 10 bit live reading
    int32_t baseline = mpr.BaselineData(pad);          // already scaled to 10 bit by libDaisy
    int32_t delta    = baseline - filtered;            // grows as pressure increases

    // Map touch threshold .. max delta onto 0 .. 1
    float norm = (float)(delta - kTouchThreshold) / (kPressureMaxDelta - kTouchThreshold);
    if(norm < 0.0f) norm = 0.0f;
    if(norm > 1.0f) norm = 1.0f;

    return kPressureExponential ? (norm * norm) : norm;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hardware.SetAudioBlockSize(4);

    hardware.StartLog();

    // Initialize touch sensor
    Mpr121I2C::Config mprConfig;
    mprConfig.touch_threshold   = kTouchThreshold;
    mprConfig.release_threshold = kReleaseThreshold;
    mpr.Init(mprConfig);
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

        hardware.PrintLine("pressure: %f% ", padPressure[3]);
        System::Delay(30);

        // Set switch values
        switchAValue = switchA.Read(); // 2 == left, 0 == center, 1 == right
        switchBValue = switchB.Read(); // 2 == left, 0 == center, 1 == right

        // Set pot values
        for(int i = 0; i < 8; i++)
            potValue[i] = pots[i].Process(); // values go from 0 to 1

        System::Delay(4);
    }
}