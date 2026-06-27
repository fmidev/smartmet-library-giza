#include "ColorMapper.h"
#include "ColorTree.h"
#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>
#include <macgyver/Exception.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

// boost::unordered_flat_map was introduced in Boost 1.81. Older toolchains
// (e.g. Boost 1.69 on RHEL8 production) fall back to the hand-rolled open
// addressing table below.
#if BOOST_VERSION >= 108100
#include <boost/unordered/unordered_flat_map.hpp>
#define GIZA_HAVE_BOOST_FLAT_MAP 1
#endif

namespace Giza
{

namespace
{

// ----------------------------------------------------------------------
/*
 * Helper classes
 */
// ----------------------------------------------------------------------

// Histogram data
struct ColorInfo
{
  explicit ColorInfo(Color c) : color(c) {}
  Color color = 0;
  Count count = 0;
  bool keeper = false;

  ColorInfo &operator++()
  {
    ++count;
    return *this;
  }
  void keep() { keeper = true; }
};

// Internal histogram
using ColorHistogram = std::vector<ColorInfo>;

// ----------------------------------------------------------------------
/*!
 * \brief Open-addressing color histogram
 *
 * Maps each color to the index of its ColorInfo record. In both variants below
 * the records live in a std::deque, whose element addresses stay stable as it
 * grows: calc_histogram caches raw ColorInfo pointers (last1/last2) across
 * insertions and relies on that. The flat lookup table only ever stores indices
 * into the deque, so it is free to rehash and relocate without invalidating
 * those cached pointers.
 */
// ----------------------------------------------------------------------

#if defined(GIZA_HAVE_BOOST_FLAT_MAP)

// Preferred implementation: a SIMD-accelerated open-addressing flat map.
class FlatHistogram
{
 public:
  explicit FlatHistogram(std::size_t expected) { itsIndex.reserve(expected); }

  // Return the record for color, creating it (with count 0) if it is new.
  ColorInfo *get(Color color)
  {
    auto result = itsIndex.try_emplace(color, static_cast<uint32_t>(itsEntries.size()));
    if (result.second)
      itsEntries.emplace_back(color);
    return &itsEntries[result.first->second];
  }

  std::deque<ColorInfo> &entries() { return itsEntries; }

 private:
  boost::unordered_flat_map<Color, uint32_t> itsIndex;
  std::deque<ColorInfo> itsEntries;
};

#else

// Fallback for Boost < 1.81: a small open-addressing, linear-probing table that
// keeps the key and a deque index inline for cache-friendly probing.
//
// The empty-slot sentinel is a color that cannot occur: a Cairo ARGB32 surface
// is premultiplied, so any pixel with alpha 0 has RGB 0, making this alpha-0
// value with non-zero RGB impossible.
constexpr Color EMPTY_SLOT = 0x00000001U;

// Multiplicative (Fibonacci) hash; 2654435761 = round(2^32 / golden ratio). Its
// high bits are well mixed, so taking them avoids primary clustering of nearby
// colors under linear probing.
inline uint32_t color_hash(Color c)
{
  return c * 2654435761U;
}

class FlatHistogram
{
 public:
  explicit FlatHistogram(std::size_t expected)
  {
    std::size_t cap = 16;
    itsShift = 28;  // 32 - log2(16)
    while (cap < 2 * expected && itsShift > 1)
    {
      cap <<= 1;
      --itsShift;
    }
    itsSlots.assign(cap, Slot{EMPTY_SLOT, 0});
    itsMask = static_cast<uint32_t>(cap - 1);
  }

  // Return the record for color, creating it (with count 0) if it is new.
  ColorInfo *get(Color color)
  {
    uint32_t h = color_hash(color) >> itsShift;
    while (true)
    {
      Slot &slot = itsSlots[h];
      if (slot.key == color)
        return &itsEntries[slot.index];
      if (slot.key == EMPTY_SLOT)
      {
        if (2 * (itsEntries.size() + 1) > itsSlots.size())  // keep load factor <= 0.5
        {
          grow();
          return get(color);
        }
        itsEntries.emplace_back(color);
        slot.key = color;
        slot.index = static_cast<uint32_t>(itsEntries.size() - 1);
        return &itsEntries.back();
      }
      h = (h + 1) & itsMask;
    }
  }

  std::deque<ColorInfo> &entries() { return itsEntries; }

 private:
  struct Slot
  {
    Color key;
    uint32_t index;
  };

  void grow()
  {
    const std::size_t cap = itsSlots.size() * 2;
    itsSlots.assign(cap, Slot{EMPTY_SLOT, 0});
    itsMask = static_cast<uint32_t>(cap - 1);
    --itsShift;
    for (uint32_t i = 0; i < itsEntries.size(); ++i)
    {
      uint32_t h = color_hash(itsEntries[i].color) >> itsShift;
      while (itsSlots[h].key != EMPTY_SLOT)
        h = (h + 1) & itsMask;
      itsSlots[h] = Slot{itsEntries[i].color, i};
    }
  }

  std::vector<Slot> itsSlots;
  std::deque<ColorInfo> itsEntries;
  uint32_t itsMask = 0;
  uint32_t itsShift = 0;
};

#endif

// ----------------------------------------------------------------------
/*!
 * \brief Extract a color from Cairo surface data
 */
// ----------------------------------------------------------------------

inline Color get_color(unsigned char *data, int i, int j, int stride)
{
  unsigned char *ptr = data + j * stride + sizeof(Color) * i;
  return *reinterpret_cast<Color *>(ptr);
}

// ----------------------------------------------------------------------
/*!
 * \brief Set a color to Cairo surface data
 */
// ----------------------------------------------------------------------

inline void set_color(unsigned char *data, int i, int j, int stride, Color color)
{
  unsigned char *ptr = data + j * stride + sizeof(Color) * i;
  *reinterpret_cast<Color *>(ptr) = color;
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate the occurrance count of each color in the given image
 *
 * \param image The image
 * \return The colormap with occurrance counts
 */
// ----------------------------------------------------------------------

ColorHistogram calc_histogram(cairo_surface_t *image)
{
  try
  {
    // Verify data is ok to process

    if (image == nullptr)
      throw Fmi::Exception(BCP, "Cannot calculate colour histogram for a null pointer");

    auto format = cairo_image_surface_get_format(image);
    if (format != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Color quantization is implemented only for ARGB32 images");

    // Shorthand variables

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);
    int stride = cairo_image_surface_get_stride(image);  // bytes to next row
    unsigned char *data = cairo_image_surface_get_data(image);

    const std::size_t pixels = static_cast<std::size_t>(width) * height;

    // Not sure if this is even valid in Cairo, check anyway

    if (pixels == 0)
      return {};

    // Size the table for an upper bound of the number of distinct colors so it
    // rarely has to grow. The pixel count is the hard upper bound; we cap it
    // because images with more colors than this go to true color anyway, where
    // per-pixel probing (not table growth) dominates.

    FlatHistogram counter(std::min<std::size_t>(pixels, std::size_t{1} << 16));

    // Insert the first color so we can prime the last1/last2 pointer cache. The
    // count starts at 0; the first loop iteration increments it.

    Color color0 = get_color(data, 0, 0, stride);
    ColorInfo *last1 = counter.get(color0);
    ColorInfo *last2 = last1;

    // Length of the current horizontal run of identical colors. A solid 3x3
    // block requires at least three identical pixels in a row, so the block
    // test below is skipped until the run reaches three. The block is anchored
    // at its top edge (the current run) and verified against the next two rows,
    // so a color in a solid region becomes a keeper at the first opportunity and
    // the rest of the region short-circuits on the keeper flag.
    int run = 0;

    for (int j = 0; j < height; j++)
      for (int i = 0; i < width; i++)
      {
        Color color = get_color(data, i, j, stride);

        if (last1->color == color)
        {
          ++(*last1);
          // A match across a row boundary does not continue a horizontal run
          run = (i == 0 ? 1 : run + 1);

          // Once three colors line up horizontally (run >= 3 implies i >= 2),
          // test whether they are the top edge of a solid 3x3 box. The current
          // run covers row j at columns i-2..i, so only the next two rows remain
          // to be checked.
          if (run >= 3 && !last1->keeper && j + 2 < height)
          {
            // Test the farther row (j+2) first: it is less spatially correlated
            // with the matched run on row j, so it is the likeliest to differ and
            // short-circuit the && chain (e.g. for a band exactly two rows tall).
            if (get_color(data, i - 2, j + 2, stride) == color &&
                get_color(data, i - 1, j + 2, stride) == color &&
                get_color(data, i, j + 2, stride) == color &&
                get_color(data, i - 2, j + 1, stride) == color &&
                get_color(data, i - 1, j + 1, stride) == color &&
                get_color(data, i, j + 1, stride) == color)
            {
              last1->keep();
            }
          }
        }
        else if (last2->color == color)
        {
          ++(*last2);
          std::swap(last1, last2);
          run = 1;
        }
        else
        {
          // get() may grow the table, but only the inline slot array is
          // reallocated; the ColorInfo records (and hence last1/last2) live in a
          // deque and keep their addresses.
          ColorInfo *p = counter.get(color);
          last2 = last1;
          last1 = p;
          ++(*last1);
          run = 1;
        }
      }

    return ColorHistogram(counter.entries().begin(), counter.entries().end());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform color replacement
 */
// ----------------------------------------------------------------------

void replace_colors(cairo_surface_t *image, const Giza::ColorMap &colormap)
{
  try
  {
    // Verify data is ok to process

    if (image == nullptr)
      throw Fmi::Exception(BCP, "Cannot calculate colour histogram for a null pointer");

    auto format = cairo_image_surface_get_format(image);
    if (format != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Color replacement is implemented only for ARGB32 images");

    // Shorthand variables

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);
    int stride = cairo_image_surface_get_stride(image);  // bytes to next row
    unsigned char *data = cairo_image_surface_get_data(image);

    // colormap is an unordered_map keyed on the color, so the per-pixel lookups
    // below are already O(1). Every color in the image is a key in it (the map
    // is built from the full histogram), so at() always succeeds.

    // Remember last color conversions for extra speed

    Color last_color1 = 0;
    Color last_choice1 = 0;
    Color last_color2 = 0;
    Color last_choice2 = 0;

    for (int j = 0; j < height; j++)
    {
      for (int i = 0; i < width; i++)
      {
        Color color = get_color(data, i, j, stride);
        if (color == last_color1)
          set_color(data, i, j, stride, last_choice1);
        else if (color == last_color2)
        {
          set_color(data, i, j, stride, last_choice2);
          std::swap(last_color1, last_color2);
          std::swap(last_choice1, last_choice2);
        }
        else
        {
          last_color2 = last_color1;
          last_choice2 = last_choice1;
          last_color1 = color;
          last_choice1 = colormap.at(color);
          set_color(data, i, j, stride, last_choice1);
        }
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Build a color tree and a colormap
 */
// ----------------------------------------------------------------------

void build_tree(cairo_surface_t *image,
                const ColorHistogram &hist,
                ColorTree &colortree,
                ColorMap &colormap,
                double quality)
{
  try
  {
    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);

    const double ratio = 1.0 / (width * height);
    const double factor = -quality / log(10.0);

    colormap.reserve(hist.size());

    for (const auto &info : hist)
    {
      if (colortree.empty() || info.keeper)
      {
        colortree.insert(info.color);
        colormap[info.color] = info.color;
      }
      else
      {
        double dist = 0;
        Color nearest = colortree.nearest(info.color, dist);

        double limit = factor * log(ratio * info.count);

        if (dist < limit)
        {
          colormap[info.color] = nearest;
        }
        else
        {
          colortree.insert(info.color);
          colormap[info.color] = info.color;
        }
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Build a color tree and a colormap
 */
// ----------------------------------------------------------------------

void build_tree(cairo_surface_t *image,
                const ColorHistogram &hist,
                ColorTree &colortree,
                ColorMap &colormap,
                double quality,
                int maxcolors,
                double errorfactor)
{
  try
  {
    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);

    const double ratio = 1.0 / (width * height);

    colormap.reserve(hist.size());

    bool done = false;
    while (!done)
    {
      done = true;
      const double factor = -quality / log(10.0);

      for (const auto &info : hist)
      {
        if (colortree.empty() || info.keeper)
        {
          colortree.insert(info.color);
          colormap[info.color] = info.color;
        }
        else
        {
          double dist = 0;
          Color nearest = colortree.nearest(info.color, dist);

          const double limit = factor * log(ratio * info.count);
          if (dist < limit)
            colormap[info.color] = nearest;
          else if (colortree.size() < maxcolors)
          {
            colortree.insert(info.color);
            colormap[info.color] = info.color;
          }
          else
          {
            colortree.clear();
            colormap.clear();
            done = false;
            quality *= errorfactor;
            break;
          }
        }
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Histogram order: keepers first, then by descending occurrence count. Kept as
// a small inlinable functor (no function-pointer indirection, no try/catch) so
// the compiler can inline it into the sort's inner loop. The comparison is pure
// integer/bool work and cannot throw.
struct ColorCmp
{
  bool operator()(const ColorInfo &info1, const ColorInfo &info2) const noexcept
  {
    if (info1.keeper == info2.keeper)
      return info2.count < info1.count;
    return info1.keeper;
  }
};

// ----------------------------------------------------------------------
/*!
 * \brief Calculate the histogram for color reduction
 *
 * \param image The image
 * \return The histogram object
 */
// ----------------------------------------------------------------------

ColorHistogram colorhistogram(cairo_surface_t *image)
{
  try
  {
    ColorHistogram histogram = calc_histogram(image);

    std::sort(histogram.begin(), histogram.end(), ColorCmp());

    return histogram;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Order the reduced colors by descending pixel use count
 *
 * The use count of a reduced (target) color is the sum of the histogram
 * counts of all the original colors that map to it. Ties are broken by
 * ascending color value so the result is fully deterministic. The returned
 * order is used directly as the palette index order, so the most used color
 * becomes index 0.
 */
// ----------------------------------------------------------------------

std::vector<Color> ordered_palette(const ColorHistogram &hist,
                                   const ColorMap &colormap,
                                   std::size_t maxcolors)
{
  try
  {
    std::unordered_map<Color, Count> usecount;
    usecount.reserve(colormap.size());
    for (const auto &info : hist)
      usecount[colormap.at(info.color)] += info.count;

    // More than maxcolors distinct colors means the image will be encoded in
    // true color, so no palette is needed and we skip the (wasted) ordering.
    if (usecount.size() > maxcolors)
      return {};

    std::vector<Color> palette;
    palette.reserve(usecount.size());
    for (const auto &value : usecount)
      palette.push_back(value.first);

    std::sort(palette.begin(),
              palette.end(),
              [&usecount](Color a, Color b)
              {
                const Count ca = usecount.at(a);
                const Count cb = usecount.at(b);
                if (ca != cb)
                  return ca > cb;
                return a < b;
              });

    return palette;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Set colormapper options
 */
// ----------------------------------------------------------------------

void ColorMapper::options(const ColorMapOptions &theOptions)
{
  try
  {
    itsOptions = theOptions;

    if (itsOptions.quality < 1)
      throw Fmi::Exception(BCP,
                           "Requested relative color map quality must be at least 1, value " +
                               boost::lexical_cast<std::string>(itsOptions.quality) + " was given");

    if (itsOptions.errorfactor < 1.1)
      throw Fmi::Exception(BCP,
                           "Image color quantization error factor must be at least 1.1, value " +
                               boost::lexical_cast<std::string>(itsOptions.errorfactor) +
                               " was given");
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return true of true color mode is forced
 */
// ----------------------------------------------------------------------

bool ColorMapper::trueColor() const
{
  try
  {
    return itsOptions.truecolor;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the color map created by the last reduce call
 */
// ----------------------------------------------------------------------

const ColorMap &ColorMapper::colormap() const
{
  try
  {
    return itsColorMap;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the palette (reduced colors in use-count order) of the last reduce call
 */
// ----------------------------------------------------------------------

const std::vector<Color> &ColorMapper::palette() const
{
  try
  {
    return itsPalette;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate the occurrance count of each color in the given image
 *
 * \param image The image
 * \return The histogram object
 */
// ----------------------------------------------------------------------

Histogram ColorMapper::histogram(cairo_surface_t *image)
{
  try
  {
    if (image == nullptr)
      throw Fmi::Exception(BCP, "Cannot calculate colour histogram for a null pointer");

    ColorHistogram hist = calc_histogram(image);

    Histogram h;

    for (const auto &info : hist)
      h.insert(Histogram::value_type(info.count, info.color));

    return h;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Reduce colors from the image adaptively
 *
 * Algorithm if maxcolors has not been set:
 *
 *  -# Calculate the histogram
 *  -# In order of popularity, feed the colors into a color tree
 *     which discards colors which are close enough to a color
 *     chosen earlier.
 *  -# For each color, ask the color tree which is the nearest
 *     color for it.
 *  -# Replace the original colors with the colors found in the previous
 *     step.
 *
 * Note that this is very very close to the common popularity based
 * algorithm. However, the twist is that we have an adaptive error
 * criteria, we are not aiming for a fixed number of colors. Hence
 * we get the benefit of the speed of the popularity based algorithm
 * but will never get too big errors.
 *
 * An algorithm which might produce better quality results would
 * be to aim for color diversity as follows:
 *
 *  -# Perform the insertion phase as in algorithm 1, but
 *     with the error limit doubled.
 *  -# Perform the insertion phase as in algorithm 1, now
 *     with a normal error limit
 *  -# Continue by asking the nearest color for all colors
 *
 * However, we require best possible speed and thus do everything
 * in a single pass. What we could do is to find a way to quickly
 * reorder the histogram to produce more diversity in the colors.
 *
 *
 *
 * If the number of max colors has been set, a modified algorithm
 * is used:
 *
 * First, a desired number of maximum colours can be set. If the desired
 * number cannot be reached using the desired quality, more passes will
 * be made, and each time the desired quality will be reduced by the
 * given errorfactor.
 *
 * However, since the algorithm is designed to maintain reasonable quality
 * for palette images, all colours that occur in at least one 3x3 block
 * will be kept. If the desired limits are tight, eventually all other
 * colours will be converted to the colours that must be kept, and the
 * process stops.
 *
 * \param image The image to modify
 */
// ----------------------------------------------------------------------

void ColorMapper::reduce(cairo_surface_t *image)
{
  try
  {
    itsPalette.clear();

    // Skip histogram etc if true color is forced
    if (itsOptions.truecolor)
      return;

    // Calculate the histogram

    ColorHistogram hist = colorhistogram(image);

    // Abort if we should use RGBA:
    // - there are many different alpha values
    // - smallest alpha is below some limit

    const int max_unique_alphas = 100;
    const unsigned char max_min_alpha = 128;

    unsigned char min_alpha = 255;
    std::vector<int> alphas(256, 0);
    for (const auto &c : hist)
    {
      auto a = alpha(c.color);
      alphas[a] = 1;
      min_alpha = std::min(min_alpha, a);
    }
    int num_alphas = std::count(alphas.begin(), alphas.end(), 1);

    if (num_alphas > max_unique_alphas || (min_alpha < max_min_alpha))
    {
      if (hist.size() >= 256)
      {
        itsOptions.truecolor = true;
        return;
      }
      // Now we want palette mode but no color reductions

      itsOptions.truecolor = false;

      // Identify mapping
      itsColorMap.clear();
      itsColorMap.reserve(hist.size());
      for (const auto &c : hist)
        itsColorMap.insert(ColorMap::value_type(c.color, c.color));

      // hist.size() < 256 here, so the palette never exceeds the limit
      itsPalette = ordered_palette(hist, itsColorMap, 256);

      return;
    }

    // Select the colors and perform the replacements. Subsequent operations
    // will use the ColorMap to produce the palette.

    ColorTree tree;
    itsColorMap.clear();

    if (itsOptions.maxcolors <= 0)
      build_tree(image, hist, tree, itsColorMap, itsOptions.quality);
    else
      build_tree(image,
                 hist,
                 tree,
                 itsColorMap,
                 itsOptions.quality,
                 itsOptions.maxcolors,
                 itsOptions.errorfactor);

    // Order the palette only if it fits; otherwise the image is encoded in true
    // color and no palette is built.
    itsPalette = ordered_palette(hist, itsColorMap, 256);
    if (itsPalette.empty())
      itsOptions.truecolor = true;

    replace_colors(image, itsColorMap);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
