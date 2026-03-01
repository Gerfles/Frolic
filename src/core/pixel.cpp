//>--- pixel.cpp ---<//
#include "pixel.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // TODO implement a pixel class that extrapolates any image format specifics, etc
  // that way we can just call pixel.r and get the right value
  // NOTE: looking at the pixel raw data, it appears the order of the bits is different
  // than might be expected
  // AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
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
