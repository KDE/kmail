/*
 * Copyright (c) 2011, 2012, 2013 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "kmknotify.h"
#include "kmkernel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <KComboBox>
#include <KNotifyConfigWidget>
#include <KLocalizedString>
#include <KConfig>
#include <KStandardDirs>
#include <KSeparator>
#include <KIconLoader>

using namespace KMail;

KMKnotify::KMKnotify( QWidget * parent )
    :KDialog( parent ), m_changed( false )
{
    setCaption( i18n("Notification") );
    setButtons( Ok|Cancel );

    QWidget *page = new QWidget( this );
    setMainWidget( page );

    QVBoxLayout *layout = new QVBoxLayout( page );
    layout->setMargin( 0 );
    m_comboNotify = new KComboBox( false );
    m_comboNotify->setSizeAdjustPolicy( QComboBox::AdjustToContents );

    QHBoxLayout *hbox = new QHBoxLayout();
    layout->addLayout( hbox );
    hbox->addWidget( m_comboNotify, 10 );

    m_notifyWidget = new KNotifyConfigWidget( page );
    layout->addWidget( m_notifyWidget );
    m_comboNotify->setFocus();

    layout->addWidget(new KSeparator);

    connect( m_comboNotify, SIGNAL(activated(int)),
             SLOT(slotComboChanged(int)) );
    connect( this, SIGNAL(okClicked()), SLOT(slotOk()) );
    connect( m_notifyWidget ,SIGNAL(changed(bool)) , this , SLOT(slotConfigChanged(bool)));
    initCombobox();
    readConfig();
}

KMKnotify::~KMKnotify()
{
    writeConfig();
}

void KMKnotify::slotConfigChanged( bool changed )
{
    m_changed = changed;
}

void KMKnotify::slotComboChanged( int index )
{
    QString text( m_comboNotify->itemData(index).toString() );
    if ( m_changed ) {
        m_notifyWidget->save();
        m_changed = false;
    }

    m_notifyWidget->setApplication( text );
}

void KMKnotify::setCurrentNotification(const QString &name)
{
    const int index = m_comboNotify->findData(name);
    if (index>-1) {
        m_comboNotify->setCurrentIndex(index);
        slotComboChanged(index);
    }
}

void KMKnotify::initCombobox()
{

    QStringList lstNotify;
    lstNotify<< QLatin1String( "kmail2/kmail2.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_maildispatcher_agent/akonadi_maildispatcher_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_mailfilter_agent/akonadi_mailfilter_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_archivemail_agent/akonadi_archivemail_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_sendlater_agent/akonadi_sendlater_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_newmailnotifier_agent/akonadi_newmailnotifier_agent.notifyrc" );
    lstNotify<< QLatin1String( "messageviewer/messageviewer.notifyrc" );
    //TODO add other notifyrc here if necessary

    Q_FOREACH ( const QString& notify, lstNotify ) {
        const QString fullPath = KStandardDirs::locate( "data", notify );

        if ( !fullPath.isEmpty() ) {
            const int slash = fullPath.lastIndexOf( QLatin1Char('/') ) - 1;
            const int slash2 = fullPath.lastIndexOf( QLatin1Char('/'), slash );
            const QString appname= ( slash2 < 0 ) ? QString() :  fullPath.mid( slash2+1 , slash-slash2  );
            if ( !appname.isEmpty() ) {
#if 0 //QT5
                KConfig config(fullPath, KConfig::NoGlobals, "data" );
                KConfigGroup globalConfig( &config, QString::fromLatin1("Global") );
                const QString icon = globalConfig.readEntry(QString::fromLatin1("IconName"), QString::fromLatin1("misc"));
                const QString description = globalConfig.readEntry( QString::fromLatin1("Comment"), appname );
                m_comboNotify->addItem( SmallIcon( icon ), description, appname );
#endif
            }
        }
    }

    m_comboNotify->model()->sort(0);
    if ( m_comboNotify->count() > 0 ) {
        m_comboNotify->setCurrentIndex(0);
        m_notifyWidget->setApplication( m_comboNotify->itemData( 0 ).toString() );
    }
}

void KMKnotify::slotOk()
{
    if ( m_changed )
        m_notifyWidget->save();
}

void KMKnotify::readConfig()
{
    KConfigGroup notifyDialog( KMKernel::self()->config(), "KMKnotifyDialog" );
    const QSize size = notifyDialog.readEntry( "Size", QSize(600, 400) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void KMKnotify::writeConfig()
{
    KConfigGroup notifyDialog( KMKernel::self()->config(), "KMKnotifyDialog" );
    notifyDialog.writeEntry( "Size", size() );
    notifyDialog.sync();
}


