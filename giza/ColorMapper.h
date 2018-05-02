#pragma once

#include "ColorMapOptions.h"
#include "ColorTypes.h"
#include <cairo/cairo.h>
#include <functional>

namespace Giza
{
class ColorMapper
{
 public:
  void options(const ColorMapOptions& theOptions);

  Histogram histogram(cairo_surface_t* image) const;
  const ColorMap& colormap() const;
  void reduce(cairo_surface_t* image);
  bool trueColor() const;

 private:
  ColorMapOptions itsOptions;
  ColorMap itsColorMap;  // latest calculated color conversion map

};  // class ColorMapper

}  // namespace Giza
