#include "Rungler.h"

Rungler::Rungler(uint8_t seed) : reg(seed), threshold(0.f) {
  if (reg == 0) reg = 1;
}

void Rungler::setThreshold(float t) {
  if (t < -1.f) t = -1.f;
  if (t > 1.f) t = 1.f;
  threshold = t;
}

float Rungler::getThreshold() const {
  return threshold;
}

void Rungler::shift(float data) {
  uint8_t dataBit = (threshold > data) ? 1 : 0;

  uint8_t outgoing = (reg >> 7) & 1;

  reg = static_cast<uint8_t>((reg << 1) | (dataBit ^ outgoing));
}

int Rungler::getFirstBit() const {
  return reg & 1;
}

float Rungler::getOutput() const {
  int value = ((reg >> 5) & 1)      
            | (((reg >> 6) & 1) << 1) 
            | (((reg >> 7) & 1) << 2);
  return static_cast<float>(value) / 7.f;
}