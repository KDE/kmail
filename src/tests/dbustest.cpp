#include <QDebug>

#include <QCommandLineParser>
#include <QCoreApplication>

#include "kmailinterface.h"

int main(int argc, char **argv)
{
    qDebug() << "Test KMail D-Bus interface.";

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    OrgKdeKmailKmailInterface kmailInterface(QStringLiteral("org.kde.kmail"), QStringLiteral("/KMail"), QDBusConnection::sessionBus());
    kmailInterface.openComposer(QStringLiteral("to 1"), QString(), QString(), QStringLiteral("First test"), QStringLiteral("simple openComp call"), false);

    QDBusReply<QDBusObjectPath> composerDbusPath =
        kmailInterface.openComposer(QStringLiteral("to 2"), QString(), QString(), QStringLiteral("Second test"), QStringLiteral("DBUS ref call"), false);

    if (!composerDbusPath.isValid()) {
        qDebug() << "We can't connect to kmail";
        exit(1);
    }

    qDebug() << "testDBus done.";

    return 0;
}
