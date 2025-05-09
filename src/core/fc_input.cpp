#include "fc_input.hpp"

#include "SDL2/SDL_keyboard.h"
#include "SDL2/SDL_keycode.h"
#include "SDL2/SDL_scancode.h"
#include "SDL2/SDL_stdinc.h"
#include "core/utilities.hpp"
#include <cstring>

namespace fc
{
  void FcInput::init(SDL_Window* window)
  {
    pWindow = window;
     // initiallize the arrays that carry the state of the keyboard keys
    std::memset(keyStates1, false, sizeof(keyStates1[0]) * SDL_NUM_SCANCODES);
    std::memset(keyStates2, false, sizeof(keyStates2[0]) * SDL_NUM_SCANCODES);
     // set the pointer variable to point to one of the key states array
    currKeyStates = keyStates1;
    prevKeyStates = keyStates2;

    mouseState = SDL_GetMouseState(&mMouseX, &mMouseY);
    for (int i = 0; i < 3; i++)
    {
      mouseKeys[i] = mouseState & SDL_BUTTON(i);
      prevMouseKeys[i] = false;
    }

    m_hasTextUpdated = false;
    p_text = NULL;
  }

  bool FcInput::mouseInWindow()
  {
    return (SDL_GetMouseFocus() == pWindow);
  }


  void FcInput::setMouseDeadzone(int radiusInPixels, int screenWidth, int screenHeight)
  {
    mDeadzoneRadius = radiusInPixels;
    mCenterX = screenWidth / 2;
    mCenterY = screenHeight / 2;
  }

  void FcInput::update()
  {
     // get the current state of all the keys on keyboard
    const Uint8* updatedKeyStates = SDL_GetKeyboardState(NULL);

     // swap pointers instead of copying all the values from currkeyStates
     // to prevKeyStates.
    std::swap(currKeyStates, prevKeyStates);

     // copy the current values of the keystates into whichever array is now the
     // current keyStates array
    std::memcpy(currKeyStates, updatedKeyStates
                , sizeof(updatedKeyStates[0]) * SDL_NUM_SCANCODES);


     // get the relative x and y position of the mouse
    //mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

    // Get the absolute values for mouse position
    mPrevMouseX = mMouseX;
    mPrevMouseY = mMouseY;
    mouseState = SDL_GetMouseState(&mMouseX, &mMouseY);

     // get the key states of the mouse
    for (int i = 0; i < 3; i++)
    {
      prevMouseKeys[i] = mouseKeys[i];
      // SDL_BUTTON starts at 1 (for left button) and goes to 3 (right button)
      mouseKeys[i] = mouseState & SDL_BUTTON(i + 1);
    }
  }


  void FcInput::receiveEvent(SDL_Event &event)
  {
    // make sure we have established a string to write to
    if (p_text != NULL)
    {
      // handle backspace while text editing
      if (event.key.keysym.sym == SDLK_BACKSPACE && p_text->length() > 0 )
      {
        p_text->pop_back();
        m_hasTextUpdated = true;
      }
      // allow return button to end the text editing events
      else if (event.key.keysym.sym == SDLK_RETURN)
      {
        p_text = NULL;
        SDL_StopTextInput();
      }
      else if (event.type == SDL_TEXTINPUT)
      {
        *p_text += event.text.text;
        m_hasTextUpdated = true;
      }
    }

    // Handle Mouse here??
    if (event.type == SDL_MOUSEMOTION)
    {
      relativeX = event.motion.xrel;
      relativeY = event.motion.yrel;
    }

  }


  void FcInput::enableTextInput(std::string* text)
  {
     // point the text update to the string we need to alter
    p_text = text;
    SDL_StartTextInput();
  }


  int FcInput::textInputStatus()
  {
     // if no more text to render
    if (p_text == NULL)
    {
       // -1 to signify that we are no longer needing to keep text input updating
      return -1;
    }
     // 1 to signify that the text input is still active and also needs to be updated
    else if (m_hasTextUpdated == true)
    {
       // note that textInputStatus will change the updated variable if it is called
      m_hasTextUpdated = false;
      return 1;
    }
     // 0 to signify that the text input is still active but does not need to update
    else
    {
      return 0;
    }
  }


  void FcInput::kill()
  {
    m_hasTextUpdated = false;
    p_text = NULL;
  }



  //TODO change all safety checks to asserts
  bool FcInput::keyDown(int key)
  {
    if (key < 0 || key > SDL_NUM_SCANCODES)
    {
      return false;
    }
    return currKeyStates[key];
  }


  bool FcInput::keyHit(int key)
  {
    if (key < 0 || key > SDL_NUM_SCANCODES)
    {
      return false;
    }
    return (currKeyStates[key] && !prevKeyStates[key]);
  }


  bool FcInput::keyUp(int key)
  {
    if (key < 0 || key > SDL_NUM_SCANCODES)
    {
      return false;
    }
    return (prevKeyStates[key] && !currKeyStates[key]);
  }


  bool FcInput::mouseDown(int key)
  {
    if (key < 0 || key > 3)
    {
      return false;
    }
    return mouseKeys[key];
  }


  bool FcInput::mouseHit(int key)
  {
    if (key < 0 || key > 3)
    {
      return false;
    }
    return (mouseKeys[key] && !prevMouseKeys[key]);
  }

  bool FcInput::mouseUp(int key)
  {
    if (key < 0 || key > 3)
    {
      return false;
    }
    return (prevMouseKeys[key] && !mouseKeys[key]);
  }

  // Not relative in the sense that we're finding the delta for mouse movement but
  // rather that we're returning the position relative to the deadzone
  void FcInput::RelativeMousePosition(int &mouseX, int &mouseY)
  {
    int correctedMouseX = mMouseX - mCenterX;
    int correctedMouseY = mMouseY - mCenterY;

    // calculate mouse deltas to see if changing in the opposite direction
    int deltaMouseX = mPrevMouseX - mMouseX;
    int deltaMouseY = mPrevMouseY - mMouseY;


    float length = std::sqrt(correctedMouseX * correctedMouseX + correctedMouseY * correctedMouseY);

    float relativeLength = length - mDeadzoneRadius;

    // Apply dampening factor if player is switching directions
    if ((correctedMouseX > 0 && deltaMouseX < 0) || (correctedMouseX < 0 && deltaMouseX > 0))
    {
      relativeLength *= 0.333f;
    }

    if (relativeLength > 0)
    {
      float scaleFactor = relativeLength / length;
      mouseX = correctedMouseX * scaleFactor;
      mouseY = correctedMouseY * scaleFactor;
    }
    else
    {
      mouseX = 0;
      mouseY = 0;
    }
  }



  void FcInput::setMousePos(int x, int y)
  {
    SDL_WarpMouseInWindow(pWindow, x, y);
  }



  void FcInput::hideCursor(bool hide)
  {
    (hide) ? SDL_ShowCursor(SDL_DISABLE) : SDL_ShowCursor(SDL_ENABLE);
    // TRY
    //SDL_SetRelativeMouseMode(SDL_TRUE);
  }
}
