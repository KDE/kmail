// -*- mode: C++; c-file-style: "gnu" -*-
// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>

#include <kuniqueapplication.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include "kmkernel.h" //control center
#include "kmail_options.h"
#include <qtimer.h>

#undef Status // stupid X headers

#include "aboutdata.h"

#include "kmstartup.h"

// OLD about text.  This is horrbly outdated.
/*const char* aboutText =
    "KMail [" KMAIL_VERSION "] by\n\n"
    "Stefan Taferner <taferner@kde.org>,\n"
    "Markus Wübben <markus.wuebben@kde.org>\n\n"
    "based on the work of:\n"
    "Lynx <lynx@topaz.hknet.com>,\n"
    "Stephan Meyer <Stephan.Meyer@pobox.com>,\n"
    "and the above authors.\n\n"
    "This program is covered by the GPL.\n\n"
    "Please send bugreports to taferner@kde.org";
*/

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
  if (dcopClient()->isSuspended())
  {
    // Try again later.
    QTimer::singleShot( 100, this, SLOT(newInstance()) );
    return 0;
  }

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

  // import i18n data from libraries:
  KMail::insertLibraryCatalogues();

#if !KDE_IS_VERSION( 3, 1, 92 ) // replacement is now in KMKernel::config()
  // Check that all updates have been run on the config file:
  KMail::checkConfigUpdates();
#endif

  // Make sure that the KNotify Daemon is running (this is necessary for people
  // using KMail without KDE)
  KNotifyClient::startDaemon();

  KMail::lockOrDie();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

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
  if (kmkernel->shuttingDown())
      kmailKernel.notClosedByUser();
  else
      kmailKernel.cleanup();

  KMail::cleanup();
  return ret;
}
