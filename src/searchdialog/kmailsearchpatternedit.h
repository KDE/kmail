/*
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
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
