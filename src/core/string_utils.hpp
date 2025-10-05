// string_utils.hpp

#include <string_view>
#include <cstdint>

namespace fc
{
  // FNV-1a 32bit hashing algorithm
  constexpr uint32_t fnv1a_32(char const* str, std::size_t count)
  {
    return ((count ? fnv1a_32(str, count - 1) : 2166136261u) ^ str[count]) * 16777619u;
  }

  constexpr size_t const_strlen(const char* str)
  {
    size_t size{0};
    while (str[size]) { size++; }

    return size;
  }


  struct StringHash
  {
     uint32_t computedHash;

     StringHash(const StringHash& other) = default;

     constexpr StringHash(uint32_t hash) noexcept : computedHash(hash) {}

     constexpr StringHash(const char* str) noexcept : computedHash(0)
      {
        computedHash = fnv1a_32(str, const_strlen(str));
      }

     constexpr StringHash(const char* str, std::size_t count) noexcept : computedHash(0)
      {
        computedHash = fnv1a_32(str, count);
      }

     constexpr StringHash(std::string_view strView) noexcept : computedHash(0)
      {
        computedHash = fnv1a_32(strView.data(), strView.size());
      }

     constexpr operator uint32_t()noexcept { return computedHash; }
  };

}
