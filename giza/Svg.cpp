#include "Svg.h"
#include "ColorMapper.h"
#include "Giza.h"
#include <macgyver/Exception.h>

#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>

// Note: In RHEL6 fontconfig, pango, cairo combination was not thread safe
// The problem was fixed in RHEL7.

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
    RsvgHandle *handle = nullptr;

    const auto *indata = reinterpret_cast<const guint8 *>(svg.c_str());

#if defined(VERSION_ID) && VERSION_ID < 8
    handle = rsvg_handle_new_from_data_with_flags(
        indata, svg.size(), RSVG_HANDLE_FLAG_UNLIMITED, nullptr);
#else
    handle = rsvg_handle_new_from_data(indata, svg.size(), nullptr);
#endif

    if (handle == nullptr)
      throw Fmi::Exception(BCP, "Failed to get rsvg handle on the SVG data");

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
    cairo_surface_t *image = nullptr;
    cairo_t *cr = nullptr;
    RsvgHandle *handle = nullptr;

    {
      // BrainStorm::WriteLock lock(globalPngMutex);

      const auto *indata = reinterpret_cast<const guint8 *>(svg.c_str());

#if defined(VERSION_ID) && VERSION_ID < 8
      handle = rsvg_handle_new_from_data_with_flags(
          indata, svg.size(), RSVG_HANDLE_FLAG_UNLIMITED, nullptr);
#else
      handle = rsvg_handle_new_from_data(indata, svg.size(), nullptr);
#endif

      if (handle == nullptr)
        throw Fmi::Exception(BCP, "Failed to get rsvg handle on the SVG data");

      RsvgDimensionData dimensions;
      rsvg_handle_get_dimensions(handle, &dimensions);
      image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
      cr = cairo_create(image);
      rsvg_handle_render_cairo(handle, cr);
    }

    g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

    std::string buffer = Giza::towebp(image, options);

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
    RsvgHandle *handle = nullptr;

    {
      // BrainStorm::WriteLock lock(globalPngMutex);

      const auto *indata = reinterpret_cast<const guint8 *>(svg.c_str());

#if defined(VERSION_ID) && VERSION_ID < 8
      handle = rsvg_handle_new_from_data_with_flags(
          indata, svg.size(), RSVG_HANDLE_FLAG_UNLIMITED, nullptr);
#else
      handle = rsvg_handle_new_from_data(indata, svg.size(), nullptr);
#endif

      if (handle == nullptr)
        throw Fmi::Exception(BCP, "Failed to get rsvg handle on the SVG data");

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
