

#include <qdebug.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include "kmailinterface.h"
#include "aboutdata.h"



int main(int argc,char **argv)
{
  qDebug() << "Test KMail D-Bus interface.";

  KAboutData aboutData( "testKMailDBUS", 0,
   ki18n("Test for KMail D-Bus interface"), "0.0" );
  KCmdLineArgs::init(argc, argv, &aboutData);
  KApplication app;

  OrgKdeKmailKmailInterface kmailInterface( QLatin1String("org.kde.kmail"), QLatin1String("/KMail"), QDBusConnection::sessionBus());
  kmailInterface.openComposer( QLatin1String("to 1"),QString(),QString(),QLatin1String("First test"),QLatin1String("simple openComp call"),0);

  QDBusReply<QDBusObjectPath> composerDbusPath =  kmailInterface.openComposer(QLatin1String("to 2"),QString(),QString(),QLatin1String("Second test"), QLatin1String("DBUS ref call"),0);

  if ( !composerDbusPath.isValid() )
  {
    qDebug()<<"We can't connect to kmail";
    exit( 1 );
  }

  qDebug() << "testDBus done.";

  return 0;
}
