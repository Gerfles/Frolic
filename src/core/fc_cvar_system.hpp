// fc_cvar_system.hpp

#include <cstdint>
#include "string_utils.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  class CVarParameter;

  enum class CVarFlags : uint32_t
  {
    None = 0,
  Noedit = 1 << 1,
  EditReadOnly = 1 << 2,
  Advance = 1 << 3,
  EditCheckbox = 1 << 8,
  EditFloatDrag = 1 << 9,
};

  class CVarSystem
  {
   public:
     // TODO rename withou capital ugh!
     static CVarSystem* Get();
     virtual CVarParameter* CreateFloatCVar(const char* name, const char* description,
                                            float defaultValue, float currentValue) = 0;
     virtual CVarParameter* GetCVar(StringHash hash) = 0;
     virtual float* GetFloatCVar(StringHash hash) = 0;
     virtual void setFloatCVar(StringHash hash, float value) = 0;
   private:

  };


  template<typename T>
  struct AutoCVar
  {
   protected:
     int mIndex;
     using CVarType = T;
  };


  struct AutoCVarFloat : AutoCVar<float>
  {
     AutoCVarFloat(const char* name, const char* description
                   , float defaultVal, CVarFlags flags = CVarFlags::None);
     float get();
     void set(float value);
  };

}// --- namespace fc --- (END)
