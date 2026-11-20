#pragma once
#include <stdint.h>
#include "daisysp.h"

// Global Voice Information
constexpr uint8_t NUM_VOICES = 5;
constexpr float MIN_TRILL_FREQ  = 0.2f;   
constexpr float MAX_TRILL_FREQ  = 40.0f;  
constexpr float MIN_OSC_FREQ  = 30.0f;   
constexpr float MAX_OSC_FREQ  = 1000.0f; 

constexpr int NUM_PAD_SCALES = 1;
constexpr float PAD_SCALES[NUM_PAD_SCALES][7] = {
    // Fifths and Fourths
    {0.33371f, 0.5f, 0.66741f, 1.0f, 1.33484f, 1.49831f, 2.0f},
};
constexpr int NULL_PLACE = -1;
constexpr int PAD_SCALE_MAP[12] = {NULL_PLACE, NULL_PLACE, NULL_PLACE, 3, 1, 0, 2, 4, 5, 6, NULL_PLACE, NULL_PLACE};


// Global Wavetable Information
constexpr int TABLE_SIZE = 4096; //Must be a power of 2
inline void SinTableFill(float (&table)[TABLE_SIZE]) 
{
        for(int i = 0; i < TABLE_SIZE; i++) {
            float x = (static_cast<float>(i) / TABLE_SIZE - 1) * TWOPI_F;
            table[i] = sinf(x);
        }
}
struct WaveTables {
    float SIN_TABLE[TABLE_SIZE];

    WaveTables() {
        SinTableFill(SIN_TABLE);
    }
};
inline WaveTables TABLES;