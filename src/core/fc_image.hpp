//> fc_image.hpp <//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vk_mem_alloc.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <filesystem>
// *-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECLARATIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fastgltf { class Image; class Asset; class Sampler; }
namespace std { template<typename> class vector; }
namespace fc { class FcBuffer; class FcDrawCollection; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FCIMAGE CLASS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  enum class FcImageTypes : uint8_t {
    Texture,                 // Default texture image
    TextureWithMipmaps,      //
    TextureGenerateMipmaps,  // Texture with mipMaps
    Cubemap,                 // Cubemap image (with layers)
    HeightMap,               // HeightMap
    DrawBuffer,              // Draw buffer
    NormalMap,               // Normal map
    DepthBuffer,             // Depth buffer
    Custom,                  // User defined image using a passed in create info
    TextureArray,            // Texture but with multiple array layers
    TextureArrayGenerateMipmaps, // Same as above but generates mipmaps
    ScreenBuffer,  // Draw image into this buffer before copying to swapchain
    ShadowMap,
  };

  // TODO Figure out if we should be defaulting to Unorm or SRGB
  // TODO think about creating a separate class for texture
  class FcImage
  {
   private:
     // -*-*-*-*-*-*-*-   USED WHEN MAPPING DATA TO READ PIXEL VALUES   -*-*-*-*-*-*-*- //
     // TODO get rid of local Copy since not always needed
     std::shared_ptr<FcBuffer> localCopy;
     void* localCopyAddress {nullptr};
     /* static uint32_t index; */
     uint32_t mHandle;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     VkSampler mSampler {VK_NULL_HANDLE};
     VkImage mImage {VK_NULL_HANDLE};
     VkImageView mImageView {VK_NULL_HANDLE};
     VmaAllocation mAllocation {nullptr};
     VkFormat mFormat;
     // TODO think about formating this differently
     int mBytesPerPixel;
     // TODO create a synced state variable to track current image layout
     // NOTE this layout may not be exactly up to date based on image barriers in GPU etc.
     // VkImageLayout mCurrentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
     // ?? might want to save a copy of ImageType locally in case we need it for ops
     uint16_t mWidth;
     uint16_t mHeight;
     uint8_t mLayerCount {1};
     uint8_t mMipLevels {1};
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     void generateMipMaps();
     void setPixelFormat();
     void writeToImage(void* pData, VkDeviceSize dataLength, bool generateMipmaps);
   public:

     // TRY
     void init(u32 handle);

// TODO could we just do this for images that need this function aka FcMappableImage?
     // TODO implement for the non GPU images->anything that is vmaAllocate-able
     // TODO should be type agnostic or get that info from somewhere since this could
     // be RGBA_8 or R16 or D32 etc.
     template <class T>
     void fetchPixel(const int x, const int y, T& pixel)
      {
        // TODO could check to make sure image is mapped and return error if not

        // Could check in debug mode
        // uint32_t xPos = std::clamp(x, 0, static_cast<int>(mWidth - 1));
        // uint32_t yPos = std::clamp(y, 0, static_cast<int>(mHeight - 1));
        uint32_t offset = x + y * mWidth;

        // Encode pixel location for testing purposes value based on test image
        // pixel = (xPos << 16) + yPos;
        pixel = *((T*)localCopyAddress + offset);// + (x + y * mImageExtent.width));
      }

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO implement or delete or default ALL constructors throughout the engine -> THEN DOCUMENT
     FcImage() = default;
     FcImage(VkImage image);
     ~FcImage() = default;
     FcImage& operator=(const FcImage&) = default;
     // ?? This must be included to allow vector.pushBack(Fcbuffer) ?? not sure if there's a better
     // way... maybe unique_ptr
     FcImage(const FcImage&) = default;
     FcImage& operator=(FcImage&&) = default;
     FcImage(FcImage&&) = default;
     FcImage(std::filesystem::path& filename, FcImageTypes imageType)
      	{ loadStbi(filename, imageType); }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   IMAGE MANIPULATION   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void createImage(uint32_t width, uint32_t height, FcImageTypes imageType);
     void createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags
                          , VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D);
     void transitionLayout(VkCommandBuffer cmd
                           , VkImageLayout oldLayout
                           , VkImageLayout newLayout
                           , VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT
                           , uint32_t mipLevels = 1);
     void copyFromBuffer(FcBuffer& srcBuffer, VkDeviceSize offset = 0, uint32_t arrayLayer = 0);
     //
     void copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source);
     //
     void clear(VkCommandBuffer cmdBuffer, VkClearColorValue* pColor);
     // *-*-*-*-*-*-*-*-*-*-*-*-   TEXTURE LOADING FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*- //
     void loadStbi(std::filesystem::path& filename, FcImageTypes imageType);
     //
     void loadKtxFile(std::filesystem::path& filename, FcImageTypes imageType);
     //
     void loadFromGltf(std::filesystem::path& path
                       , fastgltf::Asset& asset, fastgltf::Image& image);
     void loadMultipleLayers(std::vector<std::string>& filenames, FcImageTypes imageType);
     //
     void createTexture(uint32_t width, uint32_t height, void* pixelData
                        , VkDeviceSize storageSize, bool generateMipmaps = false
                        , VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
     //
     void copyToCPUAddress();
     //
     void destroyCpuCopy();
     //
     void setSampler(const fastgltf::Sampler& sampler);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   VALIDATORS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     const bool isValid() const { return mImage != VK_NULL_HANDLE; }
     //
     const bool hasSampler() const { return mSampler != VK_NULL_HANDLE; }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     uint32_t fetchPixel(const int x, const int y);
     //
     uint16_t saschaFetchPixel(const int x, const int y, uint32_t scale);
     //
     const VkImageView& ImageView() const { return mImageView; }
     //
     const VkSampler& Sampler() const { return mSampler; }
     //
     VkImage Image() { return mImage; }
     VkExtent2D Extent() { return {mWidth, mHeight}; }
     uint32_t Width() { return mWidth; }
     uint32_t Height() { return mHeight; }
     uint32_t Handle() { return mHandle; }
     uint8_t LayerCount() { return mLayerCount; }
     uint8_t MipMapCount() { return mMipLevels; }
     int byteDepth() { return mBytesPerPixel; }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CLEANUP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void destroyImageView();
     void destroy();
     // DELETE eventually
     void loadTestImage(uint32_t width, uint32_t height);
     void deleteTestImage();
  };

} // namespace fc _END_
