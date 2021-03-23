/*
   SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QString>
namespace FolderArchive
{
namespace FolderArchiveUtil
{
Q_REQUIRED_RESULT QString groupConfigPattern();
Q_REQUIRED_RESULT bool resourceSupportArchiving(const QString &resource);
Q_REQUIRED_RESULT QString configFileName();
}
}

