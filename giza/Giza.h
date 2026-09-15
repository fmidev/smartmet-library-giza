#pragma once
#include <cairo/cairo.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Giza
{
struct ColorMapOptions;
struct WebpOptions;

std::string topng(cairo_surface_t* image);
std::string topng(cairo_surface_t* image, const ColorMapOptions& options);
std::string towebp(cairo_surface_t* image);
std::string towebp(cairo_surface_t* image, const ColorMapOptions& options);
std::string towebp(cairo_surface_t* image,
                   const ColorMapOptions& options,
                   const WebpOptions& webpOptions);

// Encode an animated WebP from equal-sized frames. Frame durations are in
// milliseconds, loop_count 0 means infinite looping. Note: the frame surfaces
// are modified in place during encoding.
std::string towebpanim(const std::vector<cairo_surface_t*>& frames,
                       const std::vector<int>& durations,
                       int loop_count,
                       const ColorMapOptions& options,
                       const WebpOptions& webpOptions);

// Encode straight alpha 0xAARRGGBB pixels as an RGBA PNG as they are,
// with no colour reduction. This is for images which are already
// coloured, such as satellite composites embedded into an SVG. The level
// is the libdeflate level, 0 stores and 1 is fast and compresses well.
std::string topng_argb(const std::uint32_t* pixels, int width, int height, int level = 1);

uint* toargb(cairo_surface_t* image);
uint* toargb(cairo_surface_t* image, const ColorMapOptions& options);
}  // namespace Giza
