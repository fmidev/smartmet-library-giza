#pragma once

#include <cstdint>
#include <map>

namespace Giza
{
using Color = uint32_t;
using Count = uint32_t;
using ColorMap = std::map<Color, Color>;
using Histogram = std::multimap<Count, Color, std::greater<std::size_t>>;

inline unsigned char alpha(Color color) { return (color >> 24) & 0xFF; }
inline unsigned char red(Color color) { return (color >> 16) & 0xFF; }
inline unsigned char green(Color color) { return (color >> 8) & 0xFF; }
inline unsigned char blue(Color color) { return (color >> 0) & 0xFF; }
}
