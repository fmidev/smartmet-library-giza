#pragma once

namespace Giza
{
struct WebpOptions
{
  // libwebp lossless compression preset level passed to WebPConfigLosslessPreset:
  //   0 = fastest encoding, largest file
  //   9 = slowest encoding, smallest file
  // A negative value means "use the libwebp default configuration", which
  // preserves the historical output produced by WebPEncodeLosslessRGBA.
  int level = -1;
};
}  // namespace Giza
