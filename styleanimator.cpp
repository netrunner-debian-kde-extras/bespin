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

#include <QEvent>
#include <QPainter>
#include <QBitmap>
#include <QApplication>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QProgressBar>
#include <QAbstractScrollArea>
#include <QAbstractButton>
#include <QComboBox>
#include <Q3ScrollView>
#include <QScrollBar>

#include "styleanimator.h"
#ifndef QT_NO_XRENDER
#include "oxrender.h"
#endif
#include "eventkiller.h"

#define ANIMATIONS (activeTabs + progressbars.count() + \
hoverWidgets.count() + complexHoverWidgets.count() + indexedHoverWidgets.count())

#define startTimer if (!timer->isActive()) timer->start(50)

static QHash<QWidget*, int> progressbars;
typedef QHash<QWidget*, HoverFadeInfo> HoverFades;
static HoverFades hoverWidgets;
typedef QHash<QWidget*, ComplexHoverFadeInfo> ComplexHoverFades;
static ComplexHoverFades complexHoverWidgets;
typedef QHash<QWidget*, IndexedFadeInfo> IndexedFades;
static IndexedFades indexedHoverWidgets;
static QHash<QTabWidget*, TabAnimInfo*> tabwidgets;
static int activeTabs = 0;

bool animationUpdate;

StyleAnimator::StyleAnimator(QObject *parent,
                             TabAnimInfo::TabTransition tabTrans) :
QObject(parent), tabTransition(tabTrans) {
   animationUpdate = false;
   timer = new QTimer( this );
   connect(timer, SIGNAL(timeout()), this, SLOT(updateProgressbars()));
   connect(timer, SIGNAL(timeout()), this, SLOT(updateTabAnimation()));
   connect(timer, SIGNAL(timeout()), this, SLOT(updateFades()));
   connect(timer, SIGNAL(timeout()), this, SLOT(updateComplexFades()));
   connect(timer, SIGNAL(timeout()), this, SLOT(updateIndexedFades()));
}

StyleAnimator::~StyleAnimator(){
   progressbars.clear();
}

// ===================== TABS ================================





// ===================== TABS ================================

bool TabAnimInfo::eventFilter( QObject* object, QEvent* event ) {
   if (event->type() != QEvent::Paint || !animStep)
      return false;
   QPainter p((QWidget*)object);
   p.drawPixmap(0,0, tabPix[2]);
   p.end();
   return true;
}

void TabAnimInfo::updatePixmaps(TabTransition tabTransition) {
   switch (tabTransition) {
#ifndef QT_NO_XRENDER
   case CrossFade: // the quotient says "accelerate animation"!
      OXRender::blend(tabPix[1], tabPix[2], 1.1666-0.1666*animStep);
      break;
#endif
   case ScanlineBlend:
   default: {
      QPainter p(&tabPix[2]);
      for (int i = animStep; i < tabPix[2].height(); i+=6)
         p.drawPixmap(0, i, tabPix[1], 0, i, tabPix[1].width(), 1);
      break;
   }
   case SlideIn: {
      //TODO handle different bar positions (currently assumes top)
      QPainter p(&tabPix[2]);
      p.drawPixmap(0, 0, tabPix[0], 0, animStep*tabPix[1].height()/7,
                   tabPix[0].width(), (7-animStep)*tabPix[1].height()/7);
      break;
   }
   case SlideOut: {
      tabPix[2] = tabPix[1];
      //TODO handle different bar positions (currently assumes top)
      QPainter p(&tabPix[2]);
      p.drawPixmap(0, 0, tabPix[0], 0, (7 - animStep) * tabPix[0].height()/7,
                   tabPix[0].width(), animStep*tabPix[0].height()/7);
      break;
   }
   case RollIn: {
      QPainter p(&tabPix[2]);
      int h = (7-animStep)*tabPix[1].height()/14;
      p.drawPixmap(0, 0, tabPix[1], 0, 0, tabPix[1].width(), h);
      p.drawPixmap(0, tabPix[1].height()-h, tabPix[1],
                   0, tabPix[1].height()-h, tabPix[1].width(), h);
      break;
   }
   case RollOut: {
      QPainter p(&tabPix[2]);
      int h = (7-animStep)*tabPix[1].height()/7;
      int y = (tabPix[1].height()-h)/2;
      p.drawPixmap(0, y, tabPix[1], 0, y, tabPix[1].width(), h);
      break;
   }
   case OpenVertically: {
      tabPix[2] = tabPix[1];
      QPainter p(&tabPix[2]);
      int h = animStep*tabPix[0].height()/14;
      p.drawPixmap(0,0,tabPix[0],0,tabPix[0].height()/2-h,
                  tabPix[0].width(),h);
      p.drawPixmap(0,tabPix[0].height()-h,tabPix[0],
                  0,tabPix[0].height()/2,tabPix[0].width(),h);
      break;
   }
   case CloseVertically: {
      QPainter p(&tabPix[2]);
      int h = (7-animStep)*tabPix[1].height()/14;
      p.drawPixmap(0, 0, tabPix[1],
                   0, tabPix[1].height()/2-h, tabPix[1].width(), h);
      p.drawPixmap(0, tabPix[1].height()-h, tabPix[1],
                   0, tabPix[1].height()/2, tabPix[1].width(), h);
      break;
   }
   case OpenHorizontally: {
      tabPix[2] = tabPix[1];
      QPainter p(&tabPix[2]);
      int w = animStep*tabPix[0].width()/14;
      p.drawPixmap(0,0,tabPix[0],tabPix[0].width()/2-w,0,
                  w,tabPix[0].height());
      p.drawPixmap(tabPix[0].width()-w,0,tabPix[0],
                  tabPix[0].width()/2,0,w,tabPix[0].height());
      break;
   }
   case CloseHorizontally: {
      QPainter p(&tabPix[2]);
      int w = (7-animStep)*tabPix[1].width()/14;
      p.drawPixmap(0, 0, tabPix[1],
                   tabPix[1].width()/2-w, 0, w, tabPix[1].height());
      p.drawPixmap(tabPix[1].width()-w, 0, tabPix[1],
                   tabPix[1].width()/2, 0, w, tabPix[1].height());
      break;
   }
   }
}

bool StyleAnimator::eventFilter( QObject* object, QEvent *e ) {
   switch (e->type()) {
   case QEvent::Show: {
      if (QProgressBar *progress = qobject_cast<QProgressBar*>(object))
      if (progress->isEnabled()) {
         addProgressBar(progress);
         return false;
      }
      if (QTabWidget* tab = qobject_cast<QTabWidget*>(object)) {
         addTab(tab, tab->currentIndex());
         return false;
      }
      return false;
   }
   case QEvent::Hide: {
      if (qobject_cast<QProgressBar*>(object) ||
          qobject_cast<QTabWidget*>(object)) {
             remove(static_cast<QWidget*>(object));
         return false;
      }
      return false;
   }
#define HANDLE_SCROLL_AREA_EVENT \
         if (area->horizontalScrollBar()->isVisible())\
            fadeIn(area->horizontalScrollBar());\
         if (area->verticalScrollBar()->isVisible())\
            fadeIn(area->verticalScrollBar());
   case QEvent::Enter:
      if (qobject_cast<QAbstractButton*>(object) ||
          qobject_cast<QComboBox*>(object)) {
         QWidget *widget = (QWidget*)object;
         if (!widget->isEnabled())
            return false;
         fadeIn(widget);
         return false;
      }
      else if (QAbstractScrollArea* area =
          qobject_cast<QAbstractScrollArea*>(object)) {
         if (!area->isEnabled()) return false;
         HANDLE_SCROLL_AREA_EVENT
         return false;
      }
      else if (Q3ScrollView* area =
               qobject_cast<Q3ScrollView*>(object)) {
         if (!area->isEnabled()) return false;
         HANDLE_SCROLL_AREA_EVENT
         return false;
      }
      else if (_scrollAreas.contains(object)) {
         QObjectList kids = object->children();
         QWidget *sb;
         foreach (QObject *kid, kids) {
            if (kid->parent() == object)
            if (sb = qobject_cast<QScrollBar*>(kid))
               fadeIn(sb);
         }
         return false;
      }
      return false;

#undef HANDLE_SCROLL_AREA_EVENT
#define HANDLE_SCROLL_AREA_EVENT \
         if (area->horizontalScrollBar()->isVisible())\
            fadeOut(area->horizontalScrollBar());\
         if (area->verticalScrollBar()->isVisible())\
            fadeOut(area->verticalScrollBar());
   case QEvent::Leave:
      if (qobject_cast<QAbstractButton*>(object) || 
          qobject_cast<QComboBox*>(object)) {
         QWidget *widget = (QWidget*)object;
         if (!widget->isEnabled())
            return false;
         fadeOut(widget);
         return false;
      }
      else if (QAbstractScrollArea* area =
          qobject_cast<QAbstractScrollArea*>(object)) {
         if (!area->isEnabled()) return false;
         HANDLE_SCROLL_AREA_EVENT
         return false;
      }
      else if (Q3ScrollView* area =
               qobject_cast<Q3ScrollView*>(object)) {
         HANDLE_SCROLL_AREA_EVENT
         return false;
      }
      else if (_scrollAreas.contains(object)) {
         QObjectList kids = object->children();
         QWidget *sb;
         foreach (QObject *kid, kids) {
            if (kid->parent() == object)
            if (sb = qobject_cast<QScrollBar*>(kid))
               fadeOut(sb);
         }
         return false;
      }
      return false;
#undef HANDLE_SCROLL_AREA_EVENT
#if 0
   case QEvent::FocusIn:
      if (qobject_cast<QAbstractButton*>(object) ||
          qobject_cast<QComboBox*>(object)) {
         QWidget *widget = (QWidget*)object;
         if (!widget->isEnabled()) return false;
         if (widget->testAttribute(Qt::WA_UnderMouse))
            widget->repaint();
         else
            animator->fadeIn(widget);
         return false;
      }
      return false;
   case QEvent::FocusOut:
      if (qobject_cast<QAbstractButton*>(object) || 
          qobject_cast<QComboBox*>(object)) {
         QWidget *widget = (QWidget*)object;
         if (!widget->isEnabled()) return false;
         if (widget->testAttribute(Qt::WA_UnderMouse))
            widget->repaint();
         else
            animator->fadeOut((QWidget*)(object));
         return false;
      }
      return false;
#endif
   case QEvent::EnabledChange:
   if (QWidget* progress = qobject_cast<QProgressBar*>(object)) {
      if (progress->isEnabled())
         addProgressBar(progress);
      else
         remove(progress);
      return false;
   }
   if (QTabWidget* tab = qobject_cast<QTabWidget*>(object)) {
      if (tab->isEnabled())
         addTab(tab, tab->currentIndex());
      else
         remove(tab);
      return false;
   }
   return false;
   default:
      return false;
   }
}

void StyleAnimator::addTab(QTabWidget* tab, int currentIndex) {
   if (tabwidgets.contains(tab)) return; // accidental double add
   tabwidgets[tab] = new TabAnimInfo(tab, currentIndex);
   connect(tab, SIGNAL(currentChanged(int)),
           this, SLOT(tabChanged(int)));
   connect(tab, SIGNAL(destroyed(QObject*)),
           this, SLOT(destroyed(QObject*)));
   startTimer;
}

void StyleAnimator::addProgressBar(QWidget* progress) {
   if (progressbars.contains(progress)) return; // accidental double add
   progressbars[progress] = 0;
   connect(progress, SIGNAL(destroyed(QObject*)),
           this, SLOT(destroyed(QObject*)));
   startTimer;
}

void StyleAnimator::addScrollArea(QWidget *area) {
   if (!area) return;
   area->installEventFilter(this);
   if (qobject_cast<QAbstractScrollArea*>(area))
      return;
   if (_scrollAreas.contains(area))
      return;
   _scrollAreas.append(area);
}

#include <QStyleOption>

// to get an idea about what the bg of out tabs looks like - seems as if we
// need to paint it
static QPixmap dumpBackground(QWidget *target, const QRect &r, const QStyle *style) {
   if (!target) return QPixmap();
   QPoint zero(0,0);
   QPixmap pix(r.size());
   QWidgetList widgets; widgets << target;
   QWidget *w = target->parentWidget();
   while (w) {
      if (!w->isVisible()) { w = w->parentWidget(); continue; }
      widgets << w;
      if (w->isTopLevel() || w->autoFillBackground()) break;
      w = w->parentWidget();
   }

   QPainter p(&pix);
   const QBrush bg = w->palette().brush(w->backgroundRole());
   if (bg.style() == Qt::TexturePattern)
      p.drawTiledPixmap(pix.rect(), bg.texture(), target->mapTo(w, r.topLeft()));
   else
      p.fillRect(pix.rect(), bg);
   
   if (w->isTopLevel() && w->testAttribute(Qt::WA_StyledBackground)) {
      QStyleOption opt; opt.initFrom(w);// opt.rect = r;
      opt.rect.translate(-target->mapTo(w, r.topLeft()));
      style->drawPrimitive ( QStyle::PE_Widget, &opt, &p, w);
   }
   p.end();

   QPaintEvent e(r); int i = widgets.size();
   while (i) {
      w = widgets.at(--i);
      QPainter::setRedirected( w, &pix, target->mapTo(w, r.topLeft()) );
      e = QPaintEvent(QRect(zero, r.size()));
      QCoreApplication::sendEvent(w, &e);
      QPainter::restoreRedirected(w);
   }
   return pix;
}

static inline QAbstractScrollArea* scrollAncestor(QWidget *w, QWidget *root) {
   QWidget *parent = w;
   while (parent != root && (parent = parent->parentWidget())) {
      if (qobject_cast<QAbstractScrollArea*>(parent)) break;
   }
   if (parent != root) return static_cast<QAbstractScrollArea*>(parent);
   return 0L;
}

#include <QtDebug>

// QPixmap::grabWidget(.) currently fails on the background offset,
// so we use our own implementation
//TODO: fix scrollareas (the scrollbars aren't painted, so check for availability and usage...)
static void grabWidget(QWidget * root, QPixmap *pix) {
    if (!root)
        return;

   QPoint zero(0,0);
   
   QWidgetList widgets = root->findChildren<QWidget*>();
   
   // resizing (in case) -- NOTICE may be dropped for performance...?!
//    if (root->testAttribute(Qt::WA_PendingResizeEvent) ||
//        !root->testAttribute(Qt::WA_WState_Created)) {
//       QResizeEvent e(root->size(), QSize());
//       QApplication::sendEvent(root, &e);
//    }
//    foreach (QWidget *w, widgets) {
//       if (root->testAttribute(Qt::WA_PendingResizeEvent) ||
//          !root->testAttribute(Qt::WA_WState_Created)) {
//          QResizeEvent e(w->size(), QSize());
//          QApplication::sendEvent(w, &e);
//       }
//    }
   
   // painting ------------
   QPainter::setRedirected( root, pix );
   QPaintEvent e(QRect(zero, root->size()));
   QCoreApplication::sendEvent(root, &e);
   QPainter::restoreRedirected(root);
   
   bool hasScrollAreas = false;
   QAbstractScrollArea *scrollarea = 0;
   QPainter p; QRegion rgn;
   QPixmap *saPix = 0L;
   
   foreach (QWidget *w, widgets) {
      if (w->isVisibleTo(root)) {
         
         // solids
         if (w->autoFillBackground()) {
            const QBrush bg = w->palette().brush(w->backgroundRole());
            p.begin(pix);
            QRect wrect = QRect(zero, w->size()).translated(w->mapTo(root, zero));
            if (bg.style() == Qt::TexturePattern)
               p.drawTiledPixmap(wrect, bg.texture(),
                                 w->mapTo(root->window(), zero));
            else
               p.fillRect(wrect, bg);
            p.end();
         }
         
         // scrollarea workaround
         if (scrollarea = qobject_cast<QAbstractScrollArea*>(w))
            hasScrollAreas = true;
         if (hasScrollAreas && !qobject_cast<QScrollBar*>(w) &&
             (scrollarea = scrollAncestor(w, root))) {
            QRect rect = scrollarea->frameRect();
            rect.translate(scrollarea->mapTo(root, zero));
            if (!saPix || saPix->size() != rect.size()) {
               delete saPix; saPix = new QPixmap(rect.size());
            }
            p.begin(saPix); p.drawPixmap(zero, *pix, rect);
            p.end();
            const QPoint &pt = scrollarea->frameRect().topLeft();
            QPainter::setRedirected( w, saPix, w->mapFrom(scrollarea, pt) );
            e = QPaintEvent(QRect(zero, w->size()));
            QCoreApplication::sendEvent(w, &e);
            QPainter::restoreRedirected(w);
            p.begin(pix); p.drawPixmap(rect.topLeft(), *saPix); p.end();
         }
         // default painting redirection
         else {
            QPainter::setRedirected( w, pix, w->mapFrom(root, zero) );
            e = QPaintEvent(QRect(zero, w->size()));
            QCoreApplication::sendEvent(w, &e);
            QPainter::restoreRedirected(w);
         }
      }
   }
   delete saPix;
}

void StyleAnimator::destroyed(QObject *obj) {
   tabwidgets.remove(qobject_cast<QTabWidget*>(obj));
   progressbars.remove(static_cast<QWidget*>(obj));
   hoverWidgets.remove(static_cast<QWidget*>(obj));
   complexHoverWidgets.remove(static_cast<QWidget*>(obj));
   indexedHoverWidgets.remove(static_cast<QWidget*>(obj));
   if (!ANIMATIONS) timer->stop();
}

void StyleAnimator::remove(QWidget *w) {
   disconnect(w, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
   if (QTabWidget* tab = qobject_cast<QTabWidget*>(w)) {
      tabwidgets.remove(tab);
      disconnect(tab, SIGNAL(currentChanged(int)),
                 this, SLOT(tabChanged(int)));
   }
   if (QProgressBar *progress = qobject_cast<QProgressBar*>(w))
      progressbars.remove(progress);
   w->removeEventFilter(this);
   if (!ANIMATIONS) timer->stop();
}

void StyleAnimator::updateProgressbars() {
   if (progressbars.isEmpty())
      return;
   //Update the registered progressbars.
   QHash<QWidget*, int>::iterator iter;
   QProgressBar *pb;
   animationUpdate = true;
   for (iter = progressbars.begin(); iter != progressbars.end(); iter++) {
      if ( !qobject_cast<QProgressBar*>(iter.key()) )
         continue;
      pb = (QProgressBar*)(iter.key());
      if (pb->paintingActive() || !pb->isVisible() ||
          !(pb->value() > pb->minimum()) || !(pb->value() < pb->maximum()))
         continue;

      ++iter.value();
      
      if (iter.value() > 18) iter.value() = -18;
      
      if (iter.value() % 2) // animate only every 2nd step (to save cpu)
         pb->repaint(pb->rect().adjusted(2,2,-2,-2));
   }
   animationUpdate = false;
}

int StyleAnimator::progressStep(const QWidget *w) const {
   return qAbs(progressbars.value(const_cast<QWidget*>(w),0));
}

// --- TabWidgets --------------------
void StyleAnimator::tabChanged(int index) {
   if (tabTransition == TabAnimInfo::Jump) return; // ugly nothing ;)
   QTabWidget* tw = (QTabWidget*)sender();
   if (!tw->currentWidget()) return;
   QHash<QTabWidget*, TabAnimInfo*>::iterator i = tabwidgets.find(tw);
   if (i == tabwidgets.end()) // this tab isn't handled for some reason?
      return;
   
   TabAnimInfo* tai = i.value();
   
   QWidget *ctw = tw->widget(tai->lastTab);
   tai->lastTab = index;
   if (!ctw) return;
   tai->tabPix[0] = tai->tabPix[1] =
      dumpBackground(tw, QRect(ctw->mapTo(tw, QPoint(0,0)), ctw->size()),
                     qApp->style());
   grabWidget(ctw, &tai->tabPix[0]);
   tai->tabPix[2] = tai->tabPix[0];
   ctw = tw->currentWidget();
   grabWidget(ctw, &tai->tabPix[1]);
   
   tai->animStep = 6;
   tai->updatePixmaps(tabTransition);
   ctw->parentWidget()->installEventFilter(tai);
   _BLOCKEVENTS_(ctw);
   QList<QWidget*> widgets = ctw->findChildren<QWidget*>();
   foreach(QWidget *widget, widgets) {
      _BLOCKEVENTS_(widget);
      if (widget->autoFillBackground()) {
         tai->autofillingWidgets.append(widget);
         widget->setAutoFillBackground(false);
      }
   }
   ctw->repaint();
   startTimer;
}

void StyleAnimator::updateTabAnimation() {
   if (tabwidgets.isEmpty())
      return;
   QHash<QTabWidget*, TabAnimInfo*>::iterator i;
   activeTabs = 0;
   TabAnimInfo* tai;
   QWidget *ctw = 0, *widget = 0; QList<QWidget*> widgets;
   int index;
   for (i = tabwidgets.begin(); i != tabwidgets.end(); i++) {
      tai = i.value();
      if (!tai->animStep)
         continue;
      ctw = i.key()->currentWidget();
      if (! --(tai->animStep)) { // zero, stop animation
         tai->tabPix[2] =
            tai->tabPix[1] =
            tai->tabPix[0] = QPixmap();
         ctw->parentWidget()->removeEventFilter(tai);
         _UNBLOCKEVENTS_(ctw);
         widgets = ctw->findChildren<QWidget*>();
//          ctw->repaint();
         foreach(widget, widgets) {
            index = tai->autofillingWidgets.indexOf(widget);
            if (index != -1) {
               tai->autofillingWidgets.removeAt(index);
               widget->setAutoFillBackground(true);
            }
            _UNBLOCKEVENTS_(widget);
            widget->update(); //if necessary
         }
         ctw->repaint(); //asap
         tai->autofillingWidgets.clear();
         continue;
      }
      ++activeTabs;
      tai->updatePixmaps(tabTransition);
      ctw->parentWidget()->update();
   }
   if (!ANIMATIONS) timer->stop();
}


// -- Buttons etc. -------------------------------
void StyleAnimator::updateFades() {
   if (hoverWidgets.isEmpty())
      return;
   HoverFades::iterator it = hoverWidgets.begin();
   while (it != hoverWidgets.end()) {
      if (it.value().fadeIn) {
         it.value().step += 2;
         it.key()->update();
         if (it.value().step > 4)
            it = hoverWidgets.erase(it);
         else
            ++it;
      }
      else { // fade out
         --it.value().step;
         it.key()->update();
         if (it.value().step < 1)
            it = hoverWidgets.erase(it);
         else
            ++it;
      }
   }
   if (!ANIMATIONS) timer->stop();
}

void StyleAnimator::fadeIn(QWidget *widget) {
   HoverFades::iterator it = hoverWidgets.find(widget);
   if (it == hoverWidgets.end()) {
      it = hoverWidgets.insert(widget, HoverFadeInfo(1, true));
   }
   it.value().fadeIn = true;
   connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
   startTimer;
}

void StyleAnimator::fadeOut(QWidget *widget) {
   HoverFades::iterator it = hoverWidgets.find(widget);
   if (it == hoverWidgets.end()) {
      it = hoverWidgets.insert(widget, HoverFadeInfo(6, false));
   }
   it.value().fadeIn = false;
   connect(widget, SIGNAL(destroyed(QObject*)), this, SLOT(destroyed(QObject*)));
   startTimer;
}

int StyleAnimator::hoverStep(const QWidget *widget) const {
   if (!widget || !widget->isEnabled())
      return 0;
   HoverFades::iterator it = hoverWidgets.find(const_cast<QWidget*>(widget));
   if (it != hoverWidgets.end())
      return it.value().step;
   if (widget->testAttribute(Qt::WA_UnderMouse))
      return 6;
   return 0;
}

// -- Complex controls ----------------------

void StyleAnimator::updateComplexFades() {
   if (complexHoverWidgets.isEmpty())
      return;
   bool update;
   ComplexHoverFades::iterator it = complexHoverWidgets.begin();
   while (it != complexHoverWidgets.end()) {
      ComplexHoverFadeInfo &info = it.value();
      update = false;
      for (QStyle::SubControl control = (QStyle::SubControl)0x01;
           control <= (QStyle::SubControl)0x80;
           control = (QStyle::SubControl)(control<<1)) {
         if (info.fadeIns & control) {
            update = true;
            info.steps[control] += 2;
            if (info.steps.value(control) > 4)
               info.fadeIns &= ~control;
         }
         else if (info.fadeOuts & control) {
            update = true;
            --info.steps[control];
            if (info.steps.value(control) < 1)
               info.fadeOuts &= ~control;
         }
      }
      if (update)
         it.key()->update();
      if (info.activeSubControls == QStyle::SC_None && // needed to detect changes!
          info.fadeOuts == QStyle::SC_None &&
          info.fadeIns == QStyle::SC_None)
         it = complexHoverWidgets.erase(it);
      else
         ++it;
   }
   if (!ANIMATIONS) timer->stop();
}

const ComplexHoverFadeInfo *StyleAnimator::fadeInfo(const QWidget *widget,
   QStyle::SubControls activeSubControls) const {
      QWidget *w = const_cast<QWidget*>(widget);
   ComplexHoverFades::iterator it = complexHoverWidgets.find(w);
   if (it == complexHoverWidgets.end()) {
      // we have no entry yet
      if (activeSubControls == QStyle::SC_None)
         return 0; // no need here
      // ...but we'll need one
      it = complexHoverWidgets.insert(w, ComplexHoverFadeInfo());
      connect(widget, SIGNAL(destroyed(QObject*)),
               this, SLOT(destroyed(QObject*)));
      startTimer;
   }
   // we now have an entry - check for validity and update in case
   ComplexHoverFadeInfo *info = &it.value();
   if (info->activeSubControls != activeSubControls) { // sth. changed
      QStyle::SubControls diff = info->activeSubControls ^ activeSubControls;
      QStyle::SubControls newActive = diff & activeSubControls;
      QStyle::SubControls newDead = diff & info->activeSubControls;
      info->fadeIns &= ~newDead;
      info->fadeIns |= newActive;
      info->fadeOuts &= ~newActive;
      info->fadeOuts |= newDead;
      info->activeSubControls = activeSubControls;
      for (QStyle::SubControl control = (QStyle::SubControl)0x01;
      control <= (QStyle::SubControl)0x80;
      control = (QStyle::SubControl)(control<<1)) {
         if (newActive & control)
            info->steps[control] = 1;
         else if (newDead & control) {
            info->steps[control] = 6;
         }
      }
   }
   return info;
}

// -- Indexed items like menus, tabs ---------------------
void StyleAnimator::updateIndexedFades() {
   if (indexedHoverWidgets.isEmpty())
      return;
   IndexedFades::iterator it;
   QHash<long int, int>::iterator stepIt;
   it = indexedHoverWidgets.begin();
   while (it != indexedHoverWidgets.end()) {
      IndexedFadeInfo &info = it.value();
      if (info.fadeIns.isEmpty() && info.fadeOuts.isEmpty()) {
         ++it;
         continue;
      }
      
      stepIt = info.fadeIns.begin();
      while (stepIt != info.fadeIns.end()) {
         stepIt.value() += 2;
         if (stepIt.value() > 4)
            stepIt = info.fadeIns.erase(stepIt);
         else
            ++stepIt;
      }
      
      stepIt = info.fadeOuts.begin();
      while (stepIt != info.fadeOuts.end()) {
         --stepIt.value();
         if (stepIt.value() < 1)
            stepIt = info.fadeOuts.erase(stepIt);
         else
            ++stepIt;
      }
      
      it.key()->update();
      
      if (info.index == 0L && // nothing actually hovered
          info.fadeIns.isEmpty() && // no fade ins
          info.fadeOuts.isEmpty()) // no fade outs
         it = indexedHoverWidgets.erase(it);
      else
         ++it;
   }
   if (!ANIMATIONS) timer->stop();
}

const IndexedFadeInfo *StyleAnimator::fadeInfo(const QWidget *widget,
   long int index) const {
   QWidget *w = const_cast<QWidget*>(widget);
   IndexedFades::iterator it = indexedHoverWidgets.find(w);
   if (it == indexedHoverWidgets.end()) {
      // we have no entry yet
      if (index == 0L)
         return 0L;
      // ... but we'll need one
      it = indexedHoverWidgets.insert(w, IndexedFadeInfo(0L));
      connect(widget, SIGNAL(destroyed(QObject*)),
               this, SLOT(destroyed(QObject*)));
      startTimer;
   }
   // we now have an entry - check for validity and update in case
   IndexedFadeInfo *info = &it.value();
   if (info->index != index) { // sth. changed
      info->fadeIns[index] = 1;
      if (info->index)
         info->fadeOuts[info->index] = 6;
      info->index = index;
   }
   return info;
}

int IndexedFadeInfo::step(long int index) const {
   typedef QHash<long int, int> Index2Step;
   Index2Step::const_iterator stepIt;
   for (stepIt = fadeIns.begin(); stepIt != fadeIns.end(); stepIt++)
      if (stepIt.key() == index)
         return stepIt.value();
   for (stepIt = fadeOuts.begin(); stepIt != fadeOuts.end(); stepIt++)
      if (stepIt.key() == index)
         return stepIt.value();
   return 0;
}

