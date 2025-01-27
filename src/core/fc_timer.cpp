#include "fc_timer.hpp"

#include "SDL2/SDL_timer.h"


const float FcTimer::TICK_PERIOD = 1.0 / (double)SDL_GetPerformanceFrequency();
//static const TICK_PERIOD = 1.0 / (double)SDL_GetPerformanceFrequency();

// TODO consider using std::chrono::system_clock... etc;
void FcTimer::start()
{
  // start the timer
  mHasStarted = true;

  // make sure the timer is not paused
  mIsPaused = false;

  // save the current clock time to startTicks
  mStartTicks = SDL_GetPerformanceCounter();

  // reset paused time
  mPausedTicks = 0;
}


void FcTimer::stop()
{
  // stop the timer
  mHasStarted = false;

  // make sure the timer is not paused
  mIsPaused = false;

  // reset time to zero
  mStartTicks = 0;
  mPausedTicks = 0;
}


void FcTimer::pause()
{
  // check if the timer is running and unpaused
  if (mHasStarted && !mIsPaused)
    {
      // Pause the timer
      mIsPaused = true;

      // set the paused time to the current elapsed time
      mPausedTicks = SDL_GetPerformanceCounter() - mStartTicks;
      //?? reset the start time -- not sure it's needed
      mStartTicks = 0;
    }

}


void FcTimer::unPause()
{
  // make sure that the timer has in fact started and been paused
  if (mHasStarted && mIsPaused)
    {
      // unpause the timer
      mIsPaused = false;

      // reset the starting ticks
      mStartTicks = SDL_GetPerformanceCounter() - mPausedTicks;

      // reset the paused ticks
      mPausedTicks = 0;
    }
}


float FcTimer::elapsedTime()
{
  return static_cast<float>((SDL_GetPerformanceCounter() - mStartTicks)) * TICK_PERIOD;
}


uint64_t FcTimer::getTicks()
{
  // set the default return time
  Uint32 time = 0;

  // if the timer has been started, return the internally stored time
  if (mHasStarted)
    {
      // return the paused time if timer is not running
      if (mIsPaused)
        {
          time = mPausedTicks;
        }
      else // return the time since timer started
        {
          time = (SDL_GetTicks() - mStartTicks);
        }
    }

  return time;
}
