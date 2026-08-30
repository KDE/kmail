#include <QDebug>

#include <QCommandLineParser>
#include <QCoreApplication>

#include "kmailinterface.h"
using namespace Qt::Literals::StringLiterals;

int main(int argc, char **argv)
{
    qDebug() << "Test KMail D-Bus interface.";

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    OrgKdeKmailKmailInterface kmailInterface(u"org.kde.kmail"_s, u"/KMail"_s, QDBusConnection::sessionBus());
    kmailInterface.openComposer(u"to 1"_s, QString(), QString(), u"First test"_s, u"simple openComp call"_s, false);

    QDBusReply<QDBusObjectPath> composerDbusPath = kmailInterface.openComposer(u"to 2"_s, QString(), QString(), u"Second test"_s, u"DBUS ref call"_s, false);

    if (!composerDbusPath.isValid()) {
        qDebug() << "We can't connect to kmail";
        exit(1);
    }

    qDebug() << "testDBus done.";

    return 0;
}
