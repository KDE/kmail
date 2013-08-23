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

#include <KLocale>
#include <KFileDialog>
#include <KMessageBox>

#include <QFile>
#include <QTextStream>
#include <QPointer>

#include <errno.h>

SearchDebugDialog::SearchDebugDialog(const QString &query, QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Search query debug" ) );
    setButtons( Close | User1 );
    setButtonText(User1, i18n("Save As..."));

    mSearchDebugWidget = new SearchDebugWidget(query, this);
    setMainWidget( mSearchDebugWidget );
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotSaveAs()));
    readConfig();
}

SearchDebugDialog::~SearchDebugDialog()
{
    writeConfig();
}

void SearchDebugDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "SieveScriptParsingErrorDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize() );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    } else {
        resize( 800,600);
    }
}

void SearchDebugDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "SieveScriptParsingErrorDialog" );
    group.writeEntry( "Size", size() );
}

void SearchDebugDialog::slotSaveAs()
{
    KUrl url;
    const QString filter = i18n( "all files (*)" );
    QPointer<KFileDialog> fdlg( new KFileDialog( url, filter, this) );

    fdlg->setMode( KFile::File );
    fdlg->setOperationMode( KFileDialog::Saving );
    fdlg->setConfirmOverwrite(true);
    if ( fdlg->exec() == QDialog::Accepted && fdlg ) {
        const QString fileName = fdlg->selectedFile();
        if ( !saveToFile( fileName ) ) {
            KMessageBox::error( this,
                                i18n( "Could not write the file %1:\n"
                                      "\"%2\" is the detailed error description.",
                                      fileName,
                                      QString::fromLocal8Bit( strerror( errno ) ) ),
                                i18n( "Sieve Editor Error" ) );
        }
    }
    delete fdlg;
}

bool SearchDebugDialog::saveToFile( const QString &filename )
{
    QFile file( filename );
    if ( !file.open( QIODevice::WriteOnly|QIODevice::Text ) )
        return false;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << mSearchDebugWidget->queryStr();
    return true;
}


#include "searchdebugdialog.moc"
