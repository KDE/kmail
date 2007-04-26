/*  -*- c++ -*-
  localsubscriptiondialog.h

  This file is part of KMail, the KDE mail client.
  Copyright (C) 2006 Till Adam <adam@kde.org>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt.  If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.
*/

#ifndef __LOCALSUBSCRIPTIONDIALOG
#define __LOCALSUBSCRIPTIONDIALOG

#include "subscriptiondialog.h"

namespace KMail {
  class ImapAccountBase;

  class LocalSubscriptionDialog: public SubscriptionDialog
  {
    Q_OBJECT

    public:
      LocalSubscriptionDialog( QWidget *parent, const QString &caption,
                               ImapAccountBase* acct,
                               QString startPath = QString() );
      virtual ~LocalSubscriptionDialog();

    protected:
      /** reimpl */
      virtual void listAllAvailableAndCreateItems();
      /** reimpl */
      virtual void processFolderListing();
      /**  reimpl */
      virtual void doSave();
      virtual void loadingComplete();

    private:
      void setCheckedStateOfAllItems();

      ImapAccountBase* mAccount;
  };

} // namespace KMail

#endif
