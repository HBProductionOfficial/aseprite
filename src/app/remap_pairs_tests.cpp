// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/remap_pairs.h"

using namespace app;
using namespace doc;

namespace app {

inline std::ostream& operator<<(std::ostream& os, const RemapPair& pair)
{
  return os << pair.from.toString() << ">" << pair.to.toString();
}

} // namespace app

TEST(RemapPairs, StringRoundTripKeepsEveryColorType)
{
  const RemapPairs pairs = {
    { Color::fromIndex(3), Color::fromIndex(17) },
    { Color::fromRgb(255, 0, 128), Color::fromRgb(10, 20, 30, 200) },
    { Color::fromGray(64), Color::fromMask() },
  };

  EXPECT_EQ(pairs, remap_pairs_from_string(remap_pairs_to_string(pairs)));
}

TEST(RemapPairs, FromStringSkipsMalformedEntries)
{
  const RemapPairs pairs = remap_pairs_from_string("index{4}>index{5} garbage index{6}");

  ASSERT_EQ(1u, pairs.size());
  EXPECT_EQ(Color::fromIndex(4), pairs[0].from);
  EXPECT_EQ(Color::fromIndex(5), pairs[0].to);
}

TEST(RemapPairs, FromStringStopsAtMaxSlots)
{
  std::string str;
  for (int i = 0; i < kMaxRemapPairs + 5; ++i)
    str += "index{1}>index{2} ";

  EXPECT_EQ(size_t(kMaxRemapPairs), remap_pairs_from_string(str).size());
}

TEST(RemapPairs, ReverseSwapsAndSkipsErasingPairs)
{
  const RemapPairs pairs = {
    { Color::fromIndex(1), Color::fromIndex(2) },
    { Color::fromIndex(3), Color::fromMask()   },
  };

  const RemapPairs reversed = reverse_remap_pairs(pairs);

  ASSERT_EQ(1u, reversed.size());
  EXPECT_EQ(Color::fromIndex(2), reversed[0].from);
  EXPECT_EQ(Color::fromIndex(1), reversed[0].to);
}

TEST(PixelRemapTable, IndexedMapsOnlyListedIndexes)
{
  PixelRemapTable table(IMAGE_INDEXED);
  table.add(5, 9);

  EXPECT_EQ(9u, table(5));
  EXPECT_EQ(6u, table(6));
}

TEST(PixelRemapTable, LastPairWinsForTheSameSource)
{
  PixelRemapTable table(IMAGE_INDEXED);
  table.add(5, 9);
  table.add(5, 11);

  EXPECT_EQ(11u, table(5));
}

TEST(PixelRemapTable, RgbComparesExactColors)
{
  PixelRemapTable table(IMAGE_RGB);
  table.add(rgba(255, 0, 0, 255), rgba(0, 0, 255, 255));

  EXPECT_EQ(rgba(0, 0, 255, 255), table(rgba(255, 0, 0, 255)));
  EXPECT_EQ(rgba(254, 0, 0, 255), table(rgba(254, 0, 0, 255)));
}

TEST(PixelRemapTable, RgbTransparentPixelsMatchWhateverTheirColor)
{
  PixelRemapTable table(IMAGE_RGB);
  table.add(rgba(0, 0, 0, 0), rgba(1, 2, 3, 255));

  EXPECT_EQ(rgba(1, 2, 3, 255), table(rgba(255, 0, 255, 0)));
}

TEST(PixelRemapTable, GrayscaleTransparentPixelsMatchWhateverTheirValue)
{
  PixelRemapTable table(IMAGE_GRAYSCALE);
  table.add(graya(0, 0), graya(200, 255));

  EXPECT_EQ(graya(200, 255), table(graya(77, 0)));
  EXPECT_EQ(graya(77, 255), table(graya(77, 255)));
}

TEST(PixelRemapTable, EmptyTableLeavesPixelsAlone)
{
  PixelRemapTable table(IMAGE_RGB);

  EXPECT_TRUE(table.empty());
  EXPECT_EQ(rgba(9, 9, 9, 9), table(rgba(9, 9, 9, 9)));
}
