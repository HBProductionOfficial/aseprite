// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_REMAP_SLOTS_H_INCLUDED
#define APP_UI_REMAP_SLOTS_H_INCLUDED
#pragma once

#include "app/remap_pairs.h"
#include "obs/signal.h"
#include "ui/widget.h"

namespace app {

// Remap ink pairs as columns: color to replace on top, replacement
// below. The last column adds a pair.
class RemapSlots : public ui::Widget {
public:
  enum Mode { Editable, ClickWhole };

  RemapSlots(const RemapPairs& pairs, Mode mode);

  const RemapPairs& pairs() const { return m_pairs; }
  void setPairs(const RemapPairs& pairs);

  void addPair(const app::Color& from, const app::Color& to);
  void swapRows();

  // Emitted when the user edits the pairs (Editable) or clicks the
  // widget (ClickWhole).
  obs::signal<void()> Change;

private:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

  int columns() const;
  gfx::Rect slotsBounds() const;
  gfx::Rect cellBounds(int column, int row) const;
  void setCell(int column, int row, const app::Color& color);
  void showCellMenu(int column, int row);
  void changed();

  Mode m_mode;
  RemapPairs m_pairs;
  int m_hotColumn = -1;
  int m_hotRow = -1;
};

} // namespace app

#endif
