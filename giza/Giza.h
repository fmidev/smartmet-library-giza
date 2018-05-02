#pragma once
#include <cairo/cairo.h>
#include <string>

namespace Giza
{
class ColorMapOptions;

std::string topng(cairo_surface_t* image);
std::string topng(cairo_surface_t* image, const ColorMapOptions& options);
}  // namespace Giza
