/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "saveasfilejob.h"
#include "kmail_debug.h"
#include <KLocalizedString>
#include <QFileDialog>
#include <QPointer>
#include <QTextDocument>
#include <QTextDocumentWriter>

SaveAsFileJob::SaveAsFileJob(QObject *parent)
    : QObject(parent)
{
}

SaveAsFileJob::~SaveAsFileJob() = default;

void SaveAsFileJob::start()
{
    QPointer<QFileDialog> dlg = new QFileDialog(mParentWidget);
    dlg->setWindowTitle(i18nc("@title:window", "Save File as"));
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    QStringList lst;
    if (mHtmlMode) {
        lst << QStringLiteral("text/html") << QStringLiteral("text/plain") << QStringLiteral("application/vnd.oasis.opendocument.text");
    } else {
        lst << QStringLiteral("text/plain");
    }
    dlg->setMimeTypeFilters(lst);

    if (dlg->exec()) {
        QTextDocumentWriter writer;
        const QString filename = dlg->selectedFiles().at(0);
        writer.setFileName(filename);
        if (dlg->selectedNameFilter() == QLatin1String("text/plain") || filename.endsWith(QLatin1String(".txt"))) {
            writer.setFormat("plaintext");
        } else if (dlg->selectedNameFilter() == QLatin1String("text/html") || filename.endsWith(QLatin1String(".html"))) {
            writer.setFormat("HTML");
        } else if (dlg->selectedNameFilter() == QLatin1String("application/vnd.oasis.opendocument.text") || filename.endsWith(QLatin1String(".odf"))) {
            writer.setFormat("ODF");
        } else {
            writer.setFormat("plaintext");
        }
        if (!writer.write(mTextDocument)) {
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

void SaveAsFileJob::setTextDocument(QTextDocument *textDocument)
{
    mTextDocument = textDocument;
}

void SaveAsFileJob::setParentWidget(QWidget *parentWidget)
{
    mParentWidget = parentWidget;
}
