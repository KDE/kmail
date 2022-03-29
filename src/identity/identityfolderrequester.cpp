/*
  SPDX-FileCopyrightText: 2016-2022 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityfolderrequester.h"
#include <kconfigwidgets_version.h>
#if KCONFIGWIDGETS_VERSION >= QT_VERSION_CHECK(5, 93, 0)
#include <KStatefulBrush> // was moved to own header in 5.93.0
#endif
#include <Akonadi/Collection>
#include <KColorScheme>

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
