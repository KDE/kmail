// KMail startup and initialize code
// Author: Stefan Taferner <taferner@alpin.or.at>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#include <kapp.h>
#include <klocale.h>
#include <kglobal.h>
#include <dcopclient.h>

#include "kmkernel.h" //control center

#undef Status // stupid X headers
#include "kmailIface_stub.h" // to call control center of master kmail

#include <kaboutdata.h>
#include <kdebug.h>

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
const char *aboutText = 
  "KMail [" KMAIL_VERSION "] by\n\n"
  "Stefan Taferner <taferner@kde.org>,\n"
  "Don Sanders <don@sanders.org>,\n"
  "Andreas Gungl <Andreas.Gungle@osp-dd.de>,\n"
  "George Staikos <staikos@kde.org>,\n"
  "Espen Sand <espensa@online.no>,\n"
  "Daniel Naber <dnaber@mini.gt.owl.de>,\n"
  "Mario Weilguni <mweilguni@sime.com>\n\n"
  "based on the work of:\n"
  "Lynx <lynx@topaz.hknet.com>,\n"
  "Stephan Meyer <Stephan.Meyer@pobox.com>\n"
  "and the above authors.\n\n"
  "This program is covered by the GPL.\n\n"
  "Please send bugreports to kmail@kde.org";

static const char *description = I18N_NOOP("A KDE E-Mail client."); 

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
  { "check",			I18N_NOOP("Check for new mail only."), 0 },
  { "composer",			I18N_NOOP("Open only composer window."), 0 },
  { "+[address]",		I18N_NOOP("Send msg to 'address'."), 0 },
//  { "+[file]",                  I18N_NOOP("Show message from file 'file'."), 0 },
  { 0, 0, 0}
};


static void signalHandler(int sigId);
static void setSignalHandler(void (*handler)(int));

void remoteAction (bool mailto, bool check, QString to, QString cc,
                   QString bcc, QString subj, KURL messageFile);
void processArgs (KCmdLineArgs *args, bool remoteCall);

//-----------------------------------------------------------------------------


// Crash recovery signal handler
static void signalHandler(int sigId)
{
  fprintf(stderr, "*** KMail got signal %d\n", sigId);

  debug ("setting signal handler to default");
  setSignalHandler(SIG_DFL);
  // try to cleanup all windows
  kernel->dumpDeadLetters(); // exits there.
}
//-----------------------------------------------------------------------------


static void setSignalHandler(void (*handler)(int))
{
  signal(SIGSEGV, handler);
  signal(SIGKILL, handler);
  signal(SIGTERM, handler);
  signal(SIGHUP,  handler);
  signal(SIGFPE,  handler);
  signal(SIGABRT, handler);
}
//-----------------------------------------------------------------------------

// Sven: new from Jens Kristian Soegard:
void processArgs(KCmdLineArgs *args, bool remoteCall)
{
  QString to, cc, bcc, subj;
  KURL messageFile = QString::null;
  bool mailto = false;
  bool checkMail = false;
  //bool viewOnly = false;

  // process args:
  if (args->getOption("subject"))
  {
     mailto = true;
     subj = args->getOption("subject");
  }

  if (args->getOption("cc"))
  {
     mailto = true;
     cc = args->getOption("cc");
  }

  if (args->getOption("bcc"))
  {
     mailto = true;
     bcc = args->getOption("bcc");
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

  if (remoteCall)
    remoteAction (mailto, checkMail, to, cc, bcc, subj, messageFile);
  else
  {
    if (kernel)
      kernel->action (mailto, checkMail, to, cc, bcc, subj, messageFile);
    else
    {
      debug (":-( Local call and no kmkernel. Ouch!");
      ::exit (-1);
    }
  }
}

void remoteAction (bool mailto, bool check, QString to, QString cc,
                   QString bcc, QString subj, KURL messageFile)
{
   KMailIface_stub *kmail = new KMailIface_stub("kmail", "KMailIface");

   if (mailto)
     kmail->openComposer (to, cc, bcc, subj, 0); //returns id but we don´t mind
   else
     kmail->openReader (messageFile);
   if (check)
     kmail->checkMail();    	
}

//-----------------------------------------------------------------------------

/* TODO fix possible race condition
   Master-kmail might take some time to a) register as kmail and b) to
   create and init kmkernel object. If another kmail starts before completion of
   a), it will attempt to become new master -> must check if registerAs("kmail")
   worked. If another kmail starts before completion of b) it will try to call
   uninitialized or nonexistant master´s kmkernel stuff which will lead to SEGV
   or who knows what ->
   
 
*/
int main(int argc, char *argv[])
{
  bool remoteCall;

  KMKernel *kmailKernel = 0;
  
  KAboutData about("kmail", I18N_NOOP("KMail"), 
                   KMAIL_VERSION, 
                   description,
		   KAboutData::License_GPL,
                   "(c) 1997-2000, The KMail developers" );

  KCmdLineArgs::init(argc, argv, &about);
  KCmdLineArgs::addCmdLineOptions( kmoptions ); // Add kmail options
  
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // We must construct this in both cases thanks to fantastic
  // design of KApplication and KCmdlineArgs. KUniqueApplication?
  // naaah, it´s something different...
  KApplication *app = new KApplication;
  
  // Shouldn´t it register by default with a name from kapp?
  // register as "kmail" not "kmail-pid"
  app->dcopClient()->registerAs ("kmail", false);       //register
  if (app->dcopClient()->appId() == "kmail")            // success, we´re master?
  {
    remoteCall = false;
  }
  else
  {
    // Either we are not the first who registered as kmail
    // (Another kmail exists/was faster -> we should be slave now)
    // or there is dcop error.
    
    if (app->dcopClient()->isApplicationRegistered("kmail")) // master exists?
    {
      remoteCall = true;                                     //yes, we´re client
    }
    else
    {
      // we couldn´t register as "kmail" and no "kmail" registered at all
      // this is an dcop error, so tell it to the user
      debug( i18n("DCOP error\n KMail could not register at DCOP server.\n"));
      //this was localized for possible messagebox later
      exit(-1);
    }
  }

  if (!remoteCall)
  {
    //local, do the init
    kmailKernel = new KMKernel;
    kmailKernel->init();

    // and session management
    if (!kmailKernel->doSessionManagement())
      processArgs (args, false); //process args does everything
  }
  else
  {
    processArgs(args, true); // warning: no (unneded) kmkernel stuff!
    ::exit (0);   //we´re done
  }
  //free memory
  args->clear();
  
  // any dead letters?
  kmailKernel->recoverDeadLetters();

  setSignalHandler(signalHandler);
  
  // Go!
  app->exec();

  // clean up
  kmailKernel->cleanup();
  delete kmailKernel;
}

