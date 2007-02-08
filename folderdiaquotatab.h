// -*- mode: C++; c-file-style: "gnu" -*-
/**
 * folderdiaquotatab.h
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef FOLDERDIAQUOTA_H
#define FOLDERDIAQUOTA_H

#include "kmfolderdia.h"
#include "kmfoldertype.h"
#include "quotajobs.h"

namespace KMail {
  class QuotaWidget;
}
class QVBox;
class QWidgetStack;

namespace KMail {

class ImapAccountBase;

/**
 * "Quota" tab in the folder dialog
 * Internal class, only used by KMFolderDialog
 */
class FolderDiaQuotaTab : public FolderDiaTab
{
  Q_OBJECT

public:
  FolderDiaQuotaTab( KMFolderDialog* dlg, QWidget* parent, const char* name = 0 );

  virtual void load();
  virtual bool save();
  virtual AcceptStatus accept();

  static bool supports( KMFolder* refFolder );

private:
  void initializeWithValuesFromFolder( KMFolder* folder );
  void showQuotaWidget();
private slots:
  // Network (KIO) slots
  void slotConnectionResult( int, const QString& );
  void slotReceivedQuotaInfo( KMFolder*, KIO::Job*, const KMail::QuotaInfo& );


private:

  QLabel* mLabel;
  KMail::QuotaWidget* mQuotaWidget;
  QWidgetStack* mStack;
  ImapAccountBase* mImapAccount;
  QString mImapPath;
  KMFolderDialog* mDlg;

  QuotaInfo mQuotaInfo;
  KMFolderType mFolderType;
};

} // end of namespace KMail

#endif /* FOLDERDIAQUOTA_H */

