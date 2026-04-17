//>--- fc_pool.cpp ---<//
#include "fc_pool.hpp"
#include "core/fc_assert.hpp"
#include "fc_image.hpp"
#include "fc_buffer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  template <typename PoolT>
  template <typename ElementT>
  FcHandle<ElementT> FcPool<PoolT>::createElement(ElementT&& obj)
  {
    u32 index = 0;
    if (mFreeListHead != END_OF_LIST_SENTINEL)
    {
      index = mFreeListHead;
      mFreeListHead = mElements[index].nextFree_;
      mElements[index].element_ = std::move(obj);
    }
    else
    {
      index = static_cast<u32>(mElements.size());
      mElements.emplace_back(obj);
    }

    ++mNumObjects;

    return FcHandle<ElementT>(index, mElements[index].generation_);
  }

  //
  template <typename PoolT>
  template <typename ElementT>
  void FcPool<PoolT>::destroyElement(FcHandle<ElementT> handle)
  {
    if (handle.isEmpty())
      return;

    const u32 index = handle.index();

    FC_ASSERT(index < mElements.size() && mNumObjects > 0);

    //
    FC_ASSERT(handle.generation() == mElements[index].generation_);

    // If all sanity checks pass, replace stored obj with empty default object and
    mElements[index].element_ = ElementT{};

    // increment its generation
    ++mElements[index].generation_;

    // Finally, place the array element at the front of the free list
    mElements[index].nextFree_ = mFreeListHead;
    mFreeListHead = index;
    --mNumObjects;
  }


  //
  template <typename PoolT>
  template <typename ElementT>
  ElementT* FcPool<PoolT>::getElement(FcHandle<ElementT> handle)
  {
    if (handle.isEmpty())
      return nullptr;

    const u32 index = handle.index();

    FC_ASSERT(index < mElements.size());
    // Make sure we're not trying to access a deleted element
    FC_ASSERT(handle.generation() == mElements[index].generation_);

    return &mElements[index].element_;
  }


  template <typename PoolT>
  template <typename ElementT>
  const ElementT* FcPool<PoolT>::getElement(FcHandle<ElementT> handle) const
  {
    if (handle.isEmpty())
      return nullptr;

    const u32 index = handle.index();

    FC_ASSERT(index < mElements.size());
    // Make sure we're not trying to access a deleted element
    FC_ASSERT(handle.generation() == mElements[index].generation_);

    return &mElements[index].element_;
  }


  //
  // TODO Causes the destructor to be called for every object so make sure that all objs have
  // working DTORS
  template<typename PoolT>
  void FcPool<PoolT>::clear()
  {
    mElements.clear();

    mNumObjects = 0;
    mFreeListHead = END_OF_LIST_SENTINEL;
  }


  //
  // DELETE Unsafe function suitable only for debugging
  template <typename PoolT>
  template <typename ElementT>
  FcHandle<ElementT> FcPool<PoolT>::getHandle(u32 index) const
  {
    FC_ASSERT(index > 0 && index < mElements.size());

    return FcHandle<ElementT>(index, mElements[index].generation_);
  }


  // TODO add a const to ElementT
  template <typename PoolT>
  template <typename ElementT>
  FcHandle<ElementT> FcPool<PoolT>::findObject(ElementT* element)
  {
    if (!element)
      return {};

    for (size_t index = 0; index != mElements.size(); ++index)
    {
      if (mElements[index].element_ == *element)
      {
        return FcHandle<ElementT>(static_cast<u32>(index), mElements[index].generation_);
      }
    }
    // Return an empty handle if no element was found
    return {};
  }


  // *-*-*-*-*-*-*-*-*-*-   EXPLICIT TEMPLATE INSTANTIATIONS   *-*-*-*-*-*-*-*-*-*- //
  // - FcBuffer -
  template FcHandle<FcBuffer> FcPool<FcBuffer>::createElement(FcBuffer&& buffer);
  template void FcPool<FcBuffer>::destroyElement(FcHandle<FcBuffer> handle);
  template FcBuffer* FcPool<FcBuffer>::getElement(FcHandle<FcBuffer> handle);
  template const FcBuffer* FcPool<FcBuffer>::getElement(FcHandle<FcBuffer> handle) const;
  template FcHandle<FcBuffer> FcPool<FcBuffer>::findObject(FcBuffer* element);
  template FcHandle<FcBuffer> FcPool<FcBuffer>::getHandle(u32 index) const;
  template void FcPool<FcBuffer>::clear();
  // - FcImage -
  template FcHandle<FcImage> FcPool<FcImage>::createElement(FcImage&& buffer);
  template void FcPool<FcImage>::destroyElement(FcHandle<FcImage> handle);
  template FcImage* FcPool<FcImage>::getElement(FcHandle<FcImage> handle);
  template const FcImage* FcPool<FcImage>::getElement(FcHandle<FcImage> handle) const;
  template FcHandle<FcImage> FcPool<FcImage>::findObject(FcImage* element);
  template FcHandle<FcImage> FcPool<FcImage>::getHandle(u32 index) const;
  template void FcPool<FcImage>::clear();
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

} // --- namespace fc --- (END)
