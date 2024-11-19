#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "core/fc_descriptors.hpp"
#include "core/fc_text.hpp"
#include "fc_billboard_render_system.hpp"
#include "fc_image.hpp"
//#include "core/mesh.h"
//#include "fc_pipeline.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "glm/glm.hpp"
#include <ft2build.h>
//?? what's up with the following includes
#include FT_FREETYPE_H /* wierd font-lock issue TODO solve issue */
#include FT_GLYPH_H
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <map>


namespace fc
{

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
     BillboardPushComponent mTextBoxSpecs;
      //
//     FcMesh mTextBoxMesh;

     std::map<char, Character> mCharacters;
     int mFirstGlyph;
     int mLastGlyph;
     FcImage mRasterTexture;




   public:
//     VkBuffer& VertexBuffer() { return mTextBoxMesh.VertexBuffer(); }
     BillboardPushComponent& Push() { return mTextBoxSpecs; }
      //   VkBuffer IndexBuffer() { return mTextBoxMesh.IndexBuffer(); }
//     uint32_t IndexCount() { return mTextBoxMesh.VertexCount(); }
     void createRasterFont(std::string fontName, int size, int firstGlyph = 0, int lastGlyph = 255);
     void free();
      // GETTERS
  };

} // namespace fc _END_
