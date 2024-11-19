#pragma once


// Frolic Engine
//#include "core/fc_descriptors.hpp"
#include "fc_buffer.hpp"
// external libraries
#include "vulkan/vulkan_core.h"
// std libraries
#include<string>

namespace fc
{
   // TODO create new texture class that inherits from image
   // FORWARD DECLARATIONS
//  class FcPipeline;


  class FcImage
  {
   private:
      // TODO could get rid of most of these devices if pased to the destroy function ??
     VkImage mImage;
     VkDeviceMemory mImageMemory;
     VkImageView mImageView;
     VkSampler mTextureSampler = VK_NULL_HANDLE;
     uint32_t mWidth;
     uint32_t mHeight;
     uint32_t mMipLevels = 1;

     // void extracted(VkImageLayout& oldLayout,
     //                VkImageLayout& newLayout,
     //                VkImageMemoryBarrier& imageMemoryBarrier);
     void generateMipMaps(uint32_t mipLevels);
     void createTextureSampler(uint32_t mipLevels = 1);
     void writeToTexture(void* pixelData, uint32_t mipLevels = 1);
   public:
      // - CTORS -
     FcImage(std::string filename) { loadTexture(filename); }
     FcImage() = default;
     FcImage(VkImage image);
     FcImage& operator=(const FcImage&) = delete;
     FcImage(const FcImage&) = delete;
     FcImage& operator=(FcImage&&) = default;
     FcImage(FcImage&&) = default;

      // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better
      // way... maybe unique_ptr
      //FcImage(const FcImage&) = delete;
      //void operator=(const VkImage& image) {mImage = image;}
     void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

     void create(uint32_t width, uint32_t height, VkFormat format
                 , VkSampleCountFlagBits msaaSampleCount, VkImageTiling tiling, VkImageUsageFlags useFlags
                 , uint32_t mipLevels, VkMemoryPropertyFlags properties, VkImageAspectFlags aspectFlags);
     void createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
     void copyImage(FcBuffer& srcBuffer, VkDeviceSize bufferSize);
     void clear(VkClearColorValue* pColor);
      // TEXTURE FUNCTIONS
     uint32_t loadTexture(std::string filename);
     uint32_t createTexture(int width, int height, void* pixelData, uint32_t mipLevels = 1);

     void overwriteTexture(void* pixelData, uint32_t mipLevel = 1);
      // GETTERS
     VkImageView& ImageView() { return mImageView; }
     VkSampler TextureSampler() { return mTextureSampler; }
     VkImage Image() { return mImage; }
      // cleanup
     ~FcImage() = default;
     void destroyImageView();
     void destroy();
  };


} // namespace fc _END_
