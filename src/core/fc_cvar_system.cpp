// fc_cvar_system.cpp
#include "fc_cvar_system.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <unordered_map>


namespace fc
{

  enum class CVarType : char
  {
    INT,
    FLOAT,
    STRING,
  };

  class CVarParameter
  {
   public:
     friend class CVarSystemImpl;

     int32_t arrayIndex;
     CVarType type;
     CVarFlags flags;
     std::string name;
     std::string description;
  };


  template<typename T>
  struct CVarStorage
  {
     T initial;
     T current;
     CVarParameter* parameter;
  };


// TODO add additional function from the tutorial source code
  template<typename T>
  struct CVarArray
  {
     CVarStorage<T>* cvars {nullptr};
     int32_t lastCVar {0};

     CVarArray(size_t size)
      {
        cvars = new CVarStorage<T>[size]();
      }

     ~CVarArray()
      {
        delete cvars;
      }


     T* GetCurrentPtr(int32_t index)
      {
        return &cvars[index].current;
      }

     T GetCurrent(int32_t index)
      {
        return cvars[index].current;
      }

     void SetCurrent(const T& value, int32_t index)
      {
        cvars[index].current = value;
      }

     int Add(const T& value, CVarParameter* parameter)
      {
        int index = lastCVar;

        cvars[index].current = value;
        cvars[index].initial = value;
        cvars[index].parameter = parameter;

        parameter->arrayIndex = index;
        lastCVar++;

        return index;
      }

     int Add(const T& initialValue, const T& currentValue, CVarParameter* parameter)
      {
        int index = lastCVar;

        cvars[index].initial = initialValue;
        cvars[index].current = currentValue;
        cvars[index].parameter = parameter;

        parameter->arrayIndex = index;
        lastCVar++;

        return index;
      }
  };



  class CVarSystemImpl : public CVarSystem
  {
   private:
     std::unordered_map<uint32_t, CVarParameter> savedCVars;
     //
     CVarParameter* InitCVar(const char* name, const char* description);
   public:
     CVarParameter* GetCVar(StringHash hash) override final;
     CVarParameter* CreateFloatCVar(const char* name, const char* description,
                                    float defaultValue, float currentValue) override final;
     float* GetFloatCVar(StringHash hash) override final;
     void setFloatCVar(StringHash hash, float value) override final;
     // TODO More versions of create for ints/strings

     constexpr static int MAX_INT_CVARS = 100;
     CVarArray<int32_t> intCVars{ MAX_INT_CVARS };

     constexpr static int MAX_FLOAT_CVARS = 100;
     CVarArray<float> floatCVars{ MAX_FLOAT_CVARS };

     constexpr static int MAX_STRING_CVARS = 100;
     CVarArray<std::string> stringCVars{ MAX_STRING_CVARS };

     static CVarSystemImpl* Get()
      {
        return static_cast<CVarSystemImpl*>(CVarSystem::Get());
      }

     // Using templates with specializations to get the cvar arrays for each type.
     // If you try to use a type that doesn't have specialization, it will trigger a linker error
     template<typename T>
     CVarArray<T>* GetCVarArray();

     template<>
     CVarArray<int32_t>* GetCVarArray()
      {
        return &intCVars;
      }

     template<>
     CVarArray<float>* GetCVarArray()
      {
        return &floatCVars;
      }

     template<>
     CVarArray<std::string>* GetCVarArray()
      {
        return &stringCVars;
      }

     // templated get/set cvar versions
     template<typename T>
     T* GetCVarCurrent(uint32_t nameHash)
      {
        CVarParameter* cVarParam = GetCVar(nameHash);
        if (!cVarParam)
        {
          return nullptr;
        }
        else
        {
          return GetCVarArray<T>()->GetCurrentPtr(cVarParam->arrayIndex);
        }
      }

     template<typename T>
     void SetCVarCurrent(uint32_t nameHash, const T& value)
      {
        CVarParameter* cVarParam = GetCVar(nameHash);
        if (cVarParam)
        {
          GetCVarArray<T>()->SetCurrent(value, cVarParam->arrayIndex);
        }
      }



  }; // ---   class CVarSystemImpl : public CVarSystem --- (END)

// statically initialized singleton pattern -- modern CPP11+ way of executing singletons
  CVarSystem* CVarSystem::Get()
  {
    // fully threadsafe because of the rules of static variables inside functions
    static CVarSystemImpl cvarSys{};
    return &cvarSys;
  }


  // *-*-*-*-*-*-*-*-*-*-*-*-   FUNCTION IMPLEMENTATIONS   *-*-*-*-*-*-*-*-*-*-*-*- //
  CVarParameter* CVarSystemImpl::InitCVar(const char* name, const char* description)
  {
    // Return null if the cvar already exists
    if (GetCVar(name)) return nullptr;

    uint32_t nameHash = StringHash{ name };
    savedCVars[nameHash] = CVarParameter{};

    CVarParameter& newParam = savedCVars[nameHash];
    newParam.name = name;
    newParam.description = description;

    return &newParam;
  }


  // Get the cvar data purely by type and array index
  template<typename T>
  T GetCVarCurrentByIndex(int32_t index)
  {
    return CVarSystemImpl::Get()->GetCVarArray<T>()->GetCurrent(index);
  }

  // Set the cvar data purely by type and index
  template<typename T>
  void SetCVarCurrentByIndex(int32_t index, const T& data)
  {
    CVarSystemImpl::Get()->GetCVarArray<T>()->SetCurrent(data, index);
  }

  // CVar float constructor
  AutoCVarFloat::AutoCVarFloat(const char* name, const char* description, float defaultValue, CVarFlags flags)
  {
    CVarParameter* cvar = CVarSystem::Get()->CreateFloatCVar(name, description, defaultValue, defaultValue);
    cvar->flags = flags;
    mIndex = cvar->arrayIndex;
  }


  CVarParameter* CVarSystemImpl::CreateFloatCVar(const char *name, const char *description
                                                 , float defaultValue, float currentValue)
  {
    CVarParameter* cVarParam = InitCVar(name, description);
    if (!cVarParam) return nullptr;

    cVarParam->type = CVarType::FLOAT;

    GetCVarArray<float>()->Add(defaultValue, currentValue, cVarParam);

    return cVarParam;
  }


  CVarParameter* CVarSystemImpl::GetCVar(StringHash hash)
  {
    auto it = savedCVars.find(hash);

    if (it != savedCVars.end())
    {
      return &(*it).second;
    }

    return nullptr;
  }


  float* CVarSystemImpl::GetFloatCVar(StringHash hash)
  {
    return GetCVarCurrent<float>(hash);
  }

  void CVarSystemImpl::setFloatCVar(StringHash hash, float value)
  {
    SetCVarCurrent<float>(hash, value);
  }

}// --- namespace fc --- (END)
