// Author: Don Sanders <sanders@kde.org>
// License GPL

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include <kcrash.h>

#include "kmkernel.h" //control center
#include "kmstartup.h"

#undef Status // stupid X headers

extern "C" {

// Crash recovery signal handler
void kmsignalHandler(int sigId)
{
  kmsetSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Exiting)\n", sigId);
  // try to cleanup all windows
  kernel->dumpDeadLetters();
  ::exit(-1); //
}

// Crash recovery signal handler
void kmcrashHandler(int sigId)
{
  kmsetSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Crashing)\n", sigId);
  // try to cleanup all windows
  kernel->dumpDeadLetters();
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

namespace KMail
{
QString getMyHostName(void)
{
  char hostNameC[256];
  // null terminate this C string
  hostNameC[255] = 0;
  // set the string to 0 length if gethostname fails
  if(gethostname(hostNameC, 255))
    hostNameC[0] = 0;
  return QString::fromLocal8Bit(hostNameC);
}


void checkConfigUpdates() {
  //Please update the currentUpdateLevel when new updates are created.
  const int currentUpdateLevel = 100;
  KConfig * config = KMKernel::config();
  KConfigGroup startup( config, "Startup" );
  int configUpdateLevel = startup.readNumEntry( "update-level", 0 );
  if (configUpdateLevel == currentUpdateLevel) //Allow downgrading
      return;

  const QString updateFile = QString::fromLatin1("kmail.upd");
  QStringList updates;
  updates << "9"
	  << "3.1-update-identities"
	  << "3.1-use-identity-uoids";
  for ( QStringList::const_iterator it = updates.begin() ; it != updates.end() ; ++it )
    config->checkUpdate( *it, updateFile );
  startup.writeEntry( "update-level", currentUpdateLevel );
}

void lockOrDie() {
// Check and create a lock file to prevent concurrent access to kmail files
  QString lockLocation = locateLocal("data", "kmail/lock");
  KSimpleConfig config(lockLocation);
  int oldPid = config.readNumEntry("pid", -1);
  const QString oldHostName = config.readEntry("hostname");
  const QString hostName = getMyHostName();
  // proceed if there is no lock at present
  if (oldPid != -1 &&
  // proceed if the lock is our pid, or if the lock is from the same host
      oldPid != getpid() &&
  // proceed if the pid doesn't exist
      (kill(oldPid, 0) != -1 || errno != ESRCH))
  {
    QString msg = i18n("Only one instance of KMail can be run at "
      "any one time. It is already running "
      "with PID %1 on host %2 according to the lock file located "
      "at %3.").arg(oldPid).arg(oldHostName).arg(lockLocation);

    KNotifyClient::userEvent( msg,  KNotifyClient::Messagebox,
      KNotifyClient::Error );
    fprintf(stderr, "*** KMail is already running with PID %d on host %s\n",
            oldPid, oldHostName.local8Bit().data());
    exit(1);
  }

  config.writeEntry("pid", getpid());
  config.writeEntry("hostname", hostName);
  config.sync();
}

void cleanup()
{
  const QString lockLocation = locateLocal("data", "kmail/lock");
  KSimpleConfig config(lockLocation);
  config.writeEntry("pid", -1);
  config.sync();
}
}
