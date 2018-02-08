#include "ColorMapper.h"
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <regression/tframe.h>
#include <fstream>

using namespace std;

std::size_t filehash(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in) throw std::runtime_error(std::string("Failed to open '") + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return boost::hash_value(buffer.str());
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
  if (status != CAIRO_STATUS_SUCCESS)
    TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

  if (filehash(testfile) != filehash(outfile))
    TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

  boost::filesystem::remove(testfile);

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
    Giza::ColorMapper mapper;
    mapper.setQuality(20);
    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/quality50_quantize1.png";
    std::string testfile = "failures/quality50_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());
    Giza::ColorMapper mapper;
    mapper.setQuality(50);
    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
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
    Giza::ColorMapper mapper;
    mapper.setMaxColors(100);
    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/maxcolors60_quantize1.png";
    std::string testfile = "failures/maxcolors60_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());
    Giza::ColorMapper mapper;
    mapper.setMaxColors(60);
    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/quantize1.png";
    std::string outfile = "output/maxcolors30_quantize1.png";
    std::string testfile = "failures/maxcolors30_quantize1.png";

    auto* image = cairo_image_surface_create_from_png(infile.c_str());
    Giza::ColorMapper mapper;
    mapper.setMaxColors(30);
    mapper.reduce(image);
    auto status = cairo_surface_write_to_png(image, testfile.c_str());
    if (status != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
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
  if (status != CAIRO_STATUS_SUCCESS)
    TEST_FAILED("Failed to write " + testfile + " : " + cairo_status_to_string(status));

  if (filehash(testfile) != filehash(outfile))
    TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

  boost::filesystem::remove(testfile);

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
  cout << endl << "ColorMapper tester" << endl << "==================" << endl;
  Tests::tests t;
  return t.run();
}
