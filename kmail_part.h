/*
    This file is part of KMail.
    Copyright (c) 2002-2003 Don Sanders <sanders@kde.org>,
    Copyright (c) 2003      Zack Rusin  <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/
#ifndef KMail_PART_H
#define KMail_PART_H

#include "kmail_export.h"
#include <kparts/browserextension.h>
#include <kparts/event.h>
#include <kparts/part.h>
#include <akonadi/collection.h>

#include <QWidget>
#include <QPixmap>

class KMailStatusBarExtension;
class KMMainWidget;
namespace KPIM { class StatusbarProgressWidget; }
using KPIM::StatusbarProgressWidget;

class KMailPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.kmail.kmailpart")

  public:
    KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &);
    virtual ~KMailPart();

    QWidget* parentWidget() const;

  public slots:
    Q_SCRIPTABLE void save() { /*TODO*/ }
    Q_SCRIPTABLE void exit();
    void updateEditMenu() {}
    void slotCollectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames );

  void slotFolderChanged( const Akonadi::Collection& );
  signals:
    void textChanged( const QString& );
    void iconChanged( const QPixmap& );

  protected:
    virtual bool openFile();
    virtual void guiActivateEvent(KParts::GUIActivateEvent *e);

  private:
    KMMainWidget *mainWidget;
    QWidget *mParentWidget;
};

#endif
