#include "Svg.h"
#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <macgyver/StringConversion.h>
#include <regression/tframe.h>
#include <fstream>

using namespace std;

const double error_limit = 40.0;

std::string readfile(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in)
    throw std::runtime_error("Failed to open '" + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

void writefile(const std::string& filename, const std::string& data)
{
  std::ofstream out(filename.c_str());
  if (!out)
    throw std::runtime_error("Failed to open '" + filename + "' for writing");
  out << data;
}

double imagedifference(const std::string& file1, const std::string& file2)
{
  std::string tmpfile = "svgdiff.txt";
  auto cmd = fmt::format("compare -metric PSNR {} {} /dev/null > {} 2>&1", file1, file2, tmpfile);
  system(cmd.c_str());
  auto ret = readfile(tmpfile);
  // Newer compare appends (<another value>). Remove it if found
  auto pos = ret.find_first_of(" \t\r\n(");
  if (pos != std::string::npos) {
      ret = ret.substr(0, pos);
  }

  boost::filesystem::remove(tmpfile);
  if (ret == "inf")
    return 0;
  return Fmi::stod(ret);
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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
  boost::filesystem::remove(testfile);

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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
  boost::filesystem::remove(testfile);

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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
  boost::filesystem::remove(testfile);

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
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg4.svg";
    std::string outfile = "output/webp_svg4.webp";
    std::string testfile = "failures/webp_svg4.webp";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::towebp(svg));
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg5.svg";
    std::string outfile = "output/webp_svg5.webp";
    std::string testfile = "failures/webp_svg5.webp";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::towebp(svg));
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);
  }

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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
  boost::filesystem::remove(testfile);

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
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg4.svg";
    std::string outfile = "output/png_svg4.png";
    std::string testfile = "failures/png_svg4.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg5.svg";
    std::string outfile = "output/png_svg5.png";
    std::string testfile = "failures/png_svg5.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    auto diff = imagedifference(testfile, outfile);
    if (diff > error_limit)
      TEST_FAILED("Difference = " + Fmi::to_string(diff));
    boost::filesystem::remove(testfile);
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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
  boost::filesystem::remove(testfile);

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
  auto diff = imagedifference(testfile, outfile);
  if (diff > error_limit)
    TEST_FAILED("Difference = " + Fmi::to_string(diff));
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
    TEST(topng);
    // 2019-03-22: Disable topdf testing for now as result pdf is always different(perhaps
    // integrated timestamp in binary?) TEST(topdf);
    TEST(topng_transparency);
    TEST(topng_transparent_symbols);

    TEST(towebp);
    TEST(towebp_transparency);
    TEST(towebp_transparent_symbols);

    // TEST(tops);	// CreationDate changes every time!
  }

};  // class tests

}  // namespace Tests

int main(void)
{
  cout << endl << "Svg tester" << endl << "==========" << endl;
  Tests::tests t;
  return t.run();
}
