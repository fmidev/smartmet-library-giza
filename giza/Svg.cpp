#include "Svg.h"
#include "ColorMapper.h"
#include "Giza.h"

#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include <stdexcept>

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
  std::string *buffer = reinterpret_cast<std::string *>(closure);
  buffer->append(reinterpret_cast<const char *>(data), length);
  return CAIRO_STATUS_SUCCESS;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PDF/PS memory buffer
 */
// ----------------------------------------------------------------------

std::string svg_to_pdf_or_ps(const std::string &svg, bool ispdf)
{
  cairo_surface_t *image = NULL;
  cairo_t *cr = NULL;
  RsvgHandle *handle = NULL;

  const guint8 *indata = reinterpret_cast<const guint8 *>(svg.c_str());

  handle =
      rsvg_handle_new_from_data_with_flags(indata, svg.size(), RSVG_HANDLE_FLAG_UNLIMITED, NULL);

  if (!handle) throw std::runtime_error("Failed to get rsvg handle on the SVG data");

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
    cairo_ps_surface_set_eps(image, true);
  }

  cr = cairo_create(image);
  rsvg_handle_render_cairo(handle, cr);

  cairo_surface_destroy(image);
  cairo_destroy(cr);
  g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

  return buffer;
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

std::string topng(const std::string &svg)
{
  cairo_surface_t *image = NULL;
  cairo_t *cr = NULL;
  RsvgHandle *handle = NULL;

  {
    // BrainStorm::WriteLock lock(globalPngMutex);

    const guint8 *indata = reinterpret_cast<const guint8 *>(svg.c_str());

    handle =
        rsvg_handle_new_from_data_with_flags(indata, svg.size(), RSVG_HANDLE_FLAG_UNLIMITED, NULL);

    if (!handle) throw std::runtime_error("Failed to get rsvg handle on the SVG data");

    RsvgDimensionData dimensions;
    rsvg_handle_get_dimensions(handle, &dimensions);
    image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, dimensions.width, dimensions.height);
    cr = cairo_create(image);
    rsvg_handle_render_cairo(handle, cr);
  }

  g_object_unref(handle);  // Deprecated: rsvg_handle_free(handle);

  std::string buffer = Giza::topng(image);

  cairo_surface_destroy(image);
  cairo_destroy(cr);

  return buffer;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PDF in memory
 */
// ----------------------------------------------------------------------

std::string topdf(const std::string &svg) { return svg_to_pdf_or_ps(svg, true); }
// ----------------------------------------------------------------------
/*!
 * \brief Convert SVG to PS in memory
 */
// ----------------------------------------------------------------------

std::string tops(const std::string &svg) { return svg_to_pdf_or_ps(svg, false); }
}  // namespace Svg
}  // namespace Giza
