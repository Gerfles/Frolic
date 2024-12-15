#pragma once

#include "platform.hpp"

namespace fc
{

  struct Service
  {
     virtual void init(void* configuration){};
     virtual void shutdown(){};
      // ?? shouldn't there be an instance() function
  };

#define FC_DECLARE_SERVICE(Type) static Type* instance();

} // --- namespace fc --- (END)
