/*   -*- c++ -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright (C) 2001-2002 Marc Mutz <mutz@kde.org>
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _CONFIGURE_DIALOG_H_
#define _CONFIGURE_DIALOG_H_

#include <kcmultidialog.h>

#include <QPointer>
#include <QHideEvent>

class KConfig;
namespace KMail {
    class ImapAccountBase;
}

class ConfigureDialog : public KCMultiDialog
{
  Q_OBJECT

public:
  explicit ConfigureDialog( QWidget *parent=0, bool modal=true );
  ~ConfigureDialog();

signals:
  void configChanged();
protected:
  void hideEvent( QHideEvent *i );
protected slots:
  /** @reimplemented
   * Saves the GlobalSettings stuff before passing on to KCMultiDialog.
   */
  void slotApply();

  /** @reimplemented
   * Saves the GlobalSettings stuff before passing on to KCMultiDialog.
   */
  void slotOk();
};

/**
 * DImap accounts need to be updated after just being created to show the folders it has.
 * This has to be done a-synchronically due to the nature of the account, so this object
 * takes care of that.
 */
class AccountUpdater : public QObject {
  Q_OBJECT
  public:
    AccountUpdater(KMail::ImapAccountBase *account);
    void update();
  public slots:
    void namespacesFetched();
  private:
    KMail::ImapAccountBase *mAccount;
};


#endif
