// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <kuniqueapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include <kcrash.h>

#include "kmkernel.h" //control center

#undef Status // stupid X headers
#include "kmailIface_stub.h" // to call control center of master kmail

#include <kaboutdata.h>

#include "kmversion.h"


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

static KCmdLineOptions kmoptions[] =
{
  { "s", 0 , 0 },
  { "subject <subject>",	I18N_NOOP("Set subject of msg."), 0 },
  { "c", 0 , 0 },
  { "cc <address>",		I18N_NOOP("Send CC: to 'address'."), 0 },
  { "b", 0 , 0 },
  { "bcc <address>",		I18N_NOOP("Send BCC: to 'addres'."), 0 },
  { "h", 0 , 0 },
  { "header <header>",		I18N_NOOP("Add 'header' to msg."), 0 },
  { "msg <file>",		I18N_NOOP("Read msg-body from 'file'."), 0 },
  { "body <text>",              I18N_NOOP("Set body of msg."), 0 },
  { "attach <url>",             I18N_NOOP("Add an attachment to the mail, this can be repeated"), 0 },
  { "check",			I18N_NOOP("Check for new mail only."), 0 },
  { "composer",			I18N_NOOP("Open only composer window."), 0 },
  { "+[address]",		I18N_NOOP("Send msg to 'address'."), 0 },
//  { "+[file]",                  I18N_NOOP("Show message from file 'file'."), 0 },
  { 0, 0, 0}
};

//-----------------------------------------------------------------------------

extern "C" {

static void setSignalHandler(void (*handler)(int));

// Crash recovery signal handler
static void signalHandler(int sigId)
{
  setSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Exiting)\n", sigId);
  // try to cleanup all windows
  kernel->dumpDeadLetters();
  ::exit(-1); //
}

// Crash recovery signal handler
static void crashHandler(int sigId)
{
  setSignalHandler(SIG_DFL);
  fprintf(stderr, "*** KMail got signal %d (Crashing)\n", sigId);
  // try to cleanup all windows
  kernel->dumpDeadLetters();
  // Return to DrKonqi.
}
//-----------------------------------------------------------------------------


static void setSignalHandler(void (*handler)(int))
{
  signal(SIGKILL, handler);
  signal(SIGTERM, handler);
  signal(SIGHUP,  handler);
  KCrash::setEmergencySaveFunction(crashHandler);
}

}
//-----------------------------------------------------------------------------

class KMailApplication : public KUniqueApplication
{
public:
  KMailApplication() : KUniqueApplication() { };

  virtual int newInstance();

  void commitData(QSessionManager& sm) {
    kernel->notClosedByUser();
    KApplication::commitData( sm );
  }
};

int KMailApplication::newInstance()
{
  QString to, cc, bcc, subj, body;
  KURL messageFile = QString::null;
  KURL::List attachURLs;
  bool mailto = false;
  bool checkMail = false;
  //bool viewOnly = false;

  // process args:
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  if (args->getOption("subject"))
  {
     mailto = true;
     subj = QString::fromLocal8Bit(args->getOption("subject"));
  }

  if (args->getOption("cc"))
  {
     mailto = true;
     cc = QString::fromLocal8Bit(args->getOption("cc"));
  }

  if (args->getOption("bcc"))
  {
     mailto = true;
     bcc = QString::fromLocal8Bit(args->getOption("bcc"));
  }

  if (args->getOption("msg"))
  {
     mailto = true;
     messageFile = QString::fromLocal8Bit(args->getOption("msg"));
  }

  if (args->getOption("body"))
  {
     mailto = true;
     body = QString::fromLocal8Bit(args->getOption("body"));
  }

  QCStringList attachList = args->getOptionList("attach");
  if (!attachList.isEmpty())
  {
     mailto = true;
     for ( QCStringList::Iterator it = attachList.begin() ; it != attachList.end() ; ++it )
       if ( !(*it).isEmpty() )
         attachURLs += KURL( QString::fromLocal8Bit( *it ) );
  }

  if (args->isSet("composer"))
    mailto = true;

  if (args->isSet("check"))
    checkMail = true;

  for(int i= 0; i < args->count(); i++)
  {
    if (!to.isEmpty())
      to += ", ";
     if (strncasecmp(args->arg(i),"mailto:",7)==0)
       to += args->arg(i);
     else
       to += args->arg(i);
     mailto = true;
  }

  args->clear();

  if (!kernel->firstInstance() || !kapp->isRestored())
    kernel->action (mailto, checkMail, to, cc, bcc, subj, body, messageFile, attachURLs);
  kernel->setFirstInstance(FALSE);
  return 0;
}

int main(int argc, char *argv[])
{
  // WABA: KMail is a KUniqueApplication. Unfortunately this makes debugging
  // a bit harder: You should pass --nofork as commandline argument when using
  // a debugger. In gdb you can do this by typing "set args --nofork" before
  // typing "run".

  KAboutData about("kmail", I18N_NOOP("KMail"),
                   KMAIL_VERSION,
		   I18N_NOOP("The KDE Email client."),
		   KAboutData::License_GPL,
                   I18N_NOOP("(c) 1997-2001, The KMail developers"),
		   0,
		   "http://kmail.kde.org");
  about.addAuthor( "Michael H\303\244ckel", I18N_NOOP("Current maintainer"), "michael@haeckel.net" );
  about.addAuthor( "Don Sanders", I18N_NOOP("Core developer and former maintainer"), "sanders@kde.org" );
  about.addAuthor( "Stefan Taferner ", I18N_NOOP("Original author"), "taferner@kde.org" );
  about.addAuthor( "Ingo Kl\303\266cker", I18N_NOOP("Encryption"), "ingo.kloecker@epost.de" );
  about.addAuthor( "Marc Mutz", I18N_NOOP("Core developer"), "mutz@kde.org" );
  about.addAuthor( "Daniel Naber", I18N_NOOP("Documentation"), "daniel.naber@t-online.de" );
  about.addAuthor( "Andreas Gungl", I18N_NOOP("Encryption"), "a.gungl@gmx.de" );

  about.addAuthor( "Toyohiro Asukai", 0, "toyohiro@ksmplus.com" );
  about.addAuthor( "Waldo Bastian", 0, "bastian@kde.org" );
  about.addAuthor( "Carsten Burghardt", 0, "carsten.burghardt@web.de" );
  about.addAuthor( "Steven Brown", 0, "swbrown@ucsd.edu" );
  about.addAuthor( "Cristi Dumitrescu", 0, "cristid@chip.ro" );
  about.addAuthor( "Philippe Fremy", 0, "pfremy@chez.com" );
  about.addAuthor( "Kurt Granroth", 0, "granroth@kde.org" );
  about.addAuthor( "Heiko Hund", 0, "heiko@ist.eigentlich.net" );
  about.addAuthor( "Igor Janssen", 0, "rm@linux.ru.net" );
  about.addAuthor( "Matt Johnston", 0, "matt@caifex.org" );
  about.addAuthor( "Christer Kaivo-oja", 0, "whizkid@telia.com" );
  about.addAuthor( "Lars Knoll", 0, "knoll@kde.org" );
  about.addAuthor( "J. Nick Koston", 0, "bdraco@darkorb.net" );
  about.addAuthor( "Stephan Kulow", 0, "coolo@kde.org" );
  about.addAuthor( "Guillaume Laurent", 0, "glaurent@telegraph-road.org" );
  about.addAuthor( "Sam Magnuson", 0, "sam@trolltech.com" );
  about.addAuthor( "Laurent Montel", 0, "lmontel@mandrakesoft.com" );
  about.addAuthor( "Matt Newell", 0, "newellm@proaxis.com" );
  about.addAuthor( "Denis Perchine", 0, "dyp@perchine.com" );
  about.addAuthor( "Samuel Penn", 0, "sam@bifrost.demon.co.uk" );
  about.addAuthor( "Carsten Pfeiffer", 0, "pfeiffer@kde.org" );
  about.addAuthor( "Sven Radej", 0, "radej@kde.org" );
  about.addAuthor( "Mark Roberts", 0, "mark@taurine.demon.co.uk" );
  about.addAuthor( "Wolfgang Rohdewald", 0, "wrohdewald@dplanet.ch" );
  about.addAuthor( "Espen Sand", 0, "espen@kde.org" );
  about.addAuthor( "George Staikos", 0, "staikos@kde.org" );
  about.addAuthor( "Jason Stephenson", 0, "panda@mis.net" );
  about.addAuthor( "Jacek Stolarczyk", 0, "jacek@mer.chemia.polsl.gliwice.pl" );
  about.addAuthor( "Roberto S. Teixeira", 0, "maragato@kde.org" );
  about.addAuthor( "Ronen Tzur", 0, "rtzur@shani.net" );
  about.addAuthor( "Mario Weilguni", 0, "mweilguni@sime.com" );
  about.addAuthor( "Wynn Wilkes", 0, "wynnw@calderasystems.com" );
  about.addAuthor( "Robert D. Williams", 0, "rwilliams@kde.org" );
  about.addAuthor( "Markus Wuebben", 0, "markus.wuebben@kde.org" );
  about.addAuthor( "Thorsten Zachmann", 0, "t.zachmann@zagge.de" );
  about.addAuthor( "Karl-Heinz Zimmer", 0, "khz@kde.org" );

  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( kmoptions ); // Add kmail options

  if (!KMailApplication::start())
     return 0;

  KMailApplication app;
  KGlobal::locale()->insertCatalogue("libkdenetwork");

  KSimpleConfig config(locateLocal("appdata", "lock"));
  int oldPid = config.readNumEntry("pid", -1);
  if (oldPid != -1 && kill(oldPid, 0) != -1)
  {
    QString msg = i18n("Only one instance of KMail can be run at "
      "any one time. It is already running on a different display "
      "with PID %1.").arg(oldPid);

    KNotifyClient::userEvent( msg,  KNotifyClient::Messagebox,
      KNotifyClient::Error );
    fprintf(stderr, "*** KMail is already running with PID %d\n", oldPid);
    return 1;
  }

  config.writeEntry("pid", getpid());
  config.sync();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  //local, do the init
  KMKernel kmailKernel;
  kmailKernel.init();
  kapp->dcopClient()->setDefaultObject( kmailKernel.objId() );

  // and session management
  kmailKernel.doSessionManagement();

  // any dead letters?
  kmailKernel.recoverDeadLetters();

  setSignalHandler(signalHandler);

  kapp->dcopClient()->resume(); // Ok. We are ready for DCOP requests.
  // Go!
  int ret = kapp->exec();

  // clean up
  kmailKernel.cleanup();
  config.writeEntry("pid", -1);
  config.sync();
  return ret;
}

