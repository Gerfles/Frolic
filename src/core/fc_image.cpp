//>--- fc_image.cpp ---<//
#include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_defaults.hpp"
#include "fc_locator.hpp"
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// TODO place all implementation header defines into one header file??
#define STB_IMAGE_IMPLEMENTATION
// avoid compiline failure strings
#define STBI_NO_FAILURE_STRINGS
// More user-friendly failure messages when above is commented out
/* #define STBI_FAILURE_USERMSG */
#include <stb_image.h>
#include "fastgltf/types.hpp"
#include <ktx.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // TRY could implement a static variable for keeping track of handles??
  /* uint32_t FcImage::index = 0; */

  FcImage::FcImage(VkImage image)
  {
    mImage = image;
  }


  void FcImage::init(u32 handle)
  {
    localCopyAddress = nullptr;
    mImage = VK_NULL_HANDLE;
    mImageView = VK_NULL_HANDLE;
    mAllocation = nullptr;
    mLayerCount = 1;
    mMipLevels = 1;
    mHandle = handle;
  }

  //
  // Figure out which image sampler to use based on a given a  fastgltf sampler
  void FcImage::setSampler(const fastgltf::Sampler& sampler)
  {
    // make sure the given fastgltf sampler has a value
    if (sampler.minFilter.has_value() && sampler.magFilter.has_value())
    {
      if (sampler.minFilter.value() == fastgltf::Filter::Nearest
          && sampler.magFilter.value() == fastgltf::Filter::Nearest)
      {
        mSampler = FcDefaults::Samplers.Nearest;
      }

      // TODO make sure we are handling MagFilter properly
      // Extract the mip map filter mode
      switch (sampler.minFilter.value())
      {
          case fastgltf::Filter::NearestMipMapNearest:
          case fastgltf::Filter::LinearMipMapNearest:
            // use a single mipmap without any blending
            mSampler = FcDefaults::Samplers.Bilinear;
            break;

          case fastgltf::Filter::NearestMipMapLinear:
          case fastgltf::Filter::LinearMipMapLinear:
            // blend multiple mipmaps
            mSampler = FcDefaults::Samplers.Trilinear;
            break;

          default:
            mSampler = FcDefaults::Samplers.Linear;
            break;
      }
    }
    else
    {
      // TODO make sure the default sampler should be trilinear
      mSampler = FcDefaults::Samplers.Trilinear;
    }
  }


  // TODO create a create() function that allows us to pass a VkImageCreateInfo, etc...
  // for highly specialized image creation
  void FcImage::createImage(uint32_t width, uint32_t height, FcImageTypes imageType)
  {
    mWidth = width;
    mHeight = height;
    bool generateMipmaps;

    // *-*-*-*-*-*-*-*-*-*-*-*-   DEFAULT IMAGE CREATE INFO   *-*-*-*-*-*-*-*-*-*-*-*- //
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {mWidth, mHeight, 1};
    imageInfo.arrayLayers = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // whether image can be shared between queues
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // ?? should this be SRGB -> need to be consistent
    mFormat = VK_FORMAT_R8G8B8A8_UNORM;
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULT IMAGE VIEW   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    // -*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULT ALLOCATION INFO   -*-*-*-*-*-*-*-*-*-*-*-*- //
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    // *-*-*-*-*-*-*-*-   POPULATE IMAGE, VIEW, AND ALLOCATION INFO   *-*-*-*-*-*-*-*- //
    switch (imageType)
    {
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   BASIC TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::Texture:
        {
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
          break;
        }
        case FcImageTypes::TextureWithMipmaps:
        {
          // TODO implement
          break;
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-   TEXTURE NEEDING MIPMAPS   -*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::TextureGenerateMipmaps:
        {
          // transfer src only needs to be set when generating mipmaps from original image
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          generateMipmaps = true;
          break;
        }
        case FcImageTypes::ScreenBuffer:
        {
          mFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
          // We plan on copying into but also from the image
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
          // Storage bit allows computer shader to write to image
          imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
          // Color attachment allows graphics pipelines to draw geometry into it
          imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

          // TODO this should be a default setting and changed when necessary
          allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
          // Prefer to create such resources first and all other GPU resources (like
          //textures and vertex buffers) later. When VK_EXT_memory_priority extension is
          //enabled, it is also worth setting high priority to such allocation to decrease
          //chances to be evicted to system memory by the operating system.
          allocInfo.priority = 1.0f;
          mLayerCount = 1;
          break;
        }
        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEPTH BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::DepthBuffer:
        {
          mFormat = VK_FORMAT_D32_SFLOAT;
          imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
          imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
          allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
          allocInfo.priority = 1.0f;
          break;
        }
        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TEXTURE ARRAY   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::TextureArray:
        {
          // NOTE: unfortunately named, but we do not have a 3D image so this flag must
          // not be set, it's intended to allow a 3D image to be used as multi-array
          // sampled imageInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

          imageInfo.arrayLayers = mLayerCount;
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
          imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          imageViewInfo.subresourceRange.layerCount = mLayerCount;
          break;
        }
        // -*-*-*-*-*-*-*-*-*-*-*-   TEXTURE ARRAY WITH MIPMAPS   -*-*-*-*-*-*-*-*-*-*-*- //

        case FcImageTypes::TextureArrayGenerateMipmaps:
        {
          imageInfo.arrayLayers = mLayerCount;
          // transfer src only needs to be set when generating mipmaps from original image
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
          imageViewInfo.subresourceRange.layerCount = mLayerCount;
          generateMipmaps = true;
          break;
        }


        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CUBE MAP   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::Cubemap:
        {
          mLayerCount = 6;
          imageInfo.arrayLayers = mLayerCount;
          imageViewInfo.subresourceRange.layerCount = mLayerCount;
          imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
          mFormat = VK_FORMAT_R8G8B8A8_UNORM;
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
          imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
          allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
          mMipLevels = 1;
          break;
        }
        case FcImageTypes::HeightMap:
        {
          imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
          mFormat = VK_FORMAT_R16_UNORM;
          imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          mMipLevels = 1;
          break;
        }
        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW MAP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        case FcImageTypes::ShadowMap:
        {
          // TODO allow alternative (smaller) formats to be used
          mFormat = VK_FORMAT_D32_SFLOAT;
          imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
          imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
          imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
          mMipLevels = 1;
          break;
        }
        // TODO Custom can be utilized if needed for finer grain detail
        case FcImageTypes::Custom:
        {
          break;
        }
        default:
          break;
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FINALIZE IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    if (generateMipmaps)
    {
      mMipLevels = static_cast<uint32_t>
                   (std::floor(std::log2(std::max(mWidth, mHeight)))) + 1;
    }

    imageInfo.mipLevels = mMipLevels;
    imageInfo.format = mFormat;

    FcGpu& gpu = FcLocator::Gpu();

    if (vmaCreateImage(gpu.getAllocator(), &imageInfo
                       , &allocInfo, &mImage, &mAllocation, nullptr)
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate a Vulkan image!: ");
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   FINALIZE IMAGEVIEW   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    imageViewInfo.format = mFormat;
    imageViewInfo.image = mImage;
    imageViewInfo.subresourceRange.layerCount = mLayerCount;
    imageViewInfo.subresourceRange.levelCount = mMipLevels;

    if (vkCreateImageView(FcLocator::Device(), &imageViewInfo, nullptr, &mImageView)
        != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create an Image View!");
    }

    // Add image to the bindless array for deferred updating
    if (gpu.Properties().isBindlessSupported)
    {

    }



    // *-*-*-*-*-*-*-   CUSTOM MEMORY ALLOCATION SAVED FOR REFERENCE   *-*-*-*-*-*-*- //
    // if (vkCreateImage(device, &imageInfo, nullptr, &mImage) != VK_SUCCESS)
    // {
    //   throw std::runtime_error("Failed to create a Vulkan Image!");
    // }

    //  // get memory requirements for a type of image
    // VkMemoryRequirements memRequirments;
    // vkGetImageMemoryRequirements(device, mImage, &memRequirments);

     // get properties of physical device memory
    // VkPhysicalDeviceMemoryProperties memProperties;
    // vkGetPhysicalDeviceMemoryProperties(FcLocator::Gpu().physicalDevice(), &memProperties);

    //  // TODO since this section of code is duplicated in FcBuffer, should create a higher level
    //  // function and extract it out or even consider having a heirarchy for buffer->image such
    //  // that buffer and image both derive from the same base class.

    //  // cycle through all the memory types available and choose the one that has our required properties
    // uint32_t memoryTypeIndex = -1;
    // for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    // {
    //   if ((memRequirments.memoryTypeBits & (1 << i)) // first, only pick each allowed type of memory passed in (skip evaluation when that type is not bit-enabled by our allowedType parameter)
    //       && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) // then make sure that the allowed types also contains the property flags that we request by passing the vkmemorypropertyflags
    //   {
    //      // this memory type is an allowed type and it has the properties we want (flags) return
    //     memoryTypeIndex = i;
    //     break;
    //   }
    // }

    //  // Quit if we can't find the proper memory type
    //  // ?? Tutorial doesn't contain this check so maybe Vulkan is required to return something...
    // if (memoryTypeIndex == -1)
    // {
    //    //throw std::runtime_error("Failed to find a suitable memory type!");
    // }

    //  // create memory for image
    // VkMemoryAllocateInfo memAllocInfo{};
    // memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // memAllocInfo.allocationSize = memRequirments.size;
    // memAllocInfo.memoryTypeIndex = memoryTypeIndex;

    //  // allocate memory to VkDeviceMemory
    // if (vkAllocateMemory(device, &memAllocInfo, nullptr, &mImageMemory) != VK_SUCCESS)
    // {
    //   throw std::runtime_error("Failed to allocate Vulkan Device Memory for FcImage!");
    // }

    //  // allocate memory to given vertex buffer
    // vkBindImageMemory(device, mImage, mImageMemory, 0);

    // now create an Image view so we can interface with it
  }



  void FcImage::createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags
                                , VkImageViewType imageViewType)
  {
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = mImage;
    imageViewInfo.viewType = imageViewType;
    imageViewInfo.format = imageFormat;
    // allows remapping of rgba component to other values
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    // Subresources allow the view to view only a part of an image
    imageViewInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = mMipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(FcLocator::Device(), &imageViewInfo, nullptr, &mImageView) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create an Image View!");
    }
  }

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // TODO  may also want to create TEST using this method without first invoking a command buffer
  // an immediate transition image function (transitionAndSubmit) to eliminate command buffer stuff
  void FcImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout currentLayout
                                , VkImageLayout newLayout, VkImageAspectFlags aspectFlags
                                ,  uint32_t mipLevels)
  {
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.oldLayout = currentLayout; // layout to transition from
    imageBarrier.newLayout = newLayout; // layout to transition to
    imageBarrier.image = mImage;        // image being accessed and modifies as part of barrier

    // Queue family to transition from - IGNORED means don't bother transferring to a different queue
    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    // TODO consider storing in image
    imageBarrier.subresourceRange.aspectMask = aspectFlags;

    // if (newLayout == VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR)
    // {
    //   imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    //   imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    //   imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    // }
    if (currentLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && aspectFlags == VK_IMAGE_ASPECT_DEPTH_BIT)
    {
      imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR;
      imageBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
      imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR;
      imageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;
      imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
      // TODO
      // This is how it's done in vulkan 1.3 with extension... but this implementation is inefficient. This will stall the GPU
      // to wait for all stages and if we want post-processing, we should be more precise about when and which stages should be set
      // great documentation: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
      imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
      imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
      imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

      // set aspect of image being altered to color image (default) unless new layout required is depth image
    }


    // (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
    //                                           || currentLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    //                                          ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    // TODO
    // We are currently targeting all layers and mipmap levels and this could potentially be better implemented by targeting less
    // first mip level to start alterations on
    // TODO figure out if VK_REMAININT... will do all the mip levels we request
    // imageBarrier.subresourceRange.baseMipLevel = 0;
    //  imageBarrier.subresourceRange.layerCount = mipLevels;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;   // number of mip levels to alter starting from base mip level
    imageBarrier.subresourceRange.baseArrayLayer = 0;                     // first layer to start alterations on
    imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS; // number of layers to alter starting from basearraylayer

    //  TODO multiple imageMemoryBarriers would improve efficiency if we have multiple images to transition simultaneously
    //  ?? May also improve performance by recording this all in a predefined dependencyInfo that is around for the life of the swapchain
    VkDependencyInfo dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
  }

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OLD METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  // void FcImage::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
  // {
  //   FcGpu& gpu = FcLocator::Gpu();
  //    // create command buffer
  //    // TODO try and eliminate this creation of command buffer if it's already happening inside create texture
  //   VkCommandBuffer commandBuffer = gpu.beginCommandBuffer();

  //   VkImageMemoryBarrier imageMemoryBarrier{};
  //   imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //   imageMemoryBarrier.oldLayout = oldLayout;                                   // layout to transition from
  //   imageMemoryBarrier.newLayout = newLayout;                                   // layout to transition to
  //   imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // Queue family to transition from - IGNORED means don't bother transferring to a different queue
  //   imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // Queue family to transition to - ""
  //   imageMemoryBarrier.image = mImage;                                          // image being accessed and modifies as part of barrier
  //   imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // aspect of image being altered
  //   imageMemoryBarrier.subresourceRange.baseMipLevel = 0;                       // first mip level to start alterations on
  //   imageMemoryBarrier.subresourceRange.levelCount = mipLevels;                 // number of mip levels to alter starting from base mip level
  //   imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;                     // first layer to start alterations on
  //   imageMemoryBarrier.subresourceRange.layerCount = 1;                         // number of layers to alter starting from basearraylayer

  //    // barrier locations so that transitions must happen between the srcStage and the dstStage
  //   VkPipelineStageFlags srcStage, dstStage;
  //    //
  //    // if transitioning from transfer destination to shader readable...
  //   if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  //   {
  //     srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  //     dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  //     imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  //     imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  //   }
  //    //  - DEFAULT TRANSITION -  i.e. (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  //    // if transitioning from new image to image ready to receive data or transitioning from Destination optimal to source optimal
  //   else
  //   {
  //     srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  //     dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  //     imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;// memory access stage transition after this stage - 0 means could happen anywhere
  //     imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;                // memory access stage transition before this stage -
  //   }

  //   vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

  //   gpu.submitCommandBuffer(commandBuffer);
  // }


  // TODO add support for linear tilling
  // TODO perhaps find a better way to use some C++ features here like decltype, function ptrs, etc to allow texture type (ktx vs ktx2) to be determined without branch
  void FcImage::loadKtxFile(std::filesystem::path& filename, FcImageTypes imageType)
  {
    //
    KTX_error_code result;
    ktx_size_t dataLength;
    void* pData;
    ktxTexture* texture {nullptr};
    ktxTexture2* texture2 {nullptr};

    if (filename.extension() == ".ktx2")
    {
      result = ktxTexture2_CreateFromNamedFile(filename.c_str()
                                               , KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT
                                               , &texture2);

      if (texture2 == nullptr || result != KTX_SUCCESS)
      {
        throw std::runtime_error("Failed to load KTX format image!");
      }

      mFormat = VkFormat(texture2->vkFormat);
      mWidth = texture2->baseWidth;
      mHeight = texture2->baseHeight;
      mLayerCount = texture2->numLayers;
      mMipLevels = texture2->numLevels;
      dataLength = texture2->dataSize;
      pData = texture2->pData;
    }
    else // This appears to be a v.1 ktxTexture
    {
      result = ktxTexture_CreateFromNamedFile(filename.c_str()
                                              , KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT
                                              , &texture);

      if (texture == nullptr || result != KTX_SUCCESS)
      {
        throw std::runtime_error("Failed to load KTX format image!");
      }
      // Most tools export to this format so make this assumption for now (could allow
      // for passing in but most ktx files should be ktx2 instead since its an easy
      // conversion using CLI tool)
      //-> ktx2ktx2 -b [ktx1_filename_to_convert]
      mFormat = VK_FORMAT_R8G8B8A8_SRGB;
      mWidth = texture->baseWidth;
      mHeight = texture->baseHeight;
      mLayerCount = texture->numLayers;
      mMipLevels = texture->numLevels;
      dataLength = texture->dataSize;
      pData = texture->pData;
      // BUG set properly
      // mBytesPerPixel = texture2->kvDataLen;
    }

    setPixelFormat();
    // create the image
    createImage(mWidth, mHeight, imageType);
    // store our ktx data to image

    bool generateMipmaps = (mMipLevels > 1) ? true : false;

    writeToImage(pData, dataLength, generateMipmaps);

    // destroy any ktx texture data we have loaded
    if (texture != nullptr)
    {
      ktxTexture_Destroy((ktxTexture*)texture);
    }
    if (texture2 != nullptr)
    {
      // Note here that we have to cast since libktx has not written Destroy for ktx2
      ktxTexture_Destroy((ktxTexture*)texture2);
    }
  }



  // ?? determine if copying to image would be preferable to buffer copy
  void FcImage::copyToCPUAddress()
  {
    // Must use shared pointer here since we have default copy/assignment operators
    // TODO could change if we implement our own
    // TODO imageMemSize should be determined programatically
    VkDeviceSize imageMemSize = mWidth * mHeight * mBytesPerPixel;
    localCopy = std::make_shared<FcBuffer>(imageMemSize, FcBufferTypes::Staging);

    // store a copy of our buffer memory location in the class for quick access
    localCopyAddress = localCopy->getAddress();

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

    // TODO handle the cases where transition of image is not known,
    // Probably need to store current layout in image
    // TODO might want to create a separate image memory barrier for this type of transition
    transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // TODO handle the case where the buffer being passed is allready allocated

    VkBufferImageCopy imageCopyRegion{};
    // TODO rename to simply mExtent
    imageCopyRegion.imageExtent = {mWidth, mHeight, 1};
    imageCopyRegion.imageOffset = {0, 0, 0};
    imageCopyRegion.bufferOffset = 0;
    // for calculating data spacing (row length of data)
    imageCopyRegion.bufferRowLength = 0;
    imageCopyRegion.bufferImageHeight = 0;
    imageCopyRegion.imageSubresource.mipLevel = 0;
    imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.imageSubresource.layerCount = 1;
    imageCopyRegion.imageSubresource.baseArrayLayer = 0;

    vkCmdCopyImageToBuffer(cmdBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                           , localCopy->getVkBuffer(), 1, &imageCopyRegion);

    transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                    , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    FcLocator::Renderer().submitCommandBuffer();
  }


  void FcImage::destroyCpuCopy()
  {
    localCopy->destroy();
    localCopy.reset();
    localCopyAddress = nullptr;
  }

  // Sascha Willems Method
  // Nearest I can tell, this method will give the nearest pixel value within the
  // pixel step size, so for instance if passing x:512, y:512, scale:8 - the original
  // function will fetch pixel at (511, 511) whereas this function will fetch
  // pixel at (504, 504);
  uint16_t FcImage::saschaFetchPixel(const int x, const int y, uint32_t scale)
  {
    uint32_t dim = mWidth;

    // rpos = (x, y) * (8, 8)
    glm::ivec2 rpos = glm::ivec2(x, y);
    // Skipped since this is already accounted for in terrain.cpp
    // rpos *= glm::ivec2(scale);

    // rpos.x = clamp(rpos.x, 0, 511)
    rpos.x = std::max(0, std::min(rpos.x, static_cast<int>(dim - 1)));
    // rpos.x = clamp(rpos.y, 0, 511)
    rpos.y = std::max(0, std::min(rpos.y, static_cast<int>(dim - 1)));

    // rpos.x / 8 * 8 , rpos.y / 8 * 8 (basically round to nearest scale)
    // Note: that this can be accomplised by clamping within [0, dim - scale] instead
    rpos /= glm::ivec2(scale);
    rpos *= glm::ivec2(scale);
    // rpos = data[(rpos.x + rpos.y * 512) * 8]
    return *((uint16_t*)localCopyAddress + (rpos.x + rpos.y * dim));
  }


  // DELETE eventually
  void FcImage::loadTestImage(uint32_t width, uint32_t height)
  {
    mWidth = width;
    mHeight = height;
    int testImageArea = width * height;
    localCopyAddress = new uint32_t[testImageArea];
    std::memset(localCopyAddress, 0, testImageArea * sizeof(uint32_t));

    std::vector<uint32_t> testImage(testImageArea);
    //
    for(uint32_t x = 0; x < width; ++x)
    {
      for(uint32_t y = 0; y < height; ++y)
      {
        uint32_t val = (x << 15) + y;
        testImage[x + y * width] = val;
      }
    }
    std::memcpy(localCopyAddress, testImage.data(), testImage.size() * sizeof(testImage[0]));
  }


  void FcImage::deleteTestImage()
  {
    delete[] static_cast<uint32_t*>(localCopyAddress);
  }



  void FcImage::loadStbi(std::filesystem::path& filename, FcImageTypes imageType)
  {
    // First find out image specs and then create a suitable image
    int width, height, channels;
    stbi_info(filename.c_str(), &width, &height, &channels);
    createImage(width, height, imageType);
    setPixelFormat();

    // Now load image from file. Note: that we must pass channels to stbi_load but we will
    // not use since most GPUs require 4 channels per pixel (RGBA)
    stbi_uc* imageData = stbi_load(filename.c_str(), &width, &height
                                   , &channels, mBytesPerPixel);

    if (!imageData)
    {
      throw std::runtime_error("Failed to load texture file: " + filename.string());
    }

    // TODO separate generating mipmaps into a utility function since it would
    // not likely be part of a main game engine
    bool generateMipmaps = (imageType == FcImageTypes::TextureGenerateMipmaps)? true : false;

    // create the hardware texture used by the GPU
    writeToImage(imageData, width * height * mBytesPerPixel, generateMipmaps);

    // free original image data loaded from file
    stbi_image_free(imageData);
  }



  void FcImage::loadMultipleLayers(std::vector<std::string>& filenames, FcImageTypes imageType)
  {
    mLayerCount = filenames.size();
    mMipLevels = 1;

    // first gather dimensions of first image (all other images must have same dimension)
    int width, height, numChannels;
    stbi_info(filenames[0].c_str(), &width, &height, &numChannels);

    createImage(width, height, imageType);
    setPixelFormat();

    VkDeviceSize layerSize = width * height * mBytesPerPixel;
    VkDeviceSize imageSize = layerSize * mLayerCount;

    // create staging buffer to hold loaded data, ready to copy to device
    FcBuffer stagingBuffer(imageSize, FcBufferTypes::Staging);

    // transition the image buffer so that it is most efficient to be written to
    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
    transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED
                     , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    FcLocator::Renderer().submitCommandBuffer();

    for (size_t i = 0; i < filenames.size(); ++i)
    {
      // When loading with stbi we require 4 numChannels/pixel--even though the image itself
      // has only 3 (R8G8B8) since that is the format within vulkan that we're using to
      // store the image. Note that most GPUs don't actually support 3 channel images
      stbi_uc* pixels = stbi_load(filenames[i].c_str(), &width, &height, &numChannels
                                  , mBytesPerPixel);
      if (!pixels)
      {
        throw std::runtime_error("Failed to load texture file: " + filenames[i]);
      }

      // copy the actual image data into the staging buffer.
      stagingBuffer.write(pixels, layerSize, layerSize * i);

      // free original image data
      stbi_image_free(pixels);
    }
    // copy data to image
    copyFromBuffer(stagingBuffer, 0, 0);

    // no longer need staging buffer so get rid of
    stagingBuffer.destroy();

    // finally transition the image to GPU read
    cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
    transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                     , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    FcLocator::Renderer().submitCommandBuffer();
  }

  //
  // TODO make sure to test each of the 3 cases!!
  void FcImage::loadFromGltf(std::filesystem::path& parentPath, fastgltf::Asset& asset,
                             fastgltf::Image& image)
  {
    int width, height, numChannels;

    // TODO Look into creating a more readable version of this flow
    // (after looking at fastgltf for alternatives
    // TODO here we are passing a lambda to the visitor but might be able to pass an
    // already defined function that accomplishes what we do with the visitor
    std::visit(fastgltf::visitor {
        [](auto& arg) {},

          // -*-*-*-*-*-*-*-*-*-*-*-*-   CASE1: TEXTURE IS FILE   -*-*-*-*-*-*-*-*-*-*-*-*- //
          // Texture is stored outside of the glTF/glb file (this is common)
          [&](fastgltf::sources::URI& filepath)
           {
             assert(filepath.fileByteOffset == 0); // don't support offsets with stbi
             assert(filepath.uri.isLocalPath()); // only allow loading local files

             std::filesystem::path path{parentPath.c_str() + std::string{"//"}
                                   + std::string{filepath.uri.path().begin()
                                       , filepath.uri.path().end()}};

             loadStbi(path, FcImageTypes::TextureGenerateMipmaps);
           },

          // -*-*-*-*-*-*-*-*-*-*-*-*-   CASE2: TEXTURE IS ARRAY   -*-*-*-*-*-*-*-*-*-*-*-*- //
          // fastgltf already loaded texture into a std::array structure (this is the case
          // on base64 or if we instruct fastgltf to load external image files
          [&](fastgltf::sources::Array& array)
           {
             mBytesPerPixel = 4;
             stbi_uc* imageData = stbi_load_from_memory(
               (stbi_uc*)array.bytes.data()
               , static_cast<int>(array.bytes.size())
               , &width, &height, &numChannels, mBytesPerPixel);

             if (imageData)
             {
               // using the image data from the loaded file, create the hardware texture
               // used by the GPU TODO for create texture could just pass bytes_per_pixel
               // since dimension is already known, leaving for now since KTX format can
               // have mipmaps, as well as layers, stored with image etc
               createTexture(width, height, imageData
                             , width * height * mBytesPerPixel, true);

               // free original image data
               stbi_image_free(imageData);
             }
           },

          // -*-*-*-*-*-*-*-*-*-   CASE3 TEXTURE IS EMBEDDED INTO GLB   -*-*-*-*-*-*-*-*-*- //
          [&](fastgltf::sources::BufferView& view)
           {
             fastgltf::BufferView& bufferView = asset.bufferViews[view.bufferViewIndex];
             fastgltf::Buffer& buffer = asset.buffers[bufferView.bufferIndex];

             // print the image name in debug so we know what facets are available
             fcPrintEndl("Case3: Embedded Texture Name:", image.name.c_str());

             // Here we only care about VectorWithMime here, because we specify
             // LoadExternalBuffers, meaning all buffers are already loaded into a array.
             std::visit(fastgltf::visitor {
                 [](auto& arg) {},
                   [&] (fastgltf::sources::Array& array)
                    {
                      mBytesPerPixel = 4;
                      stbi_uc* imageData = stbi_load_from_memory(
                        (stbi_uc*)(array.bytes.data() + bufferView.byteOffset)
                         , static_cast<int>(bufferView.byteLength)
                        , &width, &height, &numChannels, mBytesPerPixel);

                      if (imageData)
                      {
                        createTexture(static_cast<uint32_t>(width)
                                      , static_cast<uint32_t>(height), imageData
                                      , width * height * mBytesPerPixel, true);

                        // free original image data
                        stbi_image_free(imageData);
                      }
                    }
                   }, buffer.data);
           }	,
          },
      image.data);

    // if any of the attempts to load the data failed, we havent written the image so set handle to null
    // TODO better error checking
    if (mImage == VK_NULL_HANDLE)
    {
      fcPrintEndl("Image is NOT valid!");
    }
  }


  // It should be noted that it is uncommon in practice to generate the mipmap levels at
  // runtime. Usually they are pregenerated and stored in the texture file alongside the
  // base level to improve loading speed.
  void FcImage::generateMipMaps()
  {
    // first select the largest dimension (widht or height), then calc the number of
    // times that can divided by 2. floor gets the gretest integer number of times and
    // then add 1 so original image has mip level.

    mMipLevels = static_cast<uint32_t>
                 (std::floor(std::log2(std::max(mWidth, mHeight)))) + 1;

    FcGpu& gpu = FcLocator::Gpu();

    // check if image format supports linear blitting first
    // TODO check this once at startup - NOT FOR EVERY IMAGE
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(gpu.physicalDevice()
                                        , VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures
          & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
      // could search for a common image format that does support linear blitting
      // here or use software like stb_image_resize to generate mipmaps before
      // hand--for now, just throw an error
      throw std::runtime_error("Failed to generate mipmaps - graphics device not capable"
                               " of linear blitting for this texture format!");
    }

    // this part will remain the same for all barriers - the rest will change
    // depending on the mip level we're on
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = mImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = mLayerCount;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = mWidth;
    int32_t mipHeight = mHeight;

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

    // TODO think about changing to VkImageBlit2
    // These aspects of image blit never change depending on mip level
    VkImageBlit blit{};
    blit.srcOffsets[0] = {0,0,0};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = mLayerCount;
    blit.dstOffsets[0] = {0,0,0};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = mLayerCount;

    // copying 2D images so always set depth to 1
    blit.srcOffsets[1].z = 1;
    blit.dstOffsets[1].z = 1;

    // record a VkCmdBlitImage() command for each mip level, except for
    // the first mipMap level (original image)
    for (size_t i = 0; i < mMipLevels - 1; ++i)
    {
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.subresourceRange.baseMipLevel = i;
      //
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                           , VK_PIPELINE_STAGE_TRANSFER_BIT
                           , 0, 0, nullptr, 0, nullptr, 1, &barrier);

      blit.srcOffsets[1].x = mipWidth;
      blit.srcOffsets[1].y = mipHeight;
      // set the next mipMap dimensions, by halving the previous mip dimension
      // making sure not to get any smaller than 1 pixel!
      blit.dstOffsets[1].x = mipWidth > 1 ? mipWidth >> 1 : 1;
      blit.dstOffsets[1].y = mipHeight > 1 ? mipHeight >> 1 : 1;
      blit.srcSubresource.mipLevel = i;
      blit.dstSubresource.mipLevel = i + 1;
      //
      vkCmdBlitImage(cmdBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage
                     , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

      // now transition image layout of previous mipLevel to shader read only format
      // (all sampling operations will wait on this transition to finish)
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      //
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                           , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                           , 0, 0, nullptr, 0, nullptr, 1, &barrier);

      // set the next loops mipmap dimensions to half the previous (unless only one pixel)
      mipWidth = blit.dstOffsets[1].x;
      mipHeight = blit.dstOffsets[1].y;
    }

    // one last transition to handle the final mip level not handled by the loop
    barrier.subresourceRange.baseMipLevel = mMipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                         , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                         , 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // finally, submit all commands to the the graphics queue
    FcLocator::Renderer().submitCommandBuffer();
  }



  void FcImage::setPixelFormat()
  {
    switch(mFormat)
    {
        case VK_FORMAT_R8G8B8A8_SRGB:
        {
          mBytesPerPixel = 4;
          break;
        }
        case VK_FORMAT_R8G8B8A8_UNORM:
        {
          mBytesPerPixel = 4;
          break;
        }
        case VK_FORMAT_R16_UNORM:
        {
          mBytesPerPixel = 2;
          break;
        }
        case VK_FORMAT_D32_SFLOAT:
        {
          mBytesPerPixel = 4;
          break;
        }
        case VK_FORMAT_D16_UNORM:
        {
          mBytesPerPixel = 2;
          break;
        }
        default:
          break;
    }
  }



  // TODO There are 2 basic methods to copy one image into another with Vulkan.  1. You
  // can use VkCmdCopyImage - faster method but more restricted in the the sense that the
  // resolutions sizes, formats, etc. must match.  2. You can use VkCmdBlitImage (which is
  // what we'll use here) which gives more flexible but also slower In a more advanced
  // engine, we should write our own function that can do extra logic on a fullscreen
  // fragment shader.
  void FcImage::copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source)
  {
    VkImageBlit2 blitRegion = {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.srcOffsets[1].x = source->mWidth;
    blitRegion.srcOffsets[1].y = source->mHeight;
    blitRegion.srcOffsets[1].z = 1;
    blitRegion.dstOffsets[1].x = source->mWidth;
    blitRegion.dstOffsets[1].y = source->mHeight;
    blitRegion.dstOffsets[1].z = 1;
    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;
    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo = {};
    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.srcImage = source->mImage;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.dstImage = mImage;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmdBuffer, &blitInfo);
  }


  // TODO should consider the use case for when we want to make this operation part of an existing cmdBuffer using nullptr default should make that pretty easy to implement
  // TODO TRY passing in a command buffer for this copy
  void FcImage::copyFromBuffer(FcBuffer& srcBuffer, VkDeviceSize offset, uint32_t arrayLayer)
  {
    // allocate and begin the command buffer to transfer an image
    FcGpu& gpu = FcLocator::Gpu();

    // region of image to copy from and to
    VkBufferImageCopy imageCopyRegion{};
    // offset into data
    imageCopyRegion.bufferOffset = offset;
    // for calculating data spacing (row length of data)
    imageCopyRegion.bufferRowLength = 0;
    // similar to above but vertical spacing
    imageCopyRegion.bufferImageHeight = 0;
    // which aspect of image to copy
    // TODO should depend on the image type
    imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // mipmap level to copy
    imageCopyRegion.imageSubresource.mipLevel = 0;
    // starting array layer if we're using an array (cubemaps etc)
    imageCopyRegion.imageSubresource.baseArrayLayer = arrayLayer;
    // number of layers to copy starting at basearraylayer
    imageCopyRegion.imageSubresource.layerCount = mLayerCount;
    // offset into image (as ooposed to raw data in bufferOffset)
    imageCopyRegion.imageOffset = {0, 0, 0};

    // TODO seems silly to store image extent but then always have to reference
    // one of the 3 dimensions, usally forcing depth to one, better to just
    // store height, width, numLayers instead!
    imageCopyRegion.imageExtent = {mWidth, mHeight, 1};

    // create the command to copy a buffer to the image
    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
    vkCmdCopyBufferToImage(cmdBuffer, srcBuffer.getVkBuffer(), mImage
                           , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    //  finally, submit the transfer command to the command buffer and submit
    FcLocator::Renderer().submitCommandBuffer();
  }


  void FcImage::clear(VkCommandBuffer cmdBuffer, VkClearColorValue* pColor)
  {
    VkImageSubresourceRange range;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    range.baseMipLevel = 0;
    range.levelCount = mMipLevels;
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCmdClearColorImage(cmdBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pColor, 1, &range);
  }


  // This is just provided to allow destruction of image view when image and memory
  // destruction is provided elsewhere, such as in the swapchain function
  // TODO combine with null checks perhaps
  void FcImage::destroyImageView()
  {
    //if (mImageView != nullptr)
    // {
    vkDestroyImageView(FcLocator::Device(), mImageView, nullptr);
    //}
  }

  void FcImage::destroy()
  {
    destroyImageView();

    // ?? I think the vk destroy already checks for NULL
    //    if (mImage != nullptr)
    //    {
    vmaDestroyImage(FcLocator::Gpu().getAllocator(), mImage, mAllocation);
    //    }
  }


  void FcImage::writeToImage(void* pixelData, VkDeviceSize dataLength, bool generateMipmaps)
  {
    // Create a staging buffer first in order to transition the image to the gpu-local later
    FcBuffer stagingBuffer(dataLength, FcBufferTypes::Staging);
    // copy the actual image data into the staging buffer.

    // TODO use vkGetImageSubresourceLayout, rather than assuming contiguous memory and
    // using memcpy. It's unlikely that it would matter for a smaller image, but I think
    // that's the correct approach in general.

    stagingBuffer.write(pixelData, dataLength);

    // remove unneeded mappings using to increase the likelihood of successful future
    // calls to vkMapMemory() ?? API without secrets suggests that the unMapping of memory
    // is not necessary...  Memory in Vulkan doesn't need to be unmapped before using it
    // on GPU, but unless a memory types has VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag
    // set, you need to manually invalidate cache before reading of mapped pointer and
    // flush cache after writing to mapped pointer. Map/unmap operations don't do that
    // automatically.  // NOTE that the API without secrets calls for flushing the mapped
    // memory but the API documentation states that: If the memory object was created with
    // the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT set, vkFlushMappedMemoryRanges and
    // vkInvalidateMappedMemoryRanges are unnecessary and may have a performance
    // cost. However, availability and visibility operations still need to be managed on
    // the device. See the description of host access types for more information.
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#memory
    // vkFlushMappedMemoryRanges(gfx->device, 1, &flushRange);

    // create image and imageView to hold final texture
    // TODO load VK_SAMPLE_COUNT from GPU.properties

    // transition the image so that it's in the most efficient state to write to
    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

    transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED
                    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                    , VK_IMAGE_ASPECT_COLOR_BIT,  mMipLevels);

    FcLocator::Renderer().submitCommandBuffer();

    // copy data to image
    copyFromBuffer(stagingBuffer);

    // no longer need staging buffer so get rid of
    stagingBuffer.destroy();

    // transitioned to SHADER_READ_ONLY_OPTIMAL during mip map generation ?? might not be
    // the most efficient to transition one level at a time no longer needed -
    // transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels); TODO try to transition all
    // mipLevels after mipmaps are generated
    if (generateMipmaps)
    {
      // transition image and submit command buffer within GenerateMipMaps()
      generateMipMaps();
   }
    else
    {
      VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
      transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                      , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      FcLocator::Renderer().submitCommandBuffer();
    }
  }


  // TODO All of the helper functions that submit commands so far have been set up to
  // execute synchronously
  // by waiting for the queue to become idle. For practical applications it is recommended
  // to combine these operations in a single command buffer and execute them asynchronously
  // for higher throughput, especially the transitions and copy in the createTextureImage
  // function. Try to experiment with this by creating a setupCommandBuffer that the helper
  // functions record commands into, and add a flushSetupCommands to execute the commands
  // that have been recorded so far. It's best to do this after the texture mapping works to
  // check if the texture resources are still set up correctly.
  void FcImage::createTexture(uint32_t width, uint32_t height, void* pixelData
                              , VkDeviceSize size, bool generateMipmaps, VkFormat format)
  {
    // TODO see if this can always be set sooner, like from loadTexture()
    mFormat = format;

    // TODO load VK_SAMPLE_COUNT from GPU.properties or pass in
    // create image and imageView to hold final texture
    if (generateMipmaps)
    {
      createImage(width, height, FcImageTypes::TextureGenerateMipmaps);
    }
    else
    {
      createImage(width, height, FcImageTypes::Texture);
    }

    writeToImage(pixelData, size, generateMipmaps);

    // ?? May want to eliminate this since may not be able to multithread
    // ?? research multithreading static variables
    /* ++index; */
  }
}// --- namespace fc --- (END)
