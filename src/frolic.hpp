#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_input.hpp"
#include "core/fc_renderer.hpp"
#include "core/fc_text.hpp"
#include "core/fc_timer.hpp"

#include <vector>
// - EXTERNAL LIBRARIES -
// ?? NOT SURE why this compiles without SDL
// - STD LIBRARIES -


namespace fc
{

  class Frolic
  {
   private:
      // TODO should consider making some/all these cenntral to the engine and not the app
     static constexpr float MAX_FRAME_TIME = 0.05f;
     static constexpr int MAX_FRAME_SAMPLES = 1000;

     // performance tracking
     FcTimer mTimer;
     float mFrameTimeSum = 0;
     int mFrameTimeIndex = 0;
     float mFrameTimeList[MAX_FRAME_SAMPLES];

      // note: order of declaration matters (member variables are allocated top to bottom and destroyed in the reverse order)

     GlobalUbo mUbo;

     // - SYSTEMS -
     FcRenderer mRenderer;
     FcInput mInput;
     bool mShouldClose = false;
     SDL_Event mEvent;

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
     void loadGameObjects();
     void loadUIobjects();
     void update(float deltaTime);
     int calcFPS(float lastFrameTime);
     void close();
  };

}
