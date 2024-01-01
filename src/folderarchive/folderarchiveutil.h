/*
   SPDX-FileCopyrightText: 2013-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QString>
namespace FolderArchive
{
namespace FolderArchiveUtil
{
[[nodiscard]] QString groupConfigPattern();
[[nodiscard]] bool resourceSupportArchiving(const QString &resource);
[[nodiscard]] QString configFileName();
}
}
