/*  -*- mode: C++; c-file-style: "gnu" -*-
    rulewidgethandlermanager.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KMAIL_RULEWIDGETHANDLERMANAGER_H__
#define __KMAIL_RULEWIDGETHANDLERMANAGER_H__

#include "kmsearchpattern.h"

#include <qvaluevector.h>

class QObject;
class QCString;
class QString;
class QWidgetStack;

namespace KMail {

  class RuleWidgetHandler;

  /**
   * @short Singleton to manage the list of @see RuleWidgetHandlers
   */
  class RuleWidgetHandlerManager {
    static RuleWidgetHandlerManager * self;

    RuleWidgetHandlerManager();
  public:
    ~RuleWidgetHandlerManager();

    static RuleWidgetHandlerManager * instance() {
      if ( !self )
	self = new RuleWidgetHandlerManager();
      return self;
    }

    void registerHandler( const RuleWidgetHandler * handler );
    void unregisterHandler( const RuleWidgetHandler * handler );

    void createWidgets( QWidgetStack *functionStack,
                        QWidgetStack *valueStack,
                        const QObject *receiver ) const;
    KMSearchRule::Function function( const QCString & field,
                                     const QWidgetStack *functionStack ) const;
    QString value( const QCString & field,
                   const QWidgetStack *functionStack,
                   const QWidgetStack *valueStack ) const;
    QString prettyValue( const QCString & field,
                         const QWidgetStack *functionStack,
                         const QWidgetStack *valueStack ) const;
    bool handlesField( const QCString & field,
                       const QWidgetStack *functionStack,
                       const QWidgetStack *valueStack ) const;
    void reset( QWidgetStack *functionStack,
                QWidgetStack *valueStack ) const;
    void setRule( QWidgetStack *functionStack,
                  QWidgetStack *valueStack,
                  const KMSearchRule *rule ) const;
    void update( const QCString & field,
                 QWidgetStack *functionStack,
                 QWidgetStack *valueStack ) const;

  private:
    typedef QValueVector<const RuleWidgetHandler*>::const_iterator const_iterator;
    typedef QValueVector<const RuleWidgetHandler*>::iterator iterator;

    QValueVector<const RuleWidgetHandler*> mHandlers;
  };

} // namespace KMail

#endif // __KMAIL_RULEWIDGETHANDLERMANAGER_H__
