/*  -*- c++ -*-
    vacationdialog.h

    KMail, the KDE mail client.
    Copyright (c) 2002 Marc Mutz <mutz@kde.org>
    Copyright (c) 2005 Klarälvdalens Datakonsult AB

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

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

#ifndef __KMAIL_VACATIONDIALOG_H__
#define __KMAIL_VACATIONDIALOG_H__

#include "kdialogbase.h"

class QString;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class KDateWidget;
class KIntSpinBox;
template <typename T> class QValueList;

namespace KMime {
  namespace Types {
    struct AddrSpec;
    typedef QValueList<AddrSpec> AddrSpecList;
  }
}

namespace KMail {

  class VacationDialog : public KDialogBase {
    Q_OBJECT
  public:
    VacationDialog( const QString & caption, QWidget * parent=0,
		    const char * name=0, bool modal=true );
    virtual ~VacationDialog();

    bool activateVacation() const;
    virtual void setActivateVacation( bool activate );

    QString messageText() const;
    virtual void setMessageText( const QString & text );

    int notificationInterval() const;
    virtual void setNotificationInterval( int days );

    KMime::Types::AddrSpecList mailAliases() const;
    virtual void setMailAliases( const KMime::Types::AddrSpecList & aliases );
    virtual void setMailAliases( const QString & aliases );

    QString domainName() const;
    virtual void setDomainName( const QString & domain );

    bool sendForSpam() const;
    virtual void setSendForSpam( bool enable );

  protected:
    QCheckBox   * mActiveCheck;
    KIntSpinBox * mIntervalSpin;
    QLineEdit   * mMailAliasesEdit;
    QTextEdit   * mTextEdit;
    QCheckBox   * mSpamCheck;
    QCheckBox   * mDomainCheck;
    QLineEdit   * mDomainEdit;
  };

} // namespace KMail

#endif // __KMAIL_VACATIONDIALOG_H__
