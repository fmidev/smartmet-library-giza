#include "ColorMapOptions.h"
#include "Svg.h"
#include "WebpOptions.h"
#include <boost/functional/hash.hpp>
#include <fmt/format.h>
#include <macgyver/StringConversion.h>
#include <regression/tframe.h>
#include <Magick++.h>
#include <filesystem>
#include <fstream>

using namespace std;

const double error_limit = 40.0;

std::ostringstream comp_results;

std::string readfile(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    throw std::runtime_error("Failed to open '" + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::size_t filehash(const std::string& filename)
{
  return boost::hash_value(readfile(filename));
}

void writefile(const std::string& filename, const std::string& data)
{
  std::ofstream out(filename.c_str());
  if (!out)
    throw std::runtime_error("Failed to open '" + filename + "' for writing");
  out << data;
}

std::string convert_exe()
{
  if (access("/usr/bin/magick", X_OK) == 0)
    return "/usr/bin/magick";
  return "convert";  // Assuming 'convert' is in the PATH
}

double imagedifference(const std::string& file1, const std::string& file2)
{
  Magick::Image image1(file1);
  Magick::Image image2(file2);

  // Ensure both images are the same size
  if (image1.size() != image2.size())
  {
    throw std::runtime_error("Images are not the same size: " + file1 + " and " + file2);
  }

  const double mse = image1.compare(image2, Magick::MetricType::MeanSquaredErrorMetric);
  const double psnr =
      (mse == 0) ? std::numeric_limits<double>::infinity() : 20 * log10(1.0 / sqrt(mse));

  return psnr;
}

bool checkimage(const std::string& output,
                const std::string& expected,
                double error_limit_ = error_limit)
{
  if (filehash(output) == filehash(expected))
  {
    return true;
  }
  else
  {
    double psnr = imagedifference(output, expected);
    if (std::isinf(psnr))
    {
      return true;
    }

    std::string diff_fn = output.substr(0, output.length() - 4) + ".difference.png";
    auto cmd = fmt::format(
        "/bin/sh -c '( composite {} {} -compose DIFFERENCE png:- |"
        "{} -quiet - -contrast-stretch 0 {} )'",
        expected,
        output,
        convert_exe(),
        diff_fn);
    system(cmd.c_str());
    comp_results << " PSNR(" << output << ", " << expected << ") = " << psnr << "\n";
    return psnr > error_limit_;
  }
}

namespace Tests
{
// ----------------------------------------------------------------------

void topng()
{
  std::string infile = "input/svg1.svg";
  std::string outfile = "output/png_svg1.png";
  std::string testfile = "failures/png_svg1.png";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::topng(svg));
  if (!checkimage(testfile, outfile, 25.0))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void towebp()
{
  std::string infile = "input/svg1.svg";
  std::string outfile = "output/webp_svg1.webp";
  std::string testfile = "failures/webp_svg1.webp";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::towebp(svg));
  if (!checkimage(testfile, outfile, 25.0))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void towebp_transparency()
{
  std::string infile = "input/svg2.svg";
  std::string outfile = "output/webp_svg2.webp";
  std::string testfile = "failures/webp_svg2.webp";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::towebp(svg));
  if (!checkimage(testfile, outfile))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void towebp_transparent_symbols()
{
  {
    std::string infile = "input/svg3.svg";
    std::string outfile = "output/webp_svg3.webp";
    std::string testfile = "failures/webp_svg3.webp";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::towebp(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);

    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg4.svg";
    std::string outfile = "output/webp_svg4.webp";
    std::string testfile = "failures/webp_svg4.webp";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::towebp(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg5.svg";
    std::string outfile = "output/webp_svg5.webp";
    std::string testfile = "failures/webp_svg5.webp";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::towebp(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void towebp_compression_level()
{
  // The lossless preset level must affect the encoded size while leaving the
  // decoded image unchanged (WebP lossless). svg1 has enough detail that the
  // fastest and slowest presets produce differently sized output.

  std::string infile = "input/svg1.svg";
  std::string svg = readfile(infile);

  Giza::ColorMapOptions colors;

  Giza::WebpOptions fast;
  fast.level = 0;  // fastest, largest
  Giza::WebpOptions best;
  best.level = 9;  // slowest, smallest

  std::string fast_data = Giza::Svg::towebp(svg, colors, fast);
  std::string best_data = Giza::Svg::towebp(svg, colors, best);

  if (fast_data.empty() || best_data.empty())
    TEST_FAILED("WebP encoding produced empty output");

  if (best_data.size() >= fast_data.size())
    TEST_FAILED(fmt::format("Expected level 9 ({} bytes) to be smaller than level 0 ({} bytes)",
                            best_data.size(),
                            fast_data.size()));

  // Both must decode to the same image as the unparameterised reference output.
  std::string reffile = "output/webp_svg1.webp";

  std::string fast_fn = "failures/webp_svg1_level0.webp";
  writefile(fast_fn, fast_data);
  if (!checkimage(fast_fn, reffile, 25.0))
    TEST_FAILED("Level 0 WebP differs too much from reference " + reffile);
  std::filesystem::remove(fast_fn);

  std::string best_fn = "failures/webp_svg1_level9.webp";
  writefile(best_fn, best_data);
  if (!checkimage(best_fn, reffile, 25.0))
    TEST_FAILED("Level 9 WebP differs too much from reference " + reffile);
  std::filesystem::remove(best_fn);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void topng_transparency()
{
  std::string infile = "input/svg2.svg";
  std::string outfile = "output/png_svg2.png";
  std::string testfile = "failures/png_svg2.png";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::topng(svg));
  if (!checkimage(testfile, outfile))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void topng_transparent_symbols()
{
  {
    std::string infile = "input/svg3.svg";
    std::string outfile = "output/png_svg3.png";
    std::string testfile = "failures/png_svg3.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);

    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg4.svg";
    std::string outfile = "output/png_svg4.png";
    std::string testfile = "failures/png_svg4.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg5.svg";
    std::string outfile = "output/png_svg5.png";
    std::string testfile = "failures/png_svg5.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    std::filesystem::remove(testfile);
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void topdf()
{
  std::string infile = "input/svg1.svg";
  std::string outfile = "output/pdf_svg1.pdf";
  std::string testfile = "failures/pdf_svg1.pdf";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::topng(svg));
  if (!checkimage(testfile, outfile))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void tops()
{
  std::string infile = "input/svg1.svg";
  std::string outfile = "output/ps_svg1.ps";
  std::string testfile = "failures/ps_svg1.ps";

  std::string svg = readfile(infile);
  writefile(testfile, Giza::Svg::topng(svg));
  if (!checkimage(testfile, outfile))
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// Test driver
class tests : public tframe::tests
{
  // Overridden message separator
  virtual const char* error_message_prefix() const { return "\n\t"; }
  // Main test suite
  void test()
  {
    TEST(topng);
    // 2019-03-22: Disable topdf testing for now as result pdf is always different(perhaps
    // integrated timestamp in binary?) TEST(topdf);
    TEST(topng_transparency);
    TEST(topng_transparent_symbols);

    TEST(towebp);
    TEST(towebp_transparency);
    TEST(towebp_transparent_symbols);
    TEST(towebp_compression_level);

    // TEST(tops);	// CreationDate changes every time!
  }

};  // class tests

}  // namespace Tests

int main(void)
{
  cout << endl << "Svg tester" << endl << "==========" << endl;
  Tests::tests t;
  const auto retval = t.run();
  if (comp_results.str() != "")
  {
    std::cout << std::endl << comp_results.str() << std::endl;
  }
  return retval;
}
