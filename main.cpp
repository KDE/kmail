/* -*- mode: C++; c-file-style: "gnu" -*-
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include <config.h>

#include <kuniqueapplication.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include "kmkernel.h" //control center
#include "kmail_options.h"

#include <kdebug.h>

#undef Status // stupid X headers

#include "aboutdata.h"

#include "kmstartup.h"

//-----------------------------------------------------------------------------

class KMailApplication : public KUniqueApplication
{
public:
  KMailApplication() : KUniqueApplication() { };
  virtual int newInstance();
  void commitData(QSessionManager& sm);

};

void KMailApplication::commitData(QSessionManager& sm) {
  kmkernel->dumpDeadLetters();
  kmkernel->setShuttingDown( true ); // Prevent further dumpDeadLetters calls
  KApplication::commitData( sm );
}


int KMailApplication::newInstance()
{
  kdDebug(5006) << "KMailApplication::newInstance()" << endl;
  if (!kmkernel)
     return 0;

  if (!kmkernel->firstInstance() || !kapp->isRestored())
    kmkernel->handleCommandLine( true );
  kmkernel->setFirstInstance(FALSE);
  return 0;
}

int main(int argc, char *argv[])
{
  // WABA: KMail is a KUniqueApplication. Unfortunately this makes debugging
  // a bit harder: You should pass --nofork as commandline argument when using
  // a debugger. In gdb you can do this by typing "set args --nofork" before
  // typing "run".

  KMail::AboutData about;

  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( kmail_options ); // Add kmail options
  if (!KMailApplication::start())
     return 0;

  KMailApplication app;

  // import i18n data and icons from libraries:
  KMail::insertLibraryCataloguesAndIcons();

  // Make sure that the KNotify Daemon is running (this is necessary for people
  // using KMail without KDE)
  KNotifyClient::startDaemon();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  KMail::lockOrDie();

  //local, do the init
  KMKernel kmailKernel;
  kmailKernel.init();
  kapp->dcopClient()->setDefaultObject( kmailKernel.objId() );

  // and session management
  kmailKernel.doSessionManagement();

  // any dead letters?
  kmailKernel.recoverDeadLetters();

  kmsetSignalHandler(kmsignalHandler);

  kapp->dcopClient()->resume(); // Ok. We are ready for DCOP requests.
  kmkernel->setStartingUp( false ); // Starting up is finished
  // Go!
  int ret = kapp->exec();
  // clean up
  kmailKernel.cleanup();

  KMail::cleanup(); // pid file (see kmstartup.cpp)
  return ret;
}
