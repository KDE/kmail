#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kdebug.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kcmdlineargs.h>
#include "aboutdata.h"

#include <kmailIface_stub.h>
#include <mailcomposerIface_stub.h>

int main(int argc,char **argv)
{
  kdDebug(5006) << "Test KMail DCOP interface." << endl;

  KAboutData aboutData( "testKMailDCOP",
   "Test for KMail DCOP interface", "0.0" );
  KCmdLineArgs::init(argc, argv, &aboutData);
  KApplication app;
  app.dcopClient()->attach();

  KMailIface_stub kmailStub("kmail","KMailIface");
  
  kmailStub.openComposer("to 1","","","First test","simple openComp call",0,
                         KURL());

  DCOPRef ref = kmailStub.openComposer("to 2","","","Second test",
                                       "DCOP ref call",0);
  MailComposerIface_stub composerStub(ref.app(),ref.object());
  QCString data = "BEGIN:VCALENDAR\nEND:VCALENDAR";
  composerStub.addAttachment("test.ics","7bit",data,"text","calendar","method",
                             "publish","attachement;");
  composerStub.send(2);

  kdDebug(5006) << "testDCOP done." << endl;

  return 0;
}
