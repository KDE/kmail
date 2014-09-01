/* -*- mode: C++; c-file-style: "gnu" -*-
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
#include <kglobal.h>
#include "kmkernel.h" //control center
#include "kmmainwidget.h"
#include "kmail_options.h"

#include <qdebug.h>
#include <kmessagebox.h>
#undef Status // stupid X headers

#include "aboutdata.h"

#include "kmstartup.h"

#ifdef Q_OS_WIN
#include <unistd.h>
#include <windows.h>
#endif

#include <QDir>

//-----------------------------------------------------------------------------

class KMailApplication : public KontactInterface::PimUniqueApplication
{
public:
    KMailApplication() : KontactInterface::PimUniqueApplication(),
        mDelayedInstanceCreation( false ),
        mEventLoopReached( false ) { }
    virtual int newInstance();
    void commitData(QSessionManager& sm);
    void setEventLoopReached();
    void delayedInstanceCreation();
protected:
    bool mDelayedInstanceCreation;
    bool mEventLoopReached;

};

void KMailApplication::commitData(QSessionManager& sm) {
    kmkernel->dumpDeadLetters();
    kmkernel->setShuttingDown( true ); // Prevent further dumpDeadLetters calls
    KApplication::commitData( sm );
}

void KMailApplication::setEventLoopReached() {
    mEventLoopReached = true;
}

int KMailApplication::newInstance()
{
    qDebug();

    // If the event loop hasn't been reached yet, the kernel is probably not
    // fully initialized. Creating an instance would therefore fail, this is why
    // that is delayed until delayedInstanceCreation() is called.
    if ( !mEventLoopReached ) {
        qDebug() << "Delaying instance creation.";
        mDelayedInstanceCreation = true;
        return 0;
    }

    if (!kmkernel)
        return 0;

    if (!kmkernel->firstInstance() || !kapp->isSessionRestored())
        kmkernel->handleCommandLine( true );
    kmkernel->setFirstInstance(false);
    return 0;
}

void KMailApplication::delayedInstanceCreation() {
    if ( mDelayedInstanceCreation )
        newInstance();
}

int main(int argc, char *argv[])
{
    // WABA: KMail is a KUniqueApplication. Unfortunately this makes debugging
    // a bit harder: You should pass --nofork as commandline argument when using
    // a debugger. In gdb you can do this by typing "set args --nofork" before
    // typing "run".
#if 0 // for testing KUniqueAppliaction on Windows
    MessageBoxA(NULL,
                QString("main() %1 pid=%2").arg(argv[0]).arg(getpid()).toLatin1(),
            QString("main() \"%1\"").arg(argv[0]).toLatin1(), MB_OK|MB_ICONINFORMATION|MB_TASKMODAL);
#endif
    KMail::AboutData about;

    KCmdLineArgs::setCwd(QDir::currentPath().toLocal8Bit());
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( kmail_options() ); // Add kmail options
    if (!KMailApplication::start())
        return 0;

    // Migrate to xdg path
    KMail::migrateConfig();

    KMailApplication app;

    // Qt doesn't treat the system tray as a window, and therefore Qt would quit
    // the event loop when an error message is clicked away while KMail is in the
    // tray.
    // Rely on KGlobal::ref() and KGlobal::deref() instead, like we did in KDE3.
    // See http://bugs.kde.org/show_bug.cgi?id=163479
    QApplication::setQuitOnLastWindowClosed( false );

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
    kmkernel->setStartingUp( false ); // Starting up is finished

    //If the instance hasn't been created yet, do that now
    app.setEventLoopReached();
    app.delayedInstanceCreation();

    // Go!
    int ret = qApp->exec();
    // clean up
    kmailKernel.cleanup();
    return ret;
}
