

#include <kdebug.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include "groupware_types.h"
#include "kmailinterface.h"
#include "mailcomposerinterface.h"
#include "aboutdata.h"

#include <QByteArray>


int main(int argc,char **argv)
{
  kDebug(5006) <<"Test KMail D-Bus interface.";

  KAboutData aboutData( "testKMailDBUS", 0,
   ki18n("Test for KMail D-Bus interface"), "0.0" );
  KCmdLineArgs::init(argc, argv, &aboutData);
  KApplication app;

  OrgKdeKmailKmailInterface kmailInterface( KMAIL_DBUS_SERVICE, "/KMail", QDBusConnection::sessionBus());
  kmailInterface.openComposer( "to 1","","","First test","simple openComp call",0);

  QDBusReply<QDBusObjectPath> composerDbusPath =  kmailInterface.openComposer("to 2","","","Second test",  "DBUS ref call",0);

  if ( !composerDbusPath.isValid() )
  {
    kDebug()<<"We can't connect to kmail";
    exit( 1 );
  }

  QDBusObjectPath composerPath = composerDbusPath;
  kDebug()<<"composerPath :"<<composerPath.path();
  OrgKdeKmailMailcomposerInterface kmailComposerInterface( KMAIL_DBUS_SERVICE, composerPath.path(),  QDBusConnection::sessionBus());

  QByteArray data = "BEGIN:VCALENDAR\nEND:VCALENDAR";
  kmailComposerInterface.addAttachment("test.ics","7bit",data,"text","calendar","method",
                             "publish","attachement;");
  kmailComposerInterface.send(2);

  kDebug(5006) <<"testDBus done.";

  return 0;
}
