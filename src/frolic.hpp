#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_descriptors.hpp"
#include "core/fc_input.hpp"
#include "core/fc_mesh.hpp"
#include "core/fc_pipeline.hpp"
#include "core/fc_renderer.hpp"
#include "core/fc_text.hpp"
#include "core/fc_font.hpp"
#include "core/fc_player.hpp"

#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{

  class Frolic
  {
   private:

      // TODO should consider making some/all these cenntral to the engine and not the app
     static constexpr float MAX_FRAME_TIME = 0.05f;
     static constexpr int MAX_FRAME_SAMPLES = 1000;

     // performance tracking
     float mFrameTimeSum = 0;
     int mFrameTimeIndex = 0;
     float mFrameTimeList[MAX_FRAME_SAMPLES];

      // note: order of declaration matters (member variables are allocated top to bottom and destroyed in the reverse order)
     // TODO delete
     //GlobalUbo mUbo;

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO delete these pipelines and add a default to FcRenderer
     FcPipeline mGradientPipeline;
     FcPipeline mSkyPipeline;
     FcPipeline mMeshPipeline;

     std::vector<FcPipeline*> mPipelines;
     std::vector<ComputePushConstants> mPushConstants{2};

     SceneDataUbo* pSceneData {nullptr};

     // Note these do not necessarily enable the feature -> ONLY when it is already included in glTF
     bool mUseColorTexture {true};
     bool mUseRoughMetalTexture {true};
     bool mUseOcclussionTexture {true};
     bool mUseNormalTexture {true};
     bool mUsePrimitiveTangents {true};
     bool mUseEmissiveTexture {true};
     bool mRotateModel{false};
     bool mCycleExpansion{false};
     bool mDebugShadowMap{false};
     bool mUseAlternatePipeline{false};
     float mPlayerMovementSpeed;
     //
     FcPlayer mPlayer;
     FcPlayer uvnPlayer;

     FcBillboard mSunBillboard;


      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   (END) NEW   -**-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcRenderer mRenderer;
     FcInput mInput;
     bool mShouldClose = false;
     SDL_Event mEvent;
     FcStats* stats;
      // TODO for now hold point lights but later place in locator
      //     std::vector<FcLight> mLights;

      // UI
     std::vector<FcText> mUItextList;
     FcFont mUIfont;

     FcImage mFallbackTexture;
      // ideally, here it would be nice to

   public:
     Frolic();
     void run();
     void drawGUI();
     void loadGameObjects();
     void loadUIobjects();

     void update(float deltaTime);
     int calcFPS(float lastFrameTime);
     void close();
  };

}
