//>--- fc_font.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
#include "fc_types.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <map>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  class FcFont
  {
   private:
     struct Character
     {
        glm::ivec2 size;
        glm::ivec2 bearing;
         // TODO might be able to eliminate some of the metrics here
         // TODO should be able to eliminate some metrics of monospaced font
        int32_t location;
        uint32_t advance;
     };

     friend class FcText;
     BillboardPushConstants mTextBoxSpecs;
     std::map<char, Character> mCharacters;
     int mFirstGlyph;
     int mLastGlyph;
     FcImage mRasterTexture;

   public:
//     VkBuffer& VertexBuffer() { return mTextBoxMesh.VertexBuffer(); }
     BillboardPushConstants& Push() { return mTextBoxSpecs; }
      //   VkBuffer IndexBuffer() { return mTextBoxMesh.IndexBuffer(); }
//     uint32_t IndexCount() { return mTextBoxMesh.VertexCount(); }
     void createRasterFont(std::string fontName
                           , int size, int firstGlyph = 0, int lastGlyph = 255);
     void free();
      // GETTERS
  };

}// --- namespace fc --- (END)
