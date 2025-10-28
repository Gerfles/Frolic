// fc_gui.cpp
#include "fc_gui.hpp"
#include "frolic.hpp"
#include "core/fc_cvar_system.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"

#include <SDL_timer.h>


namespace fc
{
  void FcGUI::drawGUI(Frolic* fc)
  {
    // test ImGui UI
    // Left here to add a demo windo that names all the features for (handy for searching)
    /* ImGui::ShowDemoWindow(); */

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   STATISTICS WINDOW   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    if (ImGui::Begin("Frolic Stats", NULL, ImGuiWindowFlags_NoTitleBar))
    {
      // Stats

      ImGui::Text("FPS(avg): %i ", fc->stats->fpsAvg);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Frame(ms): %.2f", fc->stats->frametime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Draw(ms): %.2f", fc->stats->meshDrawTime);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Update(ms): %.2f", fc->stats->sceneUpdateTime);
      ImGui::SameLine(0.f, 10.f);

      // TODO UNCOMMENT
      // TODO should probably have a Position() method in FcPlayer
      glm::vec3 pos = fc->mPlayer.Camera().Position();
      ImGui::Text("| Position: <%.3f,%.3f,%.3f>", pos.x, pos.y, pos.z);
      ImGui::SameLine(0.f, 10.f);

      ImGui::Text("| Triangles Drawn: %i", fc->stats->triangleCount);
      ImGui::SameLine(0.f, 10.f);
      ImGui::Text("| Objects Drawn: %i", fc->stats->objectsRendered);
      // int relX, relY;
      // mInput.RelativeMousePosition(relX, relY);
      // ImGui::Text("Mouse X pos: %i", mInput.getMouseX());
      // ImGui::Text("Mouse Y pos: %i", mInput.getMouseY());

      // TODO check the official way to use ImGui via the imgui example source code
      ImGui::End();
    }

    if (fc->mPlayer.lookSpeed() == 0.0f)
    {
      // TODO probably best to enable disable this stuff eventually in a dedicated pipeline shader
      // and then just bind the appropriate pipeline
      // Draw Configuration panel
      if (ImGui::Begin("Scene Data"))
      {
        // TODO update all options with bitfields instead of bools

        if (ImGui::Checkbox("Wire Frame", &fc->mRenderer.drawWireframe))
        {
          // b
        }

        if (ImGui::Checkbox("Color Texture", &mUseColorTexture))
        {
          fc->helmet.toggleTextureUse(MaterialFeatures::HasColorTexture,
                                      helmetTexIndices);
          fc->sponza.toggleTextureUse(MaterialFeatures::HasColorTexture,
                                     sponzaTexIndices);
        }

        if (ImGui::Checkbox("Rough/Metal Texture", &mUseRoughMetalTexture))
        {
          fc->helmet.toggleTextureUse(MaterialFeatures::HasRoughMetalTexture,
                                     helmetTexIndices);
          fc->sponza.toggleTextureUse(MaterialFeatures::HasRoughMetalTexture,
                                      sponzaTexIndices);
        }

        if(ImGui::Checkbox("Ambient Occlussion Texture", &mUseOcclussionTexture))
        {
          fc->helmet.toggleTextureUse(MaterialFeatures::HasOcclusionTexture,
                                     helmetTexIndices);
          fc->sponza.toggleTextureUse(MaterialFeatures::HasOcclusionTexture,
                                     sponzaTexIndices);
        }

        if (ImGui::Checkbox("Normal Texture", &mUseNormalTexture))
        {
          fc->helmet.toggleTextureUse(MaterialFeatures::HasNormalTexture,
                                     helmetTexIndices);
          fc->sponza.toggleTextureUse(MaterialFeatures::HasNormalTexture,
                                     sponzaTexIndices);
        }

        if (ImGui::Checkbox("Emissive Texture", &mUseEmissiveTexture))
        {
          fc->helmet.toggleTextureUse(MaterialFeatures::HasEmissiveTexture, helmetTexIndices);
          fc->sponza.toggleTextureUse(MaterialFeatures::HasEmissiveTexture, sponzaTexIndices);
        }

        ImGui::Checkbox("Draw Normals", &fc->mRenderer.mDrawNormalVectors);

        ImGui::Checkbox("Box Bounds", &fc->mRenderer.mDrawBoundingBoxes);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);

        // FIXME maybe create a struct of variables that we then pass
        // as a whole to mRenderer...
        if (ImGui::InputInt("Bounding Box", &fc->mRenderer.mBoundingBoxId))
        {
          if (fc->mRenderer.mBoundingBoxId < 0)
            fc->mRenderer.mBoundingBoxId = -1;
        }

        ImGui::SetNextItemWidth(60);
        ImGui::SliderInt("Model Rotation Speed", &fc->rotationSpeed, -5, 5);
        ImGui::SetNextItemWidth(60);

        // TODO this shouldn't be a CVAR but left for reference
        float* movementSpeed = CVarSystem::Get()->GetFloatCVar("movementSpeed.float");
        if (ImGui::SliderFloat("Movement Speed", movementSpeed, 1, 50, "%1.f"))
        {
          fc->mPlayer.setMoveSpeed(*movementSpeed);
        }

        ImGui::Checkbox("Cycle", &mCycleExpansion);

        if (mCycleExpansion)
        {
          float time = SDL_GetTicks() / 1000.0f;

          // FIXME
          fc->mRenderer.ExpansionFactor() = sin(time) + 1.0f;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);

        // FIXME
        ImGui::SliderFloat("Expansion Factor", &fc->mRenderer.ExpansionFactor(), -1.f, 2.f);

        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SUNLIGHT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        glm::vec4 sunlightPos = fc->pSceneData->sunlightDirection;
        if (ImGui::SliderFloat("Sun X", &sunlightPos.x, -100.f, 100.f)
            || ImGui::SliderFloat("Sun Y", &sunlightPos.y, 5.f, 100.f)
            || ImGui::SliderFloat("Sun Z", &sunlightPos.z, -100.f, 100.f))
        {
          // Update sun's location
          fc->mSunBillboard.setPosition(sunlightPos);
          fc->pSceneData->sunlightDirection = sunlightPos;

          // Update shadow map light source
          glm::vec3 lookDirection{sunlightPos.x, 0.f, sunlightPos.z};
          fc->mRenderer.mShadowMap.updateLightSource(sunlightPos, lookDirection);
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SHADOW MAP   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // TODO use frustum instead
        Box& frustum = fc->mRenderer.mShadowMap.Frustum();
        ImGui::Checkbox("Draw Shadow Map", &fc->mShouldDrawDebugShadowMap);
        if(ImGui::SliderFloat("Left", &frustum.left, -20.f, 20.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Right", &frustum.right, -20.f, 20.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Top", &frustum.top, -20.f, 20.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Bottom", &frustum.bottom, -20.f, 20.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Near", &frustum.near, -.01f, 75.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }
        if(ImGui::SliderFloat("Far", &frustum.far, 1.f, 100.f))
        {
          fc->mRenderer.mShadowMap.updateLightSpaceTransform();
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-   VIEW MATRIX COMPARISON   -*-*-*-*-*-*-*-*-*-*-*-*- //
        if (ImGui::Button("Display View Matrix"))
        {
          ImGui::OpenPopup("MatrixView");
        }
        if (ImGui::BeginPopup("MatrixView"))
        {
          glm::mat4 mat = fc->mPlayer.Camera().getViewMatrix();
          /* glm::mat4 mat2 = uvnPlayer.Camera().getViewMatrix(); */

          ImGui::Text("Quaternion View Matrix");
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
          ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
          // ImGui::Text("UVN View Matrix");
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][0], mat2[1][0], mat2[2][0], mat2[3][0]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][1], mat2[1][1], mat2[2][1], mat2[3][1]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][2], mat2[1][2], mat2[2][2], mat2[3][2]);
          // ImGui::Text("|%.3f, %.3f, %.3f, %.3f|", mat2[0][3], mat2[1][3], mat2[2][3], mat2[3][3]);
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


} // *-END- (namespace fc) -END-*
