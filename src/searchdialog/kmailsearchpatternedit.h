/*
  SPDX-FileCopyrightText: 2013-2025 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <MailCommon/SearchPatternEdit>

namespace KMail
{
class KMailSearchPatternEdit : public MailCommon::SearchPatternEdit
{
    Q_OBJECT
public:
    explicit KMailSearchPatternEdit(QWidget *parent = nullptr);
    ~KMailSearchPatternEdit() override;
};
}
