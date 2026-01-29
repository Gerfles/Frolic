#include "frolic.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_cvar_system.hpp"
#include "core/fc_font.hpp"
#include "core/fc_light.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_player.hpp"
#include "core/fc_text.hpp"
#include "core/utilities.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_version.h"
#include "SDL2/SDL_video.h"
//
#include "imgui.h"
#include "imgui_impl_sdl2.h"
//
#include "vulkan/vulkan_core.h"
#include <SDL_log.h>
#include <SDL_timer.h>
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>


namespace fc
{
  // float CVar

  Frolic::Frolic()
  {
    // Application Specs for developer use
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Frolic"; 		   // Our application name
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // our application version
    appInfo.pEngineName        = "Frolic Engine";          // custom engine name
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0); // custom engine version
    // the following does affect the program unlike the above
    appInfo.apiVersion         = VK_API_VERSION_1_3;       // the version of Vulkan ()

    VkExtent2D screenDims {2100, 1600};
    FcLocator::provide(screenDims);
    // Version info
    //TODO should do this in a builder class that goes out of scope when no longer needed
    SDL_version compiled;
    SDL_VERSION(&compiled);

    SDL_version linked;
    SDL_GetVersion(&linked);

    SDL_Log("SDL version(s): %u.%u.%u (compiled),  %u.%u.%u. (linked)\n",
            compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);
    SDL_Log("Window Dimensions: %i x %i", screenDims.width, screenDims.height);

    // Initialize the memory allocator
    // TODO make this part of our FcLocator scheme instead of the singleton it is
    MemoryService::instance()->init(nullptr);
    pAllocator = &MemoryService::instance()->systemAllocator;
    // TODO Make sure this deallocates properly
    mStackAllocator.init( megabytes(8) );

    // TODO should explicitly enable or disable non-uniform scaling utilizing functors for
    // actual function call to


    // TODO pull some stuff out of render initialize and have init VK systems?
    // TODO define our own exception classes and failure codes for debugging later
    if (mRenderer.init(appInfo, screenDims, &pSceneData) != EXIT_SUCCESS)
    {
      mShouldClose = true;
      return;
    }

    // TODO set this as a static variable
    mInput.init(mRenderer.Window());

    // NOTE: must use screen dimension not pixel width and height
    // TODO should make a better distinction and perhaps have it available globally
    mInput.setMouseDeadzone(50, screenDims.width, screenDims.height);

    mPlayer.init(&mInput);

    // Initialize simple first person camera
    mPlayer.Camera().setPerspectiveProjection(60.0f, FcLocator::ScreenDims().width
                                             , FcLocator::ScreenDims().height, 512.f, 0.01f);
    // TODO make sure all reference returns are const to avoid something like:
    /* mPlayer.Camera().Projection()[1][1] *= -1; */

    mRenderer.setActiveCamera(&mPlayer.Camera());
    mRenderer.initDefaults();//mSceneDataBuffer, &mSceneData);

    stats = &mRenderer.getStats();

  }// --- Frolic::Frolic (_) --- (END)

  //
  //
  void Frolic::loadGameObjects()
  {
    // TODO draw collection should be part of frolic or cartridge since
    // we may want multiple drawcollections to add to but may want to implement
    // system in which we can swap out draw collections...
    mSunBillboard.loadTexture(mRenderer, 1., 1., "..//textures//sun.png");

    // TODO set sun position as absolute -> not direction;
    mSunBillboard.setPosition(pSceneData->sunlightDirection);

    mRenderer.addBillboard(mSunBillboard);

    // TODO implement with std::optional
    // TODO should not load and add to draw collection simultaneously. WHY??
    /* helmet.loadGltf(mRenderer, "..//models//helmet//DamagedHelmet.gltf"); */
    /* helmet.loadGltf(mRenderer, "..//models//Box.gltf"); */
    /* helmet.loadGltf(mRenderer, "..//models//MaterialsVariantsShoe.glb"); */
    helmet.loadGltf(mRenderer, "..//models//rustediron//MetalRoughSpheres.gltf");
    /* helmet.loadGltf(mRenderer, "..//models//structure.glb"); */

    /* helmet.loadGltf(mRenderer, "..//models//ToyCar.glb"); */
    /* helmet.loadGltf(mRenderer, "..//models//MosquitoInAmber.glb"); */
    /* helmet.loadGltf(mRenderer, "..//models//BoomBoxWithAxes//glTF//BoomBoxWithAxes.gltf"); */
    /* helmet.loadGltf(mRenderer, "..//models//sponza//Sponza.gltf"); */

      sponza.loadGltf(mRenderer, "..//models//sponza//Sponza.gltf");

    /* sponza.loadGltf(mRenderer, "..//models//Box.gltf"); */
    // sponza.loadGltf(this, "..//models//GlassHurricaneCandleHolder.glb");
    /* sponza.loadGltf(mRenderer, "..//models//ToyCar.glb"); */
    /* sponza.loadGltf(mRenderer, "..//models//structure.glb"); */

    // Posistion the loaded scenes
    glm::vec3 translationVec = {45.0f, 10.0f, 20.0f};
    /* helmet.scale(glm::vec3{10.0f, 10.0f, 10.0f}); */

    helmet.translate(translationVec);
    helmet.update();

    translationVec.y = 7.0f;
    sponza.translate(translationVec);

    // // update the moved objects
    sponza.update();
  } // --- Frolic::loadGameObjects (_) --- (END)



  void Frolic::run()
  {
    // FIXME if possible, may be an issue with linux
    SDL_ShowCursor(SDL_DISABLE);

    // Initialize player controls and position
    mPlayer.setPosition(glm::vec3(36.f, 25.f, 19.f));

    // load everything we need for the scene
    loadGameObjects();

    AutoCVarFloat cvarMovementSpeed("movementSpeed.float", "controls camera movement speed", 10.0f);

    // zero out the ticklist for performance tracking
    std::memset(mFrameTimeList, 0, sizeof(mFrameTimeList));

    FcTimer mTimer;
    mTimer.start();
    float deltaTime = 0.0f;

    // TODO separate to make entirely own function
    while (!mShouldClose)
    {
      bool shouldResize = false;
      // Check for events every cycle of the game loop
      while (SDL_PollEvent(&mEvent))
      {
        if (mEvent.type == SDL_WINDOWEVENT)
        {
          // TODO should think about updating input here instead of doing it asynchronously...
          switch (mEvent.window.event)
          {
            // ?? check that this is proper
              case SDL_WINDOWEVENT_MINIMIZED:
              case SDL_WINDOWEVENT_HIDDEN:
              {
                // TODO replace here with mRenderer.pause();
                SDL_WaitEvent(nullptr);
                break;
              }
              // ?? this event seems to cause issues in Wayland, seems like wayland already handles resize
              // case SDL_WINDOWEVENT_SIZE_CHANGED:
              // {
              //    // TODO handle better here

              //   mRenderer.handleWindowResize();
              //   shouldResize = true;
              //    //break;
              // }
              default:
                break;
          }
        }

        if (mEvent.type == SDL_QUIT)
        {
          mShouldClose = true;
          break;
        }

        // TODO see if this is better handles (without this additional check) from SDL switch
        if (mRenderer.shouldWindowResize())
        {
          mRenderer.handleWindowResize();
        }

        // Send SDL event to imGUi for handling
        ImGui_ImplSDL2_ProcessEvent(&mEvent);
      }
      // TODO consider creating a timer that better represents time in seconds instead of milliseconds
      // that way we don't need to divide by 1000 to get a better representation of what's going on...
      // could consider bit-shifting by 1024 to get there also
      deltaTime = mTimer.elapsedTime();
      /* FcStats& stats = mRenderer.getStats(); */
      stats->frametime = deltaTime * 1000;
      stats->fpsAvg = calcFPS(deltaTime);
      // now re-start the time so that the start time is the start of each frame
      mTimer.start();

      // update keyboard and game controller state and use that to mover the mPlayer (and thus camera)
      mInput.update();

      // Move the mPlayer with the updated input, which will automatically update the camera
      mPlayer.move(deltaTime);

      update(deltaTime);

      // pSceneData->viewProj = pSceneData->projection * pSceneData->view;
      // pSceneData->lighSpaceTransform = mRenderer.mShadowMap.LightSpaceMatrix();
// mSceneDataBuffer.overwriteData(&mSceneData, sizeof(SceneData));

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   START THE NEW FRAME   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // mRenderer.generateShadowMap();

      uint32_t swapchainImgIndex = mRenderer.beginFrame();

      // FcPipeline* selected = mPipelines[currentBackgroundEffect];
      /* mRenderer.attachPipeline(selected); */

      // TODO make sure we can comment out drawGUI without crashing
      gui.drawGUI(this);

      //mRenderer.drawModels(swapchainImgIndex, mUbo);
      //mRenderer.drawBillboards(camera.Position(), frame, mUbo);
      //mRenderer.drawUI(mUItextList, frame);
      //mRenderer.drawBackground(mPushConstants[currentBackgroundEffect]);

      // TODO may want to couple shadow map tighter with sceneRenderer
      mRenderer.drawFrame(mShouldDrawDebugShadowMap);

      mRenderer.endFrame(swapchainImgIndex);

    } // _END_ while(!shouldClose);
  }


  void Frolic::update(float deltaTime)
  {
    if (rotationSpeed != 0)
    {
      angle = rotationSpeed * 0.001f;
      glm::vec3 rotationAxis = {0.f, -1.f, 0.f};

      helmet.rotateInPlace(angle, rotationAxis);
      // // TODO could flag draw collection items with nodes that "know" they need to be updated
      // // then draw collection could do all the updating in one go.
      helmet.update();
    }
  }


   // Keep a running total of 100 frame time samples in order to smooth the FPS calculation
  int Frolic::calcFPS(float lastFrameTime)
  {
    // first remove the frame time at the current index from our running total
    mFrameTimeSum -= mFrameTimeList[mFrameTimeIndex];

    // then add the most recent frame time to our frame time total
    mFrameTimeSum += lastFrameTime;

    // store the most recent frame in our array so we can subtract it from the total the
    // next time around
    mFrameTimeList[mFrameTimeIndex] = lastFrameTime;

    // TODO eventually improve on this metric
    if (mFrameTimeIndex == MAX_FRAME_SAMPLES - 1)
    {
      float avgFrameTime = 0.0f;

      // Start at one so we don't count the first frame
      for (int i = 1; i < MAX_FRAME_SAMPLES; ++i)
      {
        avgFrameTime += mFrameTimeList[i];
      }
      avgFrameTime = (MAX_FRAME_SAMPLES - 1)/avgFrameTime;
    }
     // make sure we wrap around on the MAX_FRAME_SAMPLE'th sample
    mFrameTimeIndex = (mFrameTimeIndex + 1) % MAX_FRAME_SAMPLES;

     // calculate and return the average famesPerSec
    return MAX_FRAME_SAMPLES / mFrameTimeSum;
  }



  void Frolic::close()
  {
    // TODO I think that since we have the one device wait idle, we can eliminate all others
    vkDeviceWaitIdle(FcLocator::Device());

    helmet.destroy();
    sponza.destroy();

    mInput.kill();

    FcLight::destroyDefaultTexture();

    mRenderer.shutDown();

    MemoryService::instance()->shutdown();
  }

} // namespace fc END
