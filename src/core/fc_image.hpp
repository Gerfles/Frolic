#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// //
// #include "core/fc_descriptors.hpp"
// #include "core/utilities.hpp"
// #include "fc_buffer.hpp"
//#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include "vk_mem_alloc.h"
#include "ktx.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <fastgltf/types.hpp>
#include <filesystem>
#include <ostream>
#include <string>
#include <algorithm> // std::clamp
// *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fastgltf
{
  class Image;
  class Asset;
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FCIMAGE CLASS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
   // TODO create new texture class that inherits from image
   // FORWARD DECLARATIONS
  class FcBuffer;

  enum class ImageTypes : uint8_t
  {
    Texture,             // Default texture image
    TextureWithMipmaps,  //
    TextureGenerateMipmaps,    // Texture with mipMaps
    Cubemap,           // Cubemap image (with layers)
    HeightMap,         // HeightMap
    DrawBuffer,        // Draw buffer
    NormalMap,         // Normal map
    DepthBuffer,       // Depth buffer
    Custom,            // User defined image using a passed in create info
    TextureArray,      // Texture but with multiple array layers
    ScreenBuffer,  // Draw image into this buffer before copying to swapchain
    ShadowMap,
  };


  class FcImage
  {
   private:
     // -*-*-*-*-*-*-*-   USED WHEN MAPPING DATA TO READ PIXEL VALUES   -*-*-*-*-*-*-*- //
     std::shared_ptr<FcBuffer> localCopy;
     void* localCopyAddress {nullptr};
     // TODO create a heading for blank lines (without gap)
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-      *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkImage mImage{VK_NULL_HANDLE};
     VkImageView mImageView{VK_NULL_HANDLE};
     VmaAllocation mAllocation{nullptr};
     VkImageAspectFlags mAspectFlag;
     VkFormat mFormat;
     // ?? Not sure if this will come in handy just yet
     int mBytesPerPixel;

     // TODO get rid of or simply have as pointer to texture cache
     VkSampler mTextureSampler{VK_NULL_HANDLE};

     // TODO track layout of image in transitions
     // NOTE this layout may not be exactly up to date based on image barriers in GPU etc.
     // TODO create a synced state variable to track current image layout
      // VkImageLayout mCurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};

     uint32_t mWidth;
     uint32_t mHeight;
     uint32_t mLayerCount {1};
     // TODO might want to save a copy of ImageType locally in case we need it for ops

     int mNumChannels;

     // ?? may want to store image memory size since mipmaps and layers, etc will affect this
     uint32_t mMipLevels {1};
     // void extracted(VkImageLayout& oldLayout,
     //                VkImageLayout& newLayout,
     //                VkImageMemoryBarrier& imageMemoryBarrier);
     void generateMipMaps();
     void createTextureSampler();
     void createCubeMapSampler();
     void writeToImage(void* pData, VkDeviceSize dataLength, bool generateMipmaps);
   public:
     static constexpr int BYTES_PER_PIXEL = 4;

     // TODO implement a pixel class that extrapolates any image format specifics, etc
     // that way we can just call pixel.r and get the right value
     // NOTE: looking at the pixel raw data, it appears the order of the bits is different
     // than might be expected
     // AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR

     // TODO implement for the non GPU images->anything that is vmaAllocate-able
     // TODO should be type agnostic or get that info from somewhere since this could
     // be RGBA_8 or R16 or D32 etc.
     template <class T>
     void fetchPixel(const int x, const int y, T& pixel)
      {
        // TODO could check to make sure image is mapped and return error if not
        uint32_t xPos = std::clamp(x, 0, static_cast<int>(mWidth - 1));
        uint32_t yPos = std::clamp(y, 0, static_cast<int>(mHeight - 1));
        // Doing the below will make this equivalent to Sascha's Method but must pass scale
        /* uint32_t xPos = std::clamp(x, 0, static_cast<int>(mImageExtent.width - scale)); */
        /* uint32_t yPos = std::clamp(y, 0, static_cast<int>(mImageExtent.height - scale)); */
        uint32_t offset = xPos + yPos * mWidth;

        // Encode pixel location for testing purposes value based on test image
        // pixel = (xPos << 16) + yPos;
        pixel = *((T*)localCopyAddress + offset);// + (x + y * mImageExtent.width));
  }




      // - CTORS -
     FcImage(std::filesystem::path& filename, ImageTypes imageType)
      { loadNonKtxFile(filename, imageType); }
     FcImage() = default;
     FcImage(VkImage image);
     ~FcImage() = default;
     // TODO implement or delete or default ALL constructors throughout the engine -> THEN DOCUMENT
     // TODO delete all constructors for each class then recompile to see where they're used.
     FcImage& operator=(const FcImage&) = default;
     FcImage(const FcImage&) = default; //{ };// delete;// : localCopy{nullptr} {}
     FcImage& operator=(FcImage&&) = default;
     FcImage(FcImage&&) = default;
     void create(uint32_t width, uint32_t height, ImageTypes imageType);
     void createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags
                          , VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);

      // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better
      // way... maybe unique_ptr
      //FcImage(const FcImage&) = delete;
      //void operator=(const VkImage& image) {mImage = image;}

     // TODO eliminate one of the following
      //void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
     void transitionImage(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout
                          , VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
                          , uint32_t mipLevels = 1);

     void copyFromBuffer(FcBuffer& srcBuffer
                         , VkDeviceSize offset = 0, uint32_t arrayLayer = 0);
     void copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source);
     void clear(VkCommandBuffer cmdBuffer, VkClearColorValue* pColor);
      // TEXTURE FUNCTIONS
     // TODO revise naming conventions to avoid confusion
     void loadNonKtxFile(std::filesystem::path& filename, ImageTypes imageType);
     void loadKtxFile(std::filesystem::path& filename, ImageTypes imageType);
     void loadFromGltf(std::filesystem::path& path, fastgltf::Asset& asset, fastgltf::Image& image);
     void loadCubeMap(std::array<std::filesystem::path, 6>& filenames);
     void createTexture(uint32_t width, uint32_t height, void* pixelData
                        , VkDeviceSize storageSize, bool generateMipmaps = false
                        , VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
     void setPixelFormat();
      //void overwriteTexture(void* pixelData, uint32_t mipLevel = 1);
      // GETTERS
     void copyToCPUAddress();
     void destroyCpuCopy();
//     uint32_t fetchPixel(const int x, const int y);
     uint16_t saschaFetchPixel(const int x, const int y, uint32_t scale);
     const bool isValid() const { return mImage != VK_NULL_HANDLE; }
     const VkImageView& ImageView() const { return mImageView; }
     VkSampler TextureSampler() { return mTextureSampler; }
     VkImage Image() { return mImage; }
     VkExtent2D Extent() { return {mWidth, mHeight}; }
     uint32_t Width() { return mWidth; }
     uint32_t Height() { return mHeight; }
     int byteDepth() { return mBytesPerPixel; }
      // cleanup
//     ~FcImage() = default;
     void destroyImageView();
     void destroy();
     // DELETE eventually
     void loadTestImage(uint32_t width, uint32_t height);
     void deleteTestImage();
  };


} // namespace fc _END_
