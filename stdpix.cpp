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

#include <QStyleOptionTitleBar>
#include <QApplication>
#include <QDockWidget>
#include <QPainter>
#include <QPen>

#include "bespin.h"
#include "colors.h"
#include "paths.h"

#include <QtDebug>

#define COLOR(_TYPE_) pal.color(QPalette::_TYPE_)

using namespace Bespin;

static void
setIconFont(QPainter &painter, const QRect &rect, float f = 0.75)
{
    QFont fnt = painter.font();
    fnt.setPixelSize ( (int)(f*rect.height()) );
    fnt.setBold(true); painter.setFont(fnt);
}

static inline uint qt_intensity(uint r, uint g, uint b)
{
    // 30% red, 59% green, 11% blue
    return (77 * r + 150 * g + 28 * b) / 255;
}

QPixmap
Style::standardPixmap(StandardPixmap standardPixmap,
                            const QStyleOption * option, const QWidget * widget ) const
{
    bool sunken = false, isEnabled = false, hover = false;
    if (option)
    {
        sunken = option->state & State_Sunken;
        isEnabled = option->state & State_Enabled;
        hover = isEnabled && (option->state & State_MouseOver);
    }
   
    QRect rect; QPalette pal;
//    const QStyleOptionTitleBar *opt =
//       qstyleoption_cast<const QStyleOptionTitleBar *>(option);
    if (option)
    {
        if (option->rect.isNull()) // THIS SHOULD NOT!!!! happen... but unfortunately does on dockwidgets...
            rect = QRect(0,0,14,14);
        else
        {
            rect = option->rect; rect.moveTo(0,0);
            if (rect.width() > rect.height())
                rect.setWidth(rect.height());
            else
                rect.setHeight(rect.width());
        }
        pal = option->palette;
    }
    else
    {
        rect = QRect(0,0,14,14);
        pal = qApp->palette();
    }

    const QDockWidget *dock = qobject_cast<const QDockWidget*>(widget);
    const int sz = dock ? 14 : rect.width();
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    QPainterPath shape;

    switch (standardPixmap)
    {
//         case SP_ArrowBack:
//         case SP_ArrowLeft:
//         case SP_ArrowRight:
//         case SP_ArrowForward:
//         case SP_MediaPlay:
//             shape.moveTo(pm.rect().center());
//             shape.arcTo(pm.rect(), standardPixmap == SP_ArrowLeft ? 90 :270, 180);
//             shape.closeSubpath();
//             goto paint;
//         case SP_MediaPause:
//             shape = Shapes::unAboveBelow(pm.rect());
//             goto paint;
//         SP_BrowserReload    58  Icon indicating that the current page should be reloaded.
//         SP_MediaSkipForward 63  Icon indicating that media should skip forward.
//         SP_MediaSkipBackward    64  Icon indicating that media should skip backward.
//         SP_MediaSeekForward 65  Icon indicating that media should seek forward.
//         SP_MediaSeekBackward    66  Icon indicating that media should seek backward.
//         SP_MediaVolume  67  Icon indicating a volume control.
//         SP_MediaVolumeMuted

    case SP_DockWidgetCloseButton:
    case SP_TitleBarCloseButton:
//     case SP_BrowserStop:
//     case SP_MediaStop:
        shape = Shapes::close(pm.rect(), config.newWinBtns);
        goto paint;
    case SP_TitleBarMinButton:
//     case SP_ArrowDown:
        shape = Shapes::min(pm.rect(), config.newWinBtns);
        goto paint;
    case SP_TitleBarMaxButton:
//     case SP_ArrowUp:
        shape = Shapes::max(pm.rect(), config.newWinBtns);
        goto paint;
    case SP_TitleBarMenuButton:
        shape = Shapes::menu(pm.rect(), false, config.newWinBtns);
        goto paint;
    case SP_TitleBarShadeButton:
    case SP_TitleBarUnshadeButton:
        shape = Shapes::shade(pm.rect(), config.newWinBtns);
        goto paint;
    case SP_TitleBarNormalButton:
        if (dock)
            shape = Shapes::dockControl(pm.rect(), dock->isFloating(), config.newWinBtns);
        else
            shape = Shapes::restore(pm.rect(), config.newWinBtns);
        goto paint;
    case SP_TitleBarContextHelpButton:
    {
        shape = Shapes::help(pm.rect(), config.newWinBtns);

paint:

        QPalette::ColorRole bg = QPalette::Window, fg = QPalette::WindowText;
        if (widget)
        {
            bg = widget->backgroundRole();
            fg = widget->foregroundRole();
        }
        const QColor c = Colors::mid(pal.color(bg), pal.color(fg), (!sunken)*(4-2*hover), 1);
        painter.setRenderHint ( QPainter::Antialiasing );
        painter.setPen(Qt::NoPen); painter.setBrush(c);
        painter.drawPath(shape);
        break;
    }
    case SP_MessageBoxInformation:
    { //  9  The "information" icon
        const int bs = rect.height()/14;
        rect.adjust(bs,bs,-bs,-bs);
        setIconFont(painter, rect);
        painter.setRenderHint ( QPainter::Antialiasing );
        painter.setPen(QPen(Qt::white, bs));
        painter.setBrush(QColor(0,102,255));
        painter.drawEllipse(rect);
        painter.setPen(Qt::white);
        painter.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, "i");
        break;
    }
    case SP_MessageBoxWarning:
    { //  10  The "warning" icon
        int bs = rect.width()/14;
        rect.adjust(bs,bs,-bs,-bs);
        int hm = rect.x()+rect.width()/2;
        const QPoint points[3] = {
            QPoint(hm, rect.top()),
            QPoint(rect.left(), rect.bottom()),
            QPoint(rect.right(), rect.bottom())
        };
        setIconFont(painter, rect);
        painter.setRenderHint ( QPainter::Antialiasing);
        painter.setPen(QPen(QColor(227,173,0), bs, Qt::SolidLine, Qt::SquareCap, Qt::RoundJoin));
        painter.setBrush(QColor(255,235,85));
        painter.drawPolygon(points, 3);
        painter.setPen(Qt::black);
        painter.drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, "!");
        break;
    }
    case SP_MessageBoxCritical:
    { //  11  The "critical" icon
        const int bs = rect.height()/14;
        rect.adjust(bs,bs,-bs,-bs);
        setIconFont(painter, rect);
        painter.setRenderHint ( QPainter::Antialiasing );
        painter.setPen(QPen(Qt::white/*Color(226,8,0)*/, bs));
        painter.setBrush(QColor(156,15,15));
        painter.drawEllipse(rect);
        painter.setPen(Qt::white);
        painter.drawText(rect, Qt::AlignCenter, "X");
        break;
    }
    case SP_MessageBoxQuestion:
    { //  12  The "question" icon
        setIconFont(painter, rect, 1);
        QColor c = COLOR(WindowText); c.setAlpha(128);
        painter.setPen(c);
        painter.drawText(rect, Qt::AlignCenter, "?");
        break;
    }
//    case SP_DesktopIcon: //  13   
//    case SP_TrashIcon: //  14   
//    case SP_ComputerIcon: //  15   
//    case SP_DriveFDIcon: //  16   
//    case SP_DriveHDIcon: //  17   
//    case SP_DriveCDIcon: //  18   
//    case SP_DriveDVDIcon: //  19   
//    case SP_DriveNetIcon: //  20   
//    case SP_DirOpenIcon: //  21   
//    case SP_DirClosedIcon: //  22   
//    case SP_DirLinkIcon: //  23   
//    case SP_FileIcon: //  24   
//    case SP_FileLinkIcon: //  25   
//    case SP_FileDialogStart: //  28   
//    case SP_FileDialogEnd: //  29   
//    case SP_FileDialogToParent: //  30   
//    case SP_FileDialogNewFolder: //  31   
//    case SP_FileDialogDetailedView: //  32   
//    case SP_FileDialogInfoView: //  33   
//    case SP_FileDialogContentsView: //  34   
//    case SP_FileDialogListView: //  35   
//    case SP_FileDialogBack: //  36   
    case SP_ToolBarHorizontalExtensionButton: //  26  Extension button for horizontal toolbars
    case SP_ToolBarVerticalExtensionButton: //  27  Extension button for vertical toolbars
        painter.setPen(Qt::NoPen);
        painter.setBrush(Colors::mid(COLOR(Window), COLOR(WindowText)));
        drawSolidArrow(standardPixmap == SP_ToolBarHorizontalExtensionButton ? Navi::E : Navi::S, rect, &painter);
        break;
    default:
        return QCommonStyle::standardPixmap ( standardPixmap, option, widget );
    }
    painter.end();
#if 0
    QPixmapCache::insert(key, pm);
#endif
    return pm;
}

#undef COLOR
