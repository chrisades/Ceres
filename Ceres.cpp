#include "daisy_seed.h"
#include "daisysp.h"
#include "dev/mpr121.h"
#include "Synth.h"
#include "Log.h"

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
// MidiUsbHandler midi;

constexpr uint8_t PAD_TOUCH_THRESH = 12;
constexpr uint8_t PAD_RELEASE_THRESH = 6;

bool hold = false;

void OnPadTouch(int pad) { synth.PadTouch(pad); }

void OnPadRelease(int pad) { synth.PadRelease(pad); }

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    synth.Process(out, size);
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hardware.SetAudioBlockSize(96);

    HW::hw().setHW(&hardware);
	//HW::hw().startLog();

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


// void ProcessMidi()
// {
//         /** Listen to MIDI for new changes */
//         midi.Listen();

//         /** When there are messages waiting in the queue... */
//         while(midi.HasEvents())
//         {
//             /** Pull the oldest one from the list... */
//             auto msg = midi.PopEvent();
//             switch(msg.type)
//             {
//                 case NoteOn:
//                 {
//                     auto note_msg = msg.AsNoteOn();
//                     // if(note_msg.velocity != 0)
//                         // osc.SetFreq(mtof(note_msg.note));
//                 }
//                 break;

//                 case NoteOff:
//                 {
//                     auto note_msg = msg.AsNoteOff();
//                     // ProcessNoteOff(note_msg.note);
//                 }
//                 break;

//                 default: break;
//             }
//         }
// }