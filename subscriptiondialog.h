/* This file is part of the KDE libraries
   Copyright (C) 2002 Carsten Burghardt <burghardt@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __SUBSCRIPTIONDIALOG
#define __SUBSCRIPTIONDIALOG

#include <ksubscription.h>
#include "imapaccountbase.h"

class KMMessage;

namespace KMail {

  class SubscriptionDialog : public KSubscription
  {
    Q_OBJECT

    public:
      SubscriptionDialog( QWidget *parent, const QString &caption, KAccount* acct );

    public slots:
      /**
       * get the listing from the imap-server
       */ 
      void slotListDirectory(QStringList, QStringList,
          QStringList, const ImapAccountBase::jobData &);

      /** 
       * called by Ok-button, saves the changes
       */ 
      void slotSave();

    protected slots:
      /**
       * Loads the folders
       */ 
      void slotLoadFolders();

  };

} // namespace KMail

#endif
