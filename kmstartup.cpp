// Author: Don Sanders <sanders@kde.org>
// License GPL


#include <config.h>

#include "kmstartup.h"

#include "kmkernel.h" //control center

#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include <kcrash.h>
#include <kglobal.h>

#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#undef Status // stupid X headers

extern "C" {

// Crash recovery signal handler
void kmsignalHandler(int sigId)
{
  kmsetSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Exiting)\n", sigId);
  // try to cleanup all windows
  if (kmkernel) kmkernel->dumpDeadLetters();
  ::exit(-1); //
}

// Crash recovery signal handler
void kmcrashHandler(int sigId)
{
  kmsetSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Crashing)\n", sigId);
  // try to cleanup all windows
  kmkernel->dumpDeadLetters();
  // Return to DrKonqi.
}
//-----------------------------------------------------------------------------


void kmsetSignalHandler(void (*handler)(int))
{
  signal(SIGKILL, handler);
  signal(SIGTERM, handler);
  signal(SIGHUP,  handler);
  KCrash::setEmergencySaveFunction(kmcrashHandler);
}

}
//-----------------------------------------------------------------------------

namespace {
  QString getMyHostName() {
    char hostNameC[256];
    // null terminate this C string
    hostNameC[255] = 0;
    // set the string to 0 length if gethostname fails
    if(gethostname(hostNameC, 255))
      hostNameC[0] = 0;
    return QString::fromLocal8Bit(hostNameC);
  }
} // anon namespace

namespace KMail {

void checkConfigUpdates() {
  static const char * const updates[] = {
    "9",
    "3.1-update-identities",
    "3.1-use-identity-uoids",
    "3.1-new-mail-notification",
    "3.2-update-loop-on-goto-unread-settings",
    "3.1.4-dont-use-UOID-0-for-any-identity",
    "3.2-misc",
    "3.2-moves",
  };
  static const int numUpdates = sizeof updates / sizeof *updates;

  KConfig * config = KMKernel::config();
  KConfigGroup startup( config, "Startup" );
  const int configUpdateLevel = startup.readNumEntry( "update-level", 0 );
  if ( configUpdateLevel == numUpdates ) // Optimize for the common case that everything is OK
    return;

  for ( int i = 0 ; i < numUpdates ; ++i )
    config->checkUpdate( updates[i], "kmail.upd" );
  startup.writeEntry( "update-level", numUpdates );
}

void lockOrDie() {
// Check and create a lock file to prevent concurrent access to kmail files
  QString lockLocation = locateLocal("data", "kmail/lock");
  KSimpleConfig config(lockLocation);
  int oldPid = config.readNumEntry("pid", -1);
  const QString oldHostName = config.readEntry("hostname");
  const QString hostName = getMyHostName();
  bool first_instance = (oldPid == -1);
  if (hostName == oldHostName && oldPid != getpid()) {
      if ( kill(oldPid, 0) == -1 )
          first_instance |= ( errno == ESRCH );
  }

  if ( !first_instance )
  {
    QString msg = i18n("Only one instance of KMail can be run at "
      "any one time. It is already running "
      "with PID %1 on host %2 according to the lock file located "
      "at %3.").arg(oldPid).arg(oldHostName).arg(lockLocation);

    KNotifyClient::userEvent( 0, msg,  KNotifyClient::Messagebox,
      KNotifyClient::Error );
    fprintf(stderr, "*** KMail is already running with PID %d on host %s\n",
            oldPid, oldHostName.local8Bit().data());
    exit(1);
  }

  config.writeEntry("pid", getpid());
  config.writeEntry("hostname", hostName);
  config.sync();
}

void insertLibraryCatalogues() {
  static const char * const catalogues[] = {
    "libkdenetwork",
    "libkdepim",
    "libktnef",
    "libkcal",
    "libksieve",
  };

  KLocale * l = KGlobal::locale();
  for ( unsigned int i = 0 ; i < sizeof catalogues / sizeof *catalogues ; ++i )
    l->insertCatalogue( catalogues[i] );
}

void cleanup()
{
  const QString lockLocation = locateLocal("data", "kmail/lock");
  KSimpleConfig config(lockLocation);
  config.writeEntry("pid", -1);
  config.sync();
}
}
