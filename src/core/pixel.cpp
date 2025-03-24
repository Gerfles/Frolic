// pixel.cpp
#include "pixel.hpp"

namespace fc
{




  FcPixel::FcPixel(FcImage image)
  {
    mBytesPerChanel = image.byteDepth();


  }

  float FcPixel::normalize()
  {
    uint32_t x = 0;


    typeid(mBytesPerChanel).name();
  }



}// --- namespace fc --- (END)
