/*
   SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef UTILS_H_
#define UTILS_H_

#include <functional>
#include <QHash>
#include <QString>

template<typename T>
inline QList<T> setToList(QSet<T> &&set)
{
    QList<T> rv;
    rv.reserve(set.size());
    std::copy(set.cbegin(), set.cend(), std::back_inserter(rv));
    return rv;
}

template<typename T>
inline QList<T> setToList(const QSet<T> &set)
{
    QList<T> rv;
    rv.reserve(set.size());
    std::copy(set.cbegin(), set.cend(), std::back_inserter(rv));
    return rv;
}

template<typename T>
inline QSet<T> listToSet(QList<T> &&list)
{
    QSet<T> rv;
    rv.reserve(list.size());
    for (auto t : list) {
        rv.insert(std::move(t));
    }
    return rv;
}

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
namespace std {
template<>
struct hash<QString> {
    inline size_t operator()(const QString &str) const
    {
        return qHash(str);
    }
};
}
#endif

#endif
