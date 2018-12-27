/*
  Copyright (C) 2016-2019 Montel Laurent <montel@kde.org>

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

#include "identityfolderrequester.h"
#include <AkonadiCore/Collection>
#include <KColorScheme>

using namespace KMail;

IdentityFolderRequester::IdentityFolderRequester(QWidget *parent)
    : MailCommon::FolderRequester(parent)
{
}

IdentityFolderRequester::~IdentityFolderRequester()
{
}

void IdentityFolderRequester::setIsInvalidFolder(const Akonadi::Collection &col)
{
    const KStatefulBrush bgBrush(KColorScheme::View, KColorScheme::NegativeBackground);
    setStyleSheet(QStringLiteral("QLineEdit{ background-color:%1 }").arg(bgBrush.brush(this).color().name()));
    setCollection(col);
    connect(this, &IdentityFolderRequester::folderChanged, this, &IdentityFolderRequester::slotFolderChanged, Qt::UniqueConnection);
}

void IdentityFolderRequester::slotFolderChanged(const Akonadi::Collection &col)
{
    if (col.isValid()) {
        setStyleSheet(QString());
    }
}
