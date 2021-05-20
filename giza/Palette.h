#pragma once
#include "ColorTypes.h"
#include <unordered_map>
#include <vector>

namespace Giza
{
class Palette
{
 public:
  Palette() = delete;
  Palette(const ColorMap& colormap);

  std::size_t size() const { return itsSize; }
  std::size_t index(Color color) const { return itsIndexes.at(color); }
  Color color(std::size_t index) const { return itsPalette.at(index); }

 private:
  std::size_t itsSize;
  std::vector<Color> itsPalette;
  std::unordered_map<Color, std::size_t> itsIndexes;

};  // class Palette
}  // namespace Giza
