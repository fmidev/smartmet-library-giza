#pragma once

#include "ColorTypes.h"
#include <cairo/cairo.h>
#include <functional>

namespace Giza
{
class ColorMapper
{
 public:
  ColorMapper();

  void setQuality(double quality);
  void setMaxColors(int maxcolors);
  void setErrorFactor(double errorfactor);

  Histogram histogram(cairo_surface_t* image) const;
  const ColorMap& colormap() const;
  void reduce(cairo_surface_t* image);
  bool truecolor() const;

 private:
  double itsQuality;          // 10 = good, 20 = poor
  double itsErrorFactor;      // > 1, 2 is the default
  int itsMaxColors;           // <=0 implies adaptive reduction
  bool itsTrueColor = false;  // true if truecolor is forced
  ColorMap itsColorMap;       // latest calculated color conversion map

};  // class ColorMapper

}  // namespace Giza
