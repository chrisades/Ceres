#pragma once

#include <cstdint>

class Rungler {
public:
  explicit Rungler(uint8_t seed = 0x2D);

  void setThreshold(float threshold);

  float getThreshold() const;

  void shift(float data);

  int getFirstBit() const;

  float getOutput() const;

private:
  uint8_t reg;
  float threshold;
};