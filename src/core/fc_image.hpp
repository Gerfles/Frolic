#pragma once


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_descriptors.hpp"
//#include "core/utilities.hpp"
//#include "fc_buffer.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <fastgltf/types.hpp>
#include <filesystem>
#include<string>
// *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fastgltf
{
  class Image;
  class Asset;
}


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
     VkImage mImage{VK_NULL_HANDLE};
     VkImageView mImageView{VK_NULL_HANDLE};
     VmaAllocation mAllocation{nullptr};


     // TODO get rid of or simply have as pointer to texture cache
     VkSampler mTextureSampler{VK_NULL_HANDLE};

     // TODO track layout of image in transitions
     // NOTE this layout may not be exactly up to date based on image barriers in GPU etc.

      // TODO create a synced state variable to track current image layout
      // VkImageLayout mCurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
     VkExtent3D mImageExtent;
     uint32_t mMipLevels{1};

     // void extracted(VkImageLayout& oldLayout,
     //                VkImageLayout& newLayout,
     //                VkImageMemoryBarrier& imageMemoryBarrier);
     void generateMipMaps();
     void createTextureSampler();
     void createCubeMapSampler();
     void writeToTexture(void* pixelData, bool generateMipmaps);
   public:
      // - CTORS -
     FcImage(std::filesystem::path& filename) { loadTexture(filename); }
     FcImage() = default;
     FcImage(VkImage image);
     ~FcImage() = default;

     //FcImage& operator=(const FcImage&) = delete;
     // TODO implement or delete or default ALL constructors throughout the engine -> THEN DOCUMENT
     // TODO delete all constructors for each class then recompile to see where they're used.
     FcImage(const FcImage&) = default;
     //FcImage& operator=(FcImage&&) = default;
     //FcImage(FcImage&&) = default;
     void create(VkExtent3D imgExtent, VkFormat format
                 , VkImageUsageFlags useFlags
                 , VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
                 , VkSampleCountFlagBits msaaSampleCount = VK_SAMPLE_COUNT_1_BIT
                 , bool generateMipMaps = false
                 , VkImageCreateFlags createFlags = 0);
     void createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags
                          , VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);

      // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better
      // way... maybe unique_ptr
      //FcImage(const FcImage&) = delete;
      //void operator=(const VkImage& image) {mImage = image;}

     // TODO eliminate one of the following
      //void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
     void transitionImage(VkCommandBuffer cmd, VkImageLayout oldLayout
                          , VkImageLayout newLayout, uint32_t mipLevels = 1);

     void copyFromBuffer(FcBuffer& srcBuffer, VkDeviceSize bufferSize
                         , VkDeviceSize offset = 0, uint32_t arrayLayer = 0);
     void copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source, VkExtent2D srcSize, VkExtent2D dstSize);
     void clear(VkCommandBuffer cmdBuffer, VkClearColorValue* pColor);
      // TEXTURE FUNCTIONS
     void loadTexture(std::filesystem::path& filename);
     void loadTexture(std::filesystem::path& path, fastgltf::Asset& asset, fastgltf::Image& image);
     void loadCubeMap(std::array<std::filesystem::path, 6>& filenames);
     void createTexture(VkExtent3D extent, void* pixelData, bool generateMipmaps = false);
      //void overwriteTexture(void* pixelData, uint32_t mipLevel = 1);
      // GETTERS
     const bool isValid() const { return mImage != VK_NULL_HANDLE; }
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
