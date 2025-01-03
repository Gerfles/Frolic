#include "fc_image.hpp"

// Frolic Engine
#include "core/fc_buffer.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_renderer.hpp"
#include "core/utilities.hpp"
#include "fc_gpu.hpp"
#include "fc_pipeline.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ratio>
// external libraries
// TODO place all implementation header defines into one header file??
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "vulkan/vulkan_core.h"
// std libraries
#include <stdexcept>



namespace fc
{

  FcImage::FcImage(VkImage image)
  {
    mImage = image;
  }


   // ?? assuming there's a reason we need vkExtent 3D and not 2D
  void FcImage::create(VkExtent3D imgExtent, VkFormat format, VkSampleCountFlagBits msaaSampleCount
                       , VkImageUsageFlags useFlags, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
  {
     //VkDevice device = FcLocator::Device();
    mImageExtent = imgExtent;

     // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = imgExtent;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1; // used in things like cube maps
    imageInfo.format = format;
//    imageInfo.tiling = tiling;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
     // ?? Not sure if this is even required for most things
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = useFlags;
    imageInfo.samples = msaaSampleCount;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // whether image can be shared between queues

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT ;
     // TODO probably need to allow passage of a specific bit to account for vmaMemory.requiredFlags
     // may no longer be necessary however with vma having usage auto set
     allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if(vmaCreateImage(FcLocator::Gpu().getAllocator(), &imageInfo, &allocInfo, &mImage, &mAllocation, nullptr)
       != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Image!");
    }


     // if (vkCreateImage(device, &imageInfo, nullptr, &mImage) != VK_SUCCESS)
     // {
     //   throw std::runtime_error("Failed to create a Vulkan Image!");
     // }

     //  // get memory requirements for a type of image
     // VkMemoryRequirements memRequirments;
     // vkGetImageMemoryRequirements(device, mImage, &memRequirments);

     //  // get properties of physical device memory
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
    createImageView(format, aspectFlags, mipLevels);
  }



  void FcImage::createImageView(VkFormat imageFormat, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
  {
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = mImage;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = imageFormat;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // allows remapping of rgba component to other values
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

     // Subresources allow the view to view only a part of an image
    imageViewInfo.subresourceRange.aspectMask = aspectFlags; // which aspect of image to view (eg COLOR_BIT for viewing color)
    imageViewInfo.subresourceRange.baseMipLevel = 0;         // start mipmap level to view from
    imageViewInfo.subresourceRange.levelCount = mipLevels;   // number of mipmap levels to view
    imageViewInfo.subresourceRange.baseArrayLayer = 0;       // start array level to view from
    imageViewInfo.subresourceRange.layerCount = 1;           // number of array levels to view

    if (vkCreateImageView(FcLocator::Device(), &imageViewInfo, nullptr, &mImageView) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create an Image View!");
    }
  }

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  void FcImage::transitionImage(VkCommandBuffer commandBuffer, VkImageLayout currentLayout
                                , VkImageLayout newLayout, uint32_t mipLevels)
  {
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.oldLayout = currentLayout; // layout to transition from
    imageBarrier.newLayout = newLayout; // layout to transition to
    imageBarrier.image = mImage;        // image being accessed and modifies as part of barrier

     // TODO try uncommenting the following
     // Queue family to transition from - IGNORED means don't bother transferring to a different queue
     // imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
     // imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

     // TODO
     // This is how it's done in vulkan 1.3 with extension... but this implementation is inefficient. This will stall the GPU
     // to wait for all stages and if we want post-processing, we should be more precise about when and which stages should be set
     // great documentation: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

     // set aspect of image being altered to color image (default) unless new layout required is depth image
    imageBarrier.subresourceRange.aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                               ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

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


// TODO eliminate function completely in favor of transition image
// *-*-*-*-*-*-*-*-*-*-*-   STOPGAP UNTIL BETTER DESIGNED   *-*-*-*-*-*-*-*-*-*-*- //
  void FcImage::imageToGpu(VkImageLayout oldLayout
                           , VkImageLayout newLayout, uint32_t mipLevels)
  {

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;
    imageMemoryBarrier.newLayout = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = mImage;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
      srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else
    {
      srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

      imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();
    vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    FcLocator::Renderer().submitCommandBuffer();
  }



  uint32_t FcImage::loadTexture(std::string filename)
  {
     // load image file
     // TODO determine if there's a better way to use stbi_load with VkExtent
    int width, height;

     // number of channels image uses --Not using but tutorial has for future
    int channels;

     //TODO make an Install path variable that is used for file finding
    std::string filepath = "textures/" + filename;

    stbi_uc* imageData = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!imageData)
    {
      throw std::runtime_error("Failed to load texture file: " + filename);
    }

     // first select the largest dimension (widht or height), then calc the number of times that can be
     // divided by 2. floor gets the gretest integer number of times and then add 1 so original image has mip level.
     // TODO determine if this really needs to go all the way down to 1x1 pixel
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkExtent3D extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

     // using the image data from the loaded file, create the hardware texture used by the GPU
    uint32_t textureId = createTexture(extent, imageData, mipLevels);

     // free original image data
    stbi_image_free(imageData);

     // return the descriptor index returned by createTextureDescriptor();
    return textureId;
  }



   // It should be noted that it is uncommon in practice to generate the mipmap levels at runtime
   // anyway. Usually they are pregenerated and stored in the texture file alongside the base level
   // to improve loading speed. TODO Implementing resizing in software and loading multiple levels from a
   // file is left as an exercise to the reader.
   // TODO - give a feature that turns mipMaping off (still creating/loading the mipLevels though)
  void FcImage::generateMipMaps(uint32_t mipLevels)
  {

    FcGpu& gpu = FcLocator::Gpu();
     // check if image format supports linear blitting first
     // TODO check this once at startup - NOT FOR EVERY IMAGE
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(gpu.physicalDevice(), VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
       // could search for a common image format that does support linear blitting here or use software
       // like stb_image_resize to generate mipmaps before hand--for now, just throw an error
      throw std::runtime_error("Failed to generate mipmaps - graphics device not capable of linear blitting for this texture format!");
    }



     // this part will remain the same for all barriers - the rest will change depending on the mip level we're on
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = mImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = mImageExtent.width;
    int32_t mipHeight = mImageExtent.height;
     //

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();


     // record a VkCmdBlitImage() command for each mip level, except for the first mimMap level (original image)
    for (size_t i = 0; i < mipLevels - 1; ++i)
    {
      barrier.subresourceRange.baseMipLevel = i;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                           , VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

       // TODO take all of the non-changing blit stuff out of the loop
      VkImageBlit blit{};
      blit.srcOffsets[0] = {0,0,0};
      blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
      blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.srcSubresource.mipLevel = i;
      blit.srcSubresource.baseArrayLayer = 0;
      blit.srcSubresource.layerCount = 1;
      blit.dstOffsets[0] = {0,0,0};
       // set the next mipMap dimensions, making sure not to get any smaller than 1 pixel!
      blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
      blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blit.dstSubresource.mipLevel = i + 1;
      blit.dstSubresource.baseArrayLayer = 0;
      blit.dstSubresource.layerCount = 1;

      vkCmdBlitImage(cmdBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                     , mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

       // now transition image layout of previous mipLevel to shader read only format (all sampling operations will wait on this transition to finish)
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
       //
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                           , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

       // halve current mip dimensions, making sure not to halve if only one pixel wide/high
       // TODO probably could bit shift here (though the compiler probably does this already)
      if (mipWidth > 1) mipWidth /= 2;
      if (mipHeight > 1) mipHeight /= 2;
    }

     // one last transition to handle the last mip level since this won't be handled by the loop
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
     //
    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT
                         , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

     // finally, submit all commands the the graphics queue
    FcLocator::Renderer().submitCommandBuffer();
  }


  void FcImage::createTextureSampler(uint32_t mipLevels)
  {
    FcGpu& gpu = FcLocator::Gpu();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;                            // How to render when image is magnified on the screen
    samplerInfo.minFilter = VK_FILTER_LINEAR;                            // How to render when image is minified on the screen
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // How to handle wrap in the U (x) direction
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // How to handle wrap in the V (y) direction
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;           // How to handle wrap in the W (z) direction
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;          // Border beyond texture (when clamp to border is used--good for shadow maps)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;			 // WILL USE NORMALIZED COORD. (coords will be between 0-1)
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;              // Mipmap interpolation mode (between two levels of mipmaps)
    samplerInfo.mipLodBias = 0.0f;                                       // used to force vulkan to use lower level of detail and mip level
    samplerInfo.minLod = 0.0f;


    samplerInfo.maxLod = static_cast<float>(mipLevels);                                           // maximum level of detail to pick mip level
    samplerInfo.anisotropyEnable = VK_TRUE;                              // enable anisotropy
     // TODO should allow this to be user definable or at least profiled at install/runtime
    samplerInfo.maxAnisotropy = gpu.Properties().maxSamplerAnisotropy; // Amount of anisotropic samples being taken

    if (vkCreateSampler(gpu.getVkDevice(), &samplerInfo, nullptr, &mTextureSampler) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }
  }

   // TODO There are 2 basic methods to copy one image into another with Vulan.
   // 1. You can use VkCmdCopyImage - faster method but more restricted in the the sense that the resolutions
   //    sizes, formats, etc. must match.
   // 2. You can use VkCmdBlitImage (which is what we'll use here) which gives more flexible but also slower
   // In a more advanced engine, we should write our own function that can do extra logic on a fullscreen
   // fragment shader.
  void FcImage::copyFromImage(VkCommandBuffer cmdBuffer, FcImage* source, VkExtent2D srcSize, VkExtent2D dstSize)
  {
    VkImageBlit2 blitRegion = {};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
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


   // TODO should consider the use case for when we want to make this operation part of an existing cmdBuffer
   // TODO TRY passing in a command buffer for this copy
  void FcImage::copyFromBuffer(FcBuffer& srcBuffer, VkDeviceSize bufferSize)
  {
     // allocate and begin the command buffer to transfer an image
    FcGpu& gpu = FcLocator::Gpu();

     // region of image to copy from and to
    VkBufferImageCopy imageCopyRegion{};
    imageCopyRegion.bufferOffset = 0;                                        // offset into data
    imageCopyRegion.bufferRowLength = 0;                                     // for calculating data spacing (row length of data)
    imageCopyRegion.bufferImageHeight = 0;                                   // similar to above but vertical spacing
    imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // which aspect of image to copy
    imageCopyRegion.imageSubresource.mipLevel = 0;                           // mipmap level to copy
    imageCopyRegion.imageSubresource.baseArrayLayer = 0;                     // starting array layer if we're using an array (cubemaps etc)
    imageCopyRegion.imageSubresource.layerCount = 1;                         // number of layers to copy starting at basearraylayer
    imageCopyRegion.imageOffset = {0, 0, 0};                                 // offset into image (as ooposed to raw data in bufferOffset)
    imageCopyRegion.imageExtent = mImageExtent;

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
    VkDevice device = FcLocator::Device();
     // destroy texture image sampler if it exists
     if (mTextureSampler != nullptr)
     {
       vkDestroySampler(device, mTextureSampler, nullptr);
     }

     destroyImageView();

      // TODO I think the vk destroy already checks for NULL
      //    if (mImage != nullptr)
      //    {
      vmaDestroyImage(FcLocator::Gpu().getAllocator(), mImage, mAllocation);
       //    }
  }


   // TODO All of the helper functions that submit commands so far have been set up to execute synchronously
// by waiting for the queue to become idle. For practical applications it is recommended to combine
// these operations in a single command buffer and execute them asynchronously for higher
// throughput, especially the transitions and copy in the createTextureImage function. Try to
// experiment with this by creating a setupCommandBuffer that the helper functions record commands
// into, and add a flushSetupCommands to execute the commands that have been recorded so far. It's
// best to do this after the texture mapping works to check if the texture resources are still set
// up correctly.

   // TODO require an init function or remove the pGpu parameter... this function doesn't use it and
   // I think other functions doen't need it either--but it's confusing and errore prone to keep an
   // unitialized pointer around
  uint32_t FcImage::createTexture(VkExtent3D extent, void* pixelData, uint32_t mipLevels)
  {
     // TODO determine if having member dimensions is useful
    mImageExtent = extent;

    writeToTexture(pixelData, mipLevels);

     // create the texture sampler
    createTextureSampler(mipLevels);

     // transitioned to SHADER_READ_ONLY_OPTIMAL during mip map generation ?? might not be the most efficient to transition one level at a time
     // no longer needed - transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);
     // TODO try to transition all mipLevels after mipmaps are generated
    if (mipLevels > 1)
    {
      generateMipMaps(mipLevels);
    }

     // return the descriptor index returned by createTextureDescriptor();
    return FcLocator::DescriptorClerk().createTextureDescriptor(mImageView, mTextureSampler);
  }




//   void FcImage::overwriteTexture(void* pixelData, uint32_t mipLevels)
//   {
//     VkDeviceSize imageSize = mWidth * mHeight * 4;

//     FcBuffer stagingBuffer;
//     stagingBuffer.storeData(pixelData, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

//     transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
//                     , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

//      // copy data to image
//     copyImage(commandBuffer, stagingBuffer, imageSize);

//     transitionImage(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
//                     , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

//     stagingBuffer.destroy();

// //    writeToTexture(width, height, pixelData, mipLevels);
//   }




  void FcImage::writeToTexture(void* pixelData, uint32_t mipLevels)
  {
     // Copy data to a staging buffer first in order to transition the image and send it to the gpu
    VkDeviceSize imageSize = mImageExtent.width * mImageExtent.height * 4;  // here we have to use 4 and not channels since ?? I think the image has no alpha channel

     // create staging buffer to hold loaded data, ready to copy to device
    FcBuffer stagingBuffer;
     // copy the actual image data into the staging buffer.

    stagingBuffer.storeData(pixelData, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

     // TODO check if static cast is recommended
     // stagingBuffer.overwriteData(pixelData, static_cast<size_t>(imageSize));

     // TODO use vkGetImageSubresourceLayout, rather than assuming contiguous memory and using memcpy. It's
     // unlikely that it would matter for a 512x512 image, but I think that's the correct approach in
     // general.

     // remove unneeded mappings using to increase the likelihood of successful future calls to vkMapMemory()
     // ?? API without secrets suggests that the unMapping of memory is not necessary...
     // Memory in Vulkan doesn't need to be unmapped before using it on GPU, but unless a memory types
     // has VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag set, you need to manually invalidate cache
     // before reading of mapped pointer and flush cache after writing to mapped pointer. Map/unmap
     // operations don't do that automatically.  // NOTE that the API without secrets calls for
     // flushing the mapped memory but the API documentation states that: If the memory object was
     // created with the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT set, vkFlushMappedMemoryRanges and
     // vkInvalidateMappedMemoryRanges are unnecessary and may have a performance cost. However,
     // availability and visibility operations still need to be managed on the device. See the
     // description of host access types for more information.
     // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#memory
     // vkFlushMappedMemoryRanges(gfx->device, 1, &flushRange);

     // create image and imageView to hold final texture
    create(mImageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT
           , VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
           , VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);

     // transition the image buffer so that it is most efficient to be written to

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

    transitionImage(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED
                    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

    FcLocator::Renderer().submitCommandBuffer();
     //DEL
     //transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

     // copy data to image
    copyFromBuffer(stagingBuffer, imageSize);

     // no longer need staging buffer so get rid of
    stagingBuffer.destroy();
  }



}                                                                                     // namespace fc _END_
