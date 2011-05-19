/* Bespin widget style for Qt4
 * Copyright (C) 2011 Thomas Luebking <thomas.luebking@web.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef SPLITTERPROXY_H
#define SPLITTERPROXY_H

#include <QHoverEvent>
#include <QMainWindow>
#include <QMouseEvent>
#include <QSplitterHandle>
#include <QWidget>

class SplitterProxy;
static SplitterProxy *splitterProxy = 0;

class SplitterProxy : public QWidget
{
public:
    static bool manage(QWidget *w)
    {
        if (qobject_cast<QMainWindow*>(w) || qobject_cast<QSplitterHandle*>(w))
        {
            if (!splitterProxy)
                splitterProxy = new SplitterProxy;
            // avoid double filtering
            w->removeEventFilter(splitterProxy);
            w->installEventFilter(splitterProxy);
            return true;
        }
        return false;
    }

    SplitterProxy() : QWidget() { hide(); }

protected:
    bool event(QEvent *e)
    {
        switch (e->type())
        {
        case QEvent::Paint:
            return true;
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        {
            e->accept();

            if (e->type() == QEvent::MouseButtonPress)
                grabMouse();

            resize(1,1);

            QMouseEvent *me = static_cast<QMouseEvent*>(e);
            const QPoint pos = (e->type() == QEvent::MouseMove) ? mySplitter->mapFromGlobal(QCursor::pos()) : myHook;
            QMouseEvent me2(me->type(), pos, mySplitter->mapToGlobal(pos), me->button(), me->buttons(), me->modifiers());
            QCoreApplication::sendEvent(mySplitter, &me2);

            if (e->type() == QEvent::MouseButtonRelease)
                setSplitter(0);
            return true;
        }
        case QEvent::Leave:
        {
            QWidget::leaveEvent(e);
            if (!rect().contains(mapFromGlobal(QCursor::pos())))
                setSplitter(0);
            return true;
        }
        default:
            return QWidget::event(e);
        }
    }
    bool eventFilter(QObject *o, QEvent *e)
    {
        if (/*o == mySplitter || */mouseGrabber())
            return false;

        switch (e->type())
        {
        case QEvent::HoverMove:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::MouseMove:
        case QEvent::Timer:
        case QEvent::Move:
            return false; // just for performance - they can occur really often
        case QEvent::CursorChange:
        {
            if (QWidget *window = qobject_cast<QMainWindow*>(o))
            if (window->cursor().shape() == Qt::SplitHCursor ||
                window->cursor().shape() == Qt::SplitVCursor)
                setSplitter(window);
            return false;
        }
        case QEvent::Enter:
            if (QSplitterHandle *handle = qobject_cast<QSplitterHandle*>(o))
            {
                setSplitter(handle);
                return true;
            }
            return false;
        case QEvent::MouseButtonRelease:
            if (qobject_cast<QSplitterHandle*>(o) || qobject_cast<QMainWindow*>(o))
                setSplitter(0);
            return false;
        default:
            return false;
        }
    }
private:
    void setSplitter(QWidget *splt)
    {
        mySplitter = splt;

        if (!mySplitter)
        {
            if (mouseGrabber() == this)
                releaseMouse();
            setParent(0);
            return;
        }

        myHook = mySplitter->mapFromGlobal(QCursor::pos());

        QWidget *w = mySplitter->window();
        QRect r(0,0,48,48);
        r.moveCenter(w->mapFromGlobal(QCursor::pos()));

        setParent(w);
        setGeometry(r);
        setCursor( mySplitter->cursor().shape() );

        raise();
        show();
    }
private:
    QWidget *mySplitter;
    QPoint myHook;
};

#endif // SPLITTERPROXY_H