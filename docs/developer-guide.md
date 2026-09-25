# giza developer guide

This guide is for developers who change `smartmet-library-giza`, or use it. giza turns the
SVG that the WMS/Dali plugin draws, or any Cairo image surface, into PNG, WebP (also
animated), PDF, PostScript or raw ARGB. Its main job is making PNGs small and fast: it
reduces the number of colours adaptively and writes palette PNGs whenever it can.

[CLAUDE.md](../CLAUDE.md) has the build summary.

## Contents

1. [Building and testing](#1-building-and-testing)
2. [The API](#2-the-api)
3. [Colour reduction](#3-colour-reduction)
4. [PNG and WebP writing](#4-png-and-webp-writing)
5. [Compatibility](#5-compatibility)
6. [Known pitfalls](#6-known-pitfalls)

---

## 1. Building and testing

```bash
make
make test
make -C test SvgTest && ./test/SvgTest
```

The tests (`regression/tframe.h`) render the SVG files in `test/input/` and compare the
results with `test/output/` using `CompareImages.sh` (a PSNR threshold); failed images go to
`test/failures/`. ImageMagick is needed for the comparison. When a change alters the output
on purpose, check the new images by eye before copying them to `test/output/`.

## 2. The API

| Call | Input → output |
|------|----------------|
| `Giza::Svg::topng(svg[, options])`, `towebp`, `topdf`, `tops`, `toargb`, `towebpanim` | SVG text → encoded image. librsvg renders the SVG onto a Cairo surface first. |
| `Giza::topng(surface[, options])`, `towebp`, `toargb`, `towebpanim` | Cairo ARGB32 surface → encoded image. |
| `Giza::topng_argb(pixels, width, height, level)` | Raw ARGB pixels → PNG, without colour reduction. |

`ColorMapOptions` controls the colour reduction (§3); `WebpOptions::level` the WebP
encoding (§4). `toargb()` returns a `new[]`-allocated array that the caller must
`delete[]`. The functions throw `Fmi::Exception` on failure.

## 3. Colour reduction

`ColorMapper::reduce()` runs before PNG and WebP output, and **modifies the surface in
place**:

1. If `truecolor` is set, nothing is done.
2. The colour histogram is computed. If the image has more than 100 distinct alpha values,
   or any alpha below 128 (soft transparency), colours are not reduced: with fewer than 256
   colours the image is written as a palette PNG as it is, otherwise as true colour.
3. Otherwise the colours are fed into a `ColorTree` in order of popularity; a colour closer
   than the error limit to an already chosen colour is mapped to it. The limit follows from
   `quality` (default 10; larger means worse quality and fewer colours, minimum 1).
4. With `maxcolors`, the reduction is repeated with the limit multiplied by `errorfactor`
   (default 2, minimum 1.1) until the colour count fits. Colours that fill at least one
   3×3 block are always kept, so the target is not always reached.
5. With 256 colours or fewer, the result is a palette PNG with a transparency table
   (`tRNS`), the palette ordered by use; otherwise a true-colour PNG.

The WMS plugin reads these settings from the product's `png` options (`quality`,
`maxcolors`, `truecolor`; `wms/Png.cpp`).

## 4. PNG and WebP writing

* PNGs are written with libdeflate at compression level 1 by default, which is faster and
  still slightly smaller than zlib level 3. The environment variable `GIZA_PNG_LEVEL`
  (1…12) overrides it, for benchmarking or when size matters more than speed.
* Cairo surfaces hold premultiplied alpha; giza unpremultiplies when writing.
* WebP is lossless. With `WebpOptions::level` < 0 (default), libwebp's simple lossless API
  is used; a level 0…9 selects libwebp's lossless preset (0 fastest and largest, 9 slowest
  and smallest).
* Animated WebP (`towebpanim`) takes one surface or SVG per frame, with a duration in
  milliseconds for each and a loop count.

## 5. Compatibility

The headers are installed and used by the WMS plugin. The API is plain functions and small option structs; adding a field to
`ColorMapOptions` or `WebpOptions` changes their layout, so rebuild the users.

The Makefile passes the RHEL major version as `VERSION_ID`, and the code selects between
the old (RHEL 7) and current librsvg APIs with it.

## 6. Known pitfalls

* **`reduce()` changes the surface.** Do not reuse the surface after `topng()` or
  `towebp()` expecting the original colours.
* **Soft transparency disables colour reduction** (§3), so semi-transparent layers make
  larger PNGs.
* **`maxcolors` is a target, not a guarantee** (§3).
* **`toargb()` results must be freed with `delete[]`.**
