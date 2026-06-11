#include "Svg.h"
#include "ColorMapper.h"
#include "Giza.h"
#include "WebpOptions.h"
#include <macgyver/Exception.h>

#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo.h>
#include <gio/gio.h>
#include <librsvg/rsvg.h>

namespace
{

// Note: In RHEL6 fontconfig, pango, cairo combination was not thread safe
// The problem was fixed in RHEL7.

// ----------------------------------------------------------------------
/*!
 * \brief Create an RsvgHandle from an in-memory SVG string
 *
 * Uses RSVG_HANDLE_FLAG_UNLIMITED so libxml2's 10/20 MB safety limits
 * do not reject large SVG inputs.
 */
// ----------------------------------------------------------------------

RsvgHandle *make_rsvg_handle(const std::string &svg)
{
  const auto *indata = reinterpret_cast<const guint8 *>(svg.c_str());
  GInputStream *stream = g_memory_input_stream_new_from_data(indata, svg.size(), nullptr);
  RsvgHandle *handle = rsvg_handle_new_from_stream_sync(
      stream, nullptr, RSVG_HANDLE_FLAG_UNLIMITED, nullptr, nullptr);
  g_object_unref(stream);

  if (handle == nullptr)
    throw Fmi::Exception(BCP, "Failed to get rsvg handle on the SVG data");

  return handle;
}

// ----------------------------------------------------------------------
/*!
 * \brief Data holder for generating raw image data
 */
// ----------------------------------------------------------------------

// Cairo callback for writing image chunks

cairo_status_t stream_to_buffer(void *closure, const unsigned char *data, unsigned int length)
{
  try
  {
    auto *buffer = reinterpret_cast<std::string *>(closure);
    buffer->append(reinterpret_cast<const char *>(data), length);
    return CAIRO_STATUS_SUCCESS;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PDF/PS memory buffer
 */
// ----------------------------------------------------------------------

std::string svg_to_pdf_or_ps(const std::string &svg, bool ispdf)
{
  try
  {
    cairo_surface_t *image = nullptr;
    cairo_t *cr = nullptr;
    RsvgHandle *handle = make_rsvg_handle(svg);

    RsvgDimensionData dimensions;
    rsvg_handle_get_dimensions(handle, &dimensions);

    std::string buffer;
    if (ispdf)
      image = cairo_pdf_surface_create_for_stream(
          stream_to_buffer, &buffer, dimensions.width, dimensions.height);
    else
    {
      image = cairo_ps_surface_create_for_stream(
          stream_to_buffer, &buffer, dimensions.width, dimensions.height);
      // we always produce eps only
      cairo_ps_surface_set_eps(image, 1);
    }

    cr = cairo_create(image);
    rsvg_handle_render_cairo(handle, cr);

    cairo_surface_destroy(image);
    cairo_destroy(cr);
    g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace

// ----------------------------------------------------------------------

namespace Giza
{
namespace Svg
{
// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PNG in memory
 */
// ----------------------------------------------------------------------

std::string towebp(const std::string &svg)
{
  try
  {
    ColorMapOptions options;
    return towebp(svg, options);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to WEBP in memory
 */
// ----------------------------------------------------------------------

std::string towebp(const std::string &svg, const ColorMapOptions &options)
{
  try
  {
    return towebp(svg, options, WebpOptions());
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to WEBP in memory with explicit encoder options
 */
// ----------------------------------------------------------------------

std::string towebp(const std::string &svg,
                   const ColorMapOptions &options,
                   const WebpOptions &webpOptions)
{
  try
  {
    cairo_surface_t *image = nullptr;
    cairo_t *cr = nullptr;
    RsvgHandle *handle = make_rsvg_handle(svg);

    {
      // BrainStorm::WriteLock lock(globalPngMutex);

      RsvgDimensionData dimensions;
      rsvg_handle_get_dimensions(handle, &dimensions);
      image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
      cr = cairo_create(image);
      rsvg_handle_render_cairo(handle, cr);
    }

    g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

    std::string buffer = Giza::towebp(image, options, webpOptions);

    cairo_surface_destroy(image);
    cairo_destroy(cr);

    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Render SVG frames and convert to an animated WEBP in memory
 */
// ----------------------------------------------------------------------

std::string towebpanim(const std::vector<std::string> &svgs,
                       const std::vector<int> &durations,
                       int loop_count,
                       const ColorMapOptions &options,
                       const WebpOptions &webpOptions)
{
  try
  {
    std::vector<cairo_surface_t *> frames;
    std::vector<cairo_t *> contexts;

    auto cleanup = [&frames, &contexts]()
    {
      for (auto *image : frames)
        cairo_surface_destroy(image);
      for (auto *cr : contexts)
        cairo_destroy(cr);
    };

    try
    {
      for (const auto &svg : svgs)
      {
        RsvgHandle *handle = make_rsvg_handle(svg);

        RsvgDimensionData dimensions;
        rsvg_handle_get_dimensions(handle, &dimensions);
        cairo_surface_t *image =
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
        frames.push_back(image);
        cairo_t *cr = cairo_create(image);
        contexts.push_back(cr);
        rsvg_handle_render_cairo(handle, cr);

        g_object_unref(handle);
      }

      std::string buffer = Giza::towebpanim(frames, durations, loop_count, options, webpOptions);

      cleanup();
      return buffer;
    }
    catch (...)
    {
      cleanup();
      throw;
    }
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PNG in memory
 */
// ----------------------------------------------------------------------

std::string topng(const std::string &svg)
{
  try
  {
    ColorMapOptions options;
    return topng(svg, options);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PNG in memory
 */
// ----------------------------------------------------------------------

std::string topng(const std::string &svg, const ColorMapOptions &options)
{
  try
  {
    cairo_surface_t *image = nullptr;
    cairo_t *cr = nullptr;
    RsvgHandle *handle = make_rsvg_handle(svg);

    {
      // BrainStorm::WriteLock lock(globalPngMutex);

      RsvgDimensionData dimensions;
      rsvg_handle_get_dimensions(handle, &dimensions);
      image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
      cr = cairo_create(image);
      rsvg_handle_render_cairo(handle, cr);
    }

    g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

    std::string buffer = Giza::topng(image, options);

    cairo_surface_destroy(image);
    cairo_destroy(cr);

    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PNG in memory
 */
// ----------------------------------------------------------------------

uint *toargb(const std::string &svg)
{
  try
  {
    ColorMapOptions options;
    return toargb(svg, options);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PNG in memory
 */
// ----------------------------------------------------------------------

uint *toargb(const std::string &svg, const ColorMapOptions &options)
{
  try
  {
    cairo_surface_t *image = nullptr;
    cairo_t *cr = nullptr;
    RsvgHandle *handle = make_rsvg_handle(svg);

    {
      // BrainStorm::WriteLock lock(globalPngMutex);

      RsvgDimensionData dimensions;
      rsvg_handle_get_dimensions(handle, &dimensions);
      image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
      cr = cairo_create(image);
      rsvg_handle_render_cairo(handle, cr);
    }

    g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

    uint *buffer = Giza::toargb(image, options);

    cairo_surface_destroy(image);
    cairo_destroy(cr);

    return buffer;
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PDF in memory
 */
// ----------------------------------------------------------------------

std::string topdf(const std::string &svg)
{
  try
  {
    return svg_to_pdf_or_ps(svg, true);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PS in memory
 */
// ----------------------------------------------------------------------

std::string tops(const std::string &svg)
{
  try
  {
    return svg_to_pdf_or_ps(svg, false);
  }
  catch (...)
  {
    throw Fmi::Exception::Trace(BCP, "Operation failed!");
  }
}
}  // namespace Svg
}  // namespace Giza
