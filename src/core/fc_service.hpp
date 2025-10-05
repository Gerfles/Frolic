#pragma once

namespace fc
{
  struct Service
  {
     virtual void init(void* configuration) {}
     virtual void shutdown() {}
  };

  // ?? Is this a singleton pattern and if so is this the best instantiation scheme
  #define FC_DECLARE_SERVICE(Type) static Type* instance();

}// --- namespace fc --- (END)
