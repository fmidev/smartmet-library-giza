#include "ColorMapper.h"
#include "Palette.h"
#include <regression/tframe.h>
#include <iomanip>

using namespace std;

std::string hexcolor(Giza::Color color)
{
  std::ostringstream out;
  out << '#' << std::hex << std::setfill('0') << std::setw(8) << color;
  return out.str();
}

namespace Tests
{
// ----------------------------------------------------------------------

void palette()
{
  // We expect this to produce 135 colours
  std::string infile = "input/quantize1.png";
  auto* image = cairo_image_surface_create_from_png(infile.c_str());
  Giza::ColorMapper mapper;
  mapper.reduce(image);

  // Actual test begins

  const auto& colormap = mapper.colormap();
  Giza::Palette palette(colormap);

  if (palette.size() != 135) TEST_FAILED("Expected to built a palette of size 135");

#if 0
	for(std::size_t i=0; i<palette.size(); i++)
	  std::cout << i << " : " << hexcolor(palette.color(i)) << std::endl;
#endif

  if (palette.color(0) != 0xddabb386)
    TEST_FAILED("Color 0 should be #ddabb386, not " + hexcolor(palette.color(0)));

  TEST_PASSED();
}

// Test driver
class tests : public tframe::tests
{
  // Overridden message separator
  virtual const char* error_message_prefix() const { return "\n\t"; }
  // Main test suite
  void test() { TEST(palette); }
};  // class tests

}  // namespace Tests

int main(void)
{
  cout << endl << "Palette tester" << endl << "==============" << endl;
  Tests::tests t;
  return t.run();
}
