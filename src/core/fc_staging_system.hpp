//>--- fc_staging_system.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_handle.hpp"
#include "fc_commands.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <deque>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc { class Frolic; class FcBuffer; class FcImage; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcStagingSystem
  {
   private:
     //
     static constexpr u8 STAGING_BUFFER_BITS_ALIGNMENT = 16; // BC7 compression requires 16bit alignment
     //
     struct MemoryRegionInfo
     {
        u64 size {0};
        u64 mOffset {0};
        // TODO rename specific to command buffers
        SubmitHandle mHandle {};
     };
     //
     Frolic& mFcContext;
     FcHolder<FcHandle<FcBuffer>> mStagingBuffer;
     u32 mStagingBufferCounter {0};
     // Allows the staging buffer to alter size within the bounds of mMinBufferSize and mMaxBufferSize
     VkDeviceSize mStagingBufferSize {0};
     // ?? if this doesn't grow/shrink make constexpr
     const VkDeviceSize mMaxBufferSize {128ull * 1024ull * 1024ull};
     const VkDeviceSize mMinBufferSize {4u * 2048u * 2048u};
     std::deque<MemoryRegionInfo> mMemoryRegions;
     //
     MemoryRegionInfo getNextFreeOffset(u32 size);
     //
     void ensureStagingBufferSize(u32 sizeRequired);
     //
     void waitAndReset();
     //
   public:
     //
     explicit FcStagingSystem(Frolic& fcContext);
     //
     void bufferSubData(FcBuffer& buffer, sizeT size, sizeT dstOffset, const void* data);
     //
     void imageData2D(FcImage& image, const VkRect2D& imageRegion, u32 baseMipLeve,
                      u32 numMipLevels, u32 layer, u32 numLayers, VkFormat format,
                      const void* data);
     // TODO implement imageData3D

  };


} // --- namespace fc --- (END)
