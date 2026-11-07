// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_REMAP_PAIRS_H_INCLUDED
#define APP_REMAP_PAIRS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "doc/color.h"
#include "doc/pixel_format.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace app {

// Pixels of "from" become "to". A transparent "to" erases them.
struct RemapPair {
  app::Color from;
  app::Color to;

  bool operator==(const RemapPair& other) const { return from == other.from && to == other.to; }
};

using RemapPairs = std::vector<RemapPair>;

// Maximum number of slots, same as the Remap pen of Fighter Factory.
constexpr int kMaxRemapPairs = 32;

// Format: "from>to from>to ..." with app::Color::toString() colors.
std::string remap_pairs_to_string(const RemapPairs& pairs);
RemapPairs remap_pairs_from_string(const std::string& str);

// Pairs for the right mouse button (to -> from). Erasing pairs are
// skipped since there's no color to bring back.
RemapPairs reverse_remap_pairs(const RemapPairs& pairs);

// Pixel lookup for one image format. add() takes colors in that format.
class PixelRemapTable {
public:
  explicit PixelRemapTable(doc::PixelFormat format) : m_format(format) {}

  // When two pairs share a "from" color the last one wins.
  void add(doc::color_t from, doc::color_t to);

  doc::color_t operator()(doc::color_t src) const;

  bool empty() const { return m_map.empty(); }

private:
  // Fully transparent pixels match each other whatever their RGB/gray values.
  doc::color_t normalize(doc::color_t color) const;

  doc::PixelFormat m_format;
  std::unordered_map<doc::color_t, doc::color_t> m_map;
};

} // namespace app

#endif
