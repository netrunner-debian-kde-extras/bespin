/* Bespin mac-a-like XBar KDE4
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

#ifndef XBAR_H
#define XBAR_H

class QGraphicsLinearLayout;

#include <QMap>
#include <QMenu>
#include <QWidget>

#include <Plasma/Applet>


class XBar : public Plasma::Applet
{
   Q_OBJECT
public:
   XBar(QObject *parent, const QVariantList &args);
   ~XBar();
   void init();
   void registerMenu(const QString &service, qlonglong key, const QString &title, const QStringList &entries);
   void unregisterMenu(qlonglong key);
   QPoint mapToGlobal(const QPointF &pt);
   void reparent(qlonglong oldKey, qlonglong newKey);
   void changeEntry(qlonglong key, int idx, const QString &entry = QString(), bool add = false);
   void setOpenPopup(int idx);
   void requestFocus(qlonglong key);
   void releaseFocus(qlonglong key);
protected:
   void wheelEvent(QGraphicsSceneWheelEvent *ev);
private:
   void hide(MenuBar *item);
   void show(MenuBar *item);
   bool dbusAction(const QObject *o, int idx, const QString &cmd);
private:
   typedef QMap<qlonglong, MenuBar*> MenuMap;
   struct {
      MenuMap menus;
      QMenu windowList;
      MenuBar *taskbar;
      MenuBar *currentBar;
   } d;
private slots:
   void hover(int);
   void trigger(int);
   void updatePalette();
   void updateWindowlist();
   void unregisterCurrentMenu();
   void activateWin();
   void callMenus();
   void byeMenus();
};

#endif //XBAR_H
