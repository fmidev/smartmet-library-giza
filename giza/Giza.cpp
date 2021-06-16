#include "Giza.h"
#include "ColorMapper.h"
#include <macgyver/Exception.h>
#include <cairo/cairo.h>
#include <cstring>
#include <map>
#include <png.h>
#include <set>
#include <vector>

namespace Giza
{
namespace
{
/* Unpremultiply a single colour component */

png_byte unpremultiply_color_component(png_byte component, png_byte alpha)
{
  try
  {
    if (alpha == 0)
      return 0;

    return (component * 255 + alpha / 2) / alpha;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

/* Unpremultiplies data and converts native endian ARGB => RGBA bytes */
void unpremultiply_data(png_structp /* png */, png_row_infop row_info, png_bytep data)
{
  try
  {
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4)
    {
      uint8_t *b = &data[i];
      uint32_t pixel;
      uint8_t alpha;

      memcpy(&pixel, b, sizeof(uint32_t));
      alpha = (pixel & 0xff000000u) >> 24;
      if (alpha == 0)
      {
        b[0] = b[1] = b[2] = b[3] = 0;
      }
      else
      {
        b[0] = (((pixel & 0xff0000u) >> 16) * 255 + alpha / 2) / alpha;
        b[1] = (((pixel & 0x00ff00u) >> 8) * 255 + alpha / 2) / alpha;
        b[2] = (((pixel & 0x0000ffu) >> 0) * 255 + alpha / 2) / alpha;
        b[3] = alpha;
      }
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// Cairo callback for writing image chunks

void append_to_string(png_structp png, png_bytep data, png_size_t length)
{
  try
  {
    auto *buffer = reinterpret_cast<std::string *>(png_get_io_ptr(png));
    buffer->append(reinterpret_cast<const char *>(data), length);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void giza_surface_write_to_png_string(cairo_surface_t *image,
                                      const ColorMapper &mapper,
                                      std::string &buffer)
{
  try
  {
    // Do pending operations
    cairo_surface_flush(image);

    if (cairo_image_surface_get_format(image) != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Giza::topng can write only Cairo ARGB32 format images");

    // Access image data directly.
    unsigned char *data = cairo_image_surface_get_data(image);

    if (data == nullptr)
      throw Fmi::Exception(BCP, "Attempt to render an invalid Cairo image as PNG");

    // The cairo image data may have a stride width, meaning once you have
    // passed a certain width you may have to skip more bytes to reach the next
    // row. Hence the position of the next row is calculated using the stride,
    // and not the width.

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);
    int stride = cairo_image_surface_get_stride(image);

    // Allocate png info variables

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr)
      throw Fmi::Exception(BCP, "Insufficient memory to allocate PNG write structure");

    png_infop info = png_create_info_struct(png);
    if (info == nullptr)
    {
      png_destroy_write_struct(&png, nullptr);
      throw Fmi::Exception(BCP, "Insufficient memory to allocate PNG info structure");
    }

    // Closure for writing the raw data generated by libpng to the actual output string.

    png_set_write_fn(png, &buffer, append_to_string, nullptr);

    // Determine the unique colours in the colour conversion map, and
    // set true colour mode if there are more than 256 colours.

    const auto &colormap = mapper.colormap();

    std::set<Color> unique_colors;
    if (!mapper.trueColor())
    {
      for (const ColorMap::value_type &from_to : colormap)
        unique_colors.insert(from_to.second);
    }

    bool truecolor = (mapper.trueColor() || unique_colors.size() > 256);

    // Generate the PNG data to the output

    if (truecolor)
    {
      // In ARGB mode we can handle all rows simultaneously.
      // Note that the pointers here are just pointers to
      // the cairo raw data, and hence no deletions afterwards
      // are necessary.

      std::vector<png_byte *> rows(height, nullptr);
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

      for (const Color color : unique_colors)
      {
        // Store the index for this new unique colour
        color_indices.insert(std::make_pair(color, color_indices.size()));

        // And inform libpng of its properties
        auto a = static_cast<png_byte>(alpha(color));
        color_values[num_colors].red =
            unpremultiply_color_component(static_cast<png_byte>(red(color)), a);
        color_values[num_colors].green =
            unpremultiply_color_component(static_cast<png_byte>(green(color)), a);
        color_values[num_colors].blue =
            unpremultiply_color_component(static_cast<png_byte>(blue(color)), a);

        // No need to skip storing here even if num_transparent increases no longer
        transparent_values[num_colors] = a;

        ++num_colors;

        if (a < 255)
          num_transparent = num_colors;
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
      if (num_transparent > 0)
        png_set_tRNS(png, info, transparent_values, num_transparent, nullptr);

      // Set the opaque RGB palette
      png_set_PLTE(png, info, color_values, num_colors);

      // Write PNG raw data

      png_write_info(png, info);

      // Using this vector of palette indices to convert all rows in the ARGB cairo input
      std::vector<png_byte> rows(width, 0);

      for (int i = 0; i < height; i++)
      {
        // Convert cairo ARGB row to array of palette indices
        auto *row = static_cast<png_byte *>(data + i * stride);
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
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a PNG string
 */
// ----------------------------------------------------------------------

std::string topng(cairo_surface_t *image)
{
  try
  {
    ColorMapper mapper;
    mapper.reduce(image);

    std::string buffer;
    giza_surface_write_to_png_string(image, mapper, buffer);
    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a PNG string
 */
// ----------------------------------------------------------------------

std::string topng(cairo_surface_t *image, const ColorMapOptions &options)
{
  try
  {
    ColorMapper mapper;
    mapper.options(options);
    mapper.reduce(image);

    std::string buffer;
    giza_surface_write_to_png_string(image, mapper, buffer);
    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
