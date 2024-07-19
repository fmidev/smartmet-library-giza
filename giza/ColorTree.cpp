#include "ColorTree.h"
#include <macgyver/Exception.h>
#include <array>
#include <cassert>
#include <cmath>
#include <gcem/gcem.hpp>

namespace
{
   constexpr std::array<double, 256>
   GammaCorrections()
  {
    std::array<double, 256> result = {0};
    double gamma = 2.2f;
    double coeff = 255/(gcem::pow(255.,gamma));
    for(int i=0; i<256; i++)
    {
      result[i] = coeff*gcem::pow(1.0*i,gamma);
    }
    return result;
  }

  constexpr std::array<double, 256> gammacorrections = GammaCorrections();

} // anonymous namespace
namespace Giza
{
// ----------------------------------------------------------------------
/*!
 * \brief Return the gamma correction
 */
// ----------------------------------------------------------------------

inline double gammacorr(int color)
{
  assert(color >= 0 && color < 256);
  return gammacorrections[color];
}

// ----------------------------------------------------------------------
/*!
 * \brief Euclidian distance between two colors
 *
 * See http://www.compuphase.com/cmetric.htm
 */
// ----------------------------------------------------------------------

#if 0
inline static double colordiff(double x, double y, double a)
{
  double b = x - y;
  double w = b + a;
  return b * b + w * w;
}
#endif

double ColorTree::distance(Color color1, Color color2)
{
#if 1
  const double r = gammacorr(red(color1)) - gammacorr(red(color2));
  const double g = gammacorr(green(color1)) - gammacorr(green(color2));
  const double b = gammacorr(blue(color1)) - gammacorr(blue(color2));
  const double a = alpha(color1) - alpha(color2);
  return sqrt(3 * r * r + 4 * g * g + 2 * b * b + a * a);
#endif

#if 0
  double a = alpha(color1) - alpha(color2);
  return (colordiff(red(color1), red(color2), a) + colordiff(green(color1), green(color2), a) +
          colordiff(blue(color1), blue(color2), a));
#endif

#if 0
  int rmean = (red(color1) + red(color2)) / 2;
  int r = red(color1) - red(color2);
  int g = green(color1) - green(color2);
  int b = blue(color1) - blue(color2);
  int a = alpha(color1) - alpha(color2);
  return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8) +
              4 * a * a);
#endif
}

// ----------------------------------------------------------------------
/*!
 * \brief Test whether the tree is empty
 */
// ----------------------------------------------------------------------

bool ColorTree::empty() const
{
  return (leftcolor == nullptr);
}
// ----------------------------------------------------------------------
/*!
 * \brief Return number of colours in the tree.
 */
// ----------------------------------------------------------------------

int ColorTree::size() const
{
  return treesize;
}
// ----------------------------------------------------------------------
/*!
 * \brief Clear the tree of all colours
 */
// ----------------------------------------------------------------------

void ColorTree::clear()
{
  try
  {
    right = std::make_unique<ColorTree>();
    left = std::make_unique<ColorTree>();
    treesize = 0;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Insert a color into the color tree
 *
 * Algorithm:
 *
 *  -# If left position is free, put the color there
 *  -# If right position is free, put the color there
 *  -# Insert into subtree whose root is closer to the color
 */
// ----------------------------------------------------------------------

void ColorTree::insert(Color color)
{
  try
  {
    if (leftcolor == nullptr)
      leftcolor = std::make_unique<Color>(color);

    else if (rightcolor == nullptr)
      rightcolor = std::make_unique<Color>(color);

    else
    {
      const double dist_left = distance(color, *leftcolor);
      const double dist_right = distance(color, *rightcolor);

      if (dist_left > dist_right)
      {
        if (right == nullptr)
          right = std::make_unique<ColorTree>();

        // note that constructor sets maxright to be negative

        maxright = std::max(maxright, dist_right);

        right->insert(color);
        ++treesize;
      }
      else
      {
        if (left == nullptr)
          left = std::make_unique<ColorTree>();

        // note that constructor sets maxleft to be negative

        maxleft = std::max(maxleft, dist_left);

        left->insert(color);
        ++treesize;
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
 * \brief Find the nearest color
 *
 * \param color The color for which to find the nearest color
 * \return The nearest color
 */
// ----------------------------------------------------------------------

Color ColorTree::nearest(Color color)
{
  try
  {
    Color bestcolor;
    double radius = -1;
    if (!nearest(color, bestcolor, radius))
      throw Fmi::Exception(BCP, "Invalid use of color reduction tables: no match was found");
    return bestcolor;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Recursively find nearest color
 *
 * This is only intended to be used by the public nearest method
 */
// ----------------------------------------------------------------------

bool ColorTree::nearest(Color color, Color& nearest, double& radius) const
{
  try
  {
    float left_dist = -1;
    float right_dist = -1;
    bool found = false;

    // first test each of the left and right positions to see if
    // one holds a color nearer than the nearest so far discovered

    if (leftcolor != nullptr)
    {
      left_dist = distance(color, *leftcolor);
      if (radius < 0 || left_dist <= radius)
      {
        radius = left_dist;
        nearest = *leftcolor;
        found = true;
      }
    }

    if (rightcolor != nullptr)
    {
      right_dist = distance(color, *rightcolor);
      if (radius < 0 || right_dist <= radius)
      {
        radius = right_dist;
        nearest = *rightcolor;
        found = true;
      }
    }

    // if radius is negative at this point, the tree is empty
    // on the other hand, if the radius is zero, we found a match

    if (radius <= 0)
      return found;

    // Now we test to see if the branches below might hold an object
    // nearer than the best so far found. The triangle rule is used
    // to test whether it's even necessary to descend. We may be
    // able to skip skanning both branches if we can guess which
    // branch is most likely to contain the nearest match. We simply
    // guess, that it is the branch which is nearer. Note that
    // the first and third if-clauses are mutually exclusive,
    // hence only parts 1-2 or 2-3 will be executed.

    const bool left_closer = (left_dist < right_dist);

    if (!left_closer && (right != nullptr) && ((radius + maxright) >= right_dist))
    {
      found |= right->nearest(color, nearest, radius);
    }

    if ((left != nullptr) && ((radius + maxleft) >= left_dist))
    {
      found |= left->nearest(color, nearest, radius);
    }

    if (left_closer && (right != nullptr) && ((radius + maxright) >= right_dist))
    {
      found |= right->nearest(color, nearest, radius);
    }

    return found;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
