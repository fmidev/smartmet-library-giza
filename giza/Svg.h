#pragma once
#include <string>
#include <vector>

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

// Render each SVG frame and encode an animated WebP. Frame durations are in
// milliseconds, loop_count 0 means infinite looping.
std::string towebpanim(const std::vector<std::string>& svgs,
                       const std::vector<int>& durations,
                       int loop_count,
                       const ColorMapOptions& options,
                       const WebpOptions& webpOptions);

uint* toargb(const std::string& svg);
uint* toargb(const std::string& svg, const ColorMapOptions& options);
}  // namespace Svg
}  // namespace Giza
