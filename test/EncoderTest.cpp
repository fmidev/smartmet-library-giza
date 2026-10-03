// Tests for the PNG encoding of ARGB pixels and the animated WebP encoding

#include "ColorMapOptions.h"
#include "Giza.h"
#include "WebpOptions.h"
#include <regression/tframe.h>
#include <cairo.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

namespace Tests
{
// cairo reads PNG data from a stream callback
struct Reader
{
  const std::string& data;
  std::size_t pos = 0;
};

cairo_status_t read_png(void* closure, unsigned char* buffer, unsigned int length)
{
  auto* reader = static_cast<Reader*>(closure);
  if (reader->pos + length > reader->data.size())
    return CAIRO_STATUS_READ_ERROR;
  std::memcpy(buffer, reader->data.data() + reader->pos, length);
  reader->pos += length;
  return CAIRO_STATUS_SUCCESS;
}

// ----------------------------------------------------------------------

void png_argb()
{
  // 3x2 image with opaque red, green, blue and a half transparent white
  const int w = 3;
  const int h = 2;
  std::vector<std::uint32_t> pixels{
      0xffff0000, 0xff00ff00, 0xff0000ff, 0x80ffffff, 0x00000000, 0xff123456};

  for (int level : {0, 1, 9})
  {
    std::string png = Giza::topng_argb(pixels.data(), w, h, level);
    if (png.size() < 8 || png.compare(1, 3, "PNG") != 0)
      TEST_FAILED("Output is not a PNG at level " + std::to_string(level));

    // Decode and compare the opaque pixels. Cairo uses premultiplied ARGB.
    Reader reader{png};
    cairo_surface_t* image = cairo_image_surface_create_from_png_stream(read_png, &reader);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
      TEST_FAILED("Failed to decode the PNG at level " + std::to_string(level));
    if (cairo_image_surface_get_width(image) != w || cairo_image_surface_get_height(image) != h)
      TEST_FAILED("Decoded image has the wrong size");

    cairo_surface_flush(image);
    const auto* data = cairo_image_surface_get_data(image);
    const int stride = cairo_image_surface_get_stride(image);
    auto pixel = [&](int i, int j)
    { return *reinterpret_cast<const std::uint32_t*>(data + j * stride + 4 * i); };

    if (pixel(0, 0) != 0xffff0000 || pixel(1, 0) != 0xff00ff00 || pixel(2, 0) != 0xff0000ff ||
        pixel(2, 1) != 0xff123456)
      TEST_FAILED("Opaque pixels changed in PNG encoding at level " + std::to_string(level));
    if ((pixel(0, 1) >> 24) != 0x80)
      TEST_FAILED("Alpha changed in PNG encoding at level " + std::to_string(level));
    if (pixel(1, 1) != 0)
      TEST_FAILED("Transparent pixel changed in PNG encoding at level " + std::to_string(level));

    cairo_surface_destroy(image);
  }
  TEST_PASSED();
}

// ----------------------------------------------------------------------

void webp_animation()
{
  std::vector<cairo_surface_t*> frames;
  for (int i = 0; i < 3; i++)
  {
    auto* frame = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16, 16);
    auto* cr = cairo_create(frame);
    cairo_set_source_rgb(cr, i / 2.0, 0, 1 - i / 2.0);
    cairo_paint(cr);
    cairo_destroy(cr);
    frames.push_back(frame);
  }

  Giza::ColorMapOptions options;
  Giza::WebpOptions webpOptions;
  std::string webp = Giza::towebpanim(frames, {100, 200, 300}, 0, options, webpOptions);

  for (auto* frame : frames)
    cairo_surface_destroy(frame);

  // RIFF container with the WEBP signature, an animation header and three frames
  if (webp.size() < 12 || webp.compare(0, 4, "RIFF") != 0 || webp.compare(8, 4, "WEBP") != 0)
    TEST_FAILED("Output is not a WebP file");
  if (webp.find("ANIM") == std::string::npos)
    TEST_FAILED("Animated WebP should contain an ANIM chunk");
  std::size_t count = 0;
  for (auto pos = webp.find("ANMF"); pos != std::string::npos; pos = webp.find("ANMF", pos + 4))
    ++count;
  if (count != 3)
    TEST_FAILED("Expected 3 animation frames, got " + std::to_string(count));

  TEST_PASSED();
}

// ----------------------------------------------------------------------

class tests : public tframe::tests
{
  const char* error_message_prefix() const override { return "\n\t"; }
  void test() override
  {
    TEST(png_argb);
    TEST(webp_animation);
  }
};

}  // namespace Tests

int main()
{
  cout << endl << "Encoder tester" << endl << "==============" << endl;
  Tests::tests t;
  return t.run();
}
