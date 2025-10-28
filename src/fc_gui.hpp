// fc_gui.hpp
#pragma once


#include "core/platform.hpp"
#include <array>
#include <vector>

namespace fc
{
  class Frolic;

  class FcGUI
  {
   private:
     // // Note these do not necessarily enable the feature -> ONLY when it is already included in glTF
     bool mUseColorTexture {true};
     bool mUseRoughMetalTexture {true};
     bool mUseOcclussionTexture {true};
     bool mUseNormalTexture {true};
     bool mUsePrimitiveTangents {true};
     bool mUseEmissiveTexture {true};
     bool mRotateModel{false};
     bool mCycleExpansion{false};

     std::array<std::vector<u32>, 5> helmetTexIndices;
     std::array<std::vector<u32>, 5> sponzaTexIndices;
   public:
     void drawGUI(Frolic* fc);
  };




}// --- namespace fc --- (END)
