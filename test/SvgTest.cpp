#include "Svg.h"
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <regression/tframe.h>
#include <fstream>

using namespace std;

std::string readfile(const std::string& filename)
{
  std::ifstream in(filename.c_str());
  if (!in) throw std::runtime_error("Failed to open '" + filename + "' for reading");
  std::stringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

void writefile(const std::string& filename, const std::string& data)
{
  std::ofstream out(filename.c_str());
  if (!out) throw std::runtime_error("Failed to open '" + filename + "' for writing");
  out << data;
}

std::size_t filehash(const std::string& filename) { return boost::hash_value(readfile(filename)); }
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
  if (filehash(testfile) != filehash(outfile))
    TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

  boost::filesystem::remove(testfile);

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
  if (filehash(testfile) != filehash(outfile))
    TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

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
    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg4.svg";
    std::string outfile = "output/png_svg4.png";
    std::string testfile = "failures/png_svg4.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

    boost::filesystem::remove(testfile);
  }

  {
    std::string infile = "input/svg5.svg";
    std::string outfile = "output/png_svg5.png";
    std::string testfile = "failures/png_svg5.png";

    std::string svg = readfile(infile);
    writefile(testfile, Giza::Svg::topng(svg));
    if (filehash(testfile) != filehash(outfile))
      TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

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
  writefile(testfile, Giza::Svg::topdf(svg));
  if (filehash(testfile) != filehash(outfile))
    TEST_FAILED("Hash for " + outfile + " and " + testfile + " differ");

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
  writefile(testfile, Giza::Svg::tops(svg));
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
    TEST(topng);
    // 2019-03-22: Disable topdf testing for now as result pdf is always different(perhaps integrated timestamp in binary?)
    // TEST(topdf);
    TEST(topng_transparency);
    TEST(topng_transparent_symbols);
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
