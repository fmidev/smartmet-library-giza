#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>

namespace Giza
{
using Color = uint32_t;
using Count = uint32_t;
// Unordered: the color reduction does only key lookups/inserts, never ordered
// iteration, so a hash map avoids the red-black tree's O(log N) cache-missing
// descent on every unique color in build_tree.
using ColorMap = std::unordered_map<Color, Color>;
using Histogram = std::multimap<Count, Color, std::greater<std::size_t>>;

inline unsigned char alpha(Color color)
{
  return (color >> 24) & 0xFF;
}
inline unsigned char red(Color color)
{
  return (color >> 16) & 0xFF;
}
inline unsigned char green(Color color)
{
  return (color >> 8) & 0xFF;
}
inline unsigned char blue(Color color)
{
  return (color >> 0) & 0xFF;
}
}  // namespace Giza
