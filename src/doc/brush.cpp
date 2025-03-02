// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/brush.h"

#include "base/pi.h"
#include "doc/algo.h"
#include "doc/algorithm/polygon.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/primitives.h"

#include <cmath>
#include <array>

namespace doc {

static int generation = 0;

Brush::Brush()
{
  m_type = kCircleBrushType;
  m_size = 1;
  m_angle = 0;
  m_pattern = BrushPattern::DEFAULT;
  m_gen = 0;

  regenerate();
}

Brush::Brush(BrushType type, int size, int angle)
{
  m_type = type;
  m_size = size;
  m_angle = angle;
  m_pattern = BrushPattern::DEFAULT;
  m_gen = 0;

  regenerate();
}

Brush::Brush(const Brush& brush)
{
  m_type = brush.m_type;
  m_size = brush.m_size;
  m_angle = brush.m_angle;
  m_image = brush.m_image;
  m_pattern = brush.m_pattern;
  m_patternOrigin = brush.m_patternOrigin;
  m_gen = 0;

  regenerate();
}

Brush::~Brush()
{
  clean();
}

void Brush::setType(BrushType type)
{
  m_type = type;
  if (m_type != kImageBrushType)
    regenerate();
  else
    clean();
}

void Brush::setSize(int size)
{
  m_size = size;
  regenerate();
}

void Brush::setAngle(int angle)
{
  m_angle = angle;
  regenerate();
}

void Brush::setImage(const Image* image)
{
  m_type = kImageBrushType;
  m_image.reset(Image::createCopy(image));
  m_backupImage.reset();
  m_mainColor.reset();
  m_bgColor.reset();

  m_bounds = gfx::Rect(
    -m_image.get()->width()/2, -m_image.get()->height()/2,
    m_image.get()->width(), m_image.get()->height());
}

template<class ImageTraits,
         color_t color_mask,
         color_t alpha_mask>
static void replace_image_colors(
  Image* image,
  const bool useMain, color_t mainColor,
  const bool useBg, color_t bgColor)
{
  LockImageBits<ImageTraits> bits(image, Image::ReadWriteLock);
  bool hasAlpha = false; // True if "image" has a pixel with alpha < 255
  color_t srcMainColor, srcBgColor;
  srcMainColor = srcBgColor = 0;

  for (const auto& pixel : bits) {
    if ((pixel & alpha_mask) != alpha_mask) {  // If alpha != 255
      hasAlpha = true;
    }
    else if (srcBgColor == 0) {
      srcMainColor = srcBgColor = pixel;
    }
    else if (pixel != srcBgColor && srcMainColor == srcBgColor) {
      srcMainColor = pixel;
    }
  }

  mainColor &= color_mask;
  bgColor &= color_mask;

  if (hasAlpha) {
    for (auto& pixel : bits) {
      if (useMain)
        pixel = (pixel & alpha_mask) | mainColor;
      else if (useBg)
        pixel = (pixel & alpha_mask) | bgColor;
    }
  }
  else {
    for (auto& pixel : bits) {
      if (useMain && ((pixel != srcBgColor) || (srcMainColor == srcBgColor))) {
        pixel = (pixel & alpha_mask) | mainColor;
      }
      else if (useBg && (pixel == srcBgColor)) {
        pixel = (pixel & alpha_mask) | bgColor;
      }
    }
  }
}

static void replace_image_colors_indexed(
  Image* image,
  const bool useMain, const color_t mainColor,
  const bool useBg, const color_t bgColor)
{
  LockImageBits<IndexedTraits> bits(image, Image::ReadWriteLock);
  bool hasAlpha = false; // True if "image" has a pixel with the mask color
  color_t maskColor = image->maskColor();
  color_t srcMainColor, srcBgColor;
  srcMainColor = srcBgColor = maskColor;

  for (const auto& pixel : bits) {
    if (pixel == maskColor) {
      hasAlpha = true;
    }
    else if (srcBgColor == maskColor) {
      srcMainColor = srcBgColor = pixel;
    }
    else if (pixel != srcBgColor && srcMainColor == srcBgColor) {
      srcMainColor = pixel;
    }
  }

  if (hasAlpha) {
    for (auto& pixel : bits) {
      if (pixel != maskColor) {
        if (useMain)
          pixel = mainColor;
        else if (useBg)
          pixel = bgColor;
      }
    }
  }
  else {
    for (auto& pixel : bits) {
      if (useMain && ((pixel != srcBgColor) || (srcMainColor == srcBgColor))) {
        pixel = mainColor;
      }
      else if (useBg && (pixel == srcBgColor)) {
        pixel = bgColor;
      }
    }
  }
}

void Brush::setImageColor(ImageColor imageColor, color_t color)
{
  ASSERT(m_image);
  if (!m_image)
    return;

  if (!m_backupImage)
    m_backupImage.reset(Image::createCopy(m_image.get()));
  else
    m_image.reset(Image::createCopy(m_backupImage.get()));

  switch (imageColor) {
    case ImageColor::MainColor:
      m_mainColor.reset(new color_t(color));
      break;
    case ImageColor::BackgroundColor:
      m_bgColor.reset(new color_t(color));
      break;
  }

  switch (m_image->pixelFormat()) {

    case IMAGE_RGB:
      replace_image_colors<RgbTraits, rgba_rgb_mask, rgba_a_mask>(
        m_image.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;

    case IMAGE_GRAYSCALE:
      replace_image_colors<GrayscaleTraits, graya_v_mask, graya_a_mask>(
        m_image.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;

    case IMAGE_INDEXED:
      replace_image_colors_indexed(
        m_image.get(),
        (m_mainColor ? true: false), (m_mainColor ? *m_mainColor: 0),
        (m_bgColor ? true: false), (m_bgColor ? *m_bgColor: 0));
      break;
  }
}

// Cleans the brush's data (image and region).
void Brush::clean()
{
  m_gen = ++generation;
  m_image.reset();
  m_backupImage.reset();
}

static void algo_hline(int x1, int y, int x2, void *data)
{
  draw_hline(reinterpret_cast<Image*>(data), x1, y, x2, BitmapTraits::max_value);
}

Image* Brush::image() {
  if (m_genSize != m_size && m_type != kImageBrushType)
    regenerate();
  return m_image.get();
}

Image* Brush::image(float scale)
{
  if (m_type == kImageBrushType)
    return m_image.get();
  auto size = m_size;
  auto bounds = m_bounds;
  m_size = m_size * scale + 0.5f;
  if (m_size <= 0) m_size = 1;
  if (m_size > size) m_size = size;
  if (m_size != m_genSize)
    regenerate();
  m_size = size;
  m_bounds = bounds;
  return m_image.get();
}

// Regenerates the brush bitmap and its rectangle's region.
void Brush::regenerate()
{
  clean();

  ASSERT(m_size > 0);

  m_genSize = m_size;
  int size = m_size;
  if (m_type == kSquareBrushType && m_angle != 0 && m_size > 2)
    size = (int)std::sqrt((double)2*m_size*m_size)+2;

  m_image.reset(Image::create(IMAGE_BITMAP, size, size));

  if (size == 1) {
    clear_image(m_image.get(), BitmapTraits::max_value);
  }
  else {
    clear_image(m_image.get(), BitmapTraits::min_value);

    switch (m_type) {

      case kCircleBrushType:
        fill_ellipse(m_image.get(), 0, 0, size-1, size-1, BitmapTraits::max_value);
        break;

      case kSquareBrushType:
        if (m_angle == 0 || size <= 2) {
          clear_image(m_image.get(), BitmapTraits::max_value);
        }
        else {
          int c = size/2;
          int r = m_size/2;
	  int sa = r * sin(m_angle * (PI / 180.0f)) + 0.5;
	  int ca = r * cos(m_angle * (PI / 180.0f)) + 0.5;
          int x1 = -ca - -sa;
          int y1 = -sa + -ca;
          int x2 =  ca - -sa;
          int y2 =  sa + -ca;
          int x3 =  ca -  sa;
          int y3 =  ca +  sa;
          int x4 = -ca -  sa;
          int y4 = -sa +  ca;
          std::array<std::pair<int,int>, 4> points {
            std::pair<int, int>{x1 + c, y1 + c},
            std::pair<int, int>{x4 + c, y4 + c},
            std::pair<int, int>{x3 + c, y3 + c},
            std::pair<int, int>{x2 + c, y2 + c}
	  };

          doc::algorithm::polygon(points, [&](int x, int y, int x2){
            algo_hline(x, y, x2, m_image.get());
          });
        }
        break;

      case kLineBrushType: {
	int r = m_size/2;
	int sa = r * sin(m_angle * (PI / 180.0)) + 0.5;
	int ca = r * cos(m_angle * (PI / 180.0)) + 0.5;
	int x1 = -ca + r;
	int y1 = -sa + r;
	int x2 =  ca + r;
	int y2 =  sa + r;
        draw_line(m_image.get(), x1, y1, x2, y2, BitmapTraits::max_value);
        break;
      }
    }
  }

  m_bounds = gfx::Rect(
    -m_image->width()/2, -m_image->height()/2,
    m_image->width(), m_image->height());

  m_scaledBounds = m_bounds;
}

} // namespace doc
