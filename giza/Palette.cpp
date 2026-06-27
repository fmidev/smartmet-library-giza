#include "Palette.h"
#include <macgyver/Exception.h>

namespace Giza
{
// ----------------------------------------------------------------------
/*!
 * \brief Palette constructor establish indexes for colors and their count
 */
// ----------------------------------------------------------------------

Palette::Palette(const std::vector<Color>& colors) : itsPalette(colors)
{
  try
  {
    // The colors are already deduplicated and in index order, so the index of
    // each color is simply its position in the vector.
    itsIndexes.reserve(itsPalette.size());
    for (std::size_t i = 0; i < itsPalette.size(); ++i)
      itsIndexes.emplace(itsPalette[i], i);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
