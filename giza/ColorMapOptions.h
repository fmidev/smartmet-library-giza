#pragma once

namespace Giza
{
struct ColorMapOptions
{
  double quality = 10.0;  // 10 = good, 20 = poor

  // The default value 2 means the requested quality is halved
  // during each iteration if the requested number of max colors
  // cannot be reached. The value must be at least 1.1 to obtain
  // sensible convergence rates.
  double errorfactor = 2.0;  // > 1, 2 is the default

  // The setting may not be obeyed if it would require removing
  // solid 3x3 colour blocks. A value <= 0 implies no maximum.
  int maxcolors = 0;

  bool truecolor = false;  // true if truecolor is forced
};
}  // namespace Giza
