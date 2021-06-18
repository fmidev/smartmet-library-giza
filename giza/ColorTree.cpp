#include "ColorTree.h"
#include <boost/move/make_unique.hpp>
#include <macgyver/Exception.h>
#include <array>
#include <cassert>
#include <cmath>

/*
 * double gamma = 2.2f;
 * double coeff = 255/(pow(255.,gamma));
 * for(i=0; i<256; i++)
 *    std::cout << coeff*pow(1.0*i,gamma) << ",\n";
 */

std::array<double, 256> gammacorrections = {0,
                                            0.0012946482346687,
                                            0.0059486411898552,
                                            0.014515050256609,
                                            0.027332777397017,
                                            0.044656614263272,
                                            0.066693657409865,
                                            0.093619748803065,
                                            0.12558846573382,
                                            0.16273662475259,
                                            0.20518791737583,
                                            0.25305544753895,
                                            0.30644357822179,
                                            0.36544932036633,
                                            0.43016340578127,
                                            0.5006711344161,
                                            0.57705305598014,
                                            0.65938552703971,
                                            0.74774117260428,
                                            0.84218927316093,
                                            0.94279609261953,
                                            1.0496251587871,
                                            1.1627375052441,
                                            1.2821918814982,
                                            1.4080449368111,
                                            1.5403513819867,
                                            1.6791641325583,
                                            1.8245344361661,
                                            1.9765119864035,
                                            2.1351450250129,
                                            2.3004804339931,
                                            2.4725638189228,
                                            2.6514395846016,
                                            2.8371510039377,
                                            3.0297402808775,
                                            3.2292486080534,
                                            3.4357162197364,
                                            3.6491824405954,
                                            3.8696857307044,
                                            4.0972637271762,
                                            4.3319532827588,
                                            4.5737905016873,
                                            4.822810773049,
                                            5.0790488018904,
                                            5.3425386382692,
                                            5.6133137044308,
                                            5.89140682027,
                                            6.1768502272212,
                                            6.4696756107073,
                                            6.7699141212606,
                                            7.0775963944221,
                                            7.3927525695125,
                                            7.7154123073591,
                                            8.0456048070577,
                                            8.3833588218379,
                                            8.7287026740959,
                                            9.0816642696543,
                                            9.4422711112999,
                                            9.8105503116496,
                                            10.186528605389,
                                            10.570232360923,
                                            10.961687591478,
                                            11.360919965687,
                                            11.767954817699,
                                            12.182817156824,
                                            12.605531676757,
                                            13.036122764406,
                                            13.474614508334,
                                            13.921030706849,
                                            14.375394875755,
                                            14.83773025579,
                                            15.30805981976,
                                            15.786406279391,
                                            16.272792091913,
                                            16.767239466384,
                                            17.269770369783,
                                            17.780406532863,
                                            18.299169455791,
                                            18.826080413585,
                                            19.361160461348,
                                            19.904430439316,
                                            20.455910977735,
                                            21.015622501557,
                                            21.583585234985,
                                            22.159819205853,
                                            22.744344249873,
                                            23.337180014724,
                                            23.938345964022,
                                            24.547861381152,
                                            25.165745372979,
                                            25.792016873448,
                                            26.42669464706,
                                            27.06979729225,
                                            27.721343244655,
                                            28.381350780288,
                                            29.049838018615,
                                            29.726822925536,
                                            30.412323316287,
                                            31.106356858253,
                                            31.808941073698,
                                            32.520093342423,
                                            33.239830904342,
                                            33.968170861996,
                                            34.705130182987,
                                            35.450725702349,
                                            36.204974124863,
                                            36.967892027294,
                                            37.739495860585,
                                            38.51980195198,
                                            39.308826507102,
                                            40.106585611969,
                                            40.913095234963,
                                            41.728371228749,
                                            42.552429332145,
                                            43.385285171945,
                                            44.226954264694,
                                            45.07745201843,
                                            45.93679373437,
                                            46.804994608562,
                                            47.682069733505,
                                            48.568034099714,
                                            49.462902597264,
                                            50.366690017287,
                                            51.279411053443,
                                            52.201080303353,
                                            53.131712269996,
                                            54.071321363083,
                                            55.019921900396,
                                            55.977528109091,
                                            56.944154126987,
                                            57.919814003813,
                                            58.904521702433,
                                            59.898291100049,
                                            60.901135989372,
                                            61.91307007977,
                                            62.934106998395,
                                            63.964260291282,
                                            65.003543424428,
                                            66.05196978485,
                                            67.109552681615,
                                            68.176305346862,
                                            69.252240936786,
                                            70.337372532618,
                                            71.431713141577,
                                            72.535275697806,
                                            73.648073063291,
                                            74.770118028756,
                                            75.901423314549,
                                            77.042001571507,
                                            78.191865381805,
                                            79.351027259786,
                                            80.519499652781,
                                            81.697294941912,
                                            82.884425442874,
                                            84.080903406716,
                                            85.286741020592,
                                            86.501950408508,
                                            87.726543632058,
                                            88.960532691134,
                                            90.203929524641,
                                            91.456746011181,
                                            92.718993969741,
                                            93.990685160359,
                                            95.271831284782,
                                            96.562443987109,
                                            97.862534854434,
                                            99.17211541746,
                                            100.49119715112,
                                            101.81979147518,
                                            103.15790975483,
                                            104.50556330126,
                                            105.86276337226,
                                            107.22952117273,
                                            108.60584785532,
                                            109.99175452089,
                                            111.38725221909,
                                            112.7923519489,
                                            114.2070646591,
                                            115.63140124886,
                                            117.06537256816,
                                            118.50898941834,
                                            119.96226255258,
                                            121.42520267635,
                                            122.89782044793,
                                            124.38012647884,
                                            125.87213133432,
                                            127.37384553377,
                                            128.8852795512,
                                            130.40644381564,
                                            131.93734871165,
                                            133.47800457965,
                                            135.02842171641,
                                            136.5886103754,
                                            138.15858076728,
                                            139.73834306023,
                                            141.32790738035,
                                            142.9272838121,
                                            144.53648239864,
                                            146.15551314222,
                                            147.78438600454,
                                            149.42311090716,
                                            151.07169773182,
                                            152.73015632079,
                                            154.39849647728,
                                            156.07672796573,
                                            157.76486051219,
                                            159.46290380462,
                                            161.17086749326,
                                            162.88876119096,
                                            164.61659447346,
                                            166.35437687976,
                                            168.10211791241,
                                            169.85982703784,
                                            171.62751368664,
                                            173.40518725388,
                                            175.19285709943,
                                            176.9905325482,
                                            178.79822289051,
                                            180.61593738229,
                                            182.44368524544,
                                            184.28147566806,
                                            186.12931780478,
                                            187.98722077695,
                                            189.85519367301,
                                            191.73324554868,
                                            193.62138542725,
                                            195.51962229985,
                                            197.4279651257,
                                            199.34642283236,
                                            201.27500431597,
                                            203.21371844153,
                                            205.16257404311,
                                            207.12157992411,
                                            209.0907448575,
                                            211.07007758603,
                                            213.05958682251,
                                            215.05928125,
                                            217.06916952205,
                                            219.08926026293,
                                            221.11956206783,
                                            223.16008350313,
                                            225.21083310655,
                                            227.27181938742,
                                            229.34305082687,
                                            231.42453587801,
                                            233.51628296621,
                                            235.61830048923,
                                            237.73059681746,
                                            239.85318029412,
                                            241.98605923544,
                                            244.12924193088,
                                            246.2827366433,
                                            248.44655160916,
                                            250.62069503872,
                                            252.8051751162,
                                            255};

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
    right = boost::movelib::make_unique<ColorTree>();
    left = boost::movelib::make_unique<ColorTree>();
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
      leftcolor = boost::movelib::make_unique<Color>(color);

    else if (rightcolor == nullptr)
      rightcolor = boost::movelib::make_unique<Color>(color);

    else
    {
      const double dist_left = distance(color, *leftcolor);
      const double dist_right = distance(color, *rightcolor);

      if (dist_left > dist_right)
      {
        if (right == nullptr)
          right = boost::movelib::make_unique<ColorTree>();

        // note that constructor sets maxright to be negative

        maxright = std::max(maxright, dist_right);

        right->insert(color);
        ++treesize;
      }
      else
      {
        if (left == nullptr)
          left = boost::movelib::make_unique<ColorTree>();

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
