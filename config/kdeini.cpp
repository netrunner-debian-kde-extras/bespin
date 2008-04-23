/* Bespin widget style configurator for Qt4
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

#include <QColor>
#include <QFile>
#include <QProcess>
#include <QTextStream>

#include "kdeini.h"

static QString confPath[2];

KdeIni*
KdeIni::open(const QString &name)
{
   if (confPath[0].isNull()) { // acquire global and local kde config paths
      QString configFile;
      QProcess kde4_config;
      kde4_config.start("kde4-config --path config");
      if (kde4_config.waitForFinished()) {
         configFile = kde4_config.readAllStandardOutput().trimmed();
         confPath[0] = configFile.section(':', 0, 0); // local
         confPath[1] = configFile.section(':', 1, 1); // global
      }
      if (confPath[0].isNull())
         return 0L; // no paths available, maybe even not KDE4
   }
   return new KdeIni(name);
}

KdeIni::KdeIni(const QString &name)
{
   QString buffer;
   localFile = confPath[0] + name;
   QFile lfile(localFile);
   if (lfile.open(QIODevice::ReadOnly)) {
      localGroup == local.end();
      QTextStream stream(&lfile);
      do {
         buffer = stream.readLine().trimmed();
         if (buffer.startsWith('[')) // group
            localGroup = local.insert(buffer.mid(1,buffer.length()-2), Entries());
         else if (!(buffer.isEmpty() || localGroup == local.end()))
            localGroup.value().insert(buffer.section('=',0,0), buffer.section('=',1));
      } while (!buffer.isNull());
      lfile.close();
   }

   QFile gfile(confPath[1] + name);
   if (gfile.open(QIODevice::ReadOnly)) {
      localGroup == global.end();
      QTextStream stream(&gfile);
      do {
         buffer = stream.readLine().trimmed();
         if (buffer.startsWith('[')) // group
            localGroup = global.insert(buffer.mid(1,buffer.length()-2), Entries());
         else if (!(buffer.isEmpty() || localGroup == global.end()))
            localGroup.value().insert(buffer.section('=',0,0), buffer.section('=',1));
      } while (!buffer.isNull());
      gfile.close();
   }
   localGroup = local.end();
   globalGroup = global.constEnd();
}

QStringList
KdeIni::groups() const
{
   return QStringList();
}

bool
KdeIni::setGroup(const QString &group)
{
   localGroup = local.find(group);
   if (localGroup == local.end())
      localGroup = local.insert(group, Entries());
   globalGroup = global.constFind(group);
   return true; //(localGroup != local.end() && globalGroup = global.constEnd());
}

void
KdeIni::setValue(const QString &key, const QVariant &value)
{
   if (localGroup == local.end()) {
      qWarning("KdeIni::setValue(): You must first set a group!");
      return;
   }
   QString val;
   switch(value.type()) {
   case QVariant::Color: {
      QColor c = value.value<QColor>();
      val = QString::number( c.red() ) + ',' + QString::number( c.green() ) + ',' + QString::number( c.blue() );
      break;
   }
   default:
      val = value.toString();
   }
   (*localGroup)[key] = val;
}

QString
KdeIni::value(const QString &key)
{
   Entries::const_iterator it = localGroup->constFind(key);
   if (it != localGroup->constEnd())
      return *it;
   it = globalGroup->constFind(key);
   if (it != globalGroup->constEnd())
      return *it;
   return QString();
}

bool
KdeIni::close()
{
   QFile file(localFile);
   if (!file.open(QIODevice::WriteOnly))
      return false;
   Config::const_iterator group = local.constBegin();
   Entries::const_iterator entry;
   QTextStream stream(&file);
   while (group != local.constEnd()) {
      stream << '[' << group.key() << ']' << endl;
      entry = group.value().constBegin();
      while (entry != group.value().constEnd()) {
         stream << entry.key() << '=' << entry.value() << endl;
         ++entry;
      }
      stream << endl;
      ++group;
   }
   stream.flush();
   file.close();
   return true;
}
