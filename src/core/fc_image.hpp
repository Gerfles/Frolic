#pragma once


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_descriptors.hpp"
//#include "core/utilities.hpp"
//#include "fc_buffer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include<string>



namespace fc
{

  class FcBuffer;

   // TODO create new texture class that inherits from image
   // FORWARD DECLARATIONS
//  class FcPipeline;


  class FcImage
  {
   private:
      // TODO could get rid of most of these devices if pased to the destroy function ??
     VkImage mImage{nullptr};
     VkImageView mImageView{nullptr};
     VmaAllocation mAllocation{nullptr};
     VkSampler mTextureSampler{nullptr};
     // TODO track layout of image in transitions
     // NOTE this layout may not be exactly up to date based on image barriers in GPU etc.

      // TODO create a synced state variable to track current image layout
      // VkImageLayout mCurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
     VkExtent3D mImageExtent;
     uint32_t mMipLevels = 1;

     // void extracted(VkImageLayout& oldLayout,
     //                VkImageLayout& newLayout,
     //                VkImageMemoryBarrier& imageMemoryBarrier);
     void generateMipMaps(uint32_t mipLevels);
     void createTextureSampler(uint32_t mipLevels = 1);
     void writeToTexture(void* pixelData, uint32_t mipLevels = 1);
   public:
      // TODO DELETE
       void imageToGpu(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

      // - CTORS -
     FcImage(std::string filename) { loadTexture(filename); }
     FcImage() = default;
     FcImage(VkImage image);
     ~FcImage() = default;

     FcImage& operator=(const FcImage&) = delete;
     FcImage(const FcImage&) = delete;
     FcImage& operator=(FcImage&&) = default;
     FcImage(FcImage&&) = default;
     void create(VkExtent3D imgExtent, VkFormat format
                 , VkSampleCountFlagBits msaaSampleCount, VkImageUsageFlags useFlags
                 , VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
                 , uint32_t mipLevels = 1);
     void createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1);

      // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better
      // way... maybe unique_ptr
      //FcImage(const FcImage&) = delete;
      //void operator=(const VkImage& image) {mImage = image;}

     // TODO eliminate one of the following
      //void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
     void transitionImage(VkCommandBuffer cmd, VkImageLayout oldLayout
                          , VkImageLayout newLayout, uint32_t mipLevels = 1);

     void copyFromBuffer(FcBuffer& srcBuffer, VkDeviceSize bufferSize);
     void copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source, VkExtent2D srcSize, VkExtent2D dstSize);
     void clear(VkCommandBuffer cmdBuffer, VkClearColorValue* pColor);
      // TEXTURE FUNCTIONS
     void loadTexture(std::string filename);
     void createTexture(VkExtent3D extent, void* pixelData, uint32_t mipLevels = 1);
      //void overwriteTexture(void* pixelData, uint32_t mipLevel = 1);
      // GETTERS
     const VkImageView& ImageView() const { return mImageView; }
     VkSampler TextureSampler() { return mTextureSampler; }
     VkImage Image() { return mImage; }
     VkExtent3D getExtent() { return mImageExtent; }
     VkExtent2D size() { return {mImageExtent.width, mImageExtent.height}; }
      // cleanup
//     ~FcImage() = default;
     void destroyImageView();
     void destroy();
  };


} // namespace fc _END_
