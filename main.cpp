// -*- mode: C++; c-file-style: "gnu" -*-
// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <config.h>

#include <kuniqueapplication.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include "kmkernel.h" //control center
#include <kcmdlineargs.h>
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

static KCmdLineOptions kmoptions[] =
{
  { "s", 0 , 0 },
  { "subject <subject>",	I18N_NOOP("Set subject of message."), 0 },
  { "c", 0 , 0 },
  { "cc <address>",		I18N_NOOP("Send CC: to 'address'."), 0 },
  { "b", 0 , 0 },
  { "bcc <address>",		I18N_NOOP("Send BCC: to 'address'."), 0 },
  { "h", 0 , 0 },
  { "header <header>",		I18N_NOOP("Add 'header' to message."), 0 },
  { "msg <file>",		I18N_NOOP("Read message body from 'file'."), 0 },
  { "body <text>",              I18N_NOOP("Set body of message."), 0 },
  { "attach <url>",             I18N_NOOP("Add an attachment to the mail. This can be repeated."), 0 },
  { "check",			I18N_NOOP("Only check for new mail."), 0 },
  { "composer",			I18N_NOOP("Only open composer window."), 0 },
  { "+[address|URL]",		I18N_NOOP("Send message to 'address' resp. "
                                          "attach the file the 'URL' points "
                                          "to."), 0 },
//  { "+[file]",                  I18N_NOOP("Show message from file 'file'."), 0 },
  KCmdLineLastOption
};

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
  QString to, cc, bcc, subj, body;
  KURL messageFile = QString::null;
  KURL::List attachURLs;
  bool mailto = false;
  bool checkMail = false;
  //bool viewOnly = false;

  if (dcopClient()->isSuspended())
  {
    // Try again later.
    QTimer::singleShot( 100, this, SLOT(newInstance()) );
    return 0;
  }

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
     messageFile.setPath( QString::fromLocal8Bit(args->getOption("msg")) );
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
    if (strncasecmp(args->arg(i),"mailto:",7)==0)
      to += args->url(i).path() + ", ";
    else {
      QString tmpArg = QString::fromLocal8Bit( args->arg(i) );
      KURL url( tmpArg );
      if ( url.isValid() )
        attachURLs += url;
      else
        to += tmpArg + ", ";
    }
    mailto = true;
  }
  if ( !to.isEmpty() ) {
    // cut off the superfluous trailing ", "
    to.truncate( to.length() - 2 );
  }

  args->clear();

  if (!kmkernel->firstInstance() || !kapp->isRestored())
    kmkernel->action (mailto, checkMail, to, cc, bcc, subj, body, messageFile, attachURLs);
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
  KCmdLineArgs::addCmdLineOptions( kmoptions ); // Add kmail options
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
