// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/remap_pairs.h"

#include "base/split_string.h"

namespace app {

std::string remap_pairs_to_string(const RemapPairs& pairs)
{
  std::string res;
  for (const RemapPair& pair : pairs) {
    if (!res.empty())
      res += " ";
    res += pair.from.toString();
    res += ">";
    res += pair.to.toString();
  }
  return res;
}

RemapPairs remap_pairs_from_string(const std::string& str)
{
  RemapPairs pairs;
  std::vector<std::string> parts;
  base::split_string(str, parts, " ");
  for (const std::string& part : parts) {
    if (int(pairs.size()) >= kMaxRemapPairs)
      break;

    const size_t sep = part.find('>');
    if (sep == std::string::npos || sep == 0 || sep + 1 >= part.size())
      continue;

    pairs.push_back({ app::Color::fromString(part.substr(0, sep)),
                      app::Color::fromString(part.substr(sep + 1)) });
  }
  return pairs;
}

RemapPairs reverse_remap_pairs(const RemapPairs& pairs)
{
  RemapPairs reversed;
  for (const RemapPair& pair : pairs) {
    if (pair.to.getType() != app::Color::MaskType)
      reversed.push_back({ pair.to, pair.from });
  }
  return reversed;
}

void PixelRemapTable::add(doc::color_t from, doc::color_t to)
{
  m_map[normalize(from)] = to;
}

doc::color_t PixelRemapTable::operator()(doc::color_t src) const
{
  auto it = m_map.find(normalize(src));
  return (it != m_map.end() ? it->second : src);
}

doc::color_t PixelRemapTable::normalize(doc::color_t color) const
{
  switch (m_format) {
    case doc::IMAGE_RGB:       return (doc::rgba_geta(color) == 0 ? 0 : color);
    case doc::IMAGE_GRAYSCALE: return (doc::graya_geta(color) == 0 ? 0 : color);
    default:                   return color;
  }
}

} // namespace app
