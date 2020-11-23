#pragma once
#include <generated/SCPXDR.h>

namespace std
{
template<>
struct hash<payshares::uint256>
{
  size_t operator()(payshares::uint256 const & x) const noexcept;
};

}
