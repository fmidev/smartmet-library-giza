#include "Giza.h"
#include "ColorMapper.h"
#include <boost/foreach.hpp>
#include <cairo/cairo.h>
#include <png.h>
#include <set>
#include <map>
#include <stdexcept>
#include <vector>

namespace Giza
{
namespace
{
/* Unpremultiplies data and converts native endian ARGB => RGBA bytes */
static void unpremultiply_data(png_structp png, png_row_infop row_info, png_bytep data)
{
  unsigned int i;

  for (i = 0; i < row_info->rowbytes; i += 4)
  {
    uint8_t *b = &data[i];
    uint32_t pixel;
    uint8_t alpha;

    memcpy(&pixel, b, sizeof(uint32_t));
    alpha = (pixel & 0xff000000) >> 24;
    if (alpha == 0)
    {
      b[0] = b[1] = b[2] = b[3] = 0;
    }
    else
    {
      b[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
      b[1] = (((pixel & 0x00ff00) >> 8) * 255 + alpha / 2) / alpha;
      b[2] = (((pixel & 0x0000ff) >> 0) * 255 + alpha / 2) / alpha;
      b[3] = alpha;
    }
  }
}

// Cairo callback for writing image chunks

static void append_to_string(png_structp png, png_bytep data, png_size_t length)
{
  std::string *buffer = reinterpret_cast<std::string *>(png_get_io_ptr(png));
  buffer->append(reinterpret_cast<const char *>(data), length);
}

void giza_surface_write_to_png_string(cairo_surface_t *image,
                                      const ColorMap &colormap,
                                      std::string &buffer)
{
  // Do pending operations
  cairo_surface_flush(image);

  if (cairo_image_surface_get_format(image) != CAIRO_FORMAT_ARGB32)
    throw std::runtime_error("Giza::topng can write only Cairo ARGB32 format images");

  // Access image data directly.
  unsigned char *data = cairo_image_surface_get_data(image);

  if (data == NULL) throw std::runtime_error("Attempt to render an invalid Cairo image as PNG");

  // The cairo image data may have a stride width, meaning once you have
  // passed a certain width you may have to skip more bytes to reach the next
  // row. Hence the position of the next row is calculated using the stride,
  // and not the width.

  int width = cairo_image_surface_get_width(image);
  int height = cairo_image_surface_get_height(image);
  int stride = cairo_image_surface_get_stride(image);

  // Allocate png info variables

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png == NULL) throw std::runtime_error("Insufficient memory to allocate PNG write structure");

  png_infop info = png_create_info_struct(png);
  if (info == NULL)
  {
    png_destroy_write_struct(&png, NULL);
    throw std::runtime_error("Insufficient memory to allocate PNG info structure");
  }

  // Closure for writing the raw data generated by libpng to the actual output string.

  png_set_write_fn(png, &buffer, append_to_string, NULL);

  // Determine the unique colours in the colour conversion map, and
  // set true colour mode if there are more than 256 colours.

  std::set<Color> unique_colors;
  BOOST_FOREACH (const ColorMap::value_type &from_to, colormap)
  {
    unique_colors.insert(from_to.second);
  }

  bool truecolor = (unique_colors.size() > 256);

  // Generate the PNG data to the output

  if (truecolor)
  {
    // In ARGB mode we can handle all rows simultaneously.
    // Note that the pointers here are just pointers to
    // the cairo raw data, and hence no deletions afterwards
    // are necessary.

    std::vector<png_byte *> rows(height, NULL);
    for (int i = 0; i < height; i++)
      rows[i] = static_cast<png_byte *>(data + i * stride);

    png_set_IHDR(png,
                 info,
                 width,
                 height,
                 8,
                 PNG_COLOR_TYPE_RGB_ALPHA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);
    // Well, I have to admit I just copied this call from elsewhere.
    // No idea if and when it is needed.
    png_set_write_user_transform_fn(png, unpremultiply_data);
    png_write_image(png, rows.data());
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
  }

  else
  {
    // Now we have to write palette data. This means we must take
    // the original raw data, convert it into palette indices
    // and write it to output. First we'll collect the necessary
    // meta data, then we'll do the actual conversion to palette
    // indices one row at a time.

    // There are no more than 256 colours at this point. There is
    // little point in allocating true size arrays on the heap,
    // the sizes are so small. Hence fixed sizes.

    // transparencies of non-opaque colours and the RGB values
    png_byte transparent_values[256];
    png_color color_values[256];

    int num_transparent = 0;
    int num_colors = 0;

    // Collect the unique colours in the palette. Note that since the
    // alpha channel is the highest byte in cairo ARGB data, any transparent
    // colour will have a smaller alpha value than 255. Hence the colours
    // stored in a set will produce the transparent colours first. This is
    // good for us, since we want to minimize the size of the tRNS table
    // storing the transparency values for the colours in the palette
    // with transparency. In effect the following loop will increment
    // num_transparent one by one until the first opaque colour is
    // encountered.

    // This will store the colour map index for each colour
    std::map<Color, int> color_indices;

    BOOST_FOREACH (Color color, unique_colors)
    {
      // Store the index for this new unique colour
      color_indices.insert(std::make_pair(color, color_indices.size()));

      // And inform libpng of its properties
      color_values[num_colors].red = static_cast<png_byte>(red(color));
      color_values[num_colors].green = static_cast<png_byte>(green(color));
      color_values[num_colors].blue = static_cast<png_byte>(blue(color));

      // No need to skip storing here even if num_transparent increases no longer
      int a = alpha(color);
      transparent_values[num_colors] = a;

      ++num_colors;

      if (a < 255) num_transparent = num_colors;
    }

    // Setup the libpng storing method
    png_set_IHDR(png,
                 info,
                 width,
                 height,
                 8,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    // This may be occasionally useful if some errors are encountered but
    // you still want to see what the image would look like.
    // png_set_benign_errors(png,1);

    // Set transparencies, if there are any
    if (num_transparent > 0) png_set_tRNS(png, info, transparent_values, num_transparent, NULL);

    // Set the opaque RGB palette
    png_set_PLTE(png, info, color_values, num_colors);

    // Write PNG preamble
    png_write_info(png, info);

    // Write PNG raw data

    // Using this vector of palette indices to convert all rows in the ARGB cairo input
    std::vector<png_byte> rows(width, 0);

    for (int i = 0; i < height; i++)
    {
      // Convert cairo ARGB row to array of palette indices
      png_byte *row = static_cast<png_byte *>(data + i * stride);
      for (int j = 0; j < width; j++)
      {
        Color c = *reinterpret_cast<Color *>(row);
        rows[j] = color_indices.at(c);
        row += 4;
      }
      // And write the indices
      png_write_row(png, &rows[0]);
    }

    // Write PNG tail, deallocate memory
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
  }
}
}

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a PNG string
 */
// ----------------------------------------------------------------------

std::string topng(cairo_surface_t *image)
{
  ColorMapper mapper;
  mapper.reduce(image);

  std::string buffer;
  giza_surface_write_to_png_string(image, mapper.colormap(), buffer);
  return buffer;
}

}  // namespace Giza