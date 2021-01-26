/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTILS_H_
#define UTILS_H_

#include <QHash>
#include <QString>
#include <functional>

template<typename T> inline QList<T> setToList(QSet<T> &&set)
{
    QList<T> rv;
    rv.reserve(set.size());
    std::copy(set.cbegin(), set.cend(), std::back_inserter(rv));
    return rv;
}

template<typename T> inline QList<T> setToList(const QSet<T> &set)
{
    QList<T> rv;
    rv.reserve(set.size());
    std::copy(set.cbegin(), set.cend(), std::back_inserter(rv));
    return rv;
}

template<typename T> inline QSet<T> listToSet(QList<T> &&list)
{
    QSet<T> rv;
    rv.reserve(list.size());
    for (auto t : list) {
        rv.insert(std::move(t));
    }
    return rv;
}

#endif
