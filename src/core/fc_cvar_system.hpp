//>--- fc_cvar_system.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "string_utils.hpp"
#include "platform.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class CVarParameter; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  enum class CVarFlags : u32
  {
    None = 0,
    Noedit = 1 << 1,
    EditReadOnly = 1 << 2,
    Advance = 1 << 3,
    EditCheckbox = 1 << 8,
    EditFloatDrag = 1 << 9,
  };


  //
  class CVarSystem
  {
   public:
     // TODO rename without capital ugh!
     static CVarSystem* Get();
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FLOAT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     virtual CVarParameter* CreateFloatCVar(const char* name, const char* description,
                                            float defaultValue, float currentValue) = 0;
     virtual float* GetFloatCVar(StringHash hash) = 0;
     virtual void setFloatCVar(StringHash hash, float value) = 0;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   BOOL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     virtual CVarParameter* CreateBoolCVar(const char* name, const char* description,
                                           bool defaultValue, bool currentValue) = 0;
     virtual bool* GetBoolCVar(StringHash hash) = 0;
     virtual void setBoolCVar(StringHash hash, bool value) = 0;
     //// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     virtual CVarParameter* GetCVar(StringHash hash) = 0;

   private:
  }; // ---   class CVarSystem --- (END)


  //
  template<typename T>
  struct AutoCVar
  {
   protected:
     int mIndex;
     using CVarType = T;
  };


  //
  struct AutoCVarFloat : AutoCVar<float>
  {
     AutoCVarFloat(const char* name, const char* description
                   , float defaultVal, CVarFlags flags = CVarFlags::None);
     float get();
     void set(float value);
  };


  //
  struct AutoCVarBool : AutoCVar<bool>
  {
     AutoCVarBool(const char* name, const char* description,
                  bool defaultValue, CVarFlags flags = CVarFlags::None);
     bool get();
     void set(bool value);
  };

  /* struct AutoC */

}// --- namespace fc --- (END)
