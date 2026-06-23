#pragma once

#include "ColorTypes.h"
#include <memory>

// ----------------------------------------------------------------------
/*!
 * \brief A near-tree of colors and their occurrance counts
 *
 * The idea is to feed in colors in the order of their popularity
 * and then use the near-tree maximum distance information
 * to group colors for color reduction.
 */
// ----------------------------------------------------------------------

namespace Giza
{
// Gamma-corrected color components, precomputed once per stored color so that
// distance evaluations in the hot nearest()/insert() paths avoid repeated
// gamma table lookups.
struct GammaColor
{
  double r = 0;
  double g = 0;
  double b = 0;
  double a = 0;
};

class ColorTree
{
 public:
  ColorTree() = default;
  ColorTree(const ColorTree& other) = delete;
  ColorTree& operator=(const ColorTree& other) = delete;
  ColorTree(ColorTree&& other) = delete;
  ColorTree& operator=(ColorTree&& other) = delete;

  void insert(Color color);
  int size() const;
  void clear();
  bool empty() const;
  Color nearest(Color color);
  // Also returns the perceptual distance to the nearest color, so callers need
  // not recompute it.
  Color nearest(Color color, double& distance);

  static double distance(Color color1, Color color2);

 private:
  static GammaColor gamma(Color color);
  static double distance(const GammaColor& color1, const GammaColor& color2);

  void insert(Color color, const GammaColor& colorgamma);
  bool nearest(const GammaColor& color, Color& nearest, double& radius) const;

  double maxleft = -1.0;
  double maxright = -1.0;
  int treesize = 0;
  bool has_leftcolor = false;
  bool has_rightcolor = false;
  Color leftcolor = 0;
  Color rightcolor = 0;
  GammaColor leftgamma;
  GammaColor rightgamma;
  std::unique_ptr<ColorTree> left;
  std::unique_ptr<ColorTree> right;
};
}  // namespace Giza
