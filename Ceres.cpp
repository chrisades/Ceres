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

ZOscillator zOsc1, zOsc2;
Oscillator lfo;

Adsr env;

OnePole levelSmoother;
float levelTarget = 0.6f;

OnePole pitchGlide;
float pitchTarget = 1.0f; 

OnePole ampModMultSmoother;
float ampModMultTarget = 0.0f;

float octave = 1.0f;

constexpr float MIN_FREQ  = 10.0f;   
constexpr float MAX_FREQ  = 200.0f;  

constexpr float FORMANT_MIN_FREQ  = 40.0f;   
constexpr float FORMANT_MAX_FREQ  = 1500.0f; 

constexpr float LFO_MIN_FREQ  = 0.05f;   
constexpr float LFO_MAX_FREQ  = 20.0f; 

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
    float freq1 = potValue[1];
    float freq2 = potValue[4];

    float formantFreq1 = potValue[2];
    float formantFreq2 = potValue[3];

    float shape = potValue[6];
    float mode = 0.15f * 2.0f * potValue[7] - 1.0f;

    float lfoFreq = potValue[0];
    float lfoAmount = potValue[5];
 
    float baseFreq1 = MIN_FREQ + freq1 * (MAX_FREQ - MIN_FREQ);
    float baseFreq2 = MIN_FREQ + freq2 * (MAX_FREQ - MIN_FREQ);

    float baseFormantFreq1 = FORMANT_MIN_FREQ + formantFreq1 * (FORMANT_MAX_FREQ - FORMANT_MIN_FREQ);
    float baseFormantFreq2 = FORMANT_MIN_FREQ + formantFreq2 * (FORMANT_MAX_FREQ - FORMANT_MIN_FREQ);

    float baseLfoFreq = LFO_MIN_FREQ + lfoFreq * (LFO_MAX_FREQ - LFO_MIN_FREQ);

    bool hold = false;
    switch (switchBValue) {
        case 2:
            env.SetAttackTime(10.0f);
            env.SetReleaseTime(5.0f);
            hardware.SetLed(false);
            break;
        case 0: 
            env.SetAttackTime(0.1f);
            env.SetReleaseTime(0.5f);
            hardware.SetLed(false);
            break;
        case 1:
            hold = true;
            hardware.SetLed(true);
            break;
    }
 
    for(size_t i = 0; i < size; i++)
    {
        float totalEnv = env.Process(hold ||padPressed[3]
            || padPressed[4] || padPressed[5] || padPressed[6]
            || padPressed[7] || padPressed[8] || padPressed[9]);

        float pitchMult = octave * pitchGlide.Process(pitchTarget);

        lfoFreq = baseLfoFreq;
        lfo.SetFreq(lfoFreq);
        lfo.SetAmp(lfoAmount);
        float lfoMod = lfo.Process();
        float freqModMult = 0.0f;
        float formantFreqModMult = 0.0f;
        ampModMultTarget = 0.0f;
        switch (switchAValue) {
            case 2:
                freqModMult = 50.0f;
                break;
            case 0: 
                formantFreqModMult = 400.0f;
                break;
            case 1:
                ampModMultTarget = 1.0f;
                break;
        }
        float ampModMult = ampModMultSmoother.Process(ampModMultTarget);

        freq1 = pitchMult * baseFreq1 + lfoMod * freqModMult;
        freq2 = pitchMult * baseFreq2 + lfoMod * freqModMult;

        formantFreq1 = pitchMult * baseFormantFreq1 + lfoMod * formantFreqModMult;
        formantFreq2 = pitchMult * baseFormantFreq2 + lfoMod * formantFreqModMult;
 
        zOsc1.SetFreq(freq1);
        zOsc1.SetFormantFreq(formantFreq1);
        zOsc1.SetMode(mode);
        zOsc1.SetShape(shape);

        zOsc2.SetFreq(freq2);
        zOsc2.SetFormantFreq(formantFreq2);
        zOsc2.SetMode(mode);
        zOsc2.SetShape(shape);
 
        float out1 = zOsc1.Process() * (1.0f + lfoMod * ampModMult);
        float out2 = zOsc2.Process() * (1.0f + lfoMod * ampModMult);
 
        float level = levelSmoother.Process(levelTarget);

        float mixL = totalEnv * 0.25f * level * (0.76f * out1 + 0.65f * out2);
        float mixR = totalEnv * 0.25f * level * (0.65f * out1 + 0.76f * out2);
 
        out[0][i] = SoftClip(mixL);
        out[1][i] = SoftClip(mixR);
    }
}

int main(void)
{
    hardware.Init();
    hardware.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hardware.SetAudioBlockSize(4);
    const float sampleRate = hardware.AudioSampleRate();
    const int blockSize = hardware.AudioBlockSize();

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

    zOsc1.Init(sampleRate);
    zOsc2.Init(sampleRate);

    lfo.Init(sampleRate);

    float onePoleFreq = 15.0f; // lower = slower/longer, higher = snappier

    pitchGlide.Init();
    pitchGlide.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    pitchGlide.SetFrequency(onePoleFreq / sampleRate);

    levelSmoother.Init();
    levelSmoother.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    levelSmoother.SetFrequency(onePoleFreq / sampleRate);

    ampModMultSmoother.Init();
    ampModMultSmoother.SetFilterMode(OnePole::FILTER_MODE_LOW_PASS);
    ampModMultSmoother.SetFrequency(onePoleFreq / sampleRate);

    env.Init(sampleRate, blockSize);

    hardware.adc.Start();

    hardware.StartAudio(AudioCallback);

    for(;;)
    {
        // Set touch pad information
        uint16_t state = mpr.Touched();
        for(int i = 0; i < 12; i++) {
            bool isTouched  = state & (1 << i);
            bool wasTouched = prevPadState & (1 << i);
            padPressed[i] = isTouched;
            if(isTouched && !wasTouched)       OnPadTouch(i);
            else if(wasTouched && !isTouched)  OnPadRelease(i);
        }
        prevPadState = state;

        // Set switch values
        switchAValue = switchA.Read();
        switchBValue = switchB.Read();

        // Set pot values
        for(int i = 0; i < 8; i++)
            potValue[i] = pots[i].Process();

        System::Delay(4);
    }
}