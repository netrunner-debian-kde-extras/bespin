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

#include <QPixmap>
#include <QTime>
#include "basic.h"

namespace Animator {

enum Transition {Jump = 0, ScanlineBlend, SlideIn, SlideOut,
RollIn, RollOut, OpenVertically, CloseVertically, OpenHorizontally,
CloseHorizontally
#ifndef QT_NO_XRENDER
, CrossFade
#endif
};

class TabInfo : public QObject {
   public:
      TabInfo(QObject* parent, QWidget *currentWidget = 0, int index = -1);
      void updatePixmaps(Transition transition);
   protected:
      friend class Tab;
      bool eventFilter( QObject* object, QEvent* event );
      float progress;
      QWidget *currentWidget;
      int index;
      QList < QWidget* > autofilling, opaque;
      QPixmap tabPix[3];
      QTime clock;
};

class Tab : public Basic {
   Q_OBJECT
   public:
      static bool manage(QWidget *w);
      static void release(QWidget *w);
      static void setDuration(uint ms);
      static void setFPS(uint fps);
      static void setTransition(Transition t);

   protected:
      Tab();
      virtual bool _manage(QWidget *w);
      virtual void _release(QWidget *w);
      virtual void timerEvent(QTimerEvent * event);
      typedef QHash<const QWidget*, TabInfo*> Items;
      Items items;
      int _activeTabs;
   protected slots:
      void changed(int);
};

}; //namespace
