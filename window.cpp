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

//    case PE_PanelTipLabel: // The panel for a tip label.

#include <QApplication>
#include <QDesktopWidget>
#include <QtDebug>
#include "draw.h"

#ifdef Q_WS_X11
#include "blib/FX.h"
#include "blib/xproperty.h"
#else
#define QT_NO_XRENDER #
#endif

void
Style::drawWindowFrame(const QStyleOption * option, QPainter * painter, const QWidget *) const
{
    // windows, docks etc. - just a frame
    const QColor border = Colors::mid(FCOLOR(Window), FCOLOR(WindowText), 5,2);
    const int right = RECT.right()-(32+4);
    const int bottom = RECT.bottom()-(32+4);
    QPen pen = painter->pen();
    painter->setPen(border);
    painter->drawLine(32+4, 0, right, 0);
    painter->drawLine(32+4, RECT.bottom(), right, RECT.bottom());
    painter->drawLine(0, 32+4, 0, bottom);
    painter->drawLine(RECT.right(), 32+4, RECT.right(), bottom);

    const QPixmap &top = Gradients::borderline(border, Gradients::Top);
    painter->drawPixmap(0,4, top);
    painter->drawPixmap(RECT.right(), 4, top);

    const QPixmap &btm = Gradients::borderline(border, Gradients::Bottom);
    painter->drawPixmap(0, bottom, btm);
    painter->drawPixmap(RECT.right(), bottom, btm);

    const QPixmap &left = Gradients::borderline(border, Gradients::Left);
    painter->drawPixmap(4, 0, left);
    painter->drawPixmap(4, RECT.bottom(), left);

    const QPixmap &rgt = Gradients::borderline(border, Gradients::Right);
    painter->drawPixmap(right, 0, rgt);
    painter->drawPixmap(right, RECT.bottom(), rgt);
}

static QPainterPath glasPath;
static QSize glasSize;

static QPixmap *rings = 0L;
#include <QTimer>
static QTimer ringResetTimer;
static inline void
createRingPix(int alpha, int value)
{
    QPainterPath ringPath;
    ringPath.addEllipse(0,0,200,200);
    ringPath.addEllipse(30,30,140,140);

    ringPath.addEllipse(210,10,230,230);
    ringPath.addEllipse(218,18,214,214);
    ringPath.addEllipse(226,26,198,198);
    ringPath.addEllipse(234,34,182,182);
    ringPath.addEllipse(300,100,50,50);

    ringPath.addEllipse(100,96,160,160);
    ringPath.addEllipse(108,104,144,144);
    ringPath.addEllipse(116,112,128,128);
    ringPath.addEllipse(122,120,112,112);

    ringPath.addEllipse(250,160,200,200);
    ringPath.addEllipse(280,190,140,140);
    ringPath.addEllipse(310,220,80,80);

    rings = new QPixmap(450,360);
    rings->fill(Qt::transparent);
    QPainter p(rings);
    QColor white(value,value,value,(alpha+16)*112/255);
    p.setPen(white);
    white.setAlpha(24*(alpha+16)/255);
    p.setBrush(white);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPath(ringPath);
    p.end();
}
void
Style::resetRingPix()
{
    ringResetTimer.stop();
    delete rings; rings = 0L;
}

void
Style::drawWindowBg(const QStyleOption*, QPainter *painter, const QWidget *widget) const
{
    if (!(widget && widget->isWindow()))
        return; // can't do anything here
//     if ( widget->windowFlags() & (Qt::SplashScreen & ~Qt::Window) ) // err... no. splashscreens want their own bg?
//         return;

//     if (widget->testAttribute(Qt::WA_NoSystemBackground))
//         return; // those shall be translucent - but should be catched by Qt

    const QPalette &pal = widget->palette();
    if (pal.brush(widget->backgroundRole()).style() > 1)
        return; // we'd cover a gradient/pixmap/whatever

    QColor c = pal.color(widget->backgroundRole());
    if (c == Qt::transparent) // plasma uses this
        return;

    const bool isPopup = widget->windowFlags() & (Qt::Popup & ~Qt::Window);
    const int opacity = isPopup ? config.menu.opacity : config.bg.opacity;

#if BESPIN_ARGB_WINDOWS
    if (opacity < c.alpha())
        c.setAlpha(opacity);
#endif
    bool translucent = false;
    if (c.alpha() < 0xff)
    {
#if QT_VERSION >= 0x040500
        if (widget->testAttribute(Qt::WA_TranslucentBackground))
            translucent = true;
        else
#endif
            c.setAlpha(0xff);
    }

    bool drawRings = false;
    if (config.bg.ringOverlay)
    {
        drawRings = !isPopup;
        if (drawRings && !rings)
        {
            const int ringValue = qMin(3*Colors::value(pal.color(widget->backgroundRole()))/2, 255);
            createRingPix(opacity, ringValue);
            disconnect(&ringResetTimer, SIGNAL(timeout()), this, SLOT(resetRingPix()));
            connect(&ringResetTimer, SIGNAL(timeout()), this, SLOT(resetRingPix()));
        }
        ringResetTimer.start(5000);
    }

    // glassy Modal dialog/Popup menu ==========
    // we just kinda abuse this mac only attribute... ;P
    if (widget->testAttribute(Qt::WA_MacBrushedMetal))
    {
        if (widget->size() != glasSize)
        {
            const QRect &wr = widget->rect();
            glasSize = widget->size();
            glasPath = QPainterPath();
            glasPath.moveTo(wr.topLeft());
            glasPath.lineTo(wr.topRight());
            glasPath.quadTo(wr.center()/2, wr.bottomLeft());
        }
        painter->save();
        painter->setPen(Qt::NoPen);
        const int v = Colors::value(c);
        if (c.alpha() < 0xff)
        {
            const int alpha = c.alpha()*v / (255*(7-v/80));
            painter->setBrush(QColor(255,255,255,alpha));
        }
        else
            painter->setBrush(c.light(115-v/20));
        painter->drawPath(glasPath);

        if (drawRings)
            painter->drawPixmap(widget->width()-450, 0, *rings);

        painter->restore();
        return;
    }

    // cause of scrollbars - kinda optimization
    if (config.bg.mode == Plain)
    {
        if (drawRings)
            painter->drawPixmap(widget->width()-450, 0, *rings);
        if (config.shadowTitlebar)
        {
            const QPixmap &shadow = shadows.sunken[false][true].tile(Tile::TopMid);
            painter->drawTiledPixmap( 0,0, widget->width(), shadow.height(), shadow );
        }
        return;
    }

    if (config.bg.mode == Scanlines)
    {
        const bool light = (widget->windowFlags() & ((Qt::Tool | Qt::Popup) & ~Qt::Window));
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(Gradients::structure(c, light));
        painter->drawRect(widget->rect());
        if (config.shadowTitlebar)
        {
            const QPixmap &shadow = shadows.sunken[false][true].tile(Tile::TopMid);
            painter->drawTiledPixmap( 0,0, widget->width(), shadow.height(), shadow );
        }
        painter->restore();
        return;
    }

#if BESPIN_ARGB_WINDOWS
    if (translucent)
    {
        painter->fillRect(widget->rect(), c);
        c = Qt::transparent;
    }
#endif

    // Complex part ===================
    const BgSet &set = Gradients::bgSet(c);
    QRect rect = widget->rect();
#ifndef QT_NO_XRENDER
    uint *decoDimP = XProperty::get<uint>(widget->winId(), XProperty::decoDim, XProperty::LONG);
    if (decoDimP)
    {
        uint decoDim = *decoDimP;
        WindowPics pics;
        if (FX::usesXRender())
        {
            pics.topTile = set.topTile.x11PictureHandle();
            pics.btmTile = set.btmTile.x11PictureHandle();
            pics.cnrTile = set.cornerTile.x11PictureHandle();
            pics.lCorner = set.lCorner.x11PictureHandle();
            pics.rCorner = set.rCorner.x11PictureHandle();
        }
        else
        {
            pics.topTile = pics.cnrTile = pics.lCorner = pics.rCorner = 0;
            /// NOTICE encoding the bg gradient intensity in the btmTile Pic!!
            pics.btmTile = config.bg.intensity;
        }
        XProperty::set<Picture>(widget->winId(), XProperty::bgPics, (Picture*)&pics, XProperty::LONG, 5);
        rect.adjust(-((decoDim >> 24) & 0xff), -((decoDim >> 16) & 0xff), (decoDim >> 8) & 0xff, decoDim & 0xff);
    }
#endif

    switch (config.bg.mode)
    {
    case BevelV:
    {   // also fallback for ComplexLights
        const bool hadClip = painter->hasClipping();
        const QRegion oldClip = (hadClip) ? painter->clipRegion() : QRegion();
        int s1 = set.topTile.height();
        int s2 = qMin(s1, (rect.height()+1)/2);
        s1 -= s2;
        if (!translucent && Colors::value(c) < 245)
        {   // no sense otherwise
            const int w = rect.width()/4 - 128;
            const int s3 = 128-s1;
            if (w > 0)
            {
                painter->drawTiledPixmap( rect.x(), rect.y(), w, s3, set.cornerTile, 0, s1 );
                painter->drawTiledPixmap( rect.right()+1-w, rect.y(), w, s3, set.cornerTile, 0, s1 );
            }
            painter->drawPixmap(rect.x()+w, rect.y(), set.lCorner, 0, s1, 128, s3);
            painter->drawPixmap(rect.right()-w-127, rect.y(), set.rCorner, 0, s1, 128, s3);
            QRegion newClip(rect.x(), rect.y(), rect.width(), s2);
            newClip -= QRegion(rect.x(), rect.y(), w+128, s3);
            newClip -= QRegion(rect.right()-w-127, rect.y(), w+128, s3);
            painter->setClipping(true);
            painter->setClipRegion(newClip, Qt::IntersectClip);
        }
        painter->drawTiledPixmap( rect.x(), rect.y(), rect.width(), s2, set.topTile, 0, s1 );
        painter->setClipRegion(oldClip);
        painter->setClipping(hadClip);
        s1 = set.btmTile.height();
        s2 = qMin(s1, (rect.height())/2);
        painter->drawTiledPixmap( rect.x(), rect.bottom() - s2, rect.width(), s2, set.btmTile );
        break;
    }
    case BevelH:
    {
        int s1 = set.topTile.width();
        int s2 = qMin(s1, (rect.width()+1)/2);
        const int h = qMin(128+32, rect.height()/8);
        const int y = rect.y()+h;
        painter->drawTiledPixmap( rect.x(), y, s2, rect.height()-h, set.topTile, s1-s2, 0 );
        painter->drawPixmap(rect.x(), y-32, set.lCorner, s1-s2, 0,0,0);
        s1 = set.btmTile.width();
        s2 = qMin(s1, (rect.width())/2);
        painter->drawTiledPixmap( rect.right() + 1 - s2, y , s2, rect.height()-h, set.btmTile );
        painter->drawPixmap(rect.right() + 1 - s2, y-32, set.rCorner);
        painter->drawTiledPixmap( rect.x(), y-(128+32), rect.width(), 128, set.cornerTile );
        break;
    }
//    case Plain: // should not happen anyway...
//    case Scanlines: // --"--
    default:
        break;
    }
    if (drawRings)
        painter->drawPixmap(widget->width()-450, 0, *rings);
    if (config.shadowTitlebar)
    {
        const QPixmap &shadow = shadows.sunken[false][true].tile(Tile::TopMid);
        painter->drawTiledPixmap( 0,0, widget->width(), shadow.height(), shadow );
    }
}

void
Style::drawToolTip(const QStyleOption * option, QPainter * painter, const QWidget *) const
{
   painter->save();
   
//    painter->setBrush(Gradients::pix(FCOLOR(ToolTipBase), RECT.height(), Qt::Vertical, Gradients::Button));
   painter->setBrush(FCOLOR(ToolTipBase));
//    painter->setPen(Qt::NoPen);
//    painter->drawRect(RECT);
   const int f1 = F(1);
//    QPen pen(Colors::mid(FCOLOR(ToolTipBase), FCOLOR(ToolTipText),6,1), f1);
   QPen pen(FCOLOR(ToolTipText), f1);
   painter->setPen(pen);
   painter->drawRect(RECT.adjusted(f1/2,f1/2,-f1,-f1));

   painter->restore();
}

#define PAINT_WINDOW_BUTTON(_btn_) {\
    tmpOpt.rect = subControlRect(CC_TitleBar, tb, SC_TitleBar##_btn_##Button, widget);\
    if (!tmpOpt.rect.isNull())\
    { \
        if (tb->activeSubControls & SC_TitleBar##_btn_##Button)\
            tmpOpt.state = tb->state;\
        else\
            tmpOpt.state &= ~(State_Sunken | State_MouseOver);\
        if (!(tmpOpt.state & State_MouseOver))\
            tmpOpt.rect.adjust(F(2), F(2), -F(2), -F(2));\
        painter->drawPixmap(tmpOpt.rect.topLeft(), standardPixmap(SP_TitleBar##_btn_##Button, &tmpOpt, widget));\
   }\
}

void
Style::drawTitleBar(const QStyleOptionComplex * option,
                          QPainter * painter, const QWidget * widget) const
{
   const QStyleOptionTitleBar *tb = qstyleoption_cast<const QStyleOptionTitleBar *>(option);
   if (!tb) return;

   QRect ir;

   // the label
   if (option->subControls & SC_TitleBarLabel) {
      ir = subControlRect(CC_TitleBar, tb, SC_TitleBarLabel, widget);
      painter->setPen(PAL.color(QPalette::WindowText));
      ir.adjust(F(2), 0, -F(2), 0);
      painter->drawText(ir, Qt::AlignCenter | Qt::TextSingleLine, tb->text);
   }
   
   QStyleOptionTitleBar tmpOpt = *tb;
   if (tb->subControls & SC_TitleBarCloseButton)
      PAINT_WINDOW_BUTTON(Close)

   if (tb->subControls & SC_TitleBarMaxButton &&
       tb->titleBarFlags & Qt::WindowMaximizeButtonHint) {
      if (tb->titleBarState & Qt::WindowMaximized)
         PAINT_WINDOW_BUTTON(Normal)
      else
         PAINT_WINDOW_BUTTON(Max)
   }
       
   if (tb->subControls & SC_TitleBarMinButton &&
       tb->titleBarFlags & Qt::WindowMinimizeButtonHint) {
      if (tb->titleBarState & Qt::WindowMinimized)
         PAINT_WINDOW_BUTTON(Normal)
      else
         PAINT_WINDOW_BUTTON(Min)
   }
       
   if (tb->subControls & SC_TitleBarNormalButton &&
       tb->titleBarFlags & Qt::WindowMinMaxButtonsHint)
      PAINT_WINDOW_BUTTON(Normal)

   if (tb->subControls & SC_TitleBarShadeButton)
      PAINT_WINDOW_BUTTON(Shade)

   if (tb->subControls & SC_TitleBarUnshadeButton)
      PAINT_WINDOW_BUTTON(Unshade)

   if (tb->subControls & SC_TitleBarContextHelpButton &&
       tb->titleBarFlags & Qt::WindowContextHelpButtonHint)
      PAINT_WINDOW_BUTTON(ContextHelp)

   if (tb->subControls & SC_TitleBarSysMenu &&
       tb->titleBarFlags & Qt::WindowSystemMenuHint) {
      if (!tb->icon.isNull()) {
         ir = subControlRect(CC_TitleBar, tb, SC_TitleBarSysMenu, widget);
         tb->icon.paint(painter, ir);
      }
//    else
//       PAINT_WINDOW_BUTTON(SC_TitleBarSysMenu, SP_TitleBarMenuButton)
   }
#undef PAINT_WINDOW_BUTTON
}

void
Style::drawSizeGrip(const QStyleOption * option, QPainter * painter, const QWidget *) const
{
   Qt::Corner corner;
   if (const QStyleOptionSizeGrip *sgOpt =
       qstyleoption_cast<const QStyleOptionSizeGrip *>(option))
      corner = sgOpt->corner;
   else if (option->direction == Qt::RightToLeft)
      corner = Qt::BottomLeftCorner;
   else
      corner = Qt::BottomRightCorner;

   QRect rect = RECT;
   rect.setWidth(7*RECT.width()/4);
   rect.setHeight(7*RECT.height()/4);
   painter->save();
   painter->setRenderHint(QPainter::Antialiasing);
   int angle = 90<<4;
   painter->setPen(Qt::NoPen);
   switch (corner) {
   default:
   case Qt::BottomLeftCorner:
      angle = 0;
      rect.moveRight(RECT.right());
   case Qt::BottomRightCorner:
      painter->setBrush(Gradients::pix(FCOLOR(Window).dark(120), rect.height(),
                                       Qt::Vertical, Gradients::Sunken));
//       painter->setBrush(FCOLOR(Window).dark(120));
//       painter->setPen(FCOLOR(Window).dark(140));
      break;
   case Qt::TopLeftCorner:
      angle += 90<<4;
      rect.moveBottomRight(RECT.bottomRight());
   case Qt::TopRightCorner:
      angle += 90<<4;
      rect.moveBottom(RECT.bottom());
      painter->setBrush(FCOLOR(Window).dark(110));
      painter->setPen(FCOLOR(Window).dark(116));
      painter->drawPie(RECT, -(90<<4), 90<<4);
      break;
   }
   painter->drawPie(rect, angle, 90<<4);
   painter->restore();
}
