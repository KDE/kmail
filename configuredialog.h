/*
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _CONFIGURE_DIALOG_H_
#define _CONFIGURE_DIALOG_H_

#include <kdialogbase.h>

class QWidget;
class KConfig;

class IdentityPage;
class NetworkPage;
class AppearancePage;
class ComposerPage;
class SecurityPage;
class MiscPage;
class PluginPage;

class ConfigureDialog : public KDialogBase
{
  Q_OBJECT

  public:
    ConfigureDialog( QWidget *parent=0, const char *name=0, bool modal=true );
    ~ConfigureDialog();
    virtual void show();

  protected slots:
    virtual void slotOk();
    virtual void slotApply();
    virtual void slotHelp();
    /** Installs a new profile (in the dislog's widgets; to apply, the
        user has to hit the apply button). Profiles are normal kmail
        config files which hae an additonal group "KMail Profile"
        containing keys "name" and "desc" for the name and
        description, resp. Only keys that this profile is supposed to
        alter should be included in the file.
    */
    virtual void slotInstallProfile( KConfig * profile );


  private slots:
    void slotCancelOrClose();

  protected:
    void setup();
    void apply(bool);

  protected:
    IdentityPage   *mIdentityPage;
    NetworkPage    *mNetworkPage;
    AppearancePage *mAppearancePage;
    ComposerPage   *mComposerPage;
    SecurityPage   *mSecurityPage;
    MiscPage       *mMiscPage;
    PluginPage     *mPluginPage;
};

#endif
