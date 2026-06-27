#pragma once

#include "ColorMapOptions.h"
#include "ColorTypes.h"
#include <cairo/cairo.h>
#include <functional>
#include <vector>

namespace Giza
{
class ColorMapper
{
 public:
  void options(const ColorMapOptions& theOptions);

  static Histogram histogram(cairo_surface_t* image);
  const ColorMap& colormap() const;
  // The reduced (target) colors ordered by descending pixel use count, ties
  // broken by ascending color value. This is the palette index order: the most
  // used color is index 0. Empty in true color mode.
  const std::vector<Color>& palette() const;
  void reduce(cairo_surface_t* image);
  bool trueColor() const;

 private:
  ColorMapOptions itsOptions;
  ColorMap itsColorMap;           // latest calculated color conversion map
  std::vector<Color> itsPalette;  // reduced colors ordered by descending use count

};  // class ColorMapper

}  // namespace Giza
