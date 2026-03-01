//>--- pixel.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // ?? friend function perhaps?
  // Wrapper class to handle any pesky conversions between formats, etc.
  // ?? Could rename class to localImage to emphasize its use
  class FcPixel
  {
   private:
     u64 mValue;
     u64 mNormalizationFactor;
     int mBytesPerChanel;
   public:
     FcPixel(FcImage image);
     float normalize();
     void setPosition(uint32_t x, uint32_t y);
     VkFormat format();
     void log();
     void print();
     uint32_t Value();
     int numChannels();
  };

}// --- namespace fc --- (END)
