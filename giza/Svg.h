#pragma once
#include <string>

namespace Giza
{
struct ColorMapOptions;
struct WebpOptions;

namespace Svg
{
std::string topng(const std::string& svg);
std::string topdf(const std::string& svg);
std::string tops(const std::string& svg);
std::string towebp(const std::string& svg);
std::string topng(const std::string& svg, const ColorMapOptions& options);
std::string towebp(const std::string& svg, const ColorMapOptions& options);
std::string towebp(const std::string& svg,
                   const ColorMapOptions& options,
                   const WebpOptions& webpOptions);

uint* toargb(const std::string& svg);
uint* toargb(const std::string& svg, const ColorMapOptions& options);
}  // namespace Svg
}  // namespace Giza
