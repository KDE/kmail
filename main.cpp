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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include <config.h>

#include <kuniqueapplication.h>
#include <kglobal.h>
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
  KMailApplication() : KUniqueApplication(),
                       mDelayedInstanceCreation( false ),
                       mEventLoopReached( false ) { };
  virtual int newInstance();
  void commitData(QSessionManager& sm);
  void setEventLoopReached();
  void delayedInstanceCreation();
protected:
  bool mDelayedInstanceCreation;
  bool mEventLoopReached;

};

void KMailApplication::commitData(QSessionManager& sm) {
  kmkernel->dumpDeadLetters();
  kmkernel->setShuttingDown( true ); // Prevent further dumpDeadLetters calls
  KApplication::commitData( sm );
}

void KMailApplication::setEventLoopReached() {
  mEventLoopReached = true;
}

int KMailApplication::newInstance()
{
  kDebug(5006) << "KMailApplication::newInstance()" << endl;
  if (!kmkernel)
     return 0;

  // If the event loop hasn't been reached yet, the kernel is probably not
  // fully initalized. Creating an instance would therefore fail, this is why
  // that is delayed until delayedInstanceCreation() is called.
  if ( !mEventLoopReached ) {
    kDebug(5006) << "Delaying instance creation." << endl;
    mDelayedInstanceCreation = true;
    return 0;
  }

  if (!kmkernel->firstInstance() || !kapp->isSessionRestored())
    kmkernel->handleCommandLine( true );
  kmkernel->setFirstInstance(false);
  return 0;
}

void KMailApplication::delayedInstanceCreation() {
  if ( mDelayedInstanceCreation )
    newInstance();
}

int main(int argc, char *argv[])
{
  // WABA: KMail is a KUniqueApplication. Unfortunately this makes debugging
  // a bit harder: You should pass --nofork as commandline argument when using
  // a debugger. In gdb you can do this by typing "set args --nofork" before
  // typing "run".

  KMail::AboutData about;

  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( kmail_options() ); // Add kmail options
  if (!KMailApplication::start())
     return 0;

  KMailApplication app;

  // import i18n data and icons from libraries:
  KMail::insertLibraryCataloguesAndIcons();


#ifdef __GNUC__
#warning Port me!
#endif
//  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  KMail::lockOrDie();

  //local, do the init
  KMKernel kmailKernel;
  kmailKernel.init();
#ifdef __GNUC__
#warning Port me!
#endif
//  kapp->dcopClient()->setDefaultObject( kmailKernel.objId() );

  // and session management
  kmailKernel.doSessionManagement();

  // any dead letters?
  kmailKernel.recoverDeadLetters();

  kmsetSignalHandler(kmsignalHandler);

#ifdef __GNUC__
#warning Port me!
#endif
//  kapp->dcopClient()->resume(); // Ok. We are ready for DCOP requests.
  kmkernel->setStartingUp( false ); // Starting up is finished

  //If the instance hasn't been created yet, do that now
  app.setEventLoopReached();
  app.delayedInstanceCreation();

  // Go!
  int ret = qApp->exec();
  // clean up
  kmailKernel.cleanup();

  KMail::cleanup(); // pid file (see kmstartup.cpp)
  return ret;
}
