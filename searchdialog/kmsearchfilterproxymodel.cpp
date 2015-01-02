/*
 * Copyright (c) 2011-2015 Montel Laurent <montel@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "kmsearchfilterproxymodel.h"
#include "kmsearchmessagemodel.h"
#include <QModelIndex>
#include <QDateTime>

using namespace KMail;

KMSearchFilterProxyModel::KMSearchFilterProxyModel( QObject *parent )
    :QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( true );
    setFilterCaseSensitivity ( Qt::CaseInsensitive );
}

KMSearchFilterProxyModel::~KMSearchFilterProxyModel()
{
}


bool KMSearchFilterProxyModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
    if ( right.model() && left.model() && left.column() == KMSearchMessageModel::Date )  {
        if ( sourceModel() ) {
            const QDateTime leftData =
                    sourceModel()->data(
                        left.sibling( left.row(), KMSearchMessageModel::DateNotTranslated ) ).toDateTime();

            const QDateTime rightData =
                    sourceModel()->data(
                        right.sibling( right.row(), KMSearchMessageModel::DateNotTranslated ) ).toDateTime();
            return  leftData<rightData;
        } else {
            return false;
        }
    }

    if ( right.model() && left.model() && left.column() == KMSearchMessageModel::Size )  {
        if ( sourceModel() ) {
            const qint64 leftData =
                    sourceModel()->data(
                        left.sibling( left.row(), KMSearchMessageModel::SizeNotLocalized ) ).toLongLong();
            const qint64 rightData =
                    sourceModel()->data(
                        right.sibling( right.row(), KMSearchMessageModel::SizeNotLocalized ) ).toLongLong();
            return  leftData<rightData;
        } else {
            return false;
        }
    }
    return QSortFilterProxyModel::lessThan( left, right );
}
