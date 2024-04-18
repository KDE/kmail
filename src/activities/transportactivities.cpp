/*
  SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "transportactivities.h"

TransportActivities::TransportActivities(QObject *parent)
    : MailTransport::TransportActivitiesAbstract{parent}
{
}

TransportActivities::~TransportActivities() = default;

bool TransportActivities::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // TODO
    return false;
}

bool TransportActivities::hasActivitySupport() const
{
    return mEnabled;
}

bool TransportActivities::enabled() const
{
    return mEnabled;
}

void TransportActivities::setEnabled(bool newEnabled)
{
    mEnabled = newEnabled;
}

#include "moc_transportactivities.cpp"
