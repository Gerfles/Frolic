// pixel.hpp

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include "platform.hpp"
#include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>


namespace fc
{
  // ?? friend function perhaps?
  // Wrapper class to handle any pesky conversions between formats, etc.
  // ?? Could rename class to localImage to emphasize its use
  class FcPixel
  {
   private:
     u64 mValue;
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
