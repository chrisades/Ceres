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
int   switchAValue;
int   switchBValue;
float potValue[8];

void OnPadTouch(int pad)   { (void)pad; }
void OnPadRelease(int pad) { (void)pad; }

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

    // Initialize touch sensor
    Mpr121I2C::Config mprConfig;
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
            if(isTouched && !wasTouched)       OnPadTouch(i);
            else if(wasTouched && !isTouched)  OnPadRelease(i);
            padPressed[i] = isTouched;
        }
        prevPadState = state;

        // Set switch values
        switchAValue = switchA.Read(); // 2 == left, 0 == center, 1 == right
        switchBValue = switchB.Read(); // 2 == left, 0 == center, 1 == right

        // Set pot values
        for(int i = 0; i < 8; i++)
            potValue[i] = pots[i].Process(); // values go from 0 to 1

        System::Delay(4);
    }
}