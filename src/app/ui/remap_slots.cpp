// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/remap_slots.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/modules/gfx.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/color_mode.h"
#include "ui/graphics.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

#include <algorithm>

namespace app {

using namespace ui;

namespace {

constexpr int kColumnWidth = 10;

// HSV/HSL colors are stored with two decimals, so they could not be
// compared exactly after a restart. Pairs keep RGB instead.
app::Color slot_color(const app::Color& color)
{
  switch (color.getType()) {
    case app::Color::HsvType:
    case app::Color::HslType:
      return app::Color::fromRgb(color.getRed(),
                                 color.getGreen(),
                                 color.getBlue(),
                                 color.getAlpha());
    default: return color;
  }
}

app::Color fg_color()
{
  return slot_color(Preferences::instance().colorBar.fgColor());
}

app::Color bg_color()
{
  return slot_color(Preferences::instance().colorBar.bgColor());
}

} // namespace

RemapSlots::RemapSlots(const RemapPairs& pairs, Mode mode)
  : Widget(kGenericWidget)
  , m_mode(mode)
  , m_pairs(pairs)
{
  setText(Strings::remap_slots_empty_hint());
  initTheme();
}

void RemapSlots::setPairs(const RemapPairs& pairs)
{
  m_pairs = pairs;
  if (int(m_pairs.size()) > kMaxRemapPairs)
    m_pairs.resize(kMaxRemapPairs);
  invalidate();
  if (parent() && parent()->parent())
    parent()->parent()->layout();
}

void RemapSlots::addPair(const app::Color& from, const app::Color& to)
{
  if (int(m_pairs.size()) >= kMaxRemapPairs)
    return;
  m_pairs.push_back({ slot_color(from), slot_color(to) });
  changed();
}

// Erasing pairs stay as they are: swapped they would fill every
// transparent pixel under the brush.
void RemapSlots::swapRows()
{
  for (RemapPair& pair : m_pairs) {
    if (pair.to.getType() != app::Color::MaskType)
      std::swap(pair.from, pair.to);
  }
  changed();
}

void RemapSlots::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  auto theme = skin::SkinTheme::get(this);
  setStyle(m_mode == Editable ? theme->styles.normalShadeView() : theme->styles.menuShadeView());
}

bool RemapSlots::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kSetCursorMessage:
      if (m_mode == Editable && m_hotColumn >= 0) {
        set_mouse_cursor(kHandCursor);
        return true;
      }
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      m_hotColumn = m_hotRow = -1;
      invalidate();
      break;

    case kMouseMoveMessage: {
      const gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position() - bounds().origin();
      int column = -1, row = -1;
      for (int c = 0; c < columns() && column < 0; ++c) {
        for (int r = 0; r < 2; ++r) {
          if (cellBounds(c, r).contains(mousePos)) {
            column = c;
            row = r;
            break;
          }
        }
      }
      if (column != m_hotColumn || row != m_hotRow) {
        m_hotColumn = column;
        m_hotRow = row;
        invalidate();
      }
      break;
    }

    case kMouseDownMessage: {
      if (m_mode != Editable || m_hotColumn < 0)
        break;

      const MouseButton button = static_cast<MouseMessage*>(msg)->button();
      const bool isPair = (m_hotColumn < int(m_pairs.size()));
      if (button == kButtonLeft) {
        if (isPair)
          setCell(m_hotColumn, m_hotRow, fg_color());
        else
          addPair(fg_color(), bg_color());
      }
      else if (button == kButtonRight && isPair) {
        showCellMenu(m_hotColumn, m_hotRow);
      }
      return true;
    }

    case kMouseUpMessage:
      if (m_mode == ClickWhole) {
        setSelected(true);
        Change();
        closeWindow();
        return true;
      }
      break;
  }
  return Widget::onProcessMessage(msg);
}

void RemapSlots::onSizeHint(SizeHintEvent& ev)
{
  const int scale = guiscale();
  int w = (6 + kColumnWidth * std::max(1, columns())) * scale;
  if (m_mode == Editable && m_pairs.empty())
    w += 4 * scale + textWidth();
  ev.setSizeHint(gfx::Size(w, 18 * scale));
}

void RemapSlots::onPaint(PaintEvent& ev)
{
  auto theme = skin::SkinTheme::get(this);
  Graphics* g = ev.graphics();
  const auto colorMode = doc::ColorMode(app_get_current_pixel_format());

  theme->paintWidget(g, this, style(), clientBounds());

  for (int c = 0; c < columns(); ++c) {
    const bool isPair = (c < int(m_pairs.size()));
    for (int r = 0; r < 2; ++r) {
      const app::Color color = (isPair ? (r == 0 ? m_pairs[c].from : m_pairs[c].to) :
                                         app::Color::fromMask());
      draw_color(g, cellBounds(c, r), color, colorMode);
    }

    // The "add" column gets a plus sign across both cells.
    if (!isPair) {
      const gfx::Rect rc = cellBounds(c, 0).createUnion(cellBounds(c, 1));
      const int s = guiscale();
      const gfx::Point mid = rc.center();
      g->fillRect(theme->colors.text(), gfx::Rect(mid.x - 3 * s, mid.y - s / 2, 6 * s, s));
      g->fillRect(theme->colors.text(), gfx::Rect(mid.x - s / 2, mid.y - 3 * s, s, 6 * s));
    }
  }

  if (m_mode == Editable && m_hotColumn >= 0 && m_hotRow >= 0) {
    gfx::Rect hot = cellBounds(m_hotColumn, m_hotRow);
    hot.enlarge(2 * guiscale());
    PaintWidgetPartInfo info;
    theme->paintWidgetPart(g, theme->styles.shadeSelection(), hot, info);
  }

  if (m_mode == Editable && m_pairs.empty()) {
    gfx::Rect textBounds = slotsBounds();
    textBounds.x += kColumnWidth * guiscale() + 4 * guiscale();
    textBounds.w = clientBounds().x2() - textBounds.x;
    PaintWidgetPartInfo info;
    const std::string& text = this->text();
    info.text = &text;
    info.baseline = textBaseline();
    theme->paintWidgetPart(g, theme->styles.shadeEmpty(), textBounds, info);
  }
}

int RemapSlots::columns() const
{
  const int n = int(m_pairs.size());
  return (m_mode == Editable && n < kMaxRemapPairs ? n + 1 : n);
}

gfx::Rect RemapSlots::slotsBounds() const
{
  gfx::Rect rc = clientBounds();
  rc.shrink(3 * guiscale());
  return rc;
}

gfx::Rect RemapSlots::cellBounds(int column, int row) const
{
  const gfx::Rect area = slotsBounds();
  const int s = guiscale();
  const int topH = area.h / 2;
  // Leave a one-pixel (scaled) gap between neighboring slots.
  return gfx::Rect(area.x + column * kColumnWidth * s,
                   row == 0 ? area.y : area.y + topH,
                   kColumnWidth * s - s,
                   row == 0 ? topH : area.h - topH);
}

void RemapSlots::setCell(int column, int row, const app::Color& color)
{
  if (column < 0 || column >= int(m_pairs.size()))
    return;
  (row == 0 ? m_pairs[column].from : m_pairs[column].to) = slot_color(color);
  changed();
}

void RemapSlots::showCellMenu(int column, int row)
{
  Menu menu;
  MenuItem useFg(Strings::remap_slots_use_foreground());
  MenuItem useBg(Strings::remap_slots_use_background());
  MenuItem transparent(Strings::remap_slots_transparent());
  MenuSeparator separator;
  MenuItem remove(Strings::remap_slots_remove_pair());
  menu.addChild(&useFg);
  menu.addChild(&useBg);
  menu.addChild(&transparent);
  menu.addChild(&separator);
  menu.addChild(&remove);

  useFg.Click.connect([this, column, row] { setCell(column, row, fg_color()); });
  useBg.Click.connect([this, column, row] { setCell(column, row, bg_color()); });
  transparent.Click.connect([this, column, row] { setCell(column, row, app::Color::fromMask()); });
  remove.Click.connect([this, column] {
    m_pairs.erase(m_pairs.begin() + column);
    m_hotColumn = m_hotRow = -1;
    changed();
  });

  const gfx::Rect cell = cellBounds(column, row).offset(bounds().origin());
  menu.showPopup(gfx::Point(cell.x, bounds().y2()), display());
}

void RemapSlots::changed()
{
  invalidate();
  if (parent() && parent()->parent())
    parent()->parent()->layout();
  Change();
}

} // namespace app
