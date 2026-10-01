#pragma once
#include <stdint.h>
#include "daisysp.h"

// Global Voice Information
constexpr uint8_t NUM_VOICES = 1;
constexpr float MIN_TRILL_FREQ  = 0.3f;   
constexpr float MAX_TRILL_FREQ  = 30.0f;  
constexpr float MIN_OSC_FREQ  = 40.0f;   
constexpr float MAX_OSC_FREQ  = 1000.0f; 

constexpr int NUM_SCALES = 4;
constexpr float SCALES[NUM_SCALES][7] = {{1.0f, 1.12246f, 1.25992, 1.49831f, 1.68179, 2.0f, 2.51984f}, //Pentatonic
                            {1.0f, 1.12246f, 1.25992f, 1.33484f, 1.49831f, 1.68179f, 2.0f}, //Major
                            {1.0f, 1.18921f, 1.33484f, 1.49831f, 1.78180f, 2.0f, 2.37841f}, // Pentatonic Minor
                            {1.0f, 1.18921f, 1.33484f, 1.41421f, 1.49831f, 1.78180f, 2.0f}}; //Blues
constexpr int NULL_PLACE = -1;
constexpr int SCALE_MAP[12] = {NULL_PLACE, NULL_PLACE, NULL_PLACE, 0, 4, 5, 6, 3, 1, 2, NULL_PLACE, NULL_PLACE};


// Global Wavetable Information
constexpr int TABLE_SIZE = 256; //Must be a power of 2
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
inline WaveTables TABLES; //make constexpr. needs above to be constexpr