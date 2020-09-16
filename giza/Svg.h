#pragma once
#include <string>

namespace Giza
{
struct ColorMapOptions;

namespace Svg
{
std::string topng(const std::string& svg);
std::string topdf(const std::string& svg);
std::string tops(const std::string& svg);
std::string topng(const std::string& svg, const ColorMapOptions& options);
}  // namespace Svg
}  // namespace Giza
