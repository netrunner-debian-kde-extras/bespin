//////////////////////////////////////////////////////////////////////////////
// button.h
// -------------------
// Bespin window decoration for KDE
// -------------------
// Copyright (c) 2008 Thomas Lübking <baghira-style@web.de>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#ifndef BUTTON_H
#define BUTTON_H

#include <QWidget>

namespace Bespin
{

class Client;

class Button : public QWidget
{
   Q_OBJECT
public:
   enum State { Normal, Hovered, Sunken };
   enum Type { Close = 0, Min, Max, Multi,
   Menu, Help, Above, Below, Stick, Special,
   //    VertMax, HoriMax,
   Restore, Unstick, UnAboveBelow, NumTypes };
   Button(Client *parent, Type type);
   static void init(int sz);
   bool isEnabled() const;
   inline bool type() {return _type;}
protected:
	void enterEvent(QEvent *e);
	void leaveEvent(QEvent *e);
	void mousePressEvent ( QMouseEvent * event );
	void mouseReleaseEvent ( QMouseEvent * event );
	void paintEvent(QPaintEvent *e);
	void timerEvent ( QTimerEvent * event );
	void wheelEvent(QWheelEvent * event);
private:
   QColor color() const;
	bool zoomOut;
	Client *client;
	Type _type;
   int state, multiIdx, zoomTimer, zoomLevel;
   static QPainterPath shape[NumTypes];
   static QString tip[NumTypes];
private slots:
   void clientStateChanged(bool);
};

} //namespace
#endif