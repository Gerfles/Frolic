//> fc_types.cpp <//
#include "fc_types.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{

// *-*-*-*-*-*-*-*-*-*-*-   BOILERPLATE BIT-WISE ENUM OPS   *-*-*-*-*-*-*-*-*-*-*- //
  MaterialFeatures operator| (MaterialFeatures lhs, MaterialFeatures rhs)
  {
    using FeaturesType = std::underlying_type<MaterialFeatures>::type;
    return MaterialFeatures(static_cast<FeaturesType>(lhs) | static_cast<FeaturesType>(rhs));
  }
  MaterialFeatures operator& (MaterialFeatures lhs, MaterialFeatures rhs)
  {
    using FeaturesType = std::underlying_type<MaterialFeatures>::type;
    return MaterialFeatures(static_cast<FeaturesType>(lhs) & static_cast<FeaturesType>(rhs));
  }
  MaterialFeatures& operator|= (MaterialFeatures& lhs, MaterialFeatures const& rhs)
  {
    lhs = lhs | rhs;
    return lhs;
  }
  MaterialFeatures& operator&= (MaterialFeatures& lhs, MaterialFeatures const& rhs)
  {
    lhs = lhs & rhs;
    return lhs;
  }
  MaterialFeatures operator~ (MaterialFeatures const & rhs)
  {
    using FeaturesType = std::underlying_type<MaterialFeatures>::type;
    return MaterialFeatures(~static_cast<FeaturesType>(rhs));
  }

}// --- namespace fc --- (END)
