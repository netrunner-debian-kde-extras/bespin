/* Bespin widget style for Qt4
   Copyright (C) 2007 Thomas Luebking <thomas.luebking@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "draw.h"

inline static bool verticalTabs(QTabBar::Shape shape) {
   return shape == QTabBar::RoundedEast ||
      shape == QTabBar::TriangularEast ||
      shape == QTabBar::RoundedWest ||
      shape == QTabBar::TriangularWest;
}

void
BespinStyle::drawTabWidget(const QStyleOption *option, QPainter *painter,
                           const QWidget * widget) const
{
   ASSURE_OPTION(twf, TabWidgetFrame);

   QLine line[2];
   QStyleOptionTabBarBase tbb; tbb.initFrom(widget);
   tbb.shape = twf->shape; tbb.rect = twf->rect;
       
#define SET_BASE_HEIGHT(_o_) \
   baseHeight = twf->tabBarSize._o_(); \
   if (baseHeight < 0) \
      baseHeight = pixelMetric( PM_TabBarBaseHeight, option, widget )
          
   int baseHeight;
   switch (twf->shape) {
   case QTabBar::RoundedNorth: case QTabBar::TriangularNorth:
      SET_BASE_HEIGHT(height);
      tbb.rect.setHeight(baseHeight);
      line[0] = line[1] = QLine(RECT.bottomLeft(), RECT.bottomRight());
      line[0].translate(0,-1);
      break;
   case QTabBar::RoundedSouth: case QTabBar::TriangularSouth:
      SET_BASE_HEIGHT(height);
      tbb.rect.setTop(tbb.rect.bottom()-baseHeight);
      line[0] = line[1] = QLine(RECT.topLeft(), RECT.topRight());
      line[1].translate(0,1);
      break;
   case QTabBar::RoundedEast: case QTabBar::TriangularEast:
      SET_BASE_HEIGHT(width);
      tbb.rect.setLeft(tbb.rect.right()-baseHeight);
      line[0] = line[1] = QLine(RECT.topLeft(), RECT.bottomLeft());
      line[1].translate(1,0);
      break;
   case QTabBar::RoundedWest: case QTabBar::TriangularWest:
      SET_BASE_HEIGHT(width);
      tbb.rect.setWidth(baseHeight);
      line[0] = line[1] = QLine(RECT.topRight(), RECT.bottomRight());
      line[0].translate(-1,0);
      break;
   }
#undef SET_BASE_HEIGHT
       
   // the "frame"
   painter->save();
   painter->setPen(FCOLOR(Window).dark(120));
   painter->drawLine(line[0]);
   painter->setPen(FCOLOR(Window).light(114));
   painter->drawLine(line[1]);
   painter->restore();

   // the bar
   drawTabBar(&tbb, painter, widget);
}

void
BespinStyle::drawTabBar(const QStyleOption *option, QPainter *painter,
                        const QWidget * widget) const
{
   ASSURE_OPTION(tbb, TabBarBase);
   if (widget && (qobject_cast<const QTabBar*>(widget)))
      return; // we alter the paintevent
   if (widget->parentWidget() && qobject_cast<QTabWidget*>(widget->parentWidget()))
      return; // KDE abuse, allready has a nice base
       
   QRect rect = RECT.adjusted(dpi.f2, 0, -dpi.f2, -dpi.f2);
   int size = RECT.height(); Qt::Orientation o = Qt::Vertical;

   if (verticalTabs(tbb->shape)) {
      o = Qt::Horizontal; size = RECT.width();
   }
   masks.tab.render(rect, painter, GRAD(tab), o, CCOLOR(tab.std, Bg), size);
   rect.setBottom(rect.bottom()+dpi.f2);
   shadows.tabSunken.render(rect, painter);
}

void
BespinStyle::drawTab(const QStyleOption *option, QPainter *painter,
                     const QWidget * widget) const
{
   ASSURE_OPTION(tab, Tab);

   // do we have to exclude the scrollers?
   bool needRestore = false;
   if (widget && (RECT.right() > widget->width())) {
      painter->save();
      needRestore = true;
      QRect r = RECT;
      r.setRight(widget->width() -
                 2*pixelMetric(PM_TabBarScrollButtonWidth,option,widget));
      painter->setClipRect(r);
   }
   
   // paint shape and label
   QStyleOptionTab copy = *tab;
   copy.rect.setBottom(copy.rect.bottom()-dpi.f2);
   drawTabShape(&copy, painter, widget);
   drawTabLabel(&copy, painter, widget);
   if (needRestore)
      painter->restore();
}

static int animStep = -1;

void
BespinStyle::drawTabShape(const QStyleOption *option, QPainter *painter,
                          const QWidget * widget) const
{
   ASSURE_OPTION(tab, Tab);
   B_STATES
   sunken = sunken || (option->state & State_Selected);

   animStep = 0;
   // animation stuff
   if (isEnabled && !sunken) {
      IndexedFadeInfo *info = 0;
      int index = -1, hoveredIndex = -1;
      if (widget)
      if (const QTabBar* tbar =
         qobject_cast<const QTabBar*>(widget)) {
         // NOTICE: the index increment is IMPORTANT to make sure it's no "0"
         index = tbar->tabAt(RECT.topLeft()) + 1; // is the action for this item!
         hoveredIndex = hover ? index :
            tbar->tabAt(tbar->mapFromGlobal(QCursor::pos())) + 1;
         info = const_cast<IndexedFadeInfo *>
            (animator->fadeInfo(widget, hoveredIndex));
      }
      if (info)
         animStep = info->step(index);
      if (hover && !animStep) animStep = 6;
   }
       
   // maybe we're done here?!
   if (!(animStep || sunken)) return;
       
   const int f2 = dpi.f2;
   QRect rect = RECT.adjusted(dpi.f3, dpi.f5, -dpi.f4, -dpi.f5);
   int size = RECT.height()-f2;
   Qt::Orientation o = Qt::Vertical;

   if (verticalTabs(tab->shape)) {
      o = Qt::Horizontal;
      size = RECT.width()-f2;
   }

   QColor c;
   int d = 0;
   if (sunken) {
      rect.adjust(f2, -dpi.f1, -f2, dpi.f1);
      c = CCOLOR(tab.active, 0);
      d = (o == Qt::Vertical) ? -dpi.f1 : f2;
   }
   else {
      c = CCOLOR(tab.std, Bg);
      int quota = 6 + (int) (.16 * Colors::contrast(c, CCOLOR(tab.active, 0)));
      c = Colors::mid(c, CCOLOR(tab.active, 0), quota, animStep);
   }
   const QPoint off(d, d+dpi.f4);
   masks.tab.render(rect, painter, GRAD(tab), o, c, size, off);
   if (config.tab.activeTabSunken && sunken) {
      rect.setBottom(rect.bottom()+f2);
      shadows.tabSunken.render(rect, painter);
   }
}

void
BespinStyle::drawTabLabel(const QStyleOption *option, QPainter *painter,
                          const QWidget *) const
{
   ASSURE_OPTION(tab, Tab);
   B_STATES

   painter->save();
   QStyleOptionTabV2 tabV2(*tab);
   QRect tr = tabV2.rect;
   
   bool verticalTabs = false;
   bool east = false;
   bool selected = tabV2.state & State_Selected;
   if (selected) hover = false;
   if (config.tab.activeTabSunken && (selected || sunken))
      tr.translate(0,dpi.f1);
   
   int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;

   switch(tab->shape) {
   case QTabBar::RoundedEast: case QTabBar::TriangularEast:
      east = true;
   case QTabBar::RoundedWest: case QTabBar::TriangularWest:
      verticalTabs = true;
      break;
   default:
      break;
   }
       
   if (verticalTabs) {
      int newX, newY, newRot;
      if (east) {
         newX = tr.width(); newY = tr.y(); newRot = 90;
      }
      else {
         newX = 0; newY = tr.y() + tr.height(); newRot = -90;
      }
      tr.setRect(0, 0, tr.height(), tr.width());
      QMatrix m;
      m.translate(newX, newY); m.rotate(newRot);
      painter->setMatrix(m, true);
   }
       
   if (!tabV2.icon.isNull()) {
      QSize iconSize = tabV2.iconSize;
      if (!iconSize.isValid()) {
         int iconExtent = pixelMetric(PM_SmallIconSize);
         iconSize = QSize(iconExtent, iconExtent);
      }
      QPixmap tabIcon = tabV2.icon.pixmap(iconSize, (isEnabled) ?
                                          QIcon::Normal : QIcon::Disabled);
      painter->drawPixmap(tr.left() + dpi.f6,
                        tr.center().y() - tabIcon.height() / 2, tabIcon);
      tr.setLeft(tr.left() + iconSize.width() + dpi.f4);
   }
       
   // color adjustment
   QColor cF, cB;
   if (selected || sunken) {
      cF = CCOLOR(tab.active, 1);
      cB = CCOLOR(tab.active, 0);
   }
   else if (hover || animStep > 2) {
      cF = CCOLOR(tab.std, 1);
      cB = Colors::mid(CCOLOR(tab.std ,0 ), FCOLOR(Window), 2, 1);
      cB = Colors::mid(cB, CCOLOR(tab.active, 0));
      if (Colors::contrast(CCOLOR(tab.active, 1), cB) > Colors::contrast(cF, cB))
         cF = CCOLOR(tab.active, 1);
   }
   else {
      cB = Colors::mid(CCOLOR(tab.std, 0), FCOLOR(Window), 2, 1);
      cF = Colors::mid(cB, CCOLOR(tab.std, 1), 1,4);
   }

   // dark background, let's paint an emboss
   if (isEnabled) {
      painter->setPen(cB.dark(120));
      tr.moveTop(tr.top()-1);
      drawItemText(painter, tr, alignment, PAL, isEnabled, tab->text);
      tr.moveTop(tr.top()+1);
   }
   painter->setPen(cF);
   drawItemText(painter, tr, alignment, PAL, isEnabled, tab->text);

   painter->restore();
   animStep = -1;
}

void
BespinStyle::drawToolboxTab(const QStyleOption *option, QPainter *painter,
                            const QWidget * widget) const
{
   ASSURE_OPTION(tbt, ToolBox);

   // color fix...
   if (widget && widget->parentWidget())
      const_cast<QStyleOption*>(option)->palette =
      widget->parentWidget()->palette();

   drawToolboxTabShape(tbt, painter, widget);
   QStyleOptionToolBox copy = *tbt;
   copy.rect.setBottom(copy.rect.bottom()-dpi.f2);
   drawToolboxTabLabel(&copy, painter, widget);
}

void
BespinStyle::drawToolboxTabShape(const QStyleOption *option, QPainter *painter,
                                 const QWidget *) const
{
   B_STATES
      
   QRect r = RECT; Tile::PosFlags pf = Tile::Full;
   if (const QStyleOptionToolBoxV2* tbt =
       qstyleoption_cast<const QStyleOptionToolBoxV2*>(option)) {
      switch (tbt->position) {
      case QStyleOptionToolBoxV2::Beginning:
         pf &= ~Tile::Bottom;
         break;
      case QStyleOptionToolBoxV2::Middle:
         pf &= ~(Tile::Top | Tile::Bottom);
         break;
      case QStyleOptionToolBoxV2::End:
         pf &= ~Tile::Top;
      default:
         r.setBottom(r.bottom()-dpi.f2);
      }
      if (option->state & State_Selected) {
         pf |= Tile::Bottom;
         r.setBottom(RECT.bottom()-dpi.f2);
      }
      else if (tbt->selectedPosition == // means we touch the displayed box bottom
               QStyleOptionToolBoxV2::PreviousIsSelected)
         pf |= Tile::Top;
   }

   const bool selected = option->state & State_Selected;
   Tile::setShape(pf);
   if (selected || hover || sunken) {
      const QColor &c = selected ?
         CCOLOR(toolbox.active, Bg) : FCOLOR(Window);
      Gradients::Type gt = selected ? GRAD(toolbox) :
         (sunken ? Gradients::Sunken : Gradients::Button);
      masks.tab.render(r, painter, gt, Qt::Vertical, c);
   }
   else
      masks.tab.render(r, painter, FCOLOR(Window).dark(108));
   Tile::setShape(pf & ~Tile::Center);
   shadows.tabSunken.render(RECT, painter);
   Tile::reset();
}

void
BespinStyle::drawToolboxTabLabel(const QStyleOption *option, QPainter *painter,
                                 const QWidget *) const
{
   ASSURE_OPTION(tbt, ToolBox);
   B_STATES
   const bool selected = option->state & (State_Selected);

   QColor cB = FCOLOR(Window), cF = FCOLOR(WindowText);
   painter->save();
   if (selected) {
      cB = CCOLOR(toolbox.active, Bg);
      cF = CCOLOR(toolbox.active, Fg);
      QFont tmpFnt = painter->font(); tmpFnt.setBold(true);
      painter->setFont(tmpFnt);
   }

   // on dark background, let's paint an emboss
   if (isEnabled) {
      QRect tr = RECT;
      painter->setPen(cB.dark(120));
      tr.moveTop(tr.top()-1);
      drawItemText(painter, tr, Qt::AlignCenter | Qt::TextShowMnemonic,
                  PAL, isEnabled, tbt->text);
      tr.moveTop(tr.top()+1);
   }
   painter->setPen(cF);
   drawItemText(painter, RECT, Qt::AlignCenter | Qt::TextShowMnemonic,
               PAL, isEnabled, tbt->text);
   painter->restore();
}
