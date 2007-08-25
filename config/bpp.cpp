/***************************************************************************
 *   Copyright (C) 2007 by Thomas L�bking                                  *
 *   thomas@bespin-icons.org                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QX11Info>
#include <signal.h>
#include <stdio.h>
#include <cmath>
#include <unistd.h>
#include <sys/types.h>
#include "bpp.h"

static volatile sig_atomic_t fatal_error_in_progress = 0;
static Atom bespin_bgYoff;
static Atom bespin_bgPicture;
static Atom bespin_bgColor;
static Display *dpy;
static WId root;

static const double sqrt_2 = 1.4142135623;

void BespinPP::cleanup(int sig)
{
   QFile::remove(QDir::tempPath() + "bespinPP.lock");
   if (sig > -1) {
      if (fatal_error_in_progress)
         raise (sig);
      fatal_error_in_progress = 1;
   }
   BespinPP::exportPicture(X::None, 0, 0);
   XDeleteProperty( QX11Info::display(), root, bespin_bgPicture);
   XDeleteProperty( dpy, root, bespin_bgYoff);
   XFlush(dpy);
//    delete bgPix; bgPix = 0;
   if (sig > -1) {
      signal (sig, SIG_DFL);
      raise (sig);
   }
}

BespinPP::BespinPP(int & argc, char ** argv) : QApplication( argc, argv ),
bgPix(0)
{
   dpy = QX11Info::display();
   root = desktop()->winId();
   bespin_bgYoff =
      XInternAtom(dpy, "BESPIN_BG_Y_OFFSET", False);
   bespin_bgPicture =
      XInternAtom(dpy, "BESPIN_BG_PICTURE", False);
   bespin_bgColor =
      XInternAtom(dpy, "BESPIN_BG_COLOR", False);
   createPixmap();
   exportPicture(bgPix->x11PictureHandle(), bgPix->height()/6,
                 palette().color(QPalette::Active, QPalette::Background).rgb());
}

void BespinPP::exportPicture(Picture pic, uint offset, uint rgb)
{
   XChangeProperty(QX11Info::display(), root, bespin_bgPicture,
                   XA_CARDINAL, 32, PropModeReplace,
                   (unsigned char *) &(pic), 1L);
   XChangeProperty(QX11Info::display(), root, bespin_bgYoff,
                   XA_CARDINAL, 32, PropModeReplace,
                   (unsigned char *) &(offset), 1L);
   XChangeProperty(QX11Info::display(), root, bespin_bgColor,
                   XA_CARDINAL, 32, PropModeReplace,
                   (unsigned char *) &(rgb), 1L);
}

void BespinPP::createPixmap()
{
   int h,s,v;
   QColor c = palette().color(QPalette::Active, QPalette::Background);
   c.getHsv(&h,&s,&v);
   if (v < 70) // very dark colors won't make nice backgrounds ;)
      c.setHsv(h,s,70);
   
   if (bgPix) delete bgPix;
   bgPix = new QPixmap(desktop()->size());
   int w = desktop()->size().width(); h = desktop()->size().height();
  
   QPainter p(bgPix);
   p.setPen(Qt::NoPen);
   
   QLinearGradient lg(QPoint(0,0), QPoint(0, h));
   lg.setColorAt(0, c.light(104)); lg.setColorAt(0.5, c.dark(104));
   lg.setColorAt(1, c.dark(108)); p.setBrush(lg);
   p.drawRect( 0, 0, w-1, h-1);
   
   int h2 = h - h/6;
   float hypo = sqrt(pow(w,2)+pow(h2,2));
   float cosalpha = ((float)w)/hypo;
   float tc = cosalpha*h2; float tb = cosalpha*tc;
   QPoint p2 = QPoint(w-(int)sqrt(pow(tc,2) - pow(tb,2)), (int)(h - tb));
   lg = QLinearGradient(QPoint(w, h), p2);
   c = c.light(140); QColor trans = c, trans2 = c;
   c.setAlpha(180); lg.setColorAt(0, c);
   trans.setAlpha(130); lg.setColorAt(0.5, trans);
   trans2.setAlpha(0); lg.setColorAt(1, trans2);
   p.setBrush(lg);
   p.drawRect( 0, 0, w-1, h-1);
   p.end();
   
   QPixmap tmp(w, w);
   tmp.fill(Qt::transparent);
   p.begin(&tmp);
   QRadialGradient rg( QPoint(w/2, 0), w );
   rg.setColorAt(0, c); rg.setColorAt(0.5, trans2); rg.setColorAt(1, trans2);
   p.setBrush(rg);
   p.setPen(Qt::NoPen);
   p.drawRect(tmp.rect());
   p.end();
   tmp = tmp.scaled(w, h/6, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
   
   p.begin(bgPix);
   p.drawPixmap(0,0,tmp);
   // scanlines
//    p.setBrush(Qt::NoBrush);
//    p.setPen(palette().color(QPalette::Active, QPalette::Background).dark(108));
//    h = bgPix->height();
//    w = bgPix->width();
//    for (int i = 3; i < h; i+=4)
//       p.drawLine(0, i, w, i);
//    for (int i = 3; i < w; i+=4)
//       p.drawLine(i, 0, i, h);
   p.end();
}