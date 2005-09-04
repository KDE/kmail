//Added by qt3to4:
#include <Q3ValueList>
/*
    sievedebugdialog.h

    KMail, the KDE mail client.
    Copyright (c) 2005 Martijn Klingens <klingens@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __sievedebugdialog_h__
#define __sievedebugdialog_h__

// This file is only compiled when debug is enabled, it is
// not useful enough for non-developers to have this in releases.
#if !defined(NDEBUG)

#include <kdialogbase.h>
#include <kurl.h>

class QString;
class QStringList;
class Q3TextEdit;
template <typename T> class Q3ValueList;

class KMAccount;

namespace KMime
{
  namespace Types
  {
    struct AddrSpec;
    typedef Q3ValueList<AddrSpec> AddrSpecList;
  }
}

namespace KMail
{
class ImapAccountBase;
class SieveJob;

/**
 * Diagnostic info for Sieve. Only compiled when debug is enabled, it is
 * not useful enough for non-developers to have this in releases.
 */
class SieveDebugDialog : public KDialogBase
{
    Q_OBJECT

public:
    SieveDebugDialog( QWidget *parent = 0, const char *name = 0 );
    virtual ~SieveDebugDialog();

protected:
    void handlePutResult( KMail::SieveJob *job, bool success, bool );

signals:
    void result( bool success );

protected slots:
    void slotGetScript( KMail::SieveJob *job, bool success, const QString &script, bool active );
    void slotGetScriptList( KMail::SieveJob *job, bool success, const QStringList &scriptList, const QString &activeScript );

    void slotDialogOk();
    void slotPutActiveResult( KMail::SieveJob*, bool );
    void slotPutInactiveResult( KMail::SieveJob*, bool );
    void slotDiagNextAccount();
    void slotDiagNextScript();

protected:
    KMail::SieveJob *mSieveJob;
    KURL mUrl;

    Q3TextEdit *mEdit;

    // Copied from AccountManager, because we have to do an async iteration
    // WARNING: When copy/pasting this code, be aware that accounts may
    //          get removed inbetween! For debugging this is good enough
    //          though. - Martijn
    Q3ValueList<KMAccount *> mAccountList;
    QStringList mScriptList;
    KMail::ImapAccountBase *mAccountBase;
};

} // namespace KMail

#endif // NDEBUG

#endif // __sievedebugdialog_h__

