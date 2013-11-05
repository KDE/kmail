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

#include "searchdebugnepomukshowdialog.h"

#include "pimcommon/util/pimutil.h"
#include "pimcommon/texteditor/plaintexteditor/plaintexteditorwidget.h"
#include "pimcommon/texteditor/plaintexteditor/plaintexteditor.h"

#include <KLocale>
#include <KStandardDirs>

#include <QProcess>
#include <QInputDialog>
#include <QPointer>

SearchDebugNepomukShowDialog::SearchDebugNepomukShowDialog(const QString &nepomukId, QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Nepomuk show result" ) );
    setButtons( Close | User1 | User2 );
    setButtonText(User1, i18n("Save As..."));
    setButtonText(User2, i18n("Search info with nepomukshow..."));

    mResult = new PimCommon::PlainTextEditorWidget;
    mResult->setReadOnly(true);
    setMainWidget( mResult );

    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotSaveAs()));
    connect(this, SIGNAL(user2Clicked()), this, SLOT(slotSearchInfoWithNepomuk()));
    readConfig();
    executeNepomukShow(nepomukId);
}

SearchDebugNepomukShowDialog::~SearchDebugNepomukShowDialog()
{
    writeConfig();
}

void SearchDebugNepomukShowDialog::slotSearchInfoWithNepomuk()
{
    QString defaultValue;
    if (mResult->editor()->textCursor().hasSelection()) {
        defaultValue = mResult->editor()->textCursor().selectedText().trimmed();
    }
    const QString nepomukId = QInputDialog::getText(this, i18n("Search with nepomukshow"), i18n("Nepomuk id:"), QLineEdit::Normal, defaultValue);
    if (nepomukId.isEmpty())
        return;
    QPointer<SearchDebugNepomukShowDialog> dlg = new SearchDebugNepomukShowDialog(nepomukId, this);
    dlg->exec();
    delete dlg;
}

void SearchDebugNepomukShowDialog::executeNepomukShow(const QString &nepomukId)
{
    const QString path = KStandardDirs::findExe( QLatin1String("nepomukshow") );
    if ( path.isEmpty() ) {
        mResult->editor()->setPlainText(i18n("Sorry you don't have \"nepomukshow\" installed on your computer."));
    } else {
        QStringList arguments;
        arguments << nepomukId;
        QProcess proc;
        proc.start(path, arguments);
        if (!proc.waitForFinished()) {
            mResult->editor()->setPlainText(i18n("Sorry there is a problem with virtuoso."));
            return;
        }
        QByteArray result = proc.readAll();
        proc.close();
        mResult->editor()->setPlainText(QString::fromUtf8(result));
    }
}

void SearchDebugNepomukShowDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "SearchDebugNepomukShowDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(800,600) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void SearchDebugNepomukShowDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "SearchDebugNepomukShowDialog" );
    group.writeEntry( "Size", size() );
}

void SearchDebugNepomukShowDialog::slotSaveAs()
{
    const QString filter = i18n( "all files (*)" );
    PimCommon::Util::saveTextAs(mResult->editor()->toPlainText(), filter, this);
}



