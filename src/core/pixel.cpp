// pixel.cpp
#include "pixel.hpp"

namespace fc
{




  FcPixel::FcPixel(FcImage image)
  {
    mBytesPerChanel = image.byteDepth();

    switch (mBytesPerChanel)
    {
        case(2):
        {
          mNormalizationFactor = UINT16_MAX;
          break;
        }
        case(4):
        {
          mNormalizationFactor = UINT32_MAX;
          break;
        }
        default:
          break;
    }
  }

  float FcPixel::normalize()
  {
    return static_cast<double>(mValue) / mNormalizationFactor;
  }



}// --- namespace fc --- (END)
