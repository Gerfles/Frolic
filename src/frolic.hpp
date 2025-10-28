#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_input.hpp"
#include "core/fc_pipeline.hpp"
#include "core/fc_renderer.hpp"
#include "core/fc_text.hpp"
#include "core/fc_font.hpp"
#include "core/fc_player.hpp"
#include "fc_gui.hpp"

#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{

  class Frolic
  {
   private:

      // TODO should consider making some/all these central to the engine and not the app
     static constexpr float MAX_FRAME_TIME = 0.05f;
     static constexpr int MAX_FRAME_SAMPLES = 1024;

     // performance tracking TODO place in stats class
     float mFrameTimeSum {0};
     int mFrameTimeIndex {0};
     float mFrameTimeList[MAX_FRAME_SAMPLES];

     // note: order of declaration matters (member variables are allocated top to bottom
     // and destroyed in the reverse order)
     SceneDataUbo* pSceneData {nullptr};
     //
     bool mShouldDrawDebugShadowMap{false};
     //
     FcPlayer mPlayer;
     //
     FcScene helmet;
     FcScene sponza;
     //
     FcBillboard mSunBillboard;
     FcBillboard mTest;
     // UI
     FcGUI gui;
     //
     FcAllocator* pAllocator;
     StackAllocator mStackAllocator;
     FcRenderer mRenderer;
     FcInput mInput;
     bool mShouldClose {false};
     SDL_Event mEvent;
     FcStats* stats;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TEMP   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     float angle {0.f};
     int rotationSpeed {0};

   public:
     Frolic();
     void run();
     void loadGameObjects();
     void update(float deltaTime);
     int calcFPS(float lastFrameTime);
     void close();
     friend class FcGUI;
  };

}
