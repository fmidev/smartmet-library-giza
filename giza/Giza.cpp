#include "Giza.h"
#include "ColorMapper.h"
#include "WebpOptions.h"
#include <cairo/cairo.h>
#include <macgyver/Exception.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include <algorithm>
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
      alpha = (pixel & 0xff000000U) >> 24;
      if (alpha == 0)
      {
        b[0] = b[1] = b[2] = b[3] = 0;  // normalize fully transparent colours
      }
      else
      {
        b[0] = (((pixel & 0xff0000U) >> 16) * 255 + alpha / 2) / alpha;
        b[1] = (((pixel & 0x00ff00U) >> 8) * 255 + alpha / 2) / alpha;
        b[2] = (((pixel & 0x0000ffU) >> 0) * 255 + alpha / 2) / alpha;
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

// Convert premultiplied ARGB32 surface data in place to unpremultiplied RGBA
// byte order as expected by libwebp, and return the pixel data pointer.

unsigned char *surface_to_unpremultiplied_rgba(cairo_surface_t *image,
                                               int &width,
                                               int &height,
                                               int &stride)
{
  try
  {
    cairo_surface_flush(image);

    if (cairo_image_surface_get_format(image) != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Giza::towebp can write only Cairo ARGB32 format images");

    // Access image data directly.
    unsigned char *data = cairo_image_surface_get_data(image);

    if (data == nullptr)
      throw Fmi::Exception(BCP, "Attempt to render an invalid Cairo image as WEBP");

    // The cairo image data may have a stride width, meaning once you have
    // passed a certain width you may have to skip more bytes to reach the next
    // row. Hence the position of the next row is calculated using the stride,
    // and not the width.

    width = cairo_image_surface_get_width(image);
    height = cairo_image_surface_get_height(image);
    stride = cairo_image_surface_get_stride(image);

    // Converting ARGB32 and unpremultiplying by alpha

    for (int i = 0; i < height; i++)
    {
      uint *row = (uint *)(data + i * stride);
      for (int x = 0; x < width; x++)
      {
        uint col = row[x];
        uint8_t alpha = (col & 0xff000000U) >> 24;
        if (alpha == 0)
          row[x] = 0;  // normalize fully transparent colours
        else
        {
          uint8_t r = (((col & 0xff0000U) >> 16) * 255 + alpha / 2) / alpha;
          uint8_t g = (((col & 0x00ff00U) >> 8) * 255 + alpha / 2) / alpha;
          uint8_t b = (((col & 0x0000ffU) >> 0) * 255 + alpha / 2) / alpha;
          row[x] = (alpha << 24) | (b << 16) | (g << 8) | r;
        }
      }
    }

    return data;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void giza_surface_write_to_webp_string(cairo_surface_t *image,
                                       std::string &buffer,
                                       const WebpOptions &options)
{
  try
  {
    int width = 0;
    int height = 0;
    int stride = 0;
    unsigned char *data = surface_to_unpremultiplied_rgba(image, width, height, stride);

    if (options.level < 0)
    {
      // Default: use libwebp's simple lossless API (historical behaviour)
      uint8_t *output = nullptr;
      size_t sz = WebPEncodeLosslessRGBA(data, width, height, stride, &output);

      try
      {
        if (sz && output)
          buffer.append(reinterpret_cast<const char *>(output), sz);
      }
      catch (...)
      {
      }

      free(output);
    }
    else
    {
      // Explicit speed control: use the advanced API so the lossless preset
      // level (0 = fastest/largest ... 9 = slowest/smallest) takes effect.
      WebPConfig config;
      if (!WebPConfigInit(&config))
        throw Fmi::Exception(BCP, "Failed to initialize libwebp configuration");
      config.lossless = 1;
      if (!WebPConfigLosslessPreset(&config, std::clamp(options.level, 0, 9)))
        throw Fmi::Exception(BCP, "Invalid libwebp lossless preset level");
      if (!WebPValidateConfig(&config))
        throw Fmi::Exception(BCP, "Invalid libwebp configuration");

      WebPPicture pic;
      if (!WebPPictureInit(&pic))
        throw Fmi::Exception(BCP, "Failed to initialize libwebp picture");
      pic.use_argb = 1;
      pic.width = width;
      pic.height = height;

      WebPMemoryWriter writer;
      WebPMemoryWriterInit(&writer);
      pic.writer = WebPMemoryWrite;
      pic.custom_ptr = &writer;

      try
      {
        // The pixel data is already in RGBA byte order (see the unpremultiply
        // loop above), matching what WebPEncodeLosslessRGBA expects.
        if (!WebPPictureImportRGBA(&pic, data, stride))
          throw Fmi::Exception(BCP, "Failed to import image into libwebp picture");

        if (!WebPEncode(&config, &pic))
          throw Fmi::Exception(BCP, "libwebp encoding failed")
              .addParameter("error_code", std::to_string(pic.error_code));

        buffer.append(reinterpret_cast<const char *>(writer.mem), writer.size);
      }
      catch (...)
      {
        WebPMemoryWriterClear(&writer);
        WebPPictureFree(&pic);
        throw;
      }

      WebPMemoryWriterClear(&writer);
      WebPPictureFree(&pic);
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

uint *giza_surface_write_to_argb(cairo_surface_t *image)
{
  try
  {
    cairo_surface_flush(image);

    if (cairo_image_surface_get_format(image) != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Giza::toargb can write only Cairo ARGB32 format images");

    // Access image data directly.
    unsigned char *data = cairo_image_surface_get_data(image);

    if (data == nullptr)
      throw Fmi::Exception(BCP, "Attempt to render an invalid Cairo image as ARGB");

    // The cairo image data may have a stride width, meaning once you have
    // passed a certain width you may have to skip more bytes to reach the next
    // row. Hence the position of the next row is calculated using the stride,
    // and not the width.

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);
    int stride = cairo_image_surface_get_stride(image);

    // Converting ARGB32 and unpremultiplying by alpha

    uint sz = width * height;
    if (sz == 0)
      throw Fmi::Exception(BCP, "Cairo image size is zero.");

    uint *output = new uint[sz];

    uint c = 0;
    // uint nz = 0;
    for (int i = 0; i < height; i++)
    {
      uint *row = (uint *)(data + i * stride);
      for (int x = 0; x < width; x++)
      {
        uint col = row[x];
        // if (col & 0xFF000000)
        //    nz++;

        output[c] = col;
        c++;
      }
    }
    return output;
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

    // Speed over size: no filtering, and compression level 3 avoids deflate_slow
    // while still compressing reasonably well.

    png_set_filter(png, 0, PNG_FILTER_NONE);
    png_set_compression_level(png, 3);

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
      png_byte transparent_values[256];  // NOLINT cannot use std::array here
      png_color color_values[256];       // NOLINT cannot use std::array here

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
        png_write_row(png, rows.data());
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

std::string towebp(cairo_surface_t *image)
{
  try
  {
    ColorMapper mapper;
    mapper.reduce(image);

    std::string buffer;
    giza_surface_write_to_webp_string(image, buffer, WebpOptions());
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

std::string towebp(cairo_surface_t *image, const ColorMapOptions &options)
{
  try
  {
    return towebp(image, options, WebpOptions());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a WEBP string with explicit encoder options
 */
// ----------------------------------------------------------------------

std::string towebp(cairo_surface_t *image,
                   const ColorMapOptions &options,
                   const WebpOptions &webpOptions)
{
  try
  {
    ColorMapper mapper;
    mapper.options(options);
    mapper.reduce(image);

    std::string buffer;
    giza_surface_write_to_webp_string(image, buffer, webpOptions);
    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surfaces to an animated WEBP string
 */
// ----------------------------------------------------------------------

std::string towebpanim(const std::vector<cairo_surface_t *> &frames,
                       const std::vector<int> &durations,
                       int loop_count,
                       const ColorMapOptions &options,
                       const WebpOptions &webpOptions)
{
  try
  {
    if (frames.empty())
      throw Fmi::Exception(BCP, "Giza::towebpanim requires at least one frame");

    if (durations.size() != frames.size())
      throw Fmi::Exception(BCP, "Giza::towebpanim requires one duration for each frame");

    int width = cairo_image_surface_get_width(frames[0]);
    int height = cairo_image_surface_get_height(frames[0]);

    WebPConfig config;
    if (!WebPConfigInit(&config))
      throw Fmi::Exception(BCP, "Failed to initialize libwebp configuration");
    config.lossless = 1;
    if (webpOptions.level >= 0)
      if (!WebPConfigLosslessPreset(&config, std::clamp(webpOptions.level, 0, 9)))
        throw Fmi::Exception(BCP, "Invalid libwebp lossless preset level");
    if (!WebPValidateConfig(&config))
      throw Fmi::Exception(BCP, "Invalid libwebp configuration");

    WebPAnimEncoderOptions enc_options;
    if (!WebPAnimEncoderOptionsInit(&enc_options))
      throw Fmi::Exception(BCP, "Failed to initialize libwebp animation encoder options");
    enc_options.anim_params.loop_count = std::max(0, loop_count);

    WebPAnimEncoder *enc = WebPAnimEncoderNew(width, height, &enc_options);
    if (enc == nullptr)
      throw Fmi::Exception(BCP, "Failed to create libwebp animation encoder");

    std::string buffer;
    try
    {
      int timestamp = 0;
      for (std::size_t i = 0; i < frames.size(); i++)
      {
        auto *image = frames[i];

        if (cairo_image_surface_get_width(image) != width ||
            cairo_image_surface_get_height(image) != height)
          throw Fmi::Exception(BCP, "Giza::towebpanim frames must be of equal size");

        ColorMapper mapper;
        mapper.options(options);
        mapper.reduce(image);

        int w = 0;
        int h = 0;
        int stride = 0;
        unsigned char *data = surface_to_unpremultiplied_rgba(image, w, h, stride);

        WebPPicture pic;
        if (!WebPPictureInit(&pic))
          throw Fmi::Exception(BCP, "Failed to initialize libwebp picture");
        pic.use_argb = 1;
        pic.width = width;
        pic.height = height;

        if (!WebPPictureImportRGBA(&pic, data, stride))
        {
          WebPPictureFree(&pic);
          throw Fmi::Exception(BCP, "Failed to import frame into libwebp picture");
        }

        bool ok = WebPAnimEncoderAdd(enc, &pic, timestamp, &config);
        WebPPictureFree(&pic);

        if (!ok)
          throw Fmi::Exception(BCP, "libwebp animation encoding failed")
              .addParameter("error", WebPAnimEncoderGetError(enc));

        timestamp += durations[i];
      }

      // Flush the encoder with the total duration as the final timestamp

      if (!WebPAnimEncoderAdd(enc, nullptr, timestamp, nullptr))
        throw Fmi::Exception(BCP, "Failed to flush libwebp animation encoder")
            .addParameter("error", WebPAnimEncoderGetError(enc));

      WebPData webp_data;
      WebPDataInit(&webp_data);
      if (!WebPAnimEncoderAssemble(enc, &webp_data))
      {
        WebPDataClear(&webp_data);
        throw Fmi::Exception(BCP, "Failed to assemble libwebp animation")
            .addParameter("error", WebPAnimEncoderGetError(enc));
      }

      buffer.assign(reinterpret_cast<const char *>(webp_data.bytes), webp_data.size);
      WebPDataClear(&webp_data);
    }
    catch (...)
    {
      WebPAnimEncoderDelete(enc);
      throw;
    }

    WebPAnimEncoderDelete(enc);
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

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a ARGB image. The caller must release it.
 */
// ----------------------------------------------------------------------

uint *toargb(cairo_surface_t *image)
{
  try
  {
    // ColorMapper mapper;
    // mapper.reduce(image);

    return giza_surface_write_to_argb(image);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Write cairo surface to a ARGB image. The caller must release it.
 */
// ----------------------------------------------------------------------

uint *toargb(cairo_surface_t *image, const ColorMapOptions & /* options */)
{
  try
  {
    // ColorMapper mapper;
    // mapper.options(options);
    // mapper.reduce(image);

    return giza_surface_write_to_argb(image);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

}  // namespace Giza
