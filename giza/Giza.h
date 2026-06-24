#pragma once
#include <cairo/cairo.h>

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

uint* toargb(cairo_surface_t* image);
uint* toargb(cairo_surface_t* image, const ColorMapOptions& options);
}  // namespace Giza
