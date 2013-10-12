/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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

#include "searchdebugdialog.h"
#include "searchdebugwidget.h"
#include "searchdebugnepomukshowdialog.h"

#include "pimcommon/util/pimutil.h"

#include <KLocale>

#include <QPointer>
#include <QInputDialog>

SearchDebugDialog::SearchDebugDialog(const QString &query, QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Search query debug" ) );
    setButtons( Close | User1 | User2 );
    setButtonText(User1, i18n("Save As..."));
    setButtonText(User2, i18n("Search info with nepomukshow..."));

    mSearchDebugWidget = new SearchDebugWidget(query, this);
    setMainWidget( mSearchDebugWidget );
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotSaveAs()));
    connect(this, SIGNAL(user2Clicked()), this, SLOT(slotSearchInfoWithNepomuk()));
    readConfig();
}

SearchDebugDialog::~SearchDebugDialog()
{
    writeConfig();
}

void SearchDebugDialog::slotSearchInfoWithNepomuk()
{
    const QString nepomukId = QInputDialog::getText(this, i18n("Search with nepomukshow"), i18n("Nepomuk id:"));
    if (!nepomukId.isEmpty()) {
        QPointer<SearchDebugNepomukShowDialog> dlg = new SearchDebugNepomukShowDialog(nepomukId, this);
        dlg->exec();
        delete dlg;
    }
}

void SearchDebugDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "SearchDebugDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void SearchDebugDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "SearchDebugDialog" );
    group.writeEntry( "Size", size() );
}

void SearchDebugDialog::slotSaveAs()
{
    const QString filter = i18n( "all files (*)" );
    PimCommon::Util::saveTextAs(mSearchDebugWidget->queryStr(), filter, this);
}


#include "searchdebugdialog.moc"
