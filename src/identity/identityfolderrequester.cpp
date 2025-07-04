/*
  SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "identityfolderrequester.h"
#include <Akonadi/Collection>
#include <KColorScheme>
#include <KStatefulBrush>

using namespace KMail;

IdentityFolderRequester::IdentityFolderRequester(QWidget *parent)
    : MailCommon::FolderRequester(parent)
{
}

IdentityFolderRequester::~IdentityFolderRequester() = default;

void IdentityFolderRequester::setIsInvalidFolder(const Akonadi::Collection &col)
{
    const KStatefulBrush bgBrush(KColorScheme::View, KColorScheme::NegativeBackground);
    setStyleSheet(QStringLiteral("QLineEdit{ background-color:%1 }").arg(bgBrush.brush(palette()).color().name()));
    setCollection(col);
    connect(this, &IdentityFolderRequester::folderChanged, this, &IdentityFolderRequester::slotFolderChanged, Qt::UniqueConnection);
}

void IdentityFolderRequester::slotFolderChanged(const Akonadi::Collection &col)
{
    if (col.isValid()) {
        setStyleSheet(QString());
    }
}

#include "moc_identityfolderrequester.cpp"
