#include "ColorMapper.h"
#include "ColorTree.h"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <cmath>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#pragma message("Remove")
#include <iostream>

namespace Giza
{
// ----------------------------------------------------------------------
/*
 * Helper classes
 */
// ----------------------------------------------------------------------

// Histogram data
struct ColorInfo
{
  ColorInfo(Color c) : color(c), count(0), keeper(false) {}
  Color color;
  Count count;
  bool keeper;

  ColorInfo &operator++()
  {
    ++count;
    return *this;
  }
  void keep() { keeper = true; }
};

// Histogram information
using Counter = std::unordered_map<Color, ColorInfo>;

// Internal histogram
using ColorHistogram = std::vector<ColorInfo>;

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

Counter calc_histogram(cairo_surface_t *image)
{
  // Verify data is ok to process

  if (!image) throw std::runtime_error("Cannot calculate colour histogram for a null pointer");

  auto format = cairo_image_surface_get_format(image);
  if (format != CAIRO_FORMAT_ARGB32)
    throw std::runtime_error("Color quantization is implemented only for ARGB32 images");

  // Shorthand variables

  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);
  int stride = cairo_image_surface_get_stride(image);  // bytes to next row
  unsigned char *data = cairo_image_surface_get_data(image);

  // We assume there are going to be lots of colours, se we use
  // a sensible starting bucket size

  Counter counter(4096);

  // Not sure if this is even valid in Cairo, check anyway

  if (width * height == 0) return counter;

  // Insert the first color so that we can initialize the iterator cache
  // Note that we insert count 0, but the first loop will fix the number

  Color color0 = get_color(data, 0, 0, stride);
  counter.insert(Counter::value_type(color0, ColorInfo(color0)));

  Counter::iterator last1 = counter.begin();
  Counter::iterator last2 = counter.begin();

  for (int j = 0; j < height; j++)
    for (int i = 0; i < width; i++)
    {
      Color color = get_color(data, i, j, stride);

      if (last1->first == color)
      {
        ++last1->second;
        // test if the color is the same in a 3x3 box and is hence a new color to be kept
        if (!last1->second.keeper && i > 1 && j > 1)
        {
          // Note: image(i-1,j) is already known to have the same color (last1 points to it)
          if (get_color(data, i - 2, j, stride) == color &&
              get_color(data, i, j - 1, stride) == color &&
              get_color(data, i - 1, j - 1, stride) == color &&
              get_color(data, i - 2, j - 1, stride) == color &&
              get_color(data, i, j - 2, stride) == color &&
              get_color(data, i - 1, j - 2, stride) == color &&
              get_color(data, i - 2, j - 2, stride) == color)
          {
            last1->second.keep();
          }
        }
      }
      else if (last2->first == color)
      {
        ++last2->second;
        swap(last1, last2);
      }
      else
      {
        auto result = counter.insert(Counter::value_type(color, ColorInfo(color)));
        last2 = last1;
        last1 = result.first;
        ++last1->second;
      }
    }

  return counter;
}

// ----------------------------------------------------------------------
/*!
 * \brief Perform color replacement
 */
// ----------------------------------------------------------------------

void replace_colors(cairo_surface_t *image, const Giza::ColorMap &inputmap)
{
  // Verify data is ok to process

  if (!image) throw std::runtime_error("Cannot calculate colour histogram for a null pointer");

  auto format = cairo_image_surface_get_format(image);
  if (format != CAIRO_FORMAT_ARGB32)
    throw std::runtime_error("Color replacement is implemented only for ARGB32 images");

  // Shorthand variables

  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);
  int stride = cairo_image_surface_get_stride(image);  // bytes to next row
  unsigned char *data = cairo_image_surface_get_data(image);

  // Speed of the conversion by using an unordered map

  std::unordered_map<Color, Color> colormap(256);
  for (const auto &value : inputmap)
    colormap[value.first] = value.second;

  // Remember last color conversions for extra speed

  Color last_color1 = 0;
  Color last_choice1 = 0;
  Color last_color2 = 0;
  Color last_choice2 = 0;

  for (int j = 0; j < height; j++)
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
        last_choice1 = colormap[color];
        set_color(data, i, j, stride, last_choice1);
      }
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
  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);

  const double ratio = 1.0 / (width * height);
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
      Color nearest = colortree.nearest(info.color);
      double dist = colortree.distance(nearest, info.color);

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
  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);

  const double ratio = 1.0 / (width * height);

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
        Color nearest = colortree.nearest(info.color);
        double dist = colortree.distance(nearest, info.color);

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

}  // namespace anonymous

bool colorcmp(const ColorInfo &info1, const ColorInfo &info2)
{
  if (info1.keeper == info2.keeper) return (info2.count < info1.count);
  return info1.keeper;
}

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
  Counter counter = calc_histogram(image);

  ColorHistogram histogram;

  for (const auto &count : counter)
    histogram.push_back(count.second);

  std::sort(histogram.begin(), histogram.end(), &colorcmp);

  return histogram;
}

// ======================================================================
// The actual public interfaces
// ======================================================================

ColorMapper::ColorMapper()
    : itsQuality(10.0)  // 10 = good, 20 = poor etc
      ,
      itsErrorFactor(2.0)  // good default if max colors is set
      ,
      itsMaxColors(0)  // <=0 implies adaptive reduction
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Return true if truecolor is to be used
 */
// ----------------------------------------------------------------------

bool ColorMapper::truecolor() const { return itsTrueColor; }

// ----------------------------------------------------------------------
/*!
 * \brief Set the desired quality. Guideline: 10 = good, 20 = poor
 */
// ----------------------------------------------------------------------

void ColorMapper::setQuality(double quality)
{
  if (quality < 1)
    throw std::runtime_error("Requested relative color map quality must be at least 1, value " +
                             boost::lexical_cast<std::string>(quality) + " was given");
  itsQuality = quality;
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the desired number of maximum colors.
 *
 * The setting may not be obeyed if it would require removing
 * solid 3x3 colour blocks. A value <= 0 implies no maximum.
 */
// ----------------------------------------------------------------------

void ColorMapper::setMaxColors(int maxcolors) { itsMaxColors = maxcolors; }
// ----------------------------------------------------------------------
/*!
 * \brief Set the amount of quality reduction per iteration
 *
 * The default value 2 means the requested quality is halved
 * during each iteration if the requested number of max colors
 * cannot be reached. The value must be at least 1.1 to obtain
 * sensible convergence rates.
 */
// ----------------------------------------------------------------------

void ColorMapper::setErrorFactor(double errorfactor)
{
  if (errorfactor < 1.1)
    throw std::runtime_error("Image color quantization error factor must be at least 1.1, value " +
                             boost::lexical_cast<std::string>(errorfactor) + " was given");
  itsErrorFactor = errorfactor;
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the color map created by the last reduce call
 */
// ----------------------------------------------------------------------

const ColorMap &ColorMapper::colormap() const { return itsColorMap; }

// ----------------------------------------------------------------------
/*!
 * \brief Calculate the occurrance count of each color in the given image
 *
 * \param image The image
 * \return The histogram object
 */
// ----------------------------------------------------------------------

Histogram ColorMapper::histogram(cairo_surface_t *image) const
{
  if (!image) throw std::runtime_error("Cannot calculate colour histogram for a null pointer");

  Counter counter = calc_histogram(image);

  Histogram h;

  for (auto &value : counter)
    h.insert(Histogram::value_type(value.second.count, value.first));

  return h;
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
  // Calculate the histogram

  ColorHistogram hist = colorhistogram(image);

  // Abort if we should use RGBA:
  // - there are many unique colours
  // - there are many different alpha values
  // - smallest alpha is below some limit

  if (hist.size() >= 256)
  {
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
      itsTrueColor = true;
      return;
    }
  }

  // Select the colors

  ColorTree tree;
  itsColorMap.clear();

  if (itsMaxColors <= 0)
    build_tree(image, hist, tree, itsColorMap, itsQuality);
  else
    build_tree(image, hist, tree, itsColorMap, itsQuality, itsMaxColors, itsErrorFactor);

  replace_colors(image, itsColorMap);
}

}  // namespace Giza
