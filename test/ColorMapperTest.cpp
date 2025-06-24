#include "ColorMapper.h"
#include <filesystem>
#include <boost/functional/hash.hpp>
#include <fmt/format.h>
#include <regression/tframe.h>
#include <macgyver/StringConversion.h>
#include <fstream>
#include <sstream>
#include <Magick++.h>

using namespace std;

std::ostringstream comp_results;

std::size_t filehash(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    throw std::runtime_error(std::string("Failed to open '") + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return boost::hash_value(buffer.str());
}

std::string readfile(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    throw std::runtime_error("Failed to open '" + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

std::string convert_exe()
{
  if (access("/usr/bin/magick", X_OK) == 0)
    return "/usr/bin/magick";
  return "convert"; // Assuming 'convert' is in the PATH
}

double imagedifference(const std::string& file1, const std::string& file2)
{
  Magick::Image image1(file1);
  Magick::Image image2(file2);

  // Ensure both images are the same size
  if (image1.size() != image2.size()) {
      throw std::runtime_error("Images are not the same size: " + file1 + " and " + file2);
  }

  const double mse = image1.compare(image2, Magick::MetricType::MeanSquaredErrorMetric);
  const double psnr = (mse == 0)
    ? std::numeric_limits<double>::infinity()
    : 20 * log10(1.0 / sqrt(mse));

  return psnr;
}

bool checkimage(const std::string& output, const std::string& expected)
{
    if (filehash(output) == filehash(expected)) {
        return true;
    } else {
        double psnr = imagedifference(output, expected);

        std::string diff_fn = output.substr(0, output.length() - 4) + ".difference.png";
        auto cmd = fmt::format("/bin/sh -c '( composite {} {} -compose DIFFERENCE png:- |"
            "{} -quiet - -contrast-stretch 0 {} )'",
            expected, output, convert_exe(), diff_fn);
        system(cmd.c_str());
        comp_results << " PSNR(" << output << ", " << expected << ") = " << psnr << "\n";
        return psnr > 30;
    }
}

namespace Tests
{
// ----------------------------------------------------------------------

void defaults()
{
  std::string infile = "input/quantize1.png";
  std::string outfile = "output/defaults_quantize1.png";
  std::string testfile = "failures/defaults_quantize1.png";

  auto* image = cairo_image_surface_create_from_png(infile.c_str());
  Giza::ColorMapper mapper;
  mapper.reduce(image);
  auto status = cairo_surface_write_to_png(image, testfile.c_str());

  cairo_surface_destroy(image);

  if (status != CAIRO_STATUS_SUCCESS)
    TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

  if (!checkimage(testfile, outfile)) {
    TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  }

  std::filesystem::remove(testfile);

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void quality()
{
  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/quality20_quantize1.png";
    std::string testfile = "failures/quality20_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());

    Giza::ColorMapOptions options;
    options.quality = 20;

    Giza::ColorMapper mapper;
    mapper.options(options);
    mapper.reduce(image);

    auto status = cairo_surface_write_to_png(image, testfile.c_str());

    cairo_surface_destroy(image);

    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (!checkimage(testfile, outfile)) {
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    }

    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/quality50_quantize1.png";
    std::string testfile = "failures/quality50_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());

    Giza::ColorMapOptions options;
    options.quality = 50;

    Giza::ColorMapper mapper;
    mapper.options(options);

    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());

    cairo_surface_destroy(image);

    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (!checkimage(testfile, outfile))
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");

    std::filesystem::remove(testfile);
  }

  TEST_PASSED();
}

// ----------------------------------------------------------------------

void maxcolors()
{
  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/maxcolors100_quantize1.png";
    std::string testfile = "failures/maxcolors100_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());

    Giza::ColorMapOptions options;
    options.maxcolors = 100;

    Giza::ColorMapper mapper;
    mapper.options(options);

    mapper.reduce(image);

    auto status = cairo_surface_write_to_png(image, testfile.c_str());

    cairo_surface_destroy(image);

    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (!checkimage(testfile, outfile)) {
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    }

    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/maxcolors60_quantize1.png";
    std::string testfile = "failures/maxcolors60_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());

    Giza::ColorMapOptions options;
    options.maxcolors = 60;

    Giza::ColorMapper mapper;
    mapper.options(options);

    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());

    cairo_surface_destroy(image);

    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (!checkimage(testfile, outfile)) {
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    }

    std::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/maxcolors30_quantize1.png";
    std::string testfile = "failures/maxcolors30_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());

    Giza::ColorMapOptions options;
    options.maxcolors = 30;

    Giza::ColorMapper mapper;
    mapper.options(options);

    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    cairo_surface_destroy(image);

    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (!checkimage(testfile, outfile)) {
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
    }

    std::filesystem::remove(testfile);
  }

  TEST_PASSED();
}

void transparency()
{
  std::string infile = "input/quantize2.png";
  std::string outfile = "output/transparency_quantize2.png";
  std::string testfile = "failures/transparency_quantize2.png";

  auto* image = cairo_image_surface_create_from_png(infile.c_str());

  Giza::ColorMapper mapper;
  mapper.reduce(image);

  auto status = cairo_surface_write_to_png(image, testfile.c_str());

  cairo_surface_destroy(image);

  if (status != CAIRO_STATUS_SUCCESS)
    TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

  if (!checkimage(testfile, outfile)) {
      TEST_FAILED("Imaqes " + outfile + " and " + testfile + " difference is too large ");
  }

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
    TEST(defaults);
    TEST(quality);
    TEST(maxcolors);
    TEST(transparency);
  }

};  // class tests

}  // namespace Tests

int main(void)
{
  Magick::InitializeMagick(nullptr);
  cout << endl << "ColorMapper tester" << endl << "==================" << endl;
  Tests::tests t;
  auto result = t.run();
  if (comp_results.str() != "") {
      std::cout << std::endl << comp_results.str() << std::endl;
  }
  return result;
}
