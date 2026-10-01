#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/mpr121.h"
#include "Synth.h"

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

Synth synth;

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


bool hold = false;

void OnPadTouch(int pad) { synth.PadTouch(pad);
     
    // if (SCALE_MAP[pad] != NULL_PLACE) {
    //     pitch = SCALES[scaleIndex][SCALE_MAP[pad]];
    // }

    // if (pad == 1) { 
    //     scaleIndex = ((scaleIndex + 1) % NUM_SCALES); 

    //     for(int i = 0; i < 12; i++) {
    //         if (padPressed[i] && SCALE_MAP[i] != NULL_PLACE) {
    //             pitch = SCALES[scaleIndex][SCALE_MAP[i]];
    //         }   
    //     }
    // }

    // if (pad == 0) { octave = fmax(octave * 0.5, 0.125); }
    // if (pad == 2) { octave = fmin(octave * 2.0, 8.0); }
 }

void OnPadRelease(int pad) { synth.PadRelease(pad); }

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    synth.Process(out, size);
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    hardware.SetAudioBlockSize(4);

    float sampleRate = hardware.AudioSampleRate();
    size_t blockSize = hardware.AudioBlockSize();

    Mpr121I2C::Config mprConfig;
    mpr.Init(mprConfig);
    mpr.SetThresholds(PAD_TOUCH_THRESH, PAD_RELEASE_THRESH);
    uint16_t prevPadState = 0;

    switchA.Init(D9, D8); // S09/S10
    switchB.Init(D7, D6); // S07/S08

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

    synth.Init(sampleRate, blockSize);

    hardware.StartAudio(AudioCallback);

    for(;;)
    {
        uint16_t state = mpr.Touched();
        for(int i = 0; i < 12; i++) {
            bool isTouched  = state & (1 << i);
            bool wasTouched = prevPadState & (1 << i);

            if(isTouched) {
                float target = ReadPadPressure(i);
                if(wasTouched)
                    synth.padPressure[i] += (target - synth.padPressed[i]) * PRESS_SMOOTHING;
                else
                    synth.padPressure[i] = target;
            } else {
                synth.padPressure[i] = 0.0f;
            }

            if(isTouched && !wasTouched) {
                OnPadTouch(i);
            }
            else if(wasTouched && !isTouched) { 
                OnPadRelease(i) ;
            }
            synth.padPressed[i] = isTouched;
        }
        prevPadState = state;

        synth.switchAValue = switchA.Read();
        synth.switchBValue = switchB.Read();

        for(int i = 0; i < 8; i++)
            synth.potValue[i] = pots[i].Process(); 

        synth.UpdateParams();

        System::Delay(4);
    }
}