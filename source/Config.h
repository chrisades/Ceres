#pragma once

constexpr int TABLE_SIZE = 256; //Must be a power of 2

constexpr float MIN_TRILL_FREQ  = 0.3f;   
constexpr float MAX_TRILL_FREQ  = 30.0f;  

constexpr float MIN_OSC_FREQ  = 40.0f;   
constexpr float MAX_OSC_FREQ  = 1000.0f; 

inline void SinTableFill(float (&table)[TABLE_SIZE]) 
{
        for(int i = 0; i < TABLE_SIZE; i++) {
            float x = (static_cast<float>(i) / TABLE_SIZE - 1) * TWOPI_F;
            table[i] = sinf(x);
        }
}