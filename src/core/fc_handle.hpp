//>--- fc_handle.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
#include <utility>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcClassName; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  template<typename ObjectType>
  class FcHandle final
  {
   private:
     u32 mIndex {0};
     u32 mGeneration {0};

     FcHandle(u32 index, u32 generation) : mIndex(index), mGeneration(generation) {};

     //??
     /* template<typename mObjectType, typename ImplObjectType> friend class pool; */
     template<typename ObjectT> friend class FcPool;


   public:
     FcHandle() = default;
     //
     inline bool isEmpty() const noexcept { return mGeneration == 0; }
     //
     inline bool isValid() const noexcept { return mGeneration != 0; }
     //
     inline u32 index() const noexcept { return mIndex; }
     //
     // TODO verify that this is both needed and functions correctly
     inline void* indexAsVoidPtr() const noexcept
      { return reinterpret_cast<void*>(static_cast<ptrdiff_t>(index())); }
     //
     inline u32 generation() const noexcept { return mGeneration; }
     //
     inline bool operator==(const FcHandle<ObjectType>& other) const noexcept
      { return mIndex == other.mIndex && mGeneration == other.mGeneration; }
     //
     inline bool operator!=(const FcHandle<ObjectType>& other) const noexcept
      { return mIndex != other.mIndex || mGeneration != other.mGeneration; }
     //
     // explicit conversion to bool so we can use in cond. statements as "if(handle)..."
     inline explicit operator bool() const noexcept { return mGeneration != 0; }
  };

  // ?? verify correctness
  static_assert(sizeof(FcHandle<class ObjectType>) == sizeof(u64));

  // Ensure type safety
  /* using FcSamplerHandle = FcHandle<struct Sampler>; */
  using FcBufferHandle = FcHandle<struct FcBuffer>;
  /* using FcTextureHandle = FcHandle<struct Texture>; */


  //
  template <typename FcHandleType>
  class FcHolder final
  {
   private:
     // TODO using handle type here is confusing because it give the impression its just
     // a handle and not the actual object that holder uses...
     FcHandleType mHandle {};
   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcHolder() = default;
     //
     FcHolder(FcHandleType handle) : mHandle(handle) {};
     //
     // Ensure we only allow move semantics, like std::unique_ptr
     FcHolder(const FcHolder& other) = delete;
     FcHolder& operator=(const FcHolder& other) = delete;
     //
     // ?? would we want to leave the other with a empty object or just delete it? perhaps better for
     // segfaults
     FcHolder(FcHolder&& other) : mHandle(other.mHandle) { other.mHandle = FcHandleType{}; }
     //
     // ?? seems like swaping the handles could lead to issues with the other.mHandle... might be better as null
     FcHolder& operator=(FcHolder&& other) { std::swap(mHandle, other.mHandle); return *this; }
     //
     // ?? should this be janitor function instead
     ~FcHolder() { fcDestroy(mHandle); }
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     inline FcHandleType release() noexcept { return std::exchange(mHandle, FcHandleType {} ); }
     //
     inline void reset() noexcept { fcDestroy(mHandle); mHandle = FcHandleType{}; };
     // Special handling of assigning nullptr to this FcHolder
     inline FcHolder& operator=(std::nullptr_t) noexcept { this->reset(); return *this; }
     //
     inline u32 index() const noexcept { return mHandle.mIndex(); }
     //
     inline void* indexAsVoidPtr() const noexcept { return mHandle.indexAsVoidPtr(); }
     //
     inline operator FcHandleType() const { return mHandle; }
     //

  };

} // --- namespace fc --- (END)
