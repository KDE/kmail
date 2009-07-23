/*
 *  autoqpointer.h  -  QPointer which on destruction deletes object
 *  This is a (mostly) verbatim, private copy of kdepim/kalarm/lib/autoqpointer.h
 *
 *  Copyright © 2009 by David Jarvie <djarvie@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AUTOQPOINTER_H
#define AUTOQPOINTER_H

#include <QPointer>


/**
 *  A QPointer which when destructed, deletes the object it points to.
 *
 *  @author David Jarvie <djarvie@kde.org>
 */
template <class T>
class AutoQPointer : public QPointer<T>
{
  public:
    inline AutoQPointer() : QPointer<T>() {}
    inline AutoQPointer(T* p) : QPointer<T>(p) {}
    inline AutoQPointer(const QPointer<T>& p) : QPointer<T>(p) {}
    inline ~AutoQPointer()  { delete this->data(); }
    inline AutoQPointer<T>& operator=(const AutoQPointer<T>& p) { QPointer<T>::operator=(p); return *this; }
    inline AutoQPointer<T>& operator=(T* p) { QPointer<T>::operator=(p); return *this; }
};

#endif // AUTOQPOINTER_H
