/*
    transportmanager.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#ifndef _KMAIL_TRANSPORTMANAGER_H_
#define _KMAIL_TRANSPORTMANAGER_H_

class QStringList;

namespace KMail {

  /**
   * @short Currently only used to provide a function for reading the transport list.
   * @author Ingo Kloecker <kloecker@kde.org>
   **/
  class TransportManager {

  public:
    TransportManager() {};
    virtual ~TransportManager() {};

    /** Returns the list for transport names */
    static QStringList transportNames();

    /** Create a unique id for a transport info item */
    static unsigned int createId();
  };

} // namespace KMail

#endif // _KMAIL_TRANSPORTMANAGER_H_
