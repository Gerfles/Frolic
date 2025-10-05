#pragma once


#include "core/platform.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace  fc
{

  class FcTimer
  {
   private:

     // TODO rewrite utilizing C++ timer instead!
     static const float TICK_PERIOD; // = 1.0 / SDL_Counts_per_sec
     // the application clock time (started when application starts) when the timer started
     u64 mStartTicks{0};

     // stores the ticks if timer gets paused
     u64 mPausedTicks{0};

     // timer state
     bool mIsPaused{false};
     bool mHasStarted{false};

   public:
     // initialize variables
     FcTimer() = default;

     // clock actions
     void start();
     void stop();
     void pause();
     void unPause();

     // getters
     u64 getTicks();
     float elapsedTime();
     bool isPaused() { return (mIsPaused && mHasStarted); }
     bool hasStarted() { return mHasStarted; }
  };

}// --- namespace  fc --- (END)
