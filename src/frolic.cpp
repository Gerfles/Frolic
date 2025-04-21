#include "frolic.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_game_object.hpp"
#include "core/fc_font.hpp"
#include "core/fc_light.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_model.hpp"
#include "core/fc_player.hpp"
#include "core/fc_text.hpp"
#include "core/utilities.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_version.h"
#include "SDL2/SDL_video.h"
//
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
//
#include "vulkan/vulkan_core.h"
//
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>


namespace fc
{
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

    VkExtent2D screenDims {1200, 900};
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

    // TODO addd to builder
    // // vulkan features to request from version 1.2
    // VkPhysicalDeviceVulkan12Features features12 = {};
    // features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    // features12.bufferDeviceAddress = true;
    // features12.descriptorIndexing = true;

    //  // vulkan features to request from version 1.3
    // VkPhysicalDeviceVulkan13Features features13 = {};
    // features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    // features13.dynamicRendering = true;
    // features13.synchronization2 = true;


    // TODO TRY to pull some stuff out of render initialize and have init VK systems?
    //TODO define our own exit_success and failure codes for debugging later
    if (mRenderer.init(appInfo, screenDims, &pSceneData) != EXIT_SUCCESS)
    {
      mShouldClose = true;
      return;
    }

    // create a default texture that gets used whenever assimp cant find a texture
    // Need to load this in first so it can be Texture 0 for default
    // TODO set this as a static variable
    mInput.init(mRenderer.Window());

    // NOTE: must use screen dimension not pixel width and height
    // TODO should make a better distinction and perhaps have it available globally
    mInput.setMouseDeadzone(50, screenDims.width, screenDims.height);

    mPlayer.init(&mInput);
    uvnPlayer.init(&mInput);


    mPlayer.Camera().setPerspectiveProjection(60.0f, FcLocator::ScreenDims().width
                                             , FcLocator::ScreenDims().height, 512.f, 0.1f);
    // TODO make sure all reference returns are const to avoid something like:
    // mPlayer.Camera().Projection()[1][1] *= -1;

    /* mPlayer.Camera().setOrthographicProjection(-5.f, 5.f, -5.f, 5.f, 30.f, 0.1f); */

    // uvnPlayer.Camera().setPerspectiveProjection(66.0f, FcLocator::ScreenDims().width
    //                                 , FcLocator::ScreenDims().height, 500.f, 0.1f);

    // Initialize simple first person camera
    // FcCamera camera;
    // FcCamera uvnCamera;

    //    mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), 0.01f, 1000.f);
    //    mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), .1f, 30.f);
    //mSceneData.projection = glm::ortho(0.0f, 1350.0f, 900.0f, 0.0f, 0.1f, 1000.0f);
    // mSceneData.projection = glm::orthoRH_ZO(0.0f, 1350.0f, 900.0f, 0.0f, 1111.1f, 0.10f);
    // mSceneData.projection = glm::perspective(glm::radians(70.0f), mRenderer.aspectRatio(), 500.0f, 0.1f);
    // mSceneData.projection[1][1] *= -1;
    //

    /* pSceneData->projection = mPlayer.Camera().Projection(); */

    // mSceneData.projection = orthographic(-1.f, 1.f,
    //                                     -1.f, 1.f,  100.f, 0.1f);
    //camera.setViewTarget(glm::vec3{0,0,5}, glm::vec3{0,0,-1});


    mRenderer.setActiveCamera(&mPlayer.Camera());
    mRenderer.initDefaults();//mSceneDataBuffer, &mSceneData);

    // zero out the ticklist for performance tracking
    std::memset(mFrameTimeList, 0, sizeof(mFrameTimeList));

    stats = &mRenderer.getStats();
  }



  void Frolic::loadUIobjects()
  {
  }



  void Frolic::loadGameObjects()
  {
    // TODO keep this behavior for Fc_Scene
    // castle->transform.rotation = {glm::pi<float>(), 0.f, 0.f};
    // castle->transform.translation = {0.f, 1.f, 0.f};
    // castle->transform.scale = {0.5f, 0.5f, 0.5f};

  } // --- Frolic::loadGameObjects (_) --- (END)



  void Frolic::run()
  {
    fcLog("Initializing to begin main run loop", 0);

    // FIXME
    SDL_ShowCursor(SDL_DISABLE);


    // Initialize player controls and position

    //mPlayer.setPosition(glm::vec3(30.f, -00.f, -85.f));
    //mPlayer.setPosition(glm::vec3(-5.f, 5.5f, .20f));
    /* mPlayer.setPosition(glm::vec3(0.f, 0.f, 2.f)); */
    mPlayer.setPosition(glm::vec3(-2.5f, 21.4f, -0.28f));
    uvnPlayer.setPosition(glm::vec3(0.f, 0.f, 2.f));


    // load everything we need for the scene
    //loadUIobjects();
    //loadGameObjects();
    // TODO this should be abstracted into engine

    fcLog("Frolic Initialized: Starting main run loop", 0);
    FcTimer mTimer;
    mTimer.start();
    float deltaTime = 0.0f;


    bool ao;

    // TODO separate to make entirely own function
    while (!mShouldClose)
    {
      bool shouldresize = false;
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
              //   shouldresize = true;
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
      uvnPlayer.moveNew(deltaTime);

      //update(deltaTime);


      // pSceneData->viewProj = pSceneData->projection * pSceneData->view;
      // pSceneData->lighSpaceTransform = mRenderer.mShadowMap.LightSpaceMatrix();
// mSceneDataBuffer.overwriteData(&mSceneData, sizeof(SceneData));

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   START THE NEW FRAME   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // mRenderer.generateShadowMap();

      uint32_t swapchainImgIndex = mRenderer.beginFrame();

      // FcPipeline* selected = mPipelines[currentBackgroundEffect];
      /* mRenderer.attachPipeline(selected); */

      // TODO make sure we can comment out drawGUI without crashing
      drawGUI();

      //mRenderer.drawModels(swapchainImgIndex, mUbo);
      //mRenderer.drawBillboards(camera.Position(), frame, mUbo);
      //mRenderer.drawUI(mUItextList, frame);
      //mRenderer.drawBackground(mPushConstants[currentBackgroundEffect]);

      // TODO may want to couple shadow map tighter with sceneRenderer
      mRenderer.drawFrame(mDebugShadowMap);

      mRenderer.endFrame(swapchainImgIndex);

    } // _END_ while(!shouldClose);
  }


  void Frolic::update(float deltaTime)
  {
    // TODO put timer and FPS stuff in Renderer
    // use the timers elapsed time to calculate the average frames per second
    // PROFILING
    // int avgFPS  = calcFPS(deltaTime);
    // std::string fpsNum{std::to_string(avgFPS)};
  }



  void Frolic::drawGUI()
  {
    // test ImGui UI
    // Left here to add a demo windo that names all the features for (handy for searching)
    // ImGui::ShowDemoWindow();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   STATISTICS WINDOW   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    if (ImGui::Begin("Frolic Stats", NULL, ImGuiWindowFlags_NoTitleBar))
    {
      // Stats
      ImGui::Text("FPS(avg): %i ", stats->fpsAvg);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Frame(ms): %.2f", stats->frametime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Draw(ms): %.2f", stats->meshDrawTime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Update(ms): %.2f", stats->sceneUpdateTime);
      ImGui::SameLine(0.f, 10.f);

      // TODO UNCOMMENT
      // ImGui::Text("| Position: <%.3f,%.3f,%.3f>", pos.x, pos.y, pos.z);
      // ImGui::SameLine(0.f, 10.f);

      ImGui::Text("| Triangles Drawn: %i", stats->triangleCount);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Objects Drawn: %i", stats->objectsRendered);
      // int relX, relY;
      // mInput.RelativeMousePosition(relX, relY);
      // ImGui::Text("Mouse X pos: %i", mInput.getMouseX());
      // ImGui::Text("Mouse Y pos: %i", mInput.getMouseY());

      // TODO check the official way to use ImGui via the imgui example source code
      ImGui::End();
    }

    // ImGui::Render();
    // return;


    if (mPlayer.lookSpeed() == 0.0f)
    {
      // TODO probably best to enable disable this stuff eventually in a dedicated pipeline shader
      // and then just bind the appropriate pipeline
      // Draw Configuration panel
      // BUG collapsing the control panel kills the program
      if (ImGui::Begin("Scene Data"))
      {
        // TODO update all options with bitfields instead of bools


        if (ImGui::Checkbox("Wire Frame", &mRenderer.drawWireframe))
        {
          // b
        }
        if (ImGui::Checkbox("Color Texture", &mUseColorTexture))
        {
          mRenderer.setColorTextureUse(mUseColorTexture);
        }

        if (ImGui::Checkbox("Rough/Metal Texture", &mUseRoughMetalTexture))
        {
          mRenderer.setRoughMetalUse(mUseRoughMetalTexture);
        }

        if(ImGui::Checkbox("Ambient Occlussion Texture", &mUseOcclussionTexture))
        {
          mRenderer.setAmbientOcclussionUse(mUseOcclussionTexture);
        }

        if (ImGui::Checkbox("Normal Texture", &mUseNormalTexture))
        {
          mRenderer.setNormalMapUse(mUseNormalTexture);
        }
        if (ImGui::Checkbox("Emissive Texture", &mUseEmissiveTexture))
        {
          mRenderer.setEmissiveTextureUse(mUseEmissiveTexture);
        }

        ImGui::Checkbox("Draw Normals", &mRenderer.mDrawNormalVectors);

        ImGui::Checkbox("Box Bounds", &mRenderer.mDrawBoundingBoxes);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);

        // FIXME maybe create a struct of variables that we then pass
        // as a whole to mRenderer...
        if (ImGui::InputInt("Bounding Box", &mRenderer.mBoundingBoxId))
        {
          if (mRenderer.mBoundingBoxId < 0)
            mRenderer.mBoundingBoxId = -1;
        }

        ImGui::SetNextItemWidth(60);
        ImGui::SliderInt("Model Rotation Speed", &mRenderer.rotationSpeed, -5, 5);
        ImGui::SetNextItemWidth(60);
        // TODO don't allow access into class like this
        ImGui::SliderFloat("Movement Speed", &mPlayer.moveSpeed(), 1, 50, "%.1f");
        ImGui::Checkbox("Cycle", &mCycleExpansion);

        if (mCycleExpansion)
        {
          float time = SDL_GetTicks() / 1000.0f;

          // FIXME
          mRenderer.ExpansionFactor() = sin(time) + 1.0f;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);

        // FIXME
        ImGui::SliderFloat("Expansion Factor", &mRenderer.ExpansionFactor(), -1.f, 2.f);

        ImGui::SetNextItemWidth(60);
        if(ImGui::SliderFloat4("Sunlight", (float*)&pSceneData->sunlightDirection, -1.f, 1.f))
        {
          glm::vec4 lightPos = pSceneData->sunlightDirection - pSceneData->eye;
          mRenderer.mShadowMap.updateLightSource(lightPos, glm::vec3(0.f, 0.f, 0.f));
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW MAP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // TODO use frustum instead
        Box& frustum = mRenderer.mShadowMap.Frustum();
        ImGui::Checkbox("Draw Shadow Map", &mDebugShadowMap);
        if(ImGui::SliderFloat("Left", &frustum.left, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Right", &frustum.right, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Top", &frustum.top, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Bottom", &frustum.bottom, -20.f, 20.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Near", &frustum.near, -.01f, 10.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Far", &frustum.far, 1.f, 100.f))
        {
          mRenderer.mShadowMap.updateLightSpaceTransform();
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-   VIEW MATRIX COMPARISON   -*-*-*-*-*-*-*-*-*-*-*-*- //
        if (ImGui::Button("Display View Matrix"))
        {
          ImGui::OpenPopup("MatrixView");
        }
        if (ImGui::BeginPopup("MatrixView"))
        {
          glm::mat4 mat = mPlayer.Camera().getViewMatrix();
          glm::mat4 mat2 = uvnPlayer.Camera().getViewMatrix();

          ImGui::Text("Quaternion View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
          ImGui::Text("UVN View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][0], mat2[1][0], mat2[2][0], mat2[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][1], mat2[1][1], mat2[2][1], mat2[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][2], mat2[1][2], mat2[2][2], mat2[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][3], mat2[1][3], mat2[2][3], mat2[3][3]);
          ImGui::EndPopup();
        }
      }
      // ??
      //ImGui::EndFrame();
      ImGui::End();
    }
    // make ImGui calculate internal draw structures
    ImGui::Render();
  }


   // Keep a running total of 100 frame time samples in order to smooth the FPS calculation
  int Frolic::calcFPS(float lastFrameTime)
  {
     // first remove the frame time at the current index from our running total
    mFrameTimeSum -= mFrameTimeList[mFrameTimeIndex];

     // then add the most recent frame time to our frame time total
    mFrameTimeSum += lastFrameTime;

     // store the most recent frame in our array so we can subtract it from the total the next time around
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

       //      std::cout << "average frames per second: " << avgFrameTime << std::endl;
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

    mInput.kill();

     // free image resources
    mFallbackTexture.destroy();
    //
    mMeshPipeline.destroy();

    FcLight::destroyDefaultTexture();

     // free all bilboards
    for (auto& text : mUItextList)
    {
      text.free();
    }

    mUIfont.free();

    mGradientPipeline.destroy();
    mSkyPipeline.destroy();
     // for (auto& model : mModelList)
     // {
     //   model.destroy();
     // }

    mRenderer.shutDown();
  }

} // namespace fc END
