#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kdebug.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include <kmailIface_stub.h>
#include <mailcomposerIface_stub.h>

int main(int argc,char **argv)
{
  kdDebug(5006) << "Test KMail DCOP interface." << endl;

  KAboutData about("testKMailDCOP","TestKMailDCOP",
                   "0.0",
		   "Test for KMail DCOP interface",
		   KAboutData::License_GPL,
                   "(c) 2001, Cornelius Schumacher",
		   0,
		   "http://kmail.kde.org");
  KCmdLineArgs::init(argc, argv, &about);
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
