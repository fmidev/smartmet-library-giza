#pragma once

#include "ColorTypes.h"
#include <boost/shared_ptr.hpp>

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
class ColorTree
{
 public:
  ColorTree();
  ColorTree(const ColorTree& other) = delete;
  ColorTree& operator=(const ColorTree& other) = delete;

  void insert(Color color);
  int size() const;
  void clear();
  bool empty() const;
  Color nearest(Color color);

  static double distance(Color color1, Color color2);

 private:
  bool nearest(Color color, Color& nearest, double& radius) const;

  double maxleft;
  double maxright;
  int treesize;
  std::shared_ptr<Color> leftcolor;
  std::shared_ptr<Color> rightcolor;
  std::shared_ptr<ColorTree> left;
  std::shared_ptr<ColorTree> right;
};
}
