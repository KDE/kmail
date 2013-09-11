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

#include <KLocale>
#include <KTextEdit>
#include <KStandardDirs>

#include <QProcess>

SearchDebugNepomukShowDialog::SearchDebugNepomukShowDialog(const QString &nepomukId, QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Nepomuk show result" ) );
    setButtons( Close | User1 );
    setButtonText(User1, i18n("Save As..."));

    mResult = new KTextEdit;
    mResult->setReadOnly(true);
    setMainWidget( mResult );

    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotSaveAs()));
    readConfig();
    executeNepomukShow(nepomukId);
}

SearchDebugNepomukShowDialog::~SearchDebugNepomukShowDialog()
{
    writeConfig();
}

void SearchDebugNepomukShowDialog::executeNepomukShow(const QString &nepomukId)
{
    const QString path = KStandardDirs::findExe( QLatin1String("nepomukshow") );
    if ( path.isEmpty() ) {
        mResult->setPlainText(i18n("Sorry you don't have \"nepomukshow\" installed on your computer."));
    } else {
        QStringList arguments;
        arguments << QString::fromLatin1("akonadi:?item=%1").arg(nepomukId);
        QProcess proc;
        proc.start(path, arguments);
        if (!proc.waitForFinished()) {
            mResult->setPlainText(i18n("Sorry there is a problem with virtuoso."));
            return;
        }
        QByteArray result = proc.readAll();
        proc.close();
        mResult->setPlainText(QString::fromUtf8(result));
    }
}

void SearchDebugNepomukShowDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "SearchDebugNepomukShowDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize() );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    } else {
        resize( 800,600);
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
    PimCommon::Util::saveTextAs(mResult->toPlainText(), filter, this);
}



#include "searchdebugnepomukshowdialog.moc"
