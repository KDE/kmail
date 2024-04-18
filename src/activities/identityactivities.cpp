/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "identityactivities.h"

IdentityActivities::IdentityActivities(QObject *parent)
    : KIdentityManagementCore::IdentityActivitiesAbstract{parent}
{
}

IdentityActivities::~IdentityActivities() = default;

bool IdentityActivities::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    return false;
}

bool IdentityActivities::hasActivitySupport() const
{
    return mEnabled;
}

bool IdentityActivities::enabled() const
{
    return mEnabled;
}

void IdentityActivities::setEnabled(bool newEnabled)
{
    mEnabled = newEnabled;
}

#include "moc_identityactivities.cpp"
