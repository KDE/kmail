/*
   Copyright (C) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef UTILS_H_
#define UTILS_H_

#include <functional>
#include <QHash>
#include <QString>

template<typename T>
QList<T> setToList(QSet<T> &&set)
{
    QList<T> rv;
    rv.reserve(set.size());
    std::copy(set.cbegin(), set.cend(), std::back_inserter(rv));
    return rv;
}

template<typename T>
QSet<T> listToSet(QList<T> &&list)
{
    QSet<T> rv;
    rv.reserve(list.size());
    for (auto t : list) {
        rv.insert(std::move(t));
    }
    return rv;
}

namespace std {
    template<>
    struct hash<QString> {
        size_t operator()(const QString &str) const {
            return qHash(str);
        }
    };
}

#endif
