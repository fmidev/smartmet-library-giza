# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

**smartmet-library-giza** is a C++17 library providing Cairo-based image rendering and color reduction for the SmartMet Server ecosystem (Finnish Meteorological Institute). It converts Cairo ARGB32 surfaces to PNG (with palette optimization), WebP, PDF, EPS, and raw ARGB. It also renders SVG input to these formats via librsvg.

## Build Commands

```bash
make                # Build libsmartmet-giza.so
make test           # Build and run all tests (from repo root)
make format         # Run clang-format on all source and test files
make clean          # Remove build artifacts
make rpm            # Build RPM package
```

### Running a single test

```bash
cd test && make ColorMapperTest && ./ColorMapperTest
cd test && make SvgTest && ./SvgTest
cd test && make PaletteTest && ./PaletteTest
```

Tests use the FMI `regression/tframe.h` framework (not Boost.Test). Each test binary is standalone. Tests compare output images against expected results in `test/output/` using PSNR-based image comparison (threshold: 30 dB). Failed outputs go to `test/failures/`.

## Dependencies

- **Build**: `smartmet-library-macgyver`, `librsvg2-devel`, `cairo-devel`, `libwebp13-devel`, `boost`
- **Tests additionally**: `ImageMagick` (Magick++ for image comparison), `fmt`, `smartmet-library-regression`
- **pkg-config modules** (REQUIRES in Makefile): `librsvg`, `cairo`, `webp`

## Architecture

All public API is in the `Giza` namespace. There are two main entry points:

- **`Giza::topng/towebp/toargb`** (`Giza.h`) — convert a `cairo_surface_t*` to an output format string. Applies color reduction automatically.
- **`Giza::Svg::topng/towebp/topdf/tops/toargb`** (`Svg.h`) — render SVG string to output format. Internally creates a Cairo surface via librsvg, then delegates to the surface converters above.

Color reduction pipeline:
1. **`ColorMapper`** — analyzes a Cairo image histogram, builds a color-to-color mapping that reduces unique colors, and applies it in-place on the surface.
2. **`ColorTree`** — a near-tree spatial data structure for finding the nearest color by perceptual distance (used during reduction).
3. **`Palette`** — converts a `ColorMap` into indexed palette entries (for PNG palette mode when <= 256 colors).
4. **`ColorMapOptions`** — controls quality (default 10), max colors, error convergence factor, and truecolor forcing.

PNG output uses libpng directly (not cairo's PNG writer) to support palette mode with transparency (tRNS). Cairo surfaces use premultiplied alpha; giza handles unpremultiplication during output.

## RHEL version handling

The Makefile extracts `VERSION_ID` from `/etc/os-release` and passes it as `-DVERSION_ID=N`. The code uses `#if defined(VERSION_ID) && VERSION_ID < 8` to select between RHEL 7 and 8+ librsvg API variants.

## Code Style

Google-based clang-format with Allman braces, 100-column limit. Run `make format` before committing. Member variables use `its` prefix (e.g., `itsColorMap`, `itsOptions`).
