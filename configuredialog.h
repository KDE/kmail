/*   -*- c++ -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *              Copyright (C) 2001-2002 Marc Mutz <mutz@kde.org>
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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _CONFIGURE_DIALOG_H_
#define _CONFIGURE_DIALOG_H_

#include <kdialogbase.h>
#include <qptrlist.h>
#include <qguardedptr.h>

class KConfig;
class ConfigurationPage;
class ProfileDialog;

class ConfigureDialog : public KDialogBase
{
  Q_OBJECT

public:
  ConfigureDialog( QWidget *parent=0, const char *name=0, bool modal=true );
  ~ConfigureDialog();

public slots:
  /** @reimplemented */
  void show();

protected slots:
  /** @reimplemented */
  void slotOk();
  /** @reimplemented */
  void slotApply();
  /** @reimplemented */
  void slotHelp();
  /** @reimplemented */
  void slotUser1();
  /** Installs a new profile (in the dislog's widgets; to apply, the
      user has to hit the apply button). Profiles are normal kmail
      config files which have an additonal group "KMail Profile"
      containing keys "Name" and "Comment" for the name and description,
      resp. Only keys that this profile is supposed to alter should be
      included in the file.
  */
  void slotInstallProfile( KConfig * profile );

private slots:
  void slotCancelOrClose();

signals:
  void configChanged();

private:
  void setup();
  void apply(bool);

private:
  QPtrList<ConfigurationPage> mPages;
  QGuardedPtr<ProfileDialog>  mProfileDialog;
};

#endif
