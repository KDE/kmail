

#include <qdebug.h>

#include <QCoreApplication>
#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>

#include "kmailinterface.h"
//#include "aboutdata.h"

int main(int argc, char **argv)
{
    qDebug() << "Test KMail D-Bus interface.";

    KAboutData aboutData(QLatin1String("testKMailDBUS"), i18n("Test for KMail D-Bus interface"), QStringLiteral("0.0"));
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    OrgKdeKmailKmailInterface kmailInterface(QLatin1String("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
    kmailInterface.openComposer(QLatin1String("to 1"), QString(), QString(), QStringLiteral("First test"), QStringLiteral("simple openComp call"), 0);

    QDBusReply<QDBusObjectPath> composerDbusPath =  kmailInterface.openComposer(QLatin1String("to 2"), QString(), QString(), QStringLiteral("Second test"), QStringLiteral("DBUS ref call"), 0);

    if (!composerDbusPath.isValid()) {
        qDebug() << "We can't connect to kmail";
        exit(1);
    }

    qDebug() << "testDBus done.";

    return 0;
}
