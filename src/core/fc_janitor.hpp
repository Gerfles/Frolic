#include <deque>
#include <functional>

// TODO at the moment, this is implemented with callback functors but would
// better scale with arrays of vulkan handles to systematically delete the
// various types from a loop.
// TODO should also think about making this a class
namespace fc
{
  class fcJanitor
  {
   private:
     std::deque< std::function<void()> > deletors;

     // fcJanitor(const fcJanitor&) = delete;
     // fcJanitor(fcJanitor&&) = delete;
     // fcJanitor& operator=(const fcJanitor&) = delete;
     // fcJanitor& operator=(fcJanitor&&) = delete;
   public:
     void push_function(std::function<void()>&& function) { deletors.push_back(function); }
     void flush();
  };
}
