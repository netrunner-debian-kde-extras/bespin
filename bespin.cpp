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

/**================== Qt4 includes ======================*/

#include <QAbstractScrollArea>
#include <QApplication>
#include <QComboBox>
// #include <QDockWidget>
#include <QEvent>
#include <QFrame>
#include <QListView>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionTabWidgetFrame>
#include <QStylePlugin>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QtDBus/QDBusInterface>

/**============= Bespin includes ==========================*/

// #include "debug.h"

#ifdef Q_WS_X11
#include "blib/xproperty.h"
#endif
#include "blib/FX.h"
#include "blib/colors.h"
#include "animator/hover.h"

#include "bespin.h"

/**=========================================================*/

/**============= extern C stuff ==========================*/
class BespinStylePlugin : public QStylePlugin
{
public:
    QStringList keys() const {
        return QStringList() << "Bespin";
    }

    QStyle *create(const QString &key) {
        if (key == "bespin")
            return new Bespin::Style;
        return 0;
    }
};

Q_EXPORT_PLUGIN2(Bespin, BespinStylePlugin)
/**=========================================================*/

#include <QtDebug>


using namespace Bespin;

AppType Style::appType;
Config Style::config;
Style::Lights Style::lights;
Style::Masks Style::masks;
QPalette *Style::originalPalette = 0;
Style::Shadows Style::shadows;
Qt::Orientation Style::ori[2] = { Qt::Horizontal, Qt::Vertical };


#define N_PE 54
#define N_CE 50
#define N_CC 12
static void
(Style::*primitiveRoutine[N_PE])(const QStyleOption*, QPainter*, const QWidget*) const;
static void
(Style::*controlRoutine[N_CE])(const QStyleOption*, QPainter*, const QWidget*) const;
static void
(Style::*complexRoutine[N_CC])(const QStyleOptionComplex*, QPainter*, const QWidget*) const;

#define registerPE(_FUNC_, _ELEM_) primitiveRoutine[QStyle::_ELEM_] = &Style::_FUNC_
#define registerCE(_FUNC_, _ELEM_) controlRoutine[QStyle::_ELEM_] = &Style::_FUNC_
#define registerCC(_FUNC_, _ELEM_) complexRoutine[QStyle::_ELEM_] = &Style::_FUNC_

// static void registerPE(char *S0, ...)
// {
//    register char *s;
//    if (s=S0, s!=NULL)  { register char *sa;
//       va_list a;
//       for (va_start(a,S0);  (sa=va_arg(a,char*), sa!=NULL);  )
//          while (*s=*sa, *sa)  ++s,++sa;
//       va_end(a);
//    }
//    return ((int)(s-S0));
// }

void
Style::registerRoutines()
{
    for (int i = 0; i < N_PE; ++i)
        primitiveRoutine[i] = 0;
    for (int i = 0; i < N_CE; ++i)
        controlRoutine[i] = 0;
    for (int i = 0; i < N_CC; ++i)
        complexRoutine[i] = 0;

    // buttons.cpp
    registerPE(drawButtonFrame, PE_PanelButtonCommand);
    registerPE(drawButtonFrame, PE_PanelButtonBevel);
    registerPE(skip, PE_FrameDefaultButton);
    registerCE(drawPushButton, CE_PushButton);
    registerCE(drawPushButtonBevel, CE_PushButtonBevel);
    registerCE(drawPushButtonLabel, CE_PushButtonLabel);
    registerPE(drawCheckBox, PE_IndicatorCheckBox);
    registerPE(drawRadio, PE_IndicatorRadioButton);
    registerCE(drawCheckBoxItem, CE_CheckBox);
    registerCE(drawRadioItem, CE_RadioButton);
    registerCE(drawCheckLabel, CE_CheckBoxLabel);
    registerCE(drawCheckLabel, CE_RadioButtonLabel);
    // docks.cpp
#ifdef QT3_SUPPORT
    registerPE(skip, PE_Q3DockWindowSeparator);
    registerCE(skip, CE_Q3DockWindowEmptyArea);
#endif
    registerPE(drawDockBg, PE_FrameDockWidget);
    registerCE(drawDockTitle, CE_DockWidgetTitle);
    registerCC(drawMDIControls, CC_MdiControls);
    registerPE(drawDockHandle, PE_IndicatorDockWidgetResizeHandle);
    // frames.cpp
    registerCE(skip, CE_FocusFrame);
    registerPE(skip, PE_PanelStatusBar);
    registerPE(skip, PE_FrameStatusBarItem);
    registerPE(drawFocusFrame, PE_FrameFocusRect);
    registerPE(drawFrame, PE_Frame);
#if QT_VERSION >= 0x040500
    registerCE(drawFrame, CE_ShapedFrame);
#endif
    registerCC(drawGroupBox, CC_GroupBox);
    registerPE(drawGroupBoxFrame, PE_FrameGroupBox);
    // input.cpp
    registerPE(drawLineEditFrame, PE_FrameLineEdit);
    registerPE(drawLineEdit, PE_PanelLineEdit);
    registerCC(drawSpinBox, CC_SpinBox);
    registerCC(drawComboBox, CC_ComboBox);
    registerCE(drawComboBoxLabel, CE_ComboBoxLabel);
    // menus.cpp
    registerPE(drawMenuBarBg, PE_PanelMenuBar);
    registerCE(drawMenuBarBg, CE_MenuBarEmptyArea);
    registerCE(drawMenuBarItem, CE_MenuBarItem);
    registerCE(drawMenuItem, CE_MenuItem);
    registerCE(drawMenuScroller, CE_MenuScroller);
    registerCE(skip, CE_MenuEmptyArea);
    registerCE(skip, CE_MenuHMargin);
    registerCE(skip, CE_MenuVMargin);
    // progress.cpp
    registerCE(drawProgressBar, CE_ProgressBar);
    registerCE(drawProgressBarGroove, CE_ProgressBarGroove);
    registerCE(drawProgressBarContents, CE_ProgressBarContents);
    registerCE(drawProgressBarLabel, CE_ProgressBarLabel);
    // scrollareas.cpp
    registerPE(drawScrollAreaCorner, PE_PanelScrollAreaCorner);
    registerCC(drawScrollBar, CC_ScrollBar);
    registerCE(drawScrollBarAddLine, CE_ScrollBarAddLine);
    registerCE(drawScrollBarSubLine, CE_ScrollBarSubLine);
    registerCE(drawScrollBarGroove, CE_ScrollBarSubPage);
    registerCE(drawScrollBarGroove, CE_ScrollBarAddPage);
    registerCE(drawScrollBarSlider, CE_ScrollBarSlider);
    // shapes.cpp
    registerPE(drawItemCheck, PE_IndicatorViewItemCheck);
#ifdef QT3_SUPPORT
    registerPE(drawItemCheck, PE_Q3CheckListIndicator);
    registerPE(drawExclusiveCheck_p, PE_Q3CheckListExclusiveIndicator);
#endif
    registerPE(drawMenuCheck, PE_IndicatorMenuCheckMark);
    registerPE(drawSolidArrowN, PE_IndicatorArrowUp);
    registerPE(drawSolidArrowN, PE_IndicatorSpinUp);
    registerPE(drawSolidArrowN, PE_IndicatorSpinPlus);
    registerPE(drawSolidArrowS, PE_IndicatorArrowDown);
    registerPE(drawSolidArrowS, PE_IndicatorSpinDown);
    registerPE(drawSolidArrowS, PE_IndicatorSpinMinus);
    registerPE(drawSolidArrowS, PE_IndicatorButtonDropDown);
    registerPE(drawSolidArrowE, PE_IndicatorArrowRight);
    registerPE(drawSolidArrowW, PE_IndicatorArrowLeft);
    // slider.cpp
    registerCC(drawSlider, CC_Slider);
    registerCC(drawDial, CC_Dial);
    // tabbing.cpp
    registerPE(drawTabWidget, PE_FrameTabWidget);
    registerPE(drawTabBar, PE_FrameTabBarBase);
    registerCE(drawTab, CE_TabBarTab);
    registerCE(drawTabShape, CE_TabBarTabShape);
    registerCE(drawTabLabel, CE_TabBarTabLabel);
    registerPE(skip, PE_IndicatorTabTear);
    registerCE(drawToolboxTab, CE_ToolBoxTab);
    registerCE(drawToolboxTabShape, CE_ToolBoxTabShape);
    registerCE(drawToolboxTabLabel, CE_ToolBoxTabLabel);
#if QT_VERSION >= 0x040500
    registerPE(drawTabCloser, PE_IndicatorTabClose);
#endif
    // toolbars.cpp
    registerCC(drawToolButton, CC_ToolButton);
    registerPE(drawToolButtonShape, PE_PanelButtonTool);
    registerPE(skip, PE_IndicatorToolBarSeparator);
    registerPE(drawToolBar, PE_PanelToolBar);
    registerCE(drawToolButtonLabel, CE_ToolButtonLabel);
    registerCE(drawToolBar, CE_ToolBar);
    registerPE(skip, PE_FrameButtonTool);
#ifdef QT3_SUPPORT
    registerPE(skip, PE_Q3Separator);
#endif
    registerPE(drawToolBarHandle, PE_IndicatorToolBarHandle);
    // views.cpp
    registerCE(drawHeader, CE_Header);
    registerCE(drawHeaderSection, CE_HeaderSection);
    registerCE(drawHeaderLabel, CE_HeaderLabel);
    registerPE(drawBranch, PE_IndicatorBranch);
#ifdef QT3_SUPPORT
    registerCC(drawTree, CC_Q3ListView);
#endif
    registerCE(drawRubberBand, CE_RubberBand);
    registerPE(drawHeaderArrow, PE_IndicatorHeaderArrow);
    registerPE(drawItemRow, PE_PanelItemViewRow);
    registerPE(drawItemItem, PE_PanelItemViewItem);

    // window.cpp
    registerPE(drawWindowFrame, PE_FrameWindow);
    if (config.menu.shadow)
        registerPE(drawWindowFrame, PE_FrameMenu);
    else
        registerPE(skip, PE_FrameMenu);
    registerPE(drawWindowBg, PE_Widget);
    registerPE(drawToolTip, PE_PanelTipLabel);
    registerCC(drawTitleBar, CC_TitleBar);
    registerCE(drawDockHandle, CE_Splitter);
    registerCE(drawSizeGrip, CE_SizeGrip);
}

/**THE STYLE ITSELF*/

Style::Style() : QCommonStyle()
{
    setObjectName(QLatin1String("Bespin"));
    FX::init();
    init();
    registerRoutines();
}

Style::~Style()
{
   Gradients::wipe();
}

#include "makros.h"
#undef PAL
#define PAL pal

QColor
Style::btnBg( const QPalette &pal, bool isEnabled, bool hasFocus, int step, bool fullHover, bool translucent) const
{

    if (!isEnabled)
        return FCOLOR(Window); //Colors::mid(Qt::black, FCOLOR(Window),5,100);

    QColor c = CCOLOR(btn.std, Bg);
    if (hasFocus && config.btn.active_role[Bg] != QPalette::Highlight)
    {
        if (config.btn.layer)
            c = FCOLOR(Highlight);
        else
            c = Colors::mid(FCOLOR(Highlight), c, 1, 10 + Colors::contrast(FCOLOR(Highlight), c));
    }
    
    if (fullHover && step)
        c = Colors::mid(c, CCOLOR(btn.active, Bg), (config.btn.backLightHover ? (translucent ? 48 : 72) : 6) - step, step);

    return c;
}

QColor
Style::btnFg(const QPalette &pal, bool isEnabled, bool hasFocus, int step, bool flat) const
{
    if (!isEnabled)
        return FCOLOR(WindowText); //Colors::mid(FCOLOR(Window), FCOLOR(WindowText), 1, 3);

    if (!config.btn.layer || config.btn.active_role[Bg] == QPalette::Highlight)
        hasFocus = false;
    QColor  fg1 = hasFocus ? FCOLOR(HighlightedText) : CCOLOR(btn.std, Fg),
            fg2 = CCOLOR(btn.active, Fg);
    if (flat)
        { fg1 = FCOLOR(WindowText); fg2 = FCOLOR(Link); }

    if (!flat && config.btn.backLightHover)
        return fg1;

    if (step)
        return Colors::mid(fg1, fg2, 6 - step, step);

    return fg1;
}

void
Style::drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                          bool enabled, const QString& text, QPalette::ColorRole textRole, QRect *boundingRect) const
{
    if (text.isEmpty())
        return;
    QPen savedPen;
    bool penDirty = false;
    if (textRole != QPalette::NoRole)
    {
        penDirty = true;
        savedPen = painter->pen();
        painter->setPen(QPen(pal.brush(textRole), savedPen.widthF()));
    }
    QRect r = rect;
    if (!enabled)
    {   // let's see if we can get some blurrage here =)
        if (!penDirty)
            { savedPen = painter->pen(); penDirty = true; }

        QColor c = painter->pen().color();
        c.setAlpha(c.alpha()/4 + 2);
        painter->setPen(QPen(c, savedPen.widthF()));
        r.translate(-1,-1);
        painter->drawText(r, alignment, text);
        r.translate(1,2);
        painter->drawText(r, alignment, text);
        r.translate(2,0);
        painter->drawText(r, alignment, text);
        r.translate(-1,-2);
        painter->drawText(r, alignment, text);
    }
    else
        painter->drawText(rect, alignment, text, boundingRect);
    if (penDirty)
        painter->setPen(savedPen);
}

#define X_KdeBase 0xff000000
#define SH_KCustomStyleELement 0xff000001

enum CustomElements { _CE_CapacityBar = 0 /*, ...*/, N_CustomControls };
#if 0
enum SubElements { _SE_AmarokAnalyzerSlider = 0 /*, ...*/, N_CustomSubElements };
#endif

static QStyle::ControlElement primitives[N_CustomControls];
#if 0
static QStyle::SubElement subcontrols[N_CustomSubElements];
#endif

enum ElementType { SH, CE, SE };
static QMap<QString, int> styleElements; // yes. one is enough...
// NOTICE: using QHash instead QMap is probably overhead, there won't be too many items per app
static int counter[3] = { X_KdeBase+3 /*sic!*/, X_KdeBase, X_KdeBase };

void
Style::drawPrimitive ( PrimitiveElement pe, const QStyleOption * option,
                             QPainter * painter, const QWidget * widget) const
{
    Q_ASSERT(option);
    Q_ASSERT(painter);
    
//    if (pe == PE_IndicatorItemViewItemDrop)
// An indicator that is drawn to show where an item in an item view is about to
// be dropped during a drag-and-drop operation in an item view.
//       qWarning("IndicatorItemViewItemDrop, %d", pe);
    if (pe < N_PE && primitiveRoutine[pe])
        (this->*primitiveRoutine[pe])(option, painter, widget);
    else
    {
//         qDebug() << "BESPIN, unsupported primitive:" << pe << widget;
        QCommonStyle::drawPrimitive( pe, option, painter, widget );
    }
}

void
Style::drawControl ( ControlElement element, const QStyleOption * option,
                           QPainter * painter, const QWidget * widget) const
{
    Q_ASSERT(option);
    Q_ASSERT(painter);
    if (element < N_CE && controlRoutine[element])
        (this->*controlRoutine[element])(option, painter, widget);
    else if (element > X_KdeBase)
    {
        if (element == primitives[_CE_CapacityBar])
            drawCapacityBar(option, painter, widget);
        //if (pe == primitives[_PE_WhateverElse])
        // ...
    }
    else
    {
//         qDebug() << "BESPIN, unsupported control:" << element << widget;
        QCommonStyle::drawControl( element, option, painter, widget );
    }
}

void
Style::drawComplexControl ( ComplexControl control, const QStyleOptionComplex * option,
                                  QPainter * painter, const QWidget * widget) const
{
    Q_ASSERT(option);
    Q_ASSERT(painter);
    if (control < N_CC && complexRoutine[control])
        (this->*complexRoutine[control])(option, painter, widget);
    else
    {
//         qDebug() << "BESPIN, unsupported complex control:" << control << widget;
        QCommonStyle::drawComplexControl( control, option, painter, widget );
    }
}

int
Style::elementId(const QString &string) const
{
    int id = styleElements.value(string, 0);
    if (id)
        return id;

    if (string == "CE_CapacityBar")
        primitives[_CE_CapacityBar] = (ControlElement)(id = ++counter[CE]);
#if 0
    else if (string == "amarok.CC_Analyzer")
        complexs[_CC_AmarokAnalyzer] = (ComplexControl)(id = ++counter[CC]);
    // subcontrols (SC_) work muchg different as they're 1. flags and 2. attached to a CC
    else if (string == "amarok.CC_Analyzer:SC_Slider")
    {
        subcontrols[_SC_AmarokAnalyzerSlider] = (SubControl)(id = (1 << scCounter[_CC_AmarokAnalyzer]));
        ++scCounter[_CC_AmarokAnalyzer];
    }
//     else if blablablaba...
#endif
    if (id)
        styleElements.insert(string, id);
    return id;
}

/// ----------------------------------------------------------------------

void
Style::fillWithMask(QPainter *painter, const QPoint &xy,
                          const QBrush &brush, const QPixmap &mask,
                          QPoint offset) const
{
   QPixmap qPix(mask.size());
   if (brush.texture().isNull())
      qPix.fill(brush.color());
   else {
      QPainter p(&qPix);
      p.drawTiledPixmap(mask.rect(),brush.texture(),offset);
      p.end();
   }
   qPix = FX::applyAlpha(qPix, mask);
   painter->drawPixmap(xy, qPix);
}

void
Style::erase(const QStyleOption *option, QPainter *painter, const QWidget *widget, const QPoint *offset) const
{
    const QWidget *grampa = widget;
    while ( !(grampa->isWindow() ||
            (grampa->autoFillBackground() && grampa->objectName() != "qt_scrollarea_viewport")))
        grampa = grampa->parentWidget();

    QPoint tl = widget->mapFrom(const_cast<QWidget*>(grampa), QPoint());
    if (offset)
        tl += *offset;
    painter->save();
    painter->setPen(Qt::NoPen);

    const QBrush &brush = grampa->palette().brush(grampa->backgroundRole());

    // we may encounter apps that have semi or *cough* fully *cough* amarok *cough*
    // transparent backgrounds instead of none... ;-)
    const bool needBase = brush.style() > Qt::DiagCrossPattern || brush.color().alpha() < 0xff;

    if (grampa->isWindow() || needBase)
    {   // means we need to paint the global bg as well
        painter->setClipRect(option->rect, Qt::IntersectClip);
        QStyleOption tmpOpt = *option;
        //         tmpOpt.rect = QRect(tl, widget->size());
        tmpOpt.palette = grampa->palette();

        if (config.bg.opacity == 0xff || tmpOpt.palette.brush(QPalette::Window).style() > 1)
            painter->fillRect(tmpOpt.rect, tmpOpt.palette.brush(QPalette::Window));
        else if (config.bg.mode == Plain)
        {
            QColor c = tmpOpt.palette.color(QPalette::Window);
            c.setAlpha(config.bg.opacity);
            painter->fillRect(tmpOpt.rect, c);
        }
        
        painter->translate(tl);
        drawWindowBg(&tmpOpt, painter, grampa);
    }
    if (!grampa->isWindow())
    {
        painter->setBrush(grampa->palette().brush(grampa->backgroundRole()));
        painter->setBrushOrigin(tl);
        painter->drawRect(option->rect);
    }

    painter->restore();
}

// X11 properties for the deco ---------------
void
Style::setupDecoFor(QWidget *widget, const QPalette &palette, int mode, const Gradients::Type (&gt)[2])
{
#ifdef Q_WS_X11
    if ((appType == KWin))
        return;

    // WORKAROUND the raster graphicssystem destructor & some special virtual widget
    // as we now only set this on the show event, this should not occur anyway, but let's keep it safe
    if (!FX::usesXRender() && widget && !(widget->testAttribute(Qt::WA_WState_Created) || widget->internalWinId()))
    {
        //NOTICE esp. when using the raster engine some "virtual" widgets segfault in their destructor
        // as the winId() call below will unexpectedly generate a native window
        // known offending widgets:
        // inherits("KXMessages"), inherits("KSelectionWatcher::Private") inherits("KSelectionOwner::Private")
        // unfortunately the latter two are internal (and thus don't propagate their class through moc)
        qDebug() << "BESPIN: Not exporting decoration hints for " << widget;
        return;
    }

    // this is important because KDE apps may alter the original palette any time
    const QPalette &pal = originalPalette ? *originalPalette : palette;

    // the title region in the center
    WindowData data;
    QPalette::ColorRole active[2] = { QPalette::Window, QPalette::WindowText};
    const bool glassy = (widget && widget->testAttribute(Qt::WA_MacBrushedMetal));
    bool uno = false;
    if (config.UNO.title && widget)
    {
        QVariant h = widget->property("UnoHeight");
        uno = h.isValid() && h.toInt() > 0;
        if (uno)
        {
            active[Bg] = config.UNO.__role[Bg];
            active[Fg] = config.UNO.__role[Fg];
        }
    }

    QColor bg = pal.color(QPalette::Inactive, active[Bg]);
    if (glassy || uno)
    {
        data.style = (((Plain & 0xff) << 16) | ((Gradients::None & 0xff) << 8) | (Gradients::None & 0xff));
        if (uno)
            bg = Gradients::endColor(bg, Gradients::Top, config.UNO.gradient, true);
        else
            bg = bg.light(115-Colors::value(bg)/20);
    }
    else
        data.style = (((mode & 0xff) << 16) | ((gt[0] & 0xff) << 8) | (gt[1] & 0xff));
#if BESPIN_ARGB_WINDOWS
    if (!uno)
        bg.setAlpha(config.bg.opacity);
#endif
    data.inactiveWindow = bg.rgba();

    bg = pal.color(QPalette::Active, active[Bg]);
    if (uno)
        bg = Gradients::endColor(bg, Gradients::Top, config.UNO.gradient, true);
    else if (glassy)
        bg = bg.light(115-Colors::value(bg)/20);
    
#if BESPIN_ARGB_WINDOWS
    if (!uno)
        bg.setAlpha(config.bg.opacity);
#endif
    data.activeWindow = bg.rgba();

    QPalette::ColorRole inactive[2], text[2];
    if (uno || glassy)
    {
        inactive[Bg] = active[Bg];
        text[0] = text[1] = inactive[Fg] = active[Fg];
    }
    else
    {
        inactive[Bg]    = config.kwin.inactive_role[Bg];
        inactive[Fg]    = config.kwin.inactive_role[Fg];
        active[Bg]      = config.kwin.active_role[Bg];
        active[Fg]      = config.kwin.active_role[Fg];
        text[0]         = config.kwin.text_role[0];
        text[1]         = config.kwin.text_role[1];
    }


    data.inactiveDeco   = pal.color(QPalette::Inactive, inactive[Bg]).rgba();
    data.activeDeco     = pal.color(QPalette::Active, active[Bg]).rgba();
    data.inactiveText   = Colors::mid( pal.color(QPalette::Inactive, inactive[Bg]),
                                       pal.color(QPalette::Inactive, text[0])).rgba();
    data.activeText     = pal.color(QPalette::Active, text[1]).rgba();
    data.inactiveButton = Colors::mid( pal.color(QPalette::Inactive, inactive[Bg]),
                                       pal.color(QPalette::Inactive, inactive[Fg]),3,2).rgba();
    data.activeButton   = pal.color(QPalette::Active, active[Fg]).rgba();
    
    if (widget)
        XProperty::set<uint>(widget->winId(), XProperty::winData, (uint*)&data, XProperty::WORD, 9);
    else
    {   // dbus solution, currently for gtk
        QByteArray ba(36, 'a');
        uint *ints = (uint*)ba.data();
        ints[0] = data.inactiveWindow;
        ints[1] = data.activeWindow;
        ints[2] = data.inactiveDeco;
        ints[3] = data.activeDeco;
        ints[4] = data.inactiveText;
        ints[5] = data.activeText;
        ints[6] = data.inactiveButton;
        ints[7] = data.activeButton;
        ints[8] = data.style;

        QDBusInterface bespinDeco( "org.kde.kwin", "/BespinDeco", "org.kde.BespinDeco");
        const qint64 pid = QCoreApplication::applicationPid();
        bespinDeco.call(QDBus::NoBlock, "styleByPid", pid, ba);
    }
#endif // X11
}

static const
QPalette::ColorGroup groups[3] = { QPalette::Active, QPalette::Inactive, QPalette::Disabled };

static void
swapPalette(QWidget *widget, Style *style)
{
    // protect our KDE palette fix - in case
//     QPalette *savedPal = originalPalette;
//     originalPalette = 0;
    // looks complex? IS!
    // reason nr. 1: stylesheets. they're nasty and qt operates on the app palette here
    // reason nr. 2: some idiot must have spread the idea that pal.setColor(backgroundRole(), Qt::transparent)
    // is a great idea instead of just using setAutoFillBackground(false), preserving all colors and just not
    // using them. hey, why not call Qt to paint some nothing.... *grrrr*
    
    QMap<QWidget*, QString> shits;
    QList<QWidget*> kids = widget->findChildren<QWidget*>();
    kids.prepend(widget);

    QPalette pal;
    QPalette::ColorGroup group;
    QWidget *solidBase = 0;
    QColor c1, c2; int a;
    bool fixViewport = false;
    bool hasShit = false;
    foreach (QWidget *kid, kids)
    {
        if (kid->testAttribute(Qt::WA_StyleSheet))
        {   // first get rid of shit
            shits.insert(kid, kid->styleSheet());
            kid->setStyleSheet(QString());
            hasShit = true;
        }
        
        // now swap the palette ----------------
        if (hasShit || kid->testAttribute(Qt::WA_SetPalette) || kid == widget)
        {
            pal = kid->palette();
            solidBase = 0;
            fixViewport = false;
            hasShit = false;

            if (kid->inherits("KUrlButton") || kid->inherits("BreadcrumbItemButton"))
            {   // we mess up with it during painting
                pal.setColor(QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::Window));
                kid->setPalette(pal);
                continue;
            }
            
            // NOTE: WORKAROUND for amarok and probably others: see polish.cpp
            if (QAbstractScrollArea *area = qobject_cast<QAbstractScrollArea*>(kid) )
            if (QWidget *vp = area->viewport())
            if (!vp->autoFillBackground() || vp->palette().color(QPalette::Active, vp->backgroundRole()).alpha() == 0)
                fixViewport = true;

            if (fixViewport || kid->palette().color(QPalette::Active, QPalette::Window).alpha() == 0)
            {
                solidBase = kid;
                while ((solidBase = solidBase->parentWidget()))
                {
                    if ((solidBase->autoFillBackground() &&
                        solidBase->palette().color(QPalette::Active, solidBase->backgroundRole()).alpha() != 0) ||
                        solidBase->isWindow())
                        break;
                }
                if (solidBase->palette().brush(solidBase->backgroundRole()).style() > 1)
                    solidBase = 0; // there's some pixmap or so - better do not swap colors atm.
            }
            for (int i = 0; i < 3; ++i)
            {
                group = groups[i];
                
                if (solidBase && !fixViewport)
                {
                    pal.setColor(group, QPalette::WindowText, solidBase->palette().color(group, solidBase->foregroundRole()));
                    pal.setColor(group, QPalette::Window, solidBase->palette().color(group, solidBase->backgroundRole()));
                }
                else
                {
                    c1 = pal.color(group, QPalette::Window);
                    a = c1.alpha();
                    c2 = pal.color(group, QPalette::WindowText);
                    c1.setAlpha(c2.alpha());
                    c2.setAlpha(a);
                    pal.setColor(group, QPalette::Window, c2);
                    pal.setColor(group, QPalette::WindowText, c1);
                }

                c1 = pal.color(group, QPalette::Button);
                a = c1.alpha();
                c2 = pal.color(group, QPalette::ButtonText);
                c1.setAlpha(c2.alpha());
                c2.setAlpha(a);
                pal.setColor(group, QPalette::Button, c2);
                pal.setColor(group, QPalette::ButtonText, c1);

                if (solidBase && fixViewport)
                {   // means we have a widget w/o background, don't swap colors, but set colors to solidBase
                    // this is very much a WORKAROUND
                    pal.setColor(group, QPalette::Text, solidBase->palette().color(group, solidBase->foregroundRole()));
                }
            }
            style->polish(pal, false);
            kid->setPalette(pal);

        }
    }

    // this is funny: style shits rely on QApplication::palette() (nice trick, TrottelTech... again)
    // so to apply them with the proper color, we need to change the apps palette to the swapped one,...
    if (!shits.isEmpty())
    {
        QPalette appPal = QApplication::palette();
        // ... reapply the shits...
        QMap<QWidget*, QString>::const_iterator shit = shits.constBegin();
        while (shit != shits.constEnd())
        {
            QApplication::setPalette(shit.key()->palette());
            shit.key()->setStyleSheet(shit.value());
            ++shit;
        }
        // ... and reset the apps palette
        QApplication::setPalette(appPal);
    }
    
//     originalPalette = savedPal;
}

static QMenuBar*
bar4popup(QMenu *menu)
{
    if (!menu->menuAction())
        return NULL;
    if (menu->menuAction()->associatedWidgets().isEmpty())
        return NULL;
    foreach (QWidget *w, menu->menuAction()->associatedWidgets())
        if (qobject_cast<QMenuBar*>(w))
            return static_cast<QMenuBar *>(w);
    return NULL;
}

bool isUrlNaviButtonArrow = false;
static bool isLastNavigatorButton(const QWidget *w, const char *className)
{
    if (QWidget *navigator = w->parentWidget())
    {
        QList<QPushButton*> btns = navigator->findChildren<QPushButton*>();
        QList<QPushButton*>::const_iterator i = btns.constEnd();
        while (i != btns.constBegin())
        {
            --i;
            if ((*i)->inherits(className))
                return (*i == w);
        }
    }
    return false;
}

// settings the property can be expensive, so avoid for popups, combodrops etc.
// NOTICE:
// Qt::Dialog must be erased as it's in drawer et al. but takes away window as well
// Qt::FramelessWindowHint shall /not/ be ignored as the window could change it's opinion while being visible
static const
Qt::WindowFlags ignoreForDecoHints = ( Qt::Sheet | Qt::Drawer | Qt::Popup | Qt::SubWindow |
Qt::ToolTip | Qt::SplashScreen | Qt::Desktop | Qt::X11BypassWindowManagerHint /*| Qt::FramelessWindowHint*/ ) & (~Qt::Dialog);

static QList<QPointer<QToolBar> > unoUpdates;

static bool
updateUnoHeight(QMainWindow *mwin, bool includeToolbars)
{
    const QVariant var = mwin->property("UnoHeight");
    int oldH = 0, newH = 0;
    if (var.isValid())
        oldH = var.toInt();
    
    QList<QWidget*> dirty;
    if (includeToolbars)
    {
        QToolBar *b;
        QList<QToolBar*> bars; //= mwin->findChildren<QToolBar*>();
        foreach ( QObject *o, mwin->children() )
        {
            if (( b = qobject_cast<QToolBar*>(o) ))
            if ( b->isVisibleTo(mwin) )
                bars << b;
        }
        foreach (QToolBar *tbar, bars)
        {
            if ( mwin->toolBarArea(tbar) == Qt::TopToolBarArea )
            {
                dirty << tbar;
                const int y = tbar->geometry().bottom();
                if (y > newH)
                    newH = y;
            }
        }
    }
    if (mwin->menuBar())
    {
        newH += mwin->menuBar()->height();
        dirty << mwin->menuBar();
    }
    if ( oldH != newH )
    {
        mwin->setProperty("UnoHeight", newH);
        foreach (QWidget *w, dirty)
            w->update();
    }
    return !(oldH && newH);
}

void
Style::updateUno()
{
    foreach (QToolBar *bar, unoUpdates)
    {
        if (bar)
            updateUno(bar);
    }
    unoUpdates.clear();
}

void
Style::updateUno(QToolBar *bar)
{
    if ( QMainWindow *mwin = qobject_cast<QMainWindow*>(bar->parentWidget()) )
    {
        if (updateUnoHeight(mwin,config.UNO.toolbar) && config.UNO.title)
            setupDecoFor(mwin, mwin->palette(), config.bg.mode, GRAD(kwin));
        
        QPalette::ColorRole bg = QPalette::Window, fg = QPalette::WindowText;
        bool autoFill = false;
        if ( mwin->toolBarArea(bar) == Qt::TopToolBarArea )
        {
            autoFill = true;
            bg = config.UNO.__role[Bg];
            fg = config.UNO.__role[Fg];
        }
        if (!(autoFill == bar->autoFillBackground() &&
            bg == bar->backgroundRole() && fg == bar->foregroundRole()))
        {
            bar->setAutoFillBackground(autoFill);
            bar->setBackgroundRole(bg);
            bar->setForegroundRole(fg);
            QList<QWidget*> kids = bar->findChildren<QWidget*>();
            foreach (QWidget *kid, kids)
            {
                if (kid->isWindow())
                    continue;
                kid->setBackgroundRole(bg);
                kid->setForegroundRole(fg);
            }
            bar->update();
        }
    }
}

bool
Style::eventFilter( QObject *object, QEvent *ev )
{
    switch (ev->type())
    {
    case QEvent::MouseMove:
    case QEvent::Timer:
    case QEvent::Move:
        return false; // just for performance - they can occur really often
    case QEvent::Paint:
#if BESPIN_ARGB_WINDOWS
        if (!(config.bg.opacity == 0xff && config.menu.opacity == 0xff))
        if (QWidget *window = qobject_cast<QWidget*>(object))
        if (window->isWindow())
        {
            // NOTICE this fast check fails, because -guess who- PLAMSA [sic] or at least some stupid
            // plamsoids apparently have fun in removing it (together with dropshadows - TROTTEL!)
            // const bool isPopup = (window->windowFlags() & (Qt::Popup & ~Qt::Window));
            // so we use qobject_cast... *GRRRRRRrrrrrRRRRRRRRrr*
            const bool isPopup = bool(qobject_cast<QMenu*>(window));
            const int opacity = isPopup ? config.menu.opacity : config.bg.opacity;
            if (opacity == 0xff)
                return false;
            QPainter p(window);
            p.setPen(Qt::NoPen);
            const bool glassy = window->testAttribute(Qt::WA_MacBrushedMetal);
            const bool isPlain = config.bg.mode == Plain || (isPopup && !glassy);
            if (isPlain || glassy)
            {
                QColor c = window->palette().color(window->backgroundRole());
                c.setAlpha(opacity);
                p.setBrush(c);
                p.drawRect(window->rect());
            }
            if (!isPlain || glassy || config.bg.ringOverlay)
                drawWindowBg(0, &p, window);
            p.end();
            return false;
        }
#endif
#if  QT_VERSION < 0x040500 // 4.5 has a CE_ for this =)
        if (QFrame *frame = qobject_cast<QFrame*>(object))
        {
            if ((frame->frameShape() == QFrame::HLine || frame->frameShape() == QFrame::VLine) &&
                 frame->isVisible())
            {
                QPainter p(frame);
                Orientation3D o3D = (frame->frameShadow() == QFrame::Sunken) ? Sunken :
                                    (frame->frameShadow() == QFrame::Raised) ? Raised : Relief;
                const bool v = frame->frameShape() == QFrame::VLine;
                shadows.line[v][o3D].render(frame->rect(), &p);
                p.end();
                return true;
            }
            return false;
        } else
#endif
        if (QTabBar *tabBar = qobject_cast<QTabBar*>(object))
        {
            if (tabBar->testAttribute(Qt::WA_NoSystemBackground))
                return false; // shall be translucent
            if (!tabBar->drawBase())
                return false;
            if (QTabWidget *tw = qobject_cast<QTabWidget*>(tabBar->parentWidget()))
            {   // no extra tabbar here please... unless the border is StyleShitted away ;)
                if (tw->styleSheet().isEmpty())
                    return false;
                if ( !(tw->styleSheet().contains("pane", Qt::CaseInsensitive) && tw->styleSheet().contains("border", Qt::CaseInsensitive)) )
                    return false;
            }
            if ( appType == KDevelop )
            {   // KDevelop does that... weird. - and of course /after/ painting the label string...
                QWidget *dad = tabBar->parentWidget();
                while ( dad )
                {
                    if (dad->inherits("QMenuBar"))
                        return false;
                    dad = dad->parentWidget();
                }
            }

            QPainter p(tabBar);
            QStyleOptionTabBarBase opt;
            opt.initFrom(tabBar);
            if (QWidget *window = tabBar->window())
            {
                opt.tabBarRect = window->rect();
                opt.tabBarRect.moveTopLeft(tabBar->mapFrom(window, opt.tabBarRect.topLeft()));
            }
            drawTabBar(&opt, &p, NULL);
            p.end();
            return false;
        }
#if  QT_VERSION >= 0x040500
        else if ( QTabWidget *tw = qobject_cast<QTabWidget*>( object ) )
        {
            // those don't paint frames and rely on the tabbar, which we ruled and rule out (looks weird with e.g. cornerwidgets...)
            if (tw->documentMode())
            {
                QPainter p(tw);
                QStyleOptionTabBarBaseV2 opt;
                opt.initFrom(tw);
                opt.documentMode = true;
                const int thickness = pixelMetric( PM_TabBarBaseHeight, &opt, tw );
                switch (tw->tabPosition())
                {
                    default:
                    case QTabWidget::North:
                        opt.rect.setBottom(opt.rect.top() + thickness);
                        opt.shape = QTabBar::RoundedNorth; break;
                    case QTabWidget::South:
                        opt.rect.setTop(opt.rect.bottom() - thickness);
                        opt.shape = QTabBar::RoundedSouth; break;
                    case QTabWidget::West:
                        opt.rect.setRight(opt.rect.left() + thickness);
                        opt.shape = QTabBar::RoundedWest; break;
                    case QTabWidget::East:
                        opt.rect.setLeft(opt.rect.right() - thickness);
                        opt.shape = QTabBar::RoundedEast; break;
                }

                opt.tabBarRect = tw->rect();
                drawTabBar(&opt, &p, NULL);
                p.end();
                return true; // don't let it paint weird stuff on the cornerwidgets etc.
            }
            return false;
        }
#endif
        else if (QPushButton *w = qobject_cast<QPushButton*>(object))
        {
            bool b = false;
            if ((b = object->inherits("KUrlButton")) || object->inherits("BreadcrumbItemButton"))
            {
                isUrlNaviButtonArrow = true;
//                 object->removeEventFilter(this);
                if (w->text() == "/")
                    w->setText("/.");
                if (isLastNavigatorButton(w, b?"KUrlButton":"BreadcrumbItemButton"))
                {
                    if (w->foregroundRole() != QPalette::WindowText)
                        w->setForegroundRole(QPalette::WindowText);
                }
                else if (w->foregroundRole() != QPalette::Link)
                    w->setForegroundRole(QPalette::Link);
//                 QCoreApplication::sendEvent(object, ev);
//                 object->installEventFilter(this);
                isUrlNaviButtonArrow = false;
                return false;
            }
        }
        return false;

    case QEvent::Enter:
    case QEvent::Leave:
    {
        if (qobject_cast<QPushButton*>(object))
        {
            bool b = false;
            if ((b = object->inherits("KUrlButton")) || object->inherits("BreadcrumbItemButton"))
            {
                QWidget *w = static_cast<QWidget*>(object);
                w->setCursor(Qt::PointingHandCursor);
                QFont fnt = w->font();
                if (isLastNavigatorButton(w, b?"KUrlButton":"BreadcrumbItemButton"))
                {
                    w->setCursor(Qt::ArrowCursor);
                    fnt.setUnderline(false);
                }
                else
                    fnt.setUnderline(ev->type() == QEvent::Enter);
                w->setFont(fnt);
                return false;
            }
        }
        return false;
    }
    case QEvent::Resize:
    {
        QResizeEvent *re = static_cast<QResizeEvent*>(ev);

        if (config.UNO.used)
        if (re->size().height() != re->oldSize().height())
        if (QMainWindow *mwin = qobject_cast<QMainWindow*>(object->parent()))
        if ((config.UNO.toolbar && qobject_cast<QToolBar*>(object) &&
             mwin->toolBarArea(static_cast<QToolBar*>(object)) == Qt::TopToolBarArea) ||
             qobject_cast<QMenuBar*>(object))
        {
            if (updateUnoHeight(mwin,config.UNO.toolbar) && config.UNO.title)
                setupDecoFor(mwin, mwin->palette(), config.bg.mode, GRAD(kwin));
            return false;
        }
        
        QWidget *widget = 0/*, *dock = 0*/;
        if ((config.menu.round && (widget = qobject_cast<QMenu*>(object)))
#if 0
            || (config.bg.docks.shape && (widget = dock = qobject_cast<QDockWidget*>(object))))
        {
            // kwin yet cannot. compiz can't even menus...
            if (dock && dock->isWindow())
            {
                dock->clearMask();
                return false;
            }
#else
            )
        {
#endif
#if 0 // xPerimental code for ribbon like looking menus - not atm.
            QAction *head = menu->actions().at(0);
            QRect r = menu->fontMetrics().boundingRect(menu->actionGeometry(head),
            Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextExpandTabs | BESPIN_MNEMONIC,
            head->iconText());
            r.adjust(-dpi.f12, -dpi.f3, dpi.f16, dpi.f3);
            QResizeEvent *rev = (QResizeEvent*)ev;
            QRegion mask(menu->rect());
            mask -= QRect(0,0,menu->width(),r.bottom());
            mask += r;
            mask -= masks.corner[0]; // tl
            QRect br = masks.corner[1].boundingRect();
            mask -= masks.corner[1].translated(r.right()-br.width(), 0); // tr
            br = masks.corner[2].boundingRect();
            mask -= masks.corner[2].translated(0, menu->height()-br.height()); // bl
            br = masks.corner[3].boundingRect();
            mask -= masks.corner[3].translated(menu->width()-br.width(), menu->height()-br.height()); // br
#endif
            const int w = widget->width();
            const int h = widget->height();
            QRegion mask(4, 0, w-8, h);
            mask += QRegion(0, 4, w, h-8);
            mask += QRegion(2, 1, w-4, h-2);
            mask += QRegion(1, 2, w-2, h-4);
// only top rounded - but looks nasty
//          QRegion mask(0, 0, w, h-4);
//          mask += QRect(1, h-4, w-2, 2);
//          mask += QRect(2, h-2, w-4, 1);
//          mask += QRect(4, h-1, w-8, 1);

            widget->setMask(mask);
            return false;
        }
        return false;
    }
//    case QEvent::MouseButtonRelease:
//    case QEvent::MouseButtonPress:
//       qWarning("pressed/released");
//       if (object->inherits("QScrollBar")) {
//          qWarning("QScrollBar pressed/released");
//          QWidget *w = static_cast<QWidget*>(object)->parentWidget();
//          if (w && isSpecialFrame(w)) {
//             qWarning("set frame updates to %s",
//                      ev->type() == QEvent::MouseButtonRelease ? "active" : "INactive");
//             w->setUpdatesEnabled(ev->type() == QEvent::MouseButtonRelease);
//          }
//          return false;
//       }
//       return false;
    case QEvent::Wheel:
    {
        if (QAbstractSlider* slider = qobject_cast<QAbstractSlider*>(object))
        {
            QWheelEvent *we = static_cast<QWheelEvent*>(ev);
            if ((slider->value() == slider->minimum() && we->delta() > 0) ||
                (slider->value() == slider->maximum() && we->delta() < 0))
                Animator::Hover::Play(slider);
            return false;
        }

        if (object->objectName() == "qt_scrollarea_viewport")
        {
            if (QListView *list = qobject_cast<QListView*>(object->parent()))
            if (list->verticalScrollBar() && list->inherits("KateFileList"))
            { // suck it, jerks!
                QCoreApplication::sendEvent(list->verticalScrollBar(), ev); // tell the scrollbar to do this ;-P
                return true; // eat it
            }
            return false;
        }
        if (QListView *list = qobject_cast<QListView*>(object))
        {
//             if (list->verticalScrollMode() == QAbstractItemView::ScrollPerPixel) // this should be, but semms to be not
            if (list->iconSize().height() > -1) // happens on e.g. config views
            if (list->inherits("KCategorizedView"))
                list->verticalScrollBar()->setSingleStep(list->iconSize().height()/3);
        }
        return false;
    }
#ifdef MOUSEDEBUG
    case QEvent::MouseButtonPress:
    {
        QMouseEvent *mev = (QMouseEvent*)ev;
        qDebug() << "BESPIN:" << object;
        //       DEBUG (object);
        return false;
    }
#endif
    case QEvent::Show:
    {
        QWidget * widget = qobject_cast<QWidget*>(object);
        if (!widget)
            return false;

        // talk to kwin about colors, gradients, etc.
        if (widget->isWindow() && !(widget->windowFlags() & ignoreForDecoHints))
        {
            if (widget->isModal())
            { // setup some special stuff for modal windows
                if (config.bg.modal.invert)
                    swapPalette(widget, this);
                if (config.bg.modal.glassy)
                {
                    widget->setAttribute(Qt::WA_StyledBackground);
                    widget->setAttribute(Qt::WA_MacBrushedMetal);
                }
                widget->setWindowOpacity( config.bg.modal.opacity/100.0 );
            }
#ifdef Q_WS_X11
            setupDecoFor(widget, widget->palette(), config.bg.mode, GRAD(kwin));
#endif
            return false;
        }
        if (QMenu * menu = qobject_cast<QMenu*>(widget))
        {
            // seems to be necessary, somehow KToolBar context menus manages to take QPalette::Window...?!
            // through title setting?!
            menu->setBackgroundRole ( config.menu.std_role[Bg] );
            menu->setForegroundRole ( config.menu.std_role[Fg] );
            if (menu->parentWidget() && menu->parentWidget()->inherits("QMdiSubWindow"))
            {
                QPoint pt = menu->parentWidget()->rect().topRight();
                pt += QPoint(-menu->width(), pixelMetric(PM_TitleBarHeight,0,0));
                pt = menu->parentWidget()->mapToGlobal(pt);
                menu->move(pt);
            }
            QMenuBar *bar = bar4popup(menu);
            if (bar)
#if 0
            {
                QPoint pos(dpi.f1, 0);
                pos += bar->actionGeometry(menu->menuAction()).topLeft();
                menu->move(bar->mapToGlobal(pos));
                menu->setActiveAction(menu->actions().at(0));
            }
#else
            menu->move(menu->pos()-QPoint(0,F(2)));
#endif
            return false;
        }

        if ( config.UNO.toolbar )
        if ( QToolBar *bar = qobject_cast<QToolBar*>(object) )
        {
            if (unoUpdates.isEmpty())
                QTimer::singleShot(0, this, SLOT(updateUno()));
            unoUpdates << bar;
            return false;
        }
    }
    case QEvent::Hide:
        if (config.bg.modal.invert)
        if (QWidget * widget = qobject_cast<QWidget*>(object))
        if (widget->isModal())
            swapPalette(widget, this);
        return false;
#if 1
    case QEvent::PaletteChange:
    {
        #define CONTRAST(_C1_, _C2_) Colors::contrast(pal.color(group, _C1_), pal.color(group, _C2_))
        #define LACK_CONTRAST(_C1_, _C2_) (pal.color(group, _C1_).alpha() > 64 && Colors::contrast(pal.color(group, _C1_), pal.color(group, _C2_)) < 20)
        #define HARD_CONTRAST(_C_) pal.color(group, _C_).alpha() < 64 ? Qt::red : (Colors::value(pal.color(group, _C_)) < 128 ? Qt::white : Qt::black)
        QWidget * widget = qobject_cast<QWidget*>(object);
        if (!widget)
            return false;

#ifdef Q_WS_X11
        // talk to kwin about colors, gradients, etc.
        if (widget->isWindow() && !(widget->windowFlags() & ignoreForDecoHints))
        {
            setupDecoFor(widget, widget->palette(), config.bg.mode, GRAD(kwin));
            XProperty::remove(widget->winId(), XProperty::bgPics);
            return false;
        }
#endif
        
        // i think, i hope i got it....
        // 1. khtml sets buttontext, windowtext and text to the fg color - what leads to trouble if e.g. button doesn't contrast window
        // 2. combolists need a special kick (but their palette seems ok, though it isn't...)
        // 3. css causes more trouble - esp. with semitransparent colors...
        if (widget->objectName() == "RenderFormElementWidget")
        {
            QPalette pal = widget->palette();
            bool paletteChanged = false;
            for (int g = 0; g < 3; ++g)
            {
                QPalette::ColorGroup group = (QPalette::ColorGroup)g;
                if (pal.color(group, QPalette::Window).alpha() < 64)
                    pal.setColor(group, QPalette::Window, qApp->palette().color(group, QPalette::Window));
                if (LACK_CONTRAST(QPalette::Window, QPalette::WindowText))
                {
                    paletteChanged = true;
                    pal.setColor(group, QPalette::WindowText, HARD_CONTRAST(QPalette::Window));
                }
                if (LACK_CONTRAST(QPalette::Button, QPalette::ButtonText))
                {
                    paletteChanged = true;
                    pal.setColor(group, QPalette::ButtonText, HARD_CONTRAST(QPalette::Button));
                }
                if (LACK_CONTRAST(QPalette::Base, QPalette::Text))
                {
                    paletteChanged = true;
                    pal.setColor(group, QPalette::Text, HARD_CONTRAST(QPalette::Base));
                }
            }
            
            if (paletteChanged)
            {
                widget->removeEventFilter(this);
                widget->setPalette(pal);
                widget->installEventFilter(this);

                // TODO: this might cause trouble with palettes with translucent backgrounds...
                if (QComboBox *box = qobject_cast<QComboBox*>(widget))
                if (box->view())
                    box->view()->setPalette(pal);
            }

            return false;
        }
        return false;
    }
#endif
    case QEvent::ApplicationPaletteChange:
    {
        if (object == qApp && originalPalette)
        {
            // this fixes KApplications
            // "we create the style, then reload the palette from personal settings and reapply it" junk"
            // the order is important or we'll get reloads for sure or eventually!
            object->removeEventFilter(this);
            QPalette *pal = originalPalette;
            originalPalette = 0;
            polish(*pal);
            qApp->setPalette(*pal);
            delete pal;
        }
        return false;
    }
    default:
        return false;
    }
}


QPalette
Style::standardPalette () const
{
   QPalette pal ( QColor(70,70,70), QColor(70,70,70), // windowText, button
                     Qt::white, QColor(211,211,212), QColor(226,226,227), //light, dark, mid
                     Qt::black, Qt::white, //text, bright_text
                     Qt::white, QColor(234,234,236) ); //base, window
   pal.setColor(QPalette::ButtonText, Qt::white);
   pal.setColor(QPalette::Highlight, QColor(97, 147, 207));
   pal.setColor(QPalette::HighlightedText, Qt::white);
   return pal;
}

#undef PAL
