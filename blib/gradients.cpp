/*
 *   Bespin library for Qt style, KWin decoration and everythng else
 *   Copyright 2007-2012 by Thomas Lübking <thomas.luebking@gmail.com>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QCache>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>

#include "colors.h"
#include "gradients.h"
#include "../makros.h"
#include "FX.h"

using namespace Bespin;

typedef QCache<uint, QPixmap> PixmapCache;
static QPixmap nullPix;

// static Gradients::Type _progressBase = Gradients::Glass;

/* ========= MAGIC NUMBERS ARE COOL ;) =================
Ok, we want to cache the gradients, but unfortunately we have no idea about
what kind of gradients will be demanded in the future
Thus creating a 2 component map (uint for color and uint for size)
would be some overhead and cause a lot of unused space in the dictionaries -
while hashing by a string is stupid slow ;)

So we store all the gradients by a uint index
Therefore we squeeze the colors to 21 bit (rgba, 6665) and store the size in the
remaining 9 bits
As this would limit the size to 512 pixels we'll be a bit sloppy.
We use size progressing clusters (25). Each stores twenty values and is n-1 px
sloppy. This way we can manage up to 6800px!
So the handled size is actually demandedSize + (demandedSize % sizeSloppyness),
beeing at least demanded size and the next sloppy size above at max
====================================================== */
static inline uint
hash(int size, const QColor &c, int *sloppyAdd)
{
    uint magicNumber = 0;
    // this IS functionizable, but includes a sqrt, some multiplications and
    // subtractions
    // as the loop is typically iterated < 4, it's faster this way for our
    // purpose
    int sizeSloppyness = 1, frameBase = 0, frameSize = 20;
    while ((frameBase += frameSize) < size)
    {
        ++sizeSloppyness;
        frameSize += 20;
    }
    frameBase -=frameSize; frameSize -= 20;

    *sloppyAdd = size % sizeSloppyness;
    if (!*sloppyAdd)
        *sloppyAdd = sizeSloppyness;

    // first 9 bits to store the size, remaining 23 bits for the color (6bpc, 5b alpha)
    magicNumber =
        (((frameSize + (size - frameBase)/sizeSloppyness) & 0x1ff) << 23) |
        (((c.red() >> 2) & 0x3f) << 17) |
        (((c.green() >> 2) & 0x3f) << 11 ) |
        (((c.blue() >> 2) & 0x3f) << 5 ) |
        ((c.alpha() >> 3) & 0x1f);

    return magicNumber;
}

static QPixmap*
newPix(int size, Qt::Orientation o, QPoint *start, QPoint *stop, int other = 32)
{
    QPixmap *pix;
    if (o == Qt::Horizontal)
    {
        pix = new QPixmap(size, other);
        *start = QPoint(0, other); *stop = QPoint(pix->width(), other);
    }
    else
    {
        pix = new QPixmap(other, size);
        *start = QPoint(other, 0); *stop = QPoint(other, pix->height());
    }
    return pix;
}


static inline QLinearGradient
simpleGradient(const QColor &c, const QPoint &start, const QPoint &stop)
{
    QLinearGradient lg(start, stop);
    lg.setColorAt(0, Gradients::endColor(c, Gradients::Top, Gradients::Simple));
    lg.setColorAt(1, Gradients::endColor(c, Gradients::Bottom, Gradients::Simple));
    return lg;
}

static inline QLinearGradient
shinyGradient(const QColor &c, const QPoint &start, const QPoint &stop)
{
    QLinearGradient lg(start, stop);
    lg.setColorAt(0.1, Gradients::endColor(c, Gradients::Top, Gradients::Shiny));
    lg.setColorAt(0.9, Gradients::endColor(c, Gradients::Bottom, Gradients::Shiny));
    return lg;
}

static inline QLinearGradient
metalGradient(const QColor &c, const QPoint &start, const QPoint &stop)
{
    QLinearGradient lg(start, stop);
#if 0
    lg.setColorAt( 0, Colors::mid(c, Qt::white, 1, 1) );
    lg.setColorAt(0.45, Colors::mid(c, Qt::white, 2, 1) );
    lg.setColorAt(0.451, Colors::mid(c, Qt::black, 5, 1) );
    lg.setColorAt( 1, Colors::mid(c, Qt::black, 7, 1) );
#else
    QColor ic = c;
    int h,s,v,a;
    ic.getHsv(&h,&s,&v,&a);
    ic.setHsv(h,s,qMin(255,v+12),a); lg.setColorAt(0, ic);
    ic.setHsv(h,s,qMin(255,v+7),a); lg.setColorAt(0.55, ic);
    ic.setHsv(h,s,qMax(0,v-7),a); lg.setColorAt(0.551, ic);
    ic.setHsv(h,s,qMax(0,v-12),a); lg.setColorAt(1, ic);
#endif
    return lg;
}

static inline QLinearGradient
sunkenGradient(const QColor &c, const QPoint &start, const QPoint &stop)
{
    QLinearGradient lg(start, stop);
    lg.setColorAt(0, Gradients::endColor(c, Gradients::Top, Gradients::Sunken));
    lg.setColorAt(1, Gradients::endColor(c, Gradients::Bottom, Gradients::Sunken));
    return lg;
}

static inline QLinearGradient
buttonGradient(const QColor &c, const QPoint &start, const QPoint &stop)
{
    int h,s,v,a, inc = 15, dec = 6;
    c.getHsv(&h,&s,&v,&a);

    // calc difference
    if (v+inc > 255)
    {
        inc = 255-v;
        dec += (15-inc);
    }

    QLinearGradient lg(start, stop);
    QColor ic;
    ic.setHsv(h,s,v+inc, a);
    lg.setColorAt(0, ic);
    ic.setHsv(h,s,v-dec, a);
    lg.setColorAt(0.75, ic);
    return lg;
}

inline static void
gl_ssColors(const QColor &c, QColor *bb, QColor *dd, bool glass = false)
{
#if 0
    // this is a very trivial approach using shading. the output is poor
    const int sum = c.red() + c.green() + c.blue() + 1;
    const int fc = sum/255 + 1, fs = 3*255/sum;
    *bb = Colors::mid(Qt::white, c, fs, fc);
    *dd = Colors::mid(c, Qt::black, fc, fs);
#else
    int h,s,v,a;
    c.getHsv(&h,&s,&v,&a);

    // calculate the variation
    int add = (180 - v ) / 1;
    if (add < 0)
        add = -add/2;
    add /= glass ? 48 : 96;//(glass ? 48 : 512/qMax(v,1));

    // the brightest color (top)
    int cv = v + 27 + add, ch = h, cs = s;
    if (bb)
    {
        if (cv > 255)
        {
            int delta = cv - 255;
            cv = 255;
            cs = s - (glass?6:2)*delta;
            if (cs < 0)
                cs = 0;
            ch = h - 3*delta/2;
            while (ch < 0)
                ch += 360;
        }
        bb->setHsv(ch,cs,cv,a);
    }
    if (dd)
    {
        // the darkest color (lower center)
        cv = v - 14 - add;
        if (cv < 0)
            cv = 0;
        cs = s*(glass ? 13 : 10)/7;
        if (cs > 255) cs = 255;
        dd->setHsv(h,cs,cv,a);
    }
#endif
}

static const float p[2][3] = { {.25, .5, .8}, {.35, .42, 1.} };

static inline QLinearGradient
gl_ssGradient(const QColor &c, const QPoint &start, const QPoint &stop, bool glass = false)
{
    QColor bb,dd; // b = d = c;
    gl_ssColors(c, &bb, &dd, glass);
    QLinearGradient lg(start, stop);
    lg.setColorAt(0, bb);
    lg.setColorAt(p[glass][0], c);
    lg.setColorAt(p[glass][1], dd);
    lg.setColorAt(p[glass][2], bb);
//     if (!glass)
//         lg.setColorAt(1, Qt::white);
    return lg;
}
#if 0
static inline QPixmap *
rGlossGradient(const QColor &c, int size)
{
    QColor bb,dd; // b = d = c;
    gl_ssColors(c, &bb, &dd);
    QPixmap *pix = new QPixmap(size, size);
    QRadialGradient rg(2*pix->width()/3, pix->height(), pix->height());
    rg.setColorAt(0,c); rg.setColorAt(0.8,dd);
    rg.setColorAt(0.8, c); rg.setColorAt(1, bb);
    QPainter p(pix); p.fillRect(pix->rect(), rg); p.end();
    return pix;
}
#endif
static inline QPixmap *
progressGradient(const QColor &c, int size, Qt::Orientation o)
{
#define GLASS true
#define GLOSS false
// in addition, glosses should have the last stop at 0.9

   // some psychovisual stuff, we search a dark & bright surrounding and
   // slightly shift hue as well (e.g. for green the dark color will slide to
   // blue and the bright one to yellow - A LITTLE! ;)
    int h,s,v,a;
    c.getHsv(&h,&s,&v,&a);
    QColor dkC = c, ltC = c;
    int dv = 4*(v-70)/45; // v == 70 -> dv = 0, v = 255 -> dv = 12
//    int th = h + 400;
    int dh = qAbs((h % 120)-60)/6;
    dkC.setHsv(CLAMP(h+dh,0,360), s, v - dv, a);
    h -= dh+5;
    if (h < 0) h += 360;
    dv = 12 - dv; // NOTICE 12 from above...
    ltC.setHsv(h, s, qMin(v + dv,255), a);

//    int dc = Colors::value(c)/5; // how much darken/lighten we will
//    QColor dkC = c.dark(100+sqrt(2*dc));
//    QColor ltC = c.light(150-dc);

    QPoint start, stop;
    QPixmap *dark = newPix(size, o, &start, &stop, 4*size);
    QPixmap *pix = new QPixmap(dark->size());

    if (c.alpha() < 255)
    {
       dark->fill(Qt::transparent);
       pix->fill(Qt::transparent);
    }
    QGradient lg1 = gl_ssGradient(ltC, start, stop, true),
              lg2 = gl_ssGradient(dkC, start, stop, true);

    QPainter p(dark); p.fillRect(dark->rect(), lg1); p.end();

    QPixmap alpha = QPixmap(dark->size());
    QRadialGradient rg;
    if (o == Qt::Horizontal)
        rg = QRadialGradient(alpha.rect().center(), 10*size/4);
    else
        rg = QRadialGradient(alpha.width()/2, 5*alpha.height()/3, 10*size/4);
    rg.setColorAt(0, Qt::white);

    rg.setColorAt(0.9, Qt::transparent);
    alpha.fill(Qt::transparent);
    p.begin(&alpha); p.fillRect(alpha.rect(), rg); p.end();
    alpha = FX::applyAlpha(*dark, alpha);

    p.begin(pix);
    p.fillRect(pix->rect(), lg2);
    p.drawPixmap(0,0, alpha);
    p.end();

    delete dark;
    return pix;
#undef GLASS
#undef GLOSS
}

static inline uint costs(BgSet *set)
{
    return (set->topTile.width()*set->topTile.height() +
            set->btmTile.width()*set->btmTile.height() +
            set->cornerTile.width()*set->cornerTile.height() +
            set->lCorner.width()*set->lCorner.height() +
            set->rCorner.width()*set->rCorner.height())*set->topTile.depth()/8;
}

static int _struct = 0;
static int _bgIntensity = 110;
static bool _invertedGroups = false;
static Gradients::BgMode _mode = Gradients::BevelV;
static PixmapCache _btnAmbient, _tabShadow, _groupLight, _structure[2];
typedef QCache<uint, BgSet> BgSetCache;
static BgSetCache _bgSet;
static PixmapCache gradients[2][Gradients::TypeAmount-1];
static PixmapCache _borderline[4];

static inline uint costs(QPixmap *pix)
{
    return ((pix->width()*pix->height()*pix->depth())>>3);
}

const QPixmap&
Gradients::borderline(const QColor &c, Position pos)
{
    QPixmap *pix = _borderline[pos].object(c.rgba());
    if (pix)
        return *pix;

    QColor c2 = c; c2.setAlpha(0);

    QPoint start(0,0), stop;
    if (pos > Bottom)
    { pix = new QPixmap(32, 1); stop = QPoint(32,0); }
    else
    { pix = new QPixmap(1, 32); stop = QPoint(0,32); }
    pix->fill(Qt::transparent);

    QLinearGradient lg(start, stop);
    if (pos % 2) // Bottom, right
    { lg.setColorAt(0, c); lg.setColorAt(1, c2); }
    else
    { lg.setColorAt(0, c2); lg.setColorAt(1, c); }

    QPainter p(pix); p.fillRect(pix->rect(), lg); p.end();

    if (_borderline[pos].insert(c.rgba(), pix, costs(pix)))
        return *pix;
    return nullPix;
}

static QColor
checkValue(QColor c, int type)
{
    switch (type) {
    case Gradients::None:
    case Gradients::Simple:
    case Gradients::Sunken:
    case Gradients::Metal:
    case Gradients::Shiny:
    case Gradients::Stone:
        return c;
    default: break;
    }

    int v = Colors::value(c);
    const int minV = type ? ((type < Gradients::Gloss) ? 60 : 40) : 0;
    if (v < minV)
    {
        int h,s,a;
        c.getHsv(&h,&s,&v,&a);
        c.setHsv(h,s,minV,a);
    }
    else if (v > 200 && type > Gradients::Sunken) // glosses etc hate high value colors
    {
        int h,s,a;
        c.getHsv(&h,&s,&v,&a);
        s = 400*s/(400+v-180);
        c.setHsv(h,CLAMP(s,0,255),200,a);
    }
    return c;
}

QColor
Gradients::endColor(const QColor &oc, Position p, Type type, bool cv)
{
    QColor c = cv ? checkValue(oc, type) : oc;
    bool begin = (p == Top || p == Left);
    switch ( type )
    {
        case None:
//         case RadialGloss:
        default:
            return c;
        case Simple:
            return begin ? c.lighter(112) : c.darker(110);
        case Button:
        {
            int h,s,v,a, inc = 15, dec = 6;
            c.getHsv(&h,&s,&v,&a);
            // calc difference
            if (v+inc > 255)
            {
                inc = 255-v;
                dec += (15-inc);
            }
            return begin ? QColor::fromHsv(h,s,v+inc, a) : QColor::fromHsv(h,s,v-dec, a);
        }
        case Sunken:
            return begin ? c.darker(115) : c.lighter(117);
        case Gloss:
        case Glass:
        case Cloudy:
        {
            QColor bb;
            gl_ssColors(c, &bb, 0, type == Glass);
            return bb;
        }
        case Metal:
        {
            int h,s,v,a; c.getHsv(&h,&s,&v,&a);
            return begin ? QColor::fromHsv(h,s,qMin(255,v+10), a) : QColor::fromHsv(h,s,qMax(0,v-10), a);
        }
        case Shiny:
        {
            const int v = Colors::value(c);
            return begin ? Colors::mid(c, Qt::white, 255, 64+v) : Colors::mid(c, Qt::black, 255, 288-v);
        }
    }
}

static QPixmap s_stoneDither;
const QPixmap&
Gradients::pix(const QColor &c, int size, Qt::Orientation o, Gradients::Type type)
{
    // validity check
    if (size <= 0)
    {
        qWarning("NULL Pixmap requested, size was %d",size);
        return nullPix;
    }
    else if (size > 6800)
    {   // this is where our dictionary reaches - should be enough for the moment ;)
        qWarning("gradient with more than 6800 steps requested, returning NULL pixmap");
        return nullPix;
    }

    if (type < 1 || type >= TypeAmount)
        type = Simple;

    // very dark colors won't make nice buttons =)
    QColor iC = checkValue(c, type);

    // hash
    int sloppyAdd = 1;
    uint magicNumber = hash(size, iC, &sloppyAdd);

    PixmapCache *cache = &gradients[o == Qt::Horizontal][type-1];
    QPixmap *pix = cache->object(magicNumber);
    if (pix)
        return *pix;

    QPoint start, stop;
    if (type == Gradients::Cloudy)
        pix = progressGradient(iC, size, o);
#if 0 //#ifndef BESPIN_DECO
    else if (type == Gradients::RadialGloss)
        pix = rGlossGradient(iC, size);
#endif
    else
    {
        pix = newPix(size, o, &start, &stop);

        QGradient grad;
        // no cache entry found, so let's create one
        size += sloppyAdd; // rather to big than to small ;)
        switch (type)
        {
        case Gradients::Button:
            grad = buttonGradient(iC, start, stop);
            break;
        case Gradients::Glass:
            grad = gl_ssGradient(iC, start, stop, true); break;
        case Gradients::Simple:
        case Gradients::Stone:
        default:
            grad = simpleGradient(iC, start, stop); break;
        case Gradients::Sunken:
            grad = sunkenGradient(iC, start, stop); break;
        case Gradients::Gloss:
            grad = gl_ssGradient(iC, start, stop); break;
        case Gradients::Metal:
            grad = metalGradient(iC, start, stop); break;
        case Gradients::Shiny:
            grad = shinyGradient(iC, start, stop); break;
        }
        if (c.alpha() < 255)
            pix->fill(Qt::transparent);
        QPainter p(pix); p.fillRect(pix->rect(), grad);
        if (type == Gradients::Stone) {
            if (s_stoneDither.isNull()) {
                srand( 314159265 );
                s_stoneDither = QPixmap::fromImage(FX::newDitherImage(32, 64));
            }
            p.drawTiledPixmap(pix->rect(), s_stoneDither);
        }
        p.end();
    }

    if (cache && cache->insert(magicNumber, pix, costs(pix)))
        return *pix;
    return nullPix;
}

const QPixmap
&Gradients::structure(const QColor &c, bool light, int type, int intensity)
{
    if (type < 0) {
        if (intensity > -1) {
            qWarning("Gradients::structure(): passing generic intensity for default type is not supported!");
            intensity = _bgIntensity;
        }
        if (QPixmap *cachedBuffer = _structure[light].object(c.rgba()))
            return *cachedBuffer;
    }

    if (intensity < 0)
        intensity = _bgIntensity;

    QPixmap *structureBuffer = new QPixmap(64, 64);

    if (c.alpha() != 0xff)
        structureBuffer->fill(Qt::transparent);

    QPainter p(structureBuffer);
    int i;
    const int iType = type < 0 ? _struct : type;
    const QRect &rect = structureBuffer->rect();
    switch (iType)
    {
    default:
    case 0: // scanlines
        p.setPen(Qt::NoPen); p.setBrush( c.light(intensity) );
        p.drawRect(rect); p.setBrush( Qt::NoBrush );
        i = 100 + (light?6:3)*(intensity - 100)/10;
        p.setPen(c.light(i));
        for ( i = 1; i < 64; i += 4 ) {
            p.drawLine( 0, i, 63, i );
            p.drawLine( 0, i+2, 63, i+2 );
        }
        p.setPen( c );
        for ( i = 2; i < 63; i += 4 )
            p.drawLine( 0, i, 63, i );
        break;
    case 1: //checkboard
        i = 100 + 2*(intensity - 100)/10;
        p.setPen(Qt::NoPen);
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setBrush(c.dark(i));
        if (light) {
            for (int j = 0; j < 64; j+=16)
            for (i = bool(j%32)*16; i < 64; i+=32)
                p.drawRect(i,j,16,16);
        }
        else {
            p.drawRect(32,0,32,32);
            p.drawRect(0,32,32,32);
        }
        break;
    case 2:  // fat scans
        i = intensity - 100;
        p.setPen(Qt::NoPen);
        p.setBrush( c.light(100+3*i/10) );
        p.drawRect(rect);
        p.setPen(QPen(light ? c.light(100+i/10) : c, 2));
        p.setBrush( c.dark(100+2*i/10) );
        p.drawRect(-3,8,70,8);
        p.drawRect(-3,24,70,8);
        p.drawRect(-3,40,70,8);
        p.drawRect(-3,56,70,8);
        break;
    case 3: // "blue"print
        i = (intensity - 100);
        p.setPen(Qt::NoPen); p.setBrush( c.dark(100+i/10) );
        p.drawRect(rect); p.setBrush( Qt::NoBrush );
        p.setPen(c.light(100+(light?4:2)*i/10));
        for ( i = 0; i < 64; i += 16 )
            p.drawLine( 0, i, 63, i );
        for ( i = 0; i < 64; i += 16 )
            p.drawLine( i, 0, i, 63 );
        break;
    case 4: // verticals
        i = (intensity - 100);
        p.setPen(Qt::NoPen); p.setBrush( c.light(100+i) );
        p.drawRect(rect); p.setBrush( Qt::NoBrush );
        p.setPen(c.light(100+(light?6:3)*i/10));
        for ( i = 1; i < 64; i += 4 ) {
            p.drawLine( i, 0, i, 63 );
            p.drawLine( i+2, 0, i+2, 63 );
        }
        p.setPen( c );
        for ( i = 2; i < 63; i += 4 )
            p.drawLine( i, 0, i, 63 );
        break;
    case 5: // diagonals
        i = 100 + (intensity - 100)/4;
        p.setPen(Qt::NoPen); p.setBrush( c.light(i) );
        p.drawRect(rect); p.setBrush( Qt::NoBrush );
        p.setPen(QPen(c.dark(i), 11));
        p.setRenderHint(QPainter::Antialiasing);
        p.drawLine(-64,64,64,-64);
        p.drawLine(0,64,64,0);
        p.drawLine(0,128,128,0);
        p.drawLine(32,64,64,32);
        p.drawLine(0,32,32,0);
        break;
    case 6: // slots
        p.setPen(Qt::NoPen);
        i = 100 + 2*(intensity - 100)/10;
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setBrush(c.dark(i));
        p.setRenderHint(QPainter::Antialiasing);
        for (int j = 0; j < 64; j+=4)
        for (i = bool(j%8)*4; i < 64; i+=8)
            p.drawEllipse(i,j,4,4);
        break;
    case 7: // fence
        p.setPen(Qt::NoPen);
        i = 100 + (intensity - 100)/4;
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setPen(QPen(c.dark(i),2));
        p.setBrush(Qt::NoBrush);
        p.setRenderHint(QPainter::Antialiasing);
        for (i = -48; i < 64; i+=16)
            p.drawLine(i, 0, i+64, 64);
        for (i = -48; i < 64; i+=16)
            p.drawLine(i, 64, i+64, 0);
        break;
    case 8: // interference
    {
        p.setPen(Qt::NoPen);
        i = 100 + (intensity - 100)/4;
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setBrush(Qt::NoBrush);
        p.setRenderHint(QPainter::Antialiasing);
        QColor dc = c.dark(i);
        for (i = 1; i < 6; ++i)
        {
            float r = i*sqrt((float)i)*64.0f/(6.0f*sqrt(6.0f));
            p.setPen(QPen(Colors::mid(dc, c, 6-i, i-1), 2));
            p.drawEllipse(QPointF(32,32), r, r);
            for (int x = 0; x < 65; x+=64)
            for (int y = 0; y < 65; y+=64)
                p.drawEllipse(QPointF(x,y), r, r);
        }
        break;
    }
    case 9: // sand
        p.setPen(Qt::NoPen);
        i = 100 + (intensity - 100)/4;
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(c.dark(i),2));
        p.setRenderHint(QPainter::Antialiasing);
        for (i = 1; i < 5; ++i)
            p.drawEllipse(QPointF(0,0), i*64.0/5.0, i*64.0/5.0);
        for (i = 1; i < 5; ++i)
            p.drawEllipse(QPointF(64,64), i*64.0/5.0, i*64.0/5.0);
        break;
    case 10: // planks
    {
        i = 100 + (intensity - 100)/3;
        QLinearGradient lg(rect.topLeft(), rect.topRight());
        QColor shadow = c.dark(i);
        QColor light = c.light(i);
        lg.setColorAt(0.0, light);
        lg.setColorAt(0.5, light);
        lg.setColorAt(0.50001, shadow);
        lg.setColorAt(0.6, light);
        lg.setColorAt(0.64, shadow);
        lg.setColorAt(0.64001, light);
        lg.setColorAt(1.0, light);
        p.setBrush(lg);
        p.setPen(Qt::NoPen);
        p.drawRect(rect);
        break;
    }
    case 11: // thin diagonals
    {
        p.setPen(Qt::NoPen);
        i = 100 + (intensity - 100)/4;
        p.setBrush(c.light(i));
        p.drawRect(rect);
        p.setPen(QPen(c.dark(i)));
        p.setBrush(Qt::NoBrush);
        p.setRenderHint(QPainter::Antialiasing);
        const double off = 64.0/63.0;
        for (i = -12; i < 66; i+=3)
            p.drawLine(QLineF(0, off*(i+12), 64, off*i));
        break;
    }
    case 12: // bamboo
    case 13: // weave
    {
        i = 100 + (intensity - 100)/2;
        QLinearGradient lg(QPoint(0,0), QPoint(0,8));
        QColor dark = c.dark(i);
        QColor light = c.light(i);
        lg.setColorAt(0.0, light);
        lg.setColorAt(1.0, dark);
        lg.setSpread( QGradient::RepeatSpread );
        p.setBrush(lg);
        p.setPen(Qt::NoPen);
        p.drawRect(rect);
        if ( iType == 12 )
            break;
        lg = QLinearGradient(QPoint(0,0), QPoint(8,0));
        lg.setColorAt(0.0, light);
        lg.setColorAt(1.0, dark);
        lg.setSpread( QGradient::RepeatSpread );
        p.setBrush(lg);
        for (int j = 0; j < 8; ++j)
        for (int i = 8*(j%2); i < 64; i+=16)
            p.drawRect(i,j*8,8,8);
        break;
    }
    case 14: { // stone / noise
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawRect(rect);
        QImage img = FX::newDitherImage(qAbs(intensity-100), 64);
        p.drawTiledPixmap(rect, QPixmap::fromImage(img));
        break;
    }
    case 15: { // brushed metal
        p.end();
        delete structureBuffer;
        structureBuffer = new QPixmap(256,64);
        if (c.alpha() != 0xff)
            structureBuffer->fill(Qt::transparent);
        srand( 314159265 );
        QImage img(256,64, QImage::Format_ARGB32);
        img.fill(c.rgba());
        //expblur has a run-in, so the image needs to be slightly bigger
        QImage noise = FX::newDitherImage(qMin(512, 10*qAbs(intensity-100)), 256+48).copy(0, 64, 256+48, 72);
        FX::expblur(noise, 32, Qt::Horizontal);
        QPainter p2(&img);
        p2.drawImage(0,0, noise, 32, 0);
        p2.end();
        // now mirror blend back the right end on the left offset to create a seamless transition
        for (int row = 0; row < 64; ++row) {
            for (int col = 0; col < 32; ++col) {
                QRgb p1 = img.pixel(col, row);
                QRgb p2 = img.pixel(255-col, row);
#define MERGE(_F_, _V_) const int _V_ = (col*_F_(p1) + (32-col)*_F_(p2)) / 32
                MERGE(qRed, r); MERGE(qGreen, g);
                MERGE(qBlue, b); MERGE(qAlpha, a);
                img.setPixel(col, row, qRgba(r, g, b, a));
            }
        }
        p.begin(structureBuffer);
        p.drawTiledPixmap(structureBuffer->rect(), QPixmap::fromImage(img));
        break;
    }
    case 16: { // concrete
        p.end();
        delete structureBuffer;
        structureBuffer = new QPixmap(128,128);
        if (c.alpha() != 0xff)
            structureBuffer->fill(Qt::transparent);
        srand( 314159265 );
        QImage img(128,128, QImage::Format_ARGB32);
        img.fill(c.rgba());
        //expblur has a run-in, so the image needs to be slightly bigger
        QImage noise = FX::newDitherImage(qMin(512, 10*qAbs(intensity-100)), 128+48);
        FX::expblur(noise, 8);
        QPainter p2(&img);
        p2.drawImage(0,0, noise, 32, 32);
        p2.end();
        // now mirror blend back the right end on the left offset to create a seamless transition
        for (int row = 0; row < 128; ++row) {
            for (int col = 0; col < 32; ++col) {
                QRgb p1 = img.pixel(col, row);
                QRgb p2 = img.pixel(127-col, row);
                MERGE(qRed, r); MERGE(qGreen, g);
                MERGE(qBlue, b); MERGE(qAlpha, a);
                img.setPixel(col, row, qRgba(r, g, b, a));
            }
        }
        // and mirror blend back the bottom end on the top offset to create a seamless transition as well
        for (int col = 0; col < 128; ++col) {
            for (int row = 0; row < 32; ++row) {
                QRgb p1 = img.pixel(col, row);
                QRgb p2 = img.pixel(col, 127-row);
#undef MERGE
#define MERGE(_F_, _V_) const int _V_ = (row*_F_(p1) + (32-row)*_F_(p2)) / 32
                MERGE(qRed, r); MERGE(qGreen, g);
                MERGE(qBlue, b); MERGE(qAlpha, a);
#undef MERGE
                img.setPixel(col, row, qRgba(r, g, b, a));
            }
        }
        p.begin(structureBuffer);
        p.drawTiledPixmap(structureBuffer->rect(), QPixmap::fromImage(img));
        break;
    }
    }
    p.end();

    if (type < 0) {
        if (_structure[light].insert(c.rgba(), structureBuffer, costs(structureBuffer)))
            return *structureBuffer;

        delete structureBuffer;
        return nullPix;
    } else
        return *structureBuffer;
}

const QPixmap
&Gradients::light(int height)
{
    height = (height + 2)/3;
    height *= 3;

    if (height <= 0)
    {
        qWarning("NULL Pixmap requested, height was %d",height);
        return nullPix;
    }

    QPixmap *pix = _groupLight.object(height);
    if (pix)
        return *pix;

    const int v = _invertedGroups ? 0 : 255;
    const int a = v ? 80 : 20;
    pix = new QPixmap(32, height);
    pix->fill(Qt::transparent);
    QPoint start(0,0), stop(0,height);
    QLinearGradient lg(start, stop);
    lg.setColorAt(0, QColor(v,v,v,a));
    lg.setColorAt(1, QColor(v,v,v,0));
    QPainter p(pix); p.fillRect(pix->rect(), lg); p.end();

    if (_groupLight.insert(height, pix, costs(pix)))
        return *pix;
    return nullPix;
}

const
QPixmap &Gradients::ambient(int height)
{
    if (height <= 0)
    {
        qWarning("NULL Pixmap requested, height was %d",height);
        return nullPix;
    }

    QPixmap *pix = _btnAmbient.object(height);
    if (pix)
        return *pix;

    pix = new QPixmap(16*height/9,height); //golden mean relations
    pix->fill(Qt::transparent);
    QLinearGradient lg( QPoint(pix->width(), pix->height()),
                        QPoint(pix->width()/2,pix->height()/2) );
    lg.setColorAt(0, QColor(255,255,255,0));
    lg.setColorAt(0.2, QColor(255,255,255,38));
    lg.setColorAt(1, QColor(255,255,255,0));
    QPainter p(pix); p.fillRect(pix->rect(), lg); p.end();

    // cache for later ;)
    if (_btnAmbient.insert(height, pix, costs(pix)))
        return *pix;
    return nullPix;
}

static QPixmap _bevel[2];

const QPixmap&
Gradients::bevel(bool ltr)
{
   return _bevel[ltr];
}
#if 0
const QPixmap &
Gradients::shadow(int height, bool bottom)
{
    if (height <= 0)
    {
        qWarning("NULL Pixmap requested, height was %d",height);
        return nullPix;
    }

    uint val = height + bottom*0x80000000;
    QPixmap *pix = _tabShadow.object(val);
    if (pix)
        return *pix;

    pix = new QPixmap(height/3,height);
    pix->fill(Qt::transparent);

    float hypo = sqrt(pow(pix->width(),2)+pow(pix->height(),2));
    float cosalpha = (float)(pix->height())/hypo;
    QPoint p1, p2;
    if (bottom)
    {
        p1 = QPoint(0, 0);
        p2 = QPoint((int)(pix->width()*pow(cosalpha, 2)),
                    (int)(pow(pix->width(), 2)*cosalpha/hypo));
    }
    else
    {
        p1 = QPoint(0, pix->height());
        p2 = QPoint((int)(pix->width()*pow(cosalpha, 2)),
                    (int)pix->height() - (int)(pow(pix->width(), 2)*cosalpha/hypo));
    }
    QLinearGradient lg(p1, p2);
    lg.setColorAt(0, QColor(0,0,0,75));
    lg.setColorAt(1, QColor(0,0,0,0));
    QPainter p(pix); p.fillRect(pix->rect(), lg); p.end();

    if (_tabShadow.insert(val, pix, costs(pix)))
        return *pix;
    return nullPix;
}
#endif
static inline QPixmap *
cornerMask(bool right = false)
{
    QPixmap *alpha = new QPixmap(128,128);
    QRadialGradient rg(right ? alpha->rect().topLeft() : alpha->rect().topRight(), 128);
//    QLinearGradient rg;
//    if (right) rg = QLinearGradient(0,0, 128,0);
//    else rg = QLinearGradient(128,0, 0,0);
// #ifndef QT_NO_XRENDER
    alpha->fill(Qt::transparent);
    rg.setColorAt(0, Qt::transparent);
    rg.setColorAt(0.5, Qt::transparent);
// #else
//     rg.setColorAt(0, Qt::black);
//     rg.setColorAt(0.5, Qt::black);
// #endif
    rg.setColorAt(1, Qt::white);
    QPainter p(alpha); p.fillRect(alpha->rect(), rg); p.end();
    return alpha;
}

const BgSet&
Gradients::bgSet(const QColor &c)
{
    BgSet *set = _bgSet.object(c.rgb());
    if (set)
        return *set;
    set = bgSet(c, _mode, _bgIntensity);
    _bgSet.insert(c.rgba(), set, costs(set));
    return *set;
}

BgSet*
Gradients::bgSet(const QColor &c, BgMode mode, int bgIntensity)
{
    BgSet *set = new BgSet;
    QLinearGradient lg;
    QPainter p;
    switch (mode)
    {
    case BevelV:
    {
        set->topTile = QPixmap(32, 256);
        set->lCorner = QPixmap(128, 128);
        QColor c1, c2, c3;
        if (!c.alpha())
        {
            set->topTile.fill(Qt::transparent);
            set->lCorner.fill(Qt::transparent);
            int a = qAbs(bgIntensity - 100)*3; a = CLAMP(a,0,255);
            int v = (bgIntensity > 100)*255;
            c1 = QColor(v,v,v, a);
            c2 = QColor(v,v,v, a/2);
            v = (!v)*255;
            c3 = QColor(v,v,v, a);
        }
        else
        {
            c1 = c.light(bgIntensity);
            c2 = Colors::mid(c1, c);
            c3 = c.dark(bgIntensity);
        }
        // this can save some time on alpha enabled pixmaps
        set->cornerTile = set->topTile.copy(set->cornerTile.rect());
        set->btmTile = set->topTile.copy();
        set->rCorner = set->lCorner.copy();

        lg = QLinearGradient(QPoint(0,0), QPoint(0,256));
        QGradientStops stops;

        // Top Tile
        p.begin(&set->topTile);
        stops << QGradientStop(0, c1) << QGradientStop(1, c);
        lg.setStops(stops); p.fillRect(set->topTile.rect(), lg); stops.clear();
        p.drawTiledPixmap(set->topTile.rect(), FX::dither());
        p.end();

        // Bottom Tile
        p.begin(&set->btmTile);
        stops << QGradientStop(0, c) << QGradientStop(1, c3);
        lg.setStops(stops); p.fillRect(set->btmTile.rect(), lg); stops.clear();
        p.drawTiledPixmap(set->btmTile.rect(), FX::dither());
        p.end();

        if (Colors::value(c) > 244)
        {
            set->cornerTile = set->lCorner = set->rCorner = QPixmap();
            break; // would be mathematically nonsense, i.e. shoulders = 255...
        }

        // Corner Tile
        lg = QLinearGradient(QPoint(0,0), QPoint(0,128));
        p.begin(&set->cornerTile);
        if (c == Qt::transparent)
            c1.setAlpha(4*c1.alpha()/7);
        else
            c1 = Colors::mid(c1, c2,1,6);
        stops << QGradientStop(0, c1) << QGradientStop(1, c2);
        lg.setStops(stops);
        p.fillRect(set->cornerTile.rect(), lg);
        stops.clear();
        p.drawTiledPixmap(set->cornerTile.rect(), FX::dither());
        p.end();

        // Left Corner, right corner
        QPixmap *mask, *pix;
        for (int cnr = 0; cnr < 2; ++cnr)
        {
            pix = (cnr ? &set->rCorner : &set->lCorner);
            p.begin(pix);
            p.drawTiledPixmap(pix->rect(), set->topTile);
            p.end();
            mask = cornerMask(cnr);

            QPixmap fill(mask->size());
            p.begin(&fill);
            p.drawTiledPixmap(fill.rect(), set->cornerTile);
            p.end();

            fill = FX::applyAlpha(fill, *mask);

            p.begin(pix);
            p.drawPixmap(0,0, fill);
            p.drawTiledPixmap(pix->rect(), FX::dither());
            p.end();
            delete mask;
        }
        break;
    }
    case BevelH:
    {
        QColor c1, c3;
        set->topTile = QPixmap(256, 32);
        set->cornerTile = QPixmap(32, 128);
        if (!c.alpha())
        {
            set->topTile.fill(Qt::transparent);
            set->cornerTile.fill(Qt::transparent);

            int a = qAbs(_bgIntensity - 100)*3; a = CLAMP(a,0,255);
            int v = (_bgIntensity < 100)*255;
            c1 = QColor(v,v,v, a);
            v = (!v)*255;
            c3 = QColor(v,v,v, a);
        }
        else
        {
            c1 = c.dark(_bgIntensity);
            c3 = c.light(_bgIntensity);
        }

        // this can save some time on alpha enabled pixmaps
        set->btmTile = set->topTile.copy();
        set->lCorner = set->topTile.copy();
        set->rCorner = set->topTile.copy();


        lg = QLinearGradient(QPoint(0,0), QPoint(256, 0));
        QGradientStops stops;

        // left
        p.begin(&set->topTile);
        stops << QGradientStop(0, c1) << QGradientStop(1, c);
        lg.setStops(stops); p.fillRect(set->topTile.rect(), lg); stops.clear();
        p.drawTiledPixmap(set->topTile.rect(), FX::dither());
        p.end();

        // right
        p.begin(&set->btmTile);
        stops << QGradientStop(0, c) << QGradientStop(1, c1);
        lg.setStops(stops); p.fillRect(set->btmTile.rect(), lg); stops.clear();
        p.drawTiledPixmap(set->btmTile.rect(), FX::dither());
        p.end();

        // left corner right corner
        QPixmap *pix, *blend;
        QPixmap *mask = new QPixmap(256,32);
        mask->fill(Qt::transparent);
        QPixmap fill = mask->copy();
        lg = QLinearGradient(0,0, 0,32);
        lg.setColorAt(1, Qt::white);
        lg.setColorAt(0, Qt::transparent);
        p.begin(mask); p.fillRect(mask->rect(), lg); p.end();

        for (int cnr = 0; cnr < 2; ++cnr)
        {
            if (cnr)
                { pix = &set->rCorner; blend = &set->btmTile; }
            else
                { pix = &set->lCorner; blend = &set->topTile; }
            pix->fill(c);

            fill.fill(Qt::transparent);
            p.begin(&fill); p.drawTiledPixmap(fill.rect(), *blend); p.end();

            fill = FX::applyAlpha(fill, *mask);

            p.begin(pix);
            p.drawPixmap(0,0, fill);
            p.drawTiledPixmap(pix->rect(), FX::dither());
            p.end();
        }
        delete mask;
        lg = QLinearGradient(QPoint(0,0), QPoint(0, 128));
        lg.setColorAt(0, c3); lg.setColorAt(1, c);
        p.begin(&set->cornerTile);
        p.fillRect(set->cornerTile.rect(), lg);
        p.drawTiledPixmap(set->cornerTile.rect(), FX::dither());
        p.end();
        break;
    }
    default:
        break;
    }
    return set;
}


static bool _initialized = false;

void
Gradients::init(BgMode mode, int structure, int bgIntesity, int btnBevelSize, bool force, bool invertedGroups)
{
    if (_initialized && !force)
        return;
    _initialized = true;
    _mode = mode;
    _struct = structure;
    _bgIntensity = bgIntesity;
    _invertedGroups = invertedGroups;
    _bgSet.setMaxCost( 900<<10 ); // 832 should be enough - we keep some safety
    _btnAmbient.setMaxCost( 64<<10 );
    _tabShadow.setMaxCost( 64<<10 );
    _groupLight.setMaxCost( 256<<10 );
    _structure[0].setMaxCost( 128<<10 ); _structure[1].setMaxCost( 128<<10 );
    QLinearGradient lg(0,0,btnBevelSize,0);
    QPainter p; QGradientStops stops;
    for (int i = 0; i < 2; ++i)
    {
        _bevel[i] = i ? _bevel[0].copy() : QPixmap(btnBevelSize, 32);
        _bevel[i].fill(Qt::transparent);
        stops << QGradientStop(0, QColor(0,0,0,i?20:0)) << QGradientStop(1, QColor(0,0,0,i?0:20));
        lg.setStops(stops);
        stops.clear();
        p.begin(&_bevel[i]); p.fillRect(_bevel[i].rect(), lg); p.end();
    }

    for (int i = 0; i < 4; ++i)
        _borderline[i].setMaxCost( ((32*32)<<3)<<4 ); // enough for 16 different colors
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < Gradients::TypeAmount-1; ++j)
            gradients[i][j].setMaxCost( 1024<<10 );
   }
}

void Gradients::wipe() {
    for (int i = 0; i < 2; ++i)
        for (int j = 0; j < Gradients::TypeAmount-1; ++j)
            gradients[i][j].clear();

    _bgSet.clear();
    _btnAmbient.clear();
    _tabShadow.clear();
    _groupLight.clear();
    _structure[0].clear(); _structure[1].clear();

    for (int i = 0; i < 4; ++i)
        _borderline[i].clear();
}
