/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <kontactinterface/pimuniqueapplication.h>

#include "kmkernel.h" //control center
#include "kmmainwidget.h"
#include "kmail_options.h"
#include "kmmigrateapplication.h"

#include "kmail_debug.h"
#include <kmessagebox.h>
#undef Status // stupid X headers

#include "aboutdata.h"

#include "kmstartup.h"

#ifdef Q_OS_WIN
#include <unistd.h>
#include <windows.h>
#endif

#include <QDir>
#include <QApplication>
#include <QSessionManager>

//-----------------------------------------------------------------------------

class KMailApplication : public KontactInterface::PimUniqueApplication
{
public:
    KMailApplication(int &argc, char **argv[])
        : KontactInterface::PimUniqueApplication(argc, argv)
        , mDelayedInstanceCreation(false)
        , mEventLoopReached(false)
    { }

    int activate(const QStringList &args, const QString &workindDir) Q_DECL_OVERRIDE;
    void commitData(QSessionManager &sm);
    void setEventLoopReached();
    void delayedInstanceCreation(const QStringList &args, const QString &workindDir);
protected:
    bool mDelayedInstanceCreation;
    bool mEventLoopReached;

};

void KMailApplication::commitData(QSessionManager &sm)
{
    kmkernel->dumpDeadLetters();
    kmkernel->setShuttingDown(true);   // Prevent further dumpDeadLetters calls
}

void KMailApplication::setEventLoopReached()
{
    mEventLoopReached = true;
}

int KMailApplication::activate(const QStringList &args, const QString &workindDir)
{
    qCDebug(KMAIL_LOG);

    // If the event loop hasn't been reached yet, the kernel is probably not
    // fully initialized. Creating an instance would therefore fail, this is why
    // that is delayed until delayedInstanceCreation() is called.
    if (!mEventLoopReached) {
        qCDebug(KMAIL_LOG) << "Delaying instance creation.";
        mDelayedInstanceCreation = true;
        return 0;
    }

    if (!kmkernel) {
        return 0;
    }
    if (kmkernel->shuttingDown()) {
        qCDebug(KMAIL_LOG) << "KMail is in a shutdown mode.";
        return 0;
    }

    if (!kmkernel->firstInstance() || !qApp->isSessionRestored()) {
        kmkernel->handleCommandLine(true, args, workindDir);
    }
    kmkernel->setFirstInstance(false);
    return 0;
}

void KMailApplication::delayedInstanceCreation(const QStringList &args, const QString &workindDir)
{
    if (mDelayedInstanceCreation) {
        activate(args, workindDir);
    }
}

int main(int argc, char *argv[])
{
    KMailApplication app(argc, &argv);
    KLocalizedString::setApplicationDomain("kmail");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#if QT_VERSION >= 0x050600
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kmail")));
    KMail::AboutData about;
    app.setAboutData(about);

    QCommandLineParser *cmdArgs = app.cmdArgs();
    kmail_options(cmdArgs);

    const QStringList args = QApplication::arguments();
    cmdArgs->process(args);
    about.processCommandLine(cmdArgs);

    if (!KMailApplication::start(args)) {
        qCDebug(KMAIL_LOG) << "Another instance of KMail already running";
        return 0;
    }

    KMMigrateApplication migrate;
    migrate.migrate();

    // import i18n data and icons from libraries:
    KMail::insertLibraryCataloguesAndIcons();

    //local, do the init
    KMKernel kmailKernel;
    kmailKernel.init();

    // and session management
    kmailKernel.doSessionManagement();

    // any dead letters?
    kmailKernel.recoverDeadLetters();

    kmkernel->setupDBus(); // Ok. We are ready for D-Bus requests.

    //If the instance hasn't been created yet, do that now
    app.setEventLoopReached();
    app.delayedInstanceCreation(args, QDir::currentPath());

    // Go!
    int ret = qApp->exec();
    // clean up
    kmailKernel.cleanup();
    return ret;
}
