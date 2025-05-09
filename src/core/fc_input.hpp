#pragma once

#include "SDL2/SDL_events.h"
#include "SDL2/SDL_scancode.h"
//#include<SDL2/SDL.h>
#include <string>

namespace fc
{

  // TODO
  // Might want to tackle input more directly - refer to
  // https://discourse.libsdl.org/t/handling-mouse-buttons/27861
  class FcInput
  {
   private:
     // TODO make into constexpr
     void determineDeadZone();

     // internal storage of all the keyboard key states
     bool keyStates1[SDL_NUM_SCANCODES];
     bool keyStates2[SDL_NUM_SCANCODES];
     bool* currKeyStates;
     bool* prevKeyStates;

     bool mouseKeys[3];
     bool prevMouseKeys[3];

     //
     SDL_Window* pWindow; // Track if the mouse is within window
     int mMouseX {0};
     int mMouseY {0};
     int mPrevMouseX {0};
     int mPrevMouseY {0};
     //
     int mDeadzoneRadius {0};
     int mCenterX {0};
     int mCenterY {0};
     float relativeX {0};
     float relativeY {0};

     std::string* p_text; // pointer to string for storing text input data
     bool m_hasTextUpdated; // signifies if text input has been received
     Uint32 mouseState;

   public:
      //?? perhaps forego these to use SDL_BUTTON(x)
     // NOTE that we have to subtract one to alingn the button index with our array index
     static const int MOUSE_LEFT = SDL_BUTTON_LEFT - 1;
     static const int MOUSE_MIDDLE = SDL_BUTTON_MIDDLE - 1;
     static const int MOUSE_RIGHT = SDL_BUTTON_RIGHT - 1;

     void init(SDL_Window* window);
     void setMouseDeadzone(int radiusInPixels, int screenWidth, int screenHeight);
     void update();
     void kill();

     bool keyDown(int key);
     bool keyHit(int key);
     bool keyUp(int key);

     bool mouseDown(int key);
     bool mouseHit(int key);
     bool mouseUp(int key);
     bool mouseInWindow();

     void setMousePos(int x, int y);
     void hideCursor(bool hide = true);

     void RelativeMousePosition(int &mouseX, int &mouseY);
     int getMouseX() { return mMouseX; }
     int getMouseY() { return mMouseY; }

      // text input function
     void enableTextInput(std::string* text);
      // -1 = input no longer active, 1 = text input has been updated, 0 = input doesn't need updated;
      // -Note that text input status will change the internal variable that it has been updated,
      // meaning that the status returned is the status SINCE the last call to the function
     int textInputStatus();
      // reset our variable to signal that text has been rendered and can start sending signal of no update
     void receiveEvent(SDL_Event& event);
  };


}
