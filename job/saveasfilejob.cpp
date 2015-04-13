/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "saveasfilejob.h"
#include "kmail_debug.h"
#include <QFileDialog>
#include <qpointer.h>
#include <qtextdocumentwriter.h>
#include <QDebug>
#include <QUrl>
#include <KLocalizedString>
#include <messagecomposer/composer/kmeditor.h>

SaveAsFileJob::SaveAsFileJob(QObject *parent)
    : QObject(parent),
      mHtmlMode(false),
      mEditor(Q_NULLPTR),
      mParentWidget(Q_NULLPTR)
{

}

SaveAsFileJob::~SaveAsFileJob()
{

}

void SaveAsFileJob::start()
{
    QPointer<QFileDialog> dlg = new QFileDialog(mParentWidget);
    dlg->setWindowTitle(i18n("Save File as"));
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    QStringList lst;
    if (mHtmlMode) {
        lst << QLatin1Literal("text/html") << QLatin1Literal("text/plain") << QLatin1Literal("application/vnd.oasis.opendocument.text");
    } else {
        lst << QLatin1Literal("text/plain");
    }
    dlg->setMimeTypeFilters(lst);

    if (dlg->exec()) {
        QTextDocumentWriter writer;
        const QString filename = dlg->selectedFiles().at(0);
        writer.setFileName(filename);
        if (dlg->selectedNameFilter() == QStringLiteral("text/plain") || filename.endsWith(QLatin1String(".txt"))) {
            writer.setFormat("plaintext");
        } else if (dlg->selectedNameFilter() == QStringLiteral("text/html") || filename.endsWith(QLatin1String(".html"))) {
            writer.setFormat("HTML");
        } else if (dlg->selectedNameFilter() == QStringLiteral("application/vnd.oasis.opendocument.text") || filename.endsWith(QLatin1String(".odf"))) {
            writer.setFormat("ODF");
        } else {
            writer.setFormat("plaintext");
        }
        if (!writer.write(mEditor->document())) {
            qCDebug(KMAIL_LOG) << " Error during writing";
        }
    }
    delete dlg;
    deleteLater();
}

void SaveAsFileJob::setHtmlMode(bool htmlMode)
{
    mHtmlMode = htmlMode;
}

void SaveAsFileJob::setEditor(MessageComposer::KMeditor *editor)
{
    mEditor = editor;
}

void SaveAsFileJob::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}

