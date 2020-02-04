/*
   Copyright (C) 2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "accountinfosource.h"
#include <QVariant>
#include <KUserFeedback/Provider>
#include <KLocalizedString>

AccountInfoSource::AccountInfoSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("account"), KUserFeedback::Provider::DetailedSystemInformation)
{

}

//TODO add account type
QString AccountInfoSource::name() const
{
    return i18n("Account information");
}

QString AccountInfoSource::description() const
{
    return i18n("Account configurated in KMail (reveiver and sender).");
}

QVariant AccountInfoSource::data()
{
    return {};
}
