//>--- fc_staging_system.cpp ---<//
#include "fc_staging_system.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_assert.hpp"
#include "utilities.hpp"
#include "fc_locator.hpp"
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  FcStagingSystem::MemoryRegionInfo FcStagingSystem::getNextFreeOffset(u32 size)
  {
    const u32 requestedAlignedSize = getAlignedSize(size, STAGING_BUFFER_BITS_ALIGNMENT);

    ensureStagingBufferSize(requestedAlignedSize);

    FC_ASSERT(!mMemoryRegions.empty());

    // Find the next available region big enough to store requested size, and if we can't find a region
    // big enough, return whatever region we were able to find
    auto bestNextRegionIt = mMemoryRegions.begin();

    // Used to check that our memory region is not currently in-use
    VulkanImmediateCommands& commands = FcLocator::Renderer().mImmediateCommands;

    for (auto it = mMemoryRegions.begin(); it != mMemoryRegions.end(); ++it)
    {
      // First make sure our Immediate command buffer system is not currently using this region
      if (commands.isReady(it->mHandle))
      {
        // Check and make sure this region is big enough
        if (it->size >= requestedAlignedSize)
        {
          const u32 unusedSize = it->size - requestedAlignedSize;
          const u32 unusedOffset = it->mOffset + requestedAlignedSize;

          mMemoryRegions.erase(it);

          if (unusedSize > 0)
          {
            /* mMemoryRegions.push_front({unusedSize, unusedOffset, SubmitHandle()}); */
            // add the unused size back to the memory regions dequeu
            mMemoryRegions.insert(mMemoryRegions.begin(), {unusedSize, unusedOffset, SubmitHandle()});
          }

          return {requestedAlignedSize, it->mOffset, SubmitHandle()};
        }

        // Cache the largest available region if it's not as big as the one we're looking for
        if (it->size > bestNextRegionIt->size)
        {
          bestNextRegionIt = it;
        }
      }
    }

    // If we're here then the best we could do was to find a region smaller than the requested size
    // For now it's the best we can do, so return it and upload will proceed in chuncks
    if (bestNextRegionIt != mMemoryRegions.end() && commands.isReady(bestNextRegionIt->mHandle))
    {
      mMemoryRegions.erase(bestNextRegionIt);

      return {bestNextRegionIt->size, bestNextRegionIt->mOffset, SubmitHandle()};
    }

    // If we got this far, nothing was available, wait for the entire staging buffer to become available
    waitAndReset();

    // waitAndReset() adds a region that spans the entire buffer.
    mMemoryRegions.clear();

    // First, figure out if we have any left over space we need to deal with
    const u64 unusedSize = mStagingBufferSize > requestedAlignedSize ? mStagingBufferSize - requestedAlignedSize : 0;

    // place unused portion (if it exists) back in the memory regions dequeu
    if (unusedSize)
    {
      const u64 unusedOffset = mStagingBufferSize - unusedSize;
      mMemoryRegions.insert(mMemoryRegions.begin(), {unusedSize, unusedOffset, SubmitHandle()});
      // ?? why is above preferred?
      /* mMemoryRegions.push_front({unusedSize, unusedOffset, SubmitHandle()}); */
    }

    return {mStagingBufferSize - unusedSize, 0, SubmitHandle()};
  }


  //
  void FcStagingSystem::ensureStagingBufferSize(u32 sizeRequired)
  {
    const u32 alignedSize = std::max(getAlignedSize(sizeRequired, STAGING_BUFFER_BITS_ALIGNMENT), mMinBufferSize);

    /* sizeRequired = alignedSize <  */
  }


  //
  void FcStagingSystem::waitAndReset()
  {
    for (const MemoryRegionInfo& memRegion : mMemoryRegions)
    {
      FcLocator::Renderer().mImmediateCommands.wait(memRegion.mHandle);
    }

    mMemoryRegions.clear();
    // TODO emplace back for most push_backs within FcStagingSystem
    mMemoryRegions.push_back({mStagingBufferSize, 0, SubmitHandle()});
  }

} // --- namespace fc --- (END)
