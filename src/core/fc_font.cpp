#include "fc_font.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_renderer.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "fc_debug.hpp"
 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "freetype/config/ftheader.h"
#include "vulkan/vulkan_core.h"
#include "stb_image.h"
#include "SDL2/SDL_log.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

// UTILS
//#include <ft2build.h>
//#include FT_BITMAP_H
//#include FT_FREETYPE_H /*weird font lock issues*/
//#include FT_GLYPH_H  /* wierd font-lock issue */

namespace fc
{


   // TODO this function may need to be broken up
  void FcFont::createRasterFont(std::string fontName
                                , int size, int firstGlyph, int lastGlyph)
  {
    int result;
    FT_Library fontLibrary;

     // load the font and get metrics
    result = FT_Init_FreeType(&fontLibrary);
    DBG_ASSERT_FT(result, "Could not init FreeType Library");

    FT_Face face;
    result = FT_New_Face(fontLibrary, fontName.c_str(), 0, &face);
    DBG_ASSERT_FT(result, "Failed to load font");

     // set up the boundaries of the glyphs to render to an image
    mFirstGlyph = firstGlyph;
    mLastGlyph = lastGlyph;

     //
    if (lastGlyph <= firstGlyph || lastGlyph > face->num_glyphs)
    {
      mLastGlyph = face->num_glyphs;
    }

     // TODO might be able to implement a fixed width font creater this way passing a simple bool as FW
     // setting width to 0 lets FT determine font size dynamically
    FT_Set_Pixel_Sizes(face, 0, size);

     // keep track of dimensions needed to store all glyphs in image
     // TODO DELETE in favor of VkExtent2D and create conversion to extent 3D
    uint32_t imageWidth = 0;
    uint32_t tallestChar = 0;

     // hold onto all the glyphs for now until we can figure out the total pixel width for rasterImg
    std::vector<FT_Glyph> glyphs;
    FT_GlyphSlot curGlyph = face->glyph; // shorthand

    for (unsigned char ch = mFirstGlyph; ch < mLastGlyph; ch++)
    {
       // load each glyph and save it to the glyph vector
      unsigned char charIndex = FT_Get_Char_Index(face, ch);
      result = FT_Load_Glyph(face, charIndex, FT_LOAD_RENDER);
      DBG_ASSERT_FT(result, "Failed to load glyph");

       // ?? doesn't seem to exclude the proper characters
       // don't add any glyphy that does not have an associated bitmap
       // if (curGlyph->bitmap.rows == 0 || curGlyph->bitmap.width == 0)
       // {
       //   continue;
       // }

       // if the glyph has an image add it to our vector
      FT_Glyph loadedGlyph;
      result = FT_Get_Glyph(curGlyph, &loadedGlyph);
      DBG_ASSERT_FT(result, "Failed to retrieve currently loaded glyph");

      glyphs.push_back(loadedGlyph);

       // extract the pertinent glyph metrics
      uint32_t charHeight = curGlyph->bitmap.rows;
      uint32_t charWidth = curGlyph->bitmap.width;
      uint32_t charLowering = charHeight - (curGlyph->metrics.horiBearingY >> 6);

      Character character;
      character.size = glm::ivec2(charWidth, charHeight);
      character.bearing = glm::ivec2(curGlyph->bitmap_left, charLowering);
       // bitshift here to get the advance in terms of pixels since FT stores in longer format;
      character.advance = static_cast<uint32_t>(curGlyph->advance.x >> 6);
      character.location = imageWidth;

      mCharacters.insert(std::pair<char, Character>(ch, character));

       // tabulate the image size needed to store all characters
      imageWidth += charWidth;

       // since final raster image should just be one long row of letters, find max image height
      if (charHeight > tallestChar)
      {
        tallestChar = charHeight;
      }

    } // END for(...)

     // TODO check speed difference when using array instead, also might be faster to just go through
     // glyphs twice

     //convert the 8bit grayscale image to 32 bit with the grayscale turned into alpha map uint8_t
    uint32_t imageSize = imageWidth * tallestChar;
    uint32_t imageBuffer[imageSize];

    size_t imagePos = 0;
    size_t charAdvance = 0;

     // TODO write 4-byte versions of memcpy & memset  std::memset(imageBuffer, textColor, imageSize);
    for (unsigned char glyphIndex = 0; glyphIndex < glyphs.size(); glyphIndex++)
    {
       // adjust the y-axis offset (bearing) of the characters based on difference to tallestchar
      unsigned char ch = glyphIndex + firstGlyph;
      mCharacters.at(ch).bearing.y += tallestChar - mCharacters.at(ch).size.y;

      result = FT_Glyph_To_Bitmap(&glyphs[glyphIndex], FT_RENDER_MODE_NORMAL, nullptr, true);
      DBG_ASSERT_FT(result, "failed to render glyph to bitmap");

       // bit of tricky casting and manipulation to make bitmap easier to access
      FT_BitmapGlyph currGlyph = (FT_BitmapGlyph)glyphs[glyphIndex];
      FT_Bitmap& glyphBitmap = currGlyph->bitmap;

      uint32_t glyphWidth = glyphBitmap.width;
      uint32_t glyphHeight = glyphBitmap.rows;

      for (int y = 0, glyphPos = 0; y < glyphHeight; y++)
      {
        imagePos = imageWidth * y + charAdvance;

        for (int x = 0; x < glyphWidth; x++)
        {
          imageBuffer[imagePos++] = glyphBitmap.buffer[glyphPos++] << 24;
        }
      }
      charAdvance += glyphWidth;
    }

     // clean up all stored glyph objects and free the freeType resources
    for (FT_Glyph glyph : glyphs)
    {
      FT_Done_Glyph(glyph);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(fontLibrary);

     // TODO DELETE
    VkExtent3D temp = {imageWidth, tallestChar, 1};
     // TODO upload texture as a transfer_dst_optimal since we won't be drawing to it once it's rendered
    mRasterTexture.createTexture(temp, imageBuffer);

     // after raster image is created, transition layout so it's in the best format for being blitted from
    VkCommandBuffer cmdBuffer = FcLocator::Renderer().beginCommandBuffer();

    mRasterTexture.transitionImage(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                   , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

    FcLocator::Renderer().submitCommandBuffer();

  }


  void FcFont::free()
  {
//    mTextBoxMesh.destroy();
    mRasterTexture.destroy();
  }

} // namespace fc _END_
