//>--- fc_pool.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_handle.hpp"
#include "platform.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // Proponents of Data-Oriented Design may argue that this structure minimizes cache utilization by
  // interleaving the payload of ImplObjectType with utility val- ues gen_ and nextFree_. This is indeed
  // true. One approach mitigate to this is to store (memory alias) the nextFree_ element in-place of the
  // unused object in the pool. Another approach to is to maintain two separate arrays. The first array
  // can tightly pack ImplObjectType values, while the second one can store the neces- sary metadata for
  // bookkeeping. In fact, it can go a step further, as mentioned in the original presentation by
  // Sebastian Aaltonen (https://advances.realtimerendering.com/s2023/AaltonenHypeHypeAdvances2023.pdf),
  // by separating a high-frequency accessed “hot” object type from a low-frequency accessed “cold” type,
  // which can be stored in different arrays.
  template <typename PoolElementT>
  class FcPool
  {
     static constexpr u32 END_OF_LIST_SENTINEL = 0xFFFFFFFF;

   private:
     u32 mNumObjects {0};
     u32 mFreeListHead {END_OF_LIST_SENTINEL};

     struct PoolElement
     {
        u32 generation_ {1};
        u32 nextFree_ {END_OF_LIST_SENTINEL};
        PoolElementT element_ {};
        //
        explicit PoolElement(PoolElementT& object) : element_(std::move(object)) {}
        // bool operator==(PoolElementT& other) { return true; }
     };

   public:
     // TODO allow for initialization of number of elements
     // TODO place in private
     std::vector<PoolElement> mElements;
     //
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //
     void init(u32 initialFreeSlots) { mElements.reserve(initialFreeSlots); };
     //
     inline u32 size() const noexcept { return mNumObjects; }
     //
     template <typename Element> FcHandle<Element> createElement(Element&& obj);
     //
     template <typename Element> void destroyElement(FcHandle<Element> handle);
     //
     template <typename Element> Element* getElement(FcHandle<Element> handle);
     //
     template <typename Element> const Element* getElement(FcHandle<Element> handle) const;
     //
     void clear();
     //
     template <typename Element> FcHandle<Element> findObject(Element* element);
     //
     // DELETE Unsafe function suitable only for debugging
     template <typename Element> FcHandle<Element> getHandle(u32 index) const;
     //


  };

  // May need to guarantee the compiler won't try to create instatiations for these in the TL but search
  // for them during the linking stage instead.
  /* extern template FcHandle<FcBuffer> FcPool<FcBuffer>::create(FcBuffer&& buffer); */

} // --- namespace fc --- (END)
