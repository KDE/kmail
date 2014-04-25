/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2003 Marc Mutz, mutz@kde.org
 *   Contains code segments and ideas from earlier kmail dialog code.
 *   Copyright (C) 2010 Volker Krause <vkrause@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef KMAIL_CONFIGMODULE_H
#define KMAIL_CONFIGMODULE_H

#include "kmail_export.h"

#include <kcmodule.h>
#include <KComponentData>
class ConfigModule : public KCModule
{
public:
  explicit ConfigModule( const KComponentData &instance, QWidget *parent=0 )
     : KCModule ( /*instance,*/ parent ) //QT5
     {}
  ~ConfigModule() {}

  virtual void load() = 0;
  virtual void save() = 0;
  virtual void defaults() {}

  /** Should return the help anchor for this page or tab */
  virtual QString helpAnchor() const = 0;
};

#endif
