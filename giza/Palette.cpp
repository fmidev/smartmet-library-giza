#include "Palette.h"
#include <macgyver/Exception.h>

namespace Giza
{
// ----------------------------------------------------------------------
/*!
 * \brief Palette constructor establish indexes for colors and their count
 */
// ----------------------------------------------------------------------

Palette::Palette(const ColorMap& colormap) : itsSize(0)
{
  try
  {
    // Assign unique index for all colours staring from zero.
    // Since there are potentially long sequences of color,
    // we try to optimize for speed by remembering the last
    // color added to the palette.

    Color prevcolor;
    bool first = true;

    for (const auto& colorpair : colormap)
    {
      Color newcolor = colorpair.second;

      if (first)
      {
        itsIndexes.insert(std::make_pair(newcolor, itsSize++));
        itsPalette.push_back(newcolor);
        prevcolor = newcolor;
        first = false;
      }
      else if (newcolor != prevcolor)
      {
        if (itsIndexes.insert(std::make_pair(newcolor, itsSize)).second)
        {
          itsPalette.push_back(newcolor);
          ++itsSize;
        }
        prevcolor = newcolor;
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
