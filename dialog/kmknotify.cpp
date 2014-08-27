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
#include <KSeparator>
#include <KIconLoader>
#include <QStandardPaths>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>

using namespace KMail;

KMKnotify::KMKnotify( QWidget * parent )
    :QDialog( parent ), m_changed( false )
{
    setWindowTitle( i18n("Notification") );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &KMKnotify::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &KMKnotify::reject);

    QWidget *page = new QWidget( this );
    mainLayout->addWidget(page);

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

    mainLayout->addWidget(buttonBox);
    connect( m_comboNotify, SIGNAL(activated(int)),
             SLOT(slotComboChanged(int)) );
    connect(okButton, &QPushButton::clicked, this, &KMKnotify::slotOk);
    connect(m_notifyWidget, &KNotifyConfigWidget::changed, this , &KMKnotify::slotConfigChanged);
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
    lstNotify<< QLatin1String( "kmail2.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_maildispatcher_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_mailfilter_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_archivemail_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_sendlater_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_newmailnotifier_agent.notifyrc" );
    lstNotify<< QLatin1String( "akonadi_followupreminder_agent.notifyrc" );
    lstNotify<< QLatin1String( "messageviewer.notifyrc" );
    //TODO add other notifyrc here if necessary

    Q_FOREACH ( const QString& notify, lstNotify ) {
        const QString fullPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("knotifications5/") + notify );

        if ( !fullPath.isEmpty() ) {
            const int slash = fullPath.lastIndexOf( QLatin1Char('/') );
            QString appname= fullPath.right(fullPath.length() - slash-1);
            appname.remove(QLatin1String(".notifyrc"));
            if ( !appname.isEmpty() ) {
                KConfig config(fullPath, KConfig::NoGlobals, QStandardPaths::DataLocation );
                KConfigGroup globalConfig( &config, QString::fromLatin1("Global") );
                const QString icon = globalConfig.readEntry(QString::fromLatin1("IconName"), QString::fromLatin1("misc"));
                const QString description = globalConfig.readEntry( QString::fromLatin1("Comment"), appname );
                m_comboNotify->addItem( SmallIcon( icon ), description, appname );
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


