#include "fc_text.hpp"

// - CORE -
#include "fc_renderer.hpp"
#include "fc_locator.hpp"
#include "fc_font.hpp"
 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <vector>
#include <cstring>

namespace fc
{

// TODO prevent creating multiple text boxes or, better, make sure to ADD those to vert/ndx buffers
   // TODO, should allow color to be sent but would have to clear the background
   // TODO -- need to create a billboard for camera and movement invariance
   // TODO consider setting a default text box
  void FcText::createTextBox(int xPos, int yPos, int width, int height)
  {
     // normalize the parameters/coordinates (-1.0 to 1.0f)
    VkExtent2D& screenDimensions = FcLocator::ScreenDims();

     // TODO document why this is the way it is (setting to NDC and doing this once on the cpu so it's easier for each vertex to process)
     // TODO could also make a dynamic box that moves constantly which would be easier to let the GPU calculate it's dimensions etc.
    // FIXME these are disabled for now until class is re-written for current engine
    // mTextBoxSpecs.width = (float)(width) / screenDimensions.width;
    // mTextBoxSpecs.height = (float)(height) / screenDimensions.height;
    // mTextBoxSpecs.position.x =  -1.0f + 2.0f * (float)xPos / screenDimensions.width + mTextBoxSpecs.width;
    // mTextBoxSpecs.position.y =  -1.0f + 2.0f * (float)yPos / screenDimensions.height + mTextBoxSpecs.height;

    // TODO get rid of, TEMPORARILY CREATE A BLACK TEXTURE
     // create an array of "pixels" that will be used to create the underlying texture
    size_t imageSize = width * height;
    uint32_t pixels[imageSize];
     // color all the pixels black for simplicity
    std::memset(pixels, 0, sizeof(pixels));

    // TODO make sure that we are not create all new textures every time we alter text...
    // should only be one texture that we alter accordingly
     // create the default texture that will be used to fill the text box

    mTextImage.createTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height)
                             , pixels, width * height * 4);
     // BUG must create descriptorset
     // create the actual billboard that text will get rendered to
     //mTextBoxMesh.createMesh(pGpu, vertices, indices, 0);
  }


  // ?? possible methods for rendering text from raster texture:
// vkCmdCopyImage() - no scaling, raw data transfer
// vkCmdBlitImage() - scaling, conversion, etc.
// vkCmdCopyBufferToImage() - similar to vkcmdcopyimage() but not a lot of details documented
   // TODO let color chage text--might be better to rasterize any color we intend to use, or probably better to change in shader
  void FcText::createText(const std::string& newText, int xPos, int yPos, float scale)
  {
     // store all regions of where to copy image data from and to
    std::vector<VkImageBlit> blitsList;//(newText.size());
    VkExtent2D boxDims = writeTextTexture(newText, scale, blitsList);


     // BUG may need to include the boxOffset as was previously used
     // createTextBox(xPos, yPos - boxOffset, penPos, boxHeight + boxOffset);
    createTextBox(xPos, yPos, boxDims.width, boxDims.height);

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

     // submit all the image blits at the same time
    vkCmdBlitImage(cmdBuffer, pFont->mRasterTexture.Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                   , mTextImage.Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                   , blitsList.size(), blitsList.data(), VK_FILTER_LINEAR);

    mTextImage.transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

     // TODO - could later add mipmaps if being used a texture far away potentially (allow parameter option at least)
     // mOutputTexture.createImageView(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    // mOutputTexture.createTextureSampler();

    FcLocator::Renderer().submitCommandBuffer();
  }

   // editText is a faster operation than createTex as it overwrites the pixels in the already allocated image
   // but it is the resposibility of the caller to make sure the texture size does not need to change in this case
  void FcText::editText(const std::string newText, int xPos, int yPos, float scale)
  {
     // store all regions of where to copy image data from and to
    std::vector<VkImageBlit> blitsList;//(newText.size());
    writeTextTexture(newText, scale, blitsList);

    // mTextBoxSpecs.position.x = xPos;
    // mTextBoxSpecs.position.y = yPos;

    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

     // BUG may have to transition image initially to something else to erase/write to it
    mTextImage.transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                               , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

     // first clear out the old texture
    VkClearColorValue clearColor{0, 0, 0, 0};
    mTextImage.clear(cmdBuffer, &clearColor);

     // submit all the image blits at the same time
    vkCmdBlitImage(cmdBuffer, pFont->mRasterTexture.Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                   , mTextImage.Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                   , blitsList.size(), blitsList.data(), VK_FILTER_LINEAR);

//    FcLocator::Gpu().submitCommandBuffer(blitCommandBuffer);

    mTextImage.transitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    FcLocator::Renderer().submitCommandBuffer();
  }


   // TODO store popular glyph metrics (maxHeight, boundingBox sizes, etc. inside font class)
  VkExtent2D FcText::writeTextTexture(const std::string& text, float scale, std::vector<VkImageBlit>& blitsList)
  {
         // initial calculations to determine text box size etc;
    FcFont::Character glyph = pFont->mCharacters.at(*text.begin());
     // take care to render the first character glyph all the way to left side of text box
     // The +/- 2 shift is to keep strange artifacts from wrapping around to the bottom of the texture when REPEAT sampling is used
    int32_t penPos =  0 - glyph.bearing.x + 2;
    uint32_t boxHeight = glyph.size.y + glyph.bearing.y + 2;
    uint32_t boxOffset = glyph.bearing.y - 2;

    for (std::string::const_iterator ch = text.begin(); ch != text.end(); ch++)
    {
       // extract each glyph within the string
      FcFont::Character glyph = pFont->mCharacters.at(*ch);

       // determine if we need to "push" the box down to capture all the text
      if (glyph.bearing.y < boxOffset)
      {
        boxOffset = glyph.bearing.y;
      }

      // find the current character in the raster texture
      int32_t srcX = glyph.location;
      int32_t srcWidth = srcX + glyph.size.x;
      int32_t srcHeight = glyph.size.y;

       // figure out where to paste the character within the output texture
      int32_t dstX = penPos + glyph.bearing.x * scale;
       // BUG probably won't render size correctly
      int32_t dstY = (glyph.bearing.y - boxOffset) * scale;
      int32_t dstWidth = dstX + glyph.size.x * scale;
      int32_t dstHeight = dstY + glyph.size.y * scale;

       // note that the z = 1 since a 2D image has a depth of 1
      VkImageBlit blitInfo;
      blitInfo.srcOffsets[0] = {srcX, 0, 0};
      blitInfo.srcOffsets[1] = {srcWidth, srcHeight, 1};
      blitInfo.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blitInfo.srcSubresource.mipLevel = 0;
      blitInfo.srcSubresource.baseArrayLayer = 0;
      blitInfo.srcSubresource.layerCount = 1;
       // destination image
      blitInfo.dstOffsets[0] = {dstX, dstY, 0};
      blitInfo.dstOffsets[1] = {dstWidth, dstHeight, 1};
      blitInfo.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      blitInfo.dstSubresource.mipLevel = 0;
      blitInfo.dstSubresource.baseArrayLayer = 0;
      blitInfo.dstSubresource.layerCount = 1;

       // store copy regions in order to blit all memory at one time (faster)
       // TODO use std::move() and presize the blitsInfo vector
      blitsList.push_back(blitInfo);

       //advace the position of where to draw the next glyph
      penPos += glyph.advance;

    } // end for('each character')
    return VkExtent2D{static_cast<uint32_t>(penPos), boxHeight};
  }


  void FcText::free()
  {
    mTextImage.destroy();
  }




} // _END_ namespace fc
