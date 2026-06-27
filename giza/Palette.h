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
  // The colors must already be deduplicated and in the desired index order,
  // e.g. ColorMapper::palette().
  explicit Palette(const std::vector<Color>& colors);

  std::size_t size() const { return itsPalette.size(); }
  std::size_t index(Color color) const { return itsIndexes.at(color); }
  Color color(std::size_t index) const { return itsPalette.at(index); }

 private:
  std::vector<Color> itsPalette;
  std::unordered_map<Color, std::size_t> itsIndexes;

};  // class Palette
}  // namespace Giza
