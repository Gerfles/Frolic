#include "fc_janitor.hpp"


namespace fc
{

  void fcJanitor::flush()
  {
     // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
    {
      (*it)(); // call functors
    }

    deletors.clear();
  }

}
