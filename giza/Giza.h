#pragma once
#include <cairo/cairo.h>

#include <string>

namespace Giza
{
struct ColorMapOptions;

std::string topng(cairo_surface_t* image);
std::string topng(cairo_surface_t* image, const ColorMapOptions& options);
std::string towebp(cairo_surface_t* image);
std::string towebp(cairo_surface_t* image, const ColorMapOptions& options);
}  // namespace Giza
