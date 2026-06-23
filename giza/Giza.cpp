#include "Giza.h"
#include "ColorMapper.h"
#include "WebpOptions.h"
#include <cairo/cairo.h>
#include <macgyver/Exception.h>
#include <webp/encode.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <libdeflate.h>
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

void giza_surface_write_to_webp_string(cairo_surface_t *image,
                                       std::string &buffer,
                                       const WebpOptions &options)
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

    int width = cairo_image_surface_get_width(image);
    int height = cairo_image_surface_get_height(image);
    int stride = cairo_image_surface_get_stride(image);

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

// --- Minimal PNG container writer compressing IDAT with libdeflate ---------------------
//
// giza always writes unfiltered scanlines (PNG_FILTER_NONE), so the IDAT input is simply a
// 0x00 filter byte followed by the raw row bytes per row. PNG's IDAT payload is a zlib
// datastream, which is exactly what libdeflate_zlib_compress() produces, and PNG chunk CRCs
// are the standard CRC-32 that libdeflate_crc32() computes. This lets us bypass libpng (and
// its streaming zlib) and use libdeflate's faster one-shot compressor.

void put_be32(std::string &out, uint32_t value)
{
  out.push_back(static_cast<char>((value >> 24) & 0xff));
  out.push_back(static_cast<char>((value >> 16) & 0xff));
  out.push_back(static_cast<char>((value >> 8) & 0xff));
  out.push_back(static_cast<char>(value & 0xff));
}

// Append a PNG chunk: length, 4-byte type, data, CRC-32 over (type + data).
void png_chunk(std::string &out, const char *type, const uint8_t *data, size_t len)
{
  put_be32(out, static_cast<uint32_t>(len));
  const size_t crc_begin = out.size();
  out.append(type, 4);
  if (len > 0)
    out.append(reinterpret_cast<const char *>(data), len);
  const auto crc = libdeflate_crc32(0, out.data() + crc_begin, 4 + len);
  put_be32(out, crc);
}

// libdeflate compression level (1..12). Default 1 favors speed (as the previous zlib level 3
// did) while still producing slightly smaller output than zlib level 3, and is measurably
// faster. Overridable via GIZA_PNG_LEVEL for benchmarking or when smaller files are preferred.
int libdeflate_level()
{
  if (const char *env = std::getenv("GIZA_PNG_LEVEL"))
  {
    const int level = std::atoi(env);
    if (level >= 1 && level <= 12)
      return level;
  }
  return 1;
}

void write_png_libdeflate(cairo_surface_t *image, const ColorMapper &mapper, std::string &buffer)
{
  try
  {
    cairo_surface_flush(image);

    if (cairo_image_surface_get_format(image) != CAIRO_FORMAT_ARGB32)
      throw Fmi::Exception(BCP, "Giza::topng can write only Cairo ARGB32 format images");

    unsigned char *data = cairo_image_surface_get_data(image);
    if (data == nullptr)
      throw Fmi::Exception(BCP, "Attempt to render an invalid Cairo image as PNG");

    const int width = cairo_image_surface_get_width(image);
    const int height = cairo_image_surface_get_height(image);
    const int stride = cairo_image_surface_get_stride(image);

    // Same truecolor-vs-palette decision as the libpng path
    const auto &colormap = mapper.colormap();
    std::set<Color> unique_colors;
    if (!mapper.trueColor())
      for (const ColorMap::value_type &from_to : colormap)
        unique_colors.insert(from_to.second);

    const bool truecolor = (mapper.trueColor() || unique_colors.size() > 256);

    // Build the raw (unfiltered) scanline buffer and, for palette mode, the palette tables.
    std::string raw;
    std::vector<uint8_t> plte;  // RGB triplets
    std::vector<uint8_t> trns;  // leading transparent alphas

    if (truecolor)
    {
      const size_t rowbytes = static_cast<size_t>(width) * 4;
      raw.resize(static_cast<size_t>(height) * (1 + rowbytes));
      auto *out = reinterpret_cast<uint8_t *>(raw.data());
      for (int i = 0; i < height; i++)
      {
        const auto *row = data + static_cast<size_t>(i) * stride;
        *out++ = 0;  // PNG_FILTER_NONE
        for (int j = 0; j < width; j++)
        {
          uint32_t pixel;
          std::memcpy(&pixel, row + j * 4, sizeof(pixel));
          const uint8_t a = (pixel & 0xff000000U) >> 24;
          if (a == 0)
          {
            out[0] = out[1] = out[2] = out[3] = 0;  // normalize fully transparent pixels
          }
          else
          {
            out[0] = (((pixel & 0xff0000U) >> 16) * 255 + a / 2) / a;
            out[1] = (((pixel & 0x00ff00U) >> 8) * 255 + a / 2) / a;
            out[2] = (((pixel & 0x0000ffU) >> 0) * 255 + a / 2) / a;
            out[3] = a;
          }
          out += 4;
        }
      }
    }
    else
    {
      // Assign palette indices in the (alpha-ascending) order of the color set, exactly as the
      // libpng path, so transparent colors come first and tRNS stays minimal.
      std::map<Color, int> color_indices;
      int num_transparent = 0;
      for (const Color color : unique_colors)
      {
        const int idx = static_cast<int>(color_indices.size());
        color_indices.insert(std::make_pair(color, idx));
        const auto a = alpha(color);
        plte.push_back(unpremultiply_color_component(red(color), a));
        plte.push_back(unpremultiply_color_component(green(color), a));
        plte.push_back(unpremultiply_color_component(blue(color), a));
        trns.push_back(a);
        if (a < 255)
          num_transparent = idx + 1;
      }
      trns.resize(num_transparent);  // keep only the leading transparent entries

      const size_t rowbytes = static_cast<size_t>(width);
      raw.resize(static_cast<size_t>(height) * (1 + rowbytes));
      auto *out = reinterpret_cast<uint8_t *>(raw.data());
      for (int i = 0; i < height; i++)
      {
        const auto *row = data + static_cast<size_t>(i) * stride;
        *out++ = 0;  // PNG_FILTER_NONE
        for (int j = 0; j < width; j++)
        {
          Color c = *reinterpret_cast<const Color *>(row + j * 4);
          *out++ = static_cast<uint8_t>(color_indices.at(c));
        }
      }
    }

    // Compress the scanlines into the IDAT zlib datastream
    auto *compressor = libdeflate_alloc_compressor(libdeflate_level());
    if (compressor == nullptr)
      throw Fmi::Exception(BCP, "Failed to allocate libdeflate compressor");

    const size_t bound = libdeflate_zlib_compress_bound(compressor, raw.size());
    std::vector<uint8_t> idat(bound);
    const size_t idat_size =
        libdeflate_zlib_compress(compressor, raw.data(), raw.size(), idat.data(), bound);
    libdeflate_free_compressor(compressor);
    if (idat_size == 0)
      throw Fmi::Exception(BCP, "libdeflate failed to compress PNG image data");

    // Emit the PNG datastream
    static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    buffer.append(reinterpret_cast<const char *>(signature), sizeof(signature));

    uint8_t ihdr[13];
    ihdr[0] = (width >> 24) & 0xff;
    ihdr[1] = (width >> 16) & 0xff;
    ihdr[2] = (width >> 8) & 0xff;
    ihdr[3] = width & 0xff;
    ihdr[4] = (height >> 24) & 0xff;
    ihdr[5] = (height >> 16) & 0xff;
    ihdr[6] = (height >> 8) & 0xff;
    ihdr[7] = height & 0xff;
    ihdr[8] = 8;                     // bit depth
    ihdr[9] = truecolor ? 6 : 3;     // color type: RGBA or palette
    ihdr[10] = 0;                    // compression: deflate
    ihdr[11] = 0;                    // filter method: adaptive (we only use NONE)
    ihdr[12] = 0;                    // interlace: none
    png_chunk(buffer, "IHDR", ihdr, sizeof(ihdr));

    if (!truecolor)
    {
      png_chunk(buffer, "PLTE", plte.data(), plte.size());
      if (!trns.empty())
        png_chunk(buffer, "tRNS", trns.data(), trns.size());
    }

    png_chunk(buffer, "IDAT", idat.data(), idat_size);
    png_chunk(buffer, "IEND", nullptr, 0);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

void write_png_libpng(cairo_surface_t *image, const ColorMapper &mapper, std::string &buffer)
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

// Write a Cairo surface to a PNG string. Uses the libdeflate-based writer by default; set
// GIZA_USE_LIBPNG to fall back to the original libpng path (kept for comparison/safety while
// the libdeflate writer is being validated).
void giza_surface_write_to_png_string(cairo_surface_t *image,
                                      const ColorMapper &mapper,
                                      std::string &buffer)
{
  if (std::getenv("GIZA_USE_LIBPNG") != nullptr)
    write_png_libpng(image, mapper, buffer);
  else
    write_png_libdeflate(image, mapper, buffer);
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
