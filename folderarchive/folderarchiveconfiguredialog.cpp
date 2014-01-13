/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "folderarchiveconfiguredialog.h"
#include "folderarchivesettingpage.h"

#include "kdepim-version.h"

#include <Akonadi/AgentManager>
#include <KMime/Message>

#include <KLocale>
#include <KConfigGroup>
#include <KGlobal>
#include <KHelpMenu>
#include <KMenu>
#include <KIcon>
#include <KAboutData>

#include <QTabWidget>

FolderArchiveConfigureDialog::FolderArchiveConfigureDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n("Configure") );
    setWindowIcon( KIcon( QLatin1String("kmail") ) );
    setButtons( Help|Ok|Cancel );
    setHelp(QLatin1String("archive-folder"), QLatin1String("kmail"));
    readConfig();

    mTabWidget = new QTabWidget;
    initializeTab();
    setMainWidget(mTabWidget);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));
}

FolderArchiveConfigureDialog::~FolderArchiveConfigureDialog()
{
    writeConfig();
}

void FolderArchiveConfigureDialog::initializeTab()
{
    Akonadi::AgentManager *manager = Akonadi::AgentManager::self();
    const Akonadi::AgentInstance::List list = manager->instances();
    foreach( const Akonadi::AgentInstance &agent, list ) {
        const QStringList capabilities( agent.type().capabilities() );
        if (agent.type().mimeTypes().contains( KMime::Message::mimeType())) {
            if ( capabilities.contains( QLatin1String("Resource") ) &&
                 !capabilities.contains( QLatin1String("Virtual") ) &&
                 !capabilities.contains( QLatin1String("MailTransport") ) ) {
                const QString identifier = agent.identifier();
                //Store just imap/maildir account. Store other config when we copy data.
                if (identifier.contains(QLatin1String("imap")) || identifier.contains(QLatin1String("maildir"))) {
                    FolderArchiveSettingPage *page = new FolderArchiveSettingPage(identifier);
                    page->loadSettings();
                    mTabWidget->addTab(page, agent.name());
                }
            }
        }
    }
}

void FolderArchiveConfigureDialog::slotOkClicked()
{
    const int numberOfTab(mTabWidget->count());
    for (int i=0; i <numberOfTab; ++i) {
        FolderArchiveSettingPage *page = static_cast<FolderArchiveSettingPage *>(mTabWidget->widget(i));
        page->writeSettings();
    }
}

static const char *myConfigGroupName = "FolderArchiveConfigureDialog";

void FolderArchiveConfigureDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), myConfigGroupName );

    const QSize size = group.readEntry( "Size", QSize(500, 300) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void FolderArchiveConfigureDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), myConfigGroupName );
    group.writeEntry( "Size", size() );
    group.sync();
}


