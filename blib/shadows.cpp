
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QX11Info>

#include <QtDebug>

#include "xproperty.h"
#include "shadows.h"

using namespace Bespin;

static QPixmap (*pixmaps[2])[8];
static unsigned long globalShadowData[2][12];

static unsigned long*
shadowData(Shadows::Type t, bool storeToRoot)
{
    unsigned long *data = XProperty::get<unsigned long>(QX11Info::appRootWindow(), XProperty::bespinShadow[t-1], XProperty::LONG, 12);
    if (!data)
    {
        const int sz = t == Shadows::Large ? 24 : 12;
        if (!pixmaps[t-1])
        {
            QPixmap *store = new QPixmap[8];
            pixmaps[t-1] = (QPixmap (*)[8])store;

            // radial gradient requires the raster engine anyway... -> QImage
            QImage shadow(2*sz+1, 2*sz+1, QImage::Format_ARGB32);
            shadow.fill(Qt::transparent);
            QPainter p(&shadow);
            p.setPen(Qt::NoPen);
            QRadialGradient rg(QPoint(sz+1,sz+1),sz);
            rg.setColorAt(0, QColor(0,0,0,32)); rg.setColorAt(1, QColor(0,0,0,0));
            p.setBrush(rg);
            p.drawRect(shadow.rect());
            rg.setStops(QGradientStops());
            rg.setColorAt(0, QColor(0,0,0,96)); rg.setColorAt(0.6, QColor(0,0,0,0));
            p.drawRect(shadow.rect());
            p.end();
            QPixmap shadowPix = QPixmap::fromImage(shadow);

            store[0] = shadowPix.copy(sz, 0, 1, sz);
            store[1] = shadowPix.copy(sz+1, 0, sz, sz);
            store[2] = shadowPix.copy(sz+1, sz, sz, 1);
            store[3] = shadowPix.copy(sz+1, sz+1, sz, sz);
            store[4] = shadowPix.copy(sz, sz+1, 1, sz);
            store[5] = shadowPix.copy(0, sz+1, sz, sz);
            store[6] = shadowPix.copy(0, sz, sz, 1);
            store[7] = shadowPix.copy(0, 0, sz, sz);
        }
        for (int i = 0; i < 8; ++i)
            globalShadowData[t-1][i] = (*pixmaps[t-1])[i].handle();
        for (int i = 8; i < 12; ++i)
            globalShadowData[t-1][i] = sz;
        data = &globalShadowData[t-1][0];
        if (storeToRoot) {
            qDebug() << int(t-1) << XProperty::bespinShadow[t-1];
            XProperty::set(QX11Info::appRootWindow(), XProperty::bespinShadow[t-1], data, XProperty::LONG, 12);
        }
    }
    return data;
}


BLIB_EXPORT void
Shadows::set(WId id, Shadows::Type t, bool storeToRoot)
{
    if (id == QX11Info::appRootWindow()) {
        qWarning("BESPIN WARNING! Setting shadow to ROOT window is NOT supported");
        return;
    }
    switch(t)
    {
    case Shadows::None:
        XProperty::remove(id, XProperty::kwinShadow);
        break;
    case Shadows::Large:
    case Shadows::Small:
        XProperty::set(id, XProperty::kwinShadow, shadowData(t, storeToRoot), XProperty::LONG, 12);
    default:
        break;
    }
}
