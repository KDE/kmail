/*  -*- mode: C++ -*-
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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
#ifndef KMAIL_REDIRECTDIALOG_H
#define KMAIL_REDIRECTDIALOG_H

#include <kdialog.h>

class KMLineEdit;
class QPushButton;
class QLabel;

namespace KMail {

  //---------------------------------------------------------------------------
  /**
    @short KMail message redirection dialog.
    @author Andreas Gungl <a.gungl@gmx.de>

    The dialog is used to collect redirect addresses when
    manually redirecting messages. Only Redirect-To is
    supported so far.

  */
  class RedirectDialog : public KDialog
  {
    Q_OBJECT

    public:
      /** Constructor
        @param parent parent QWidget
        @param immediate True, if the Send Now button should be default
                         or false if the Queue button should be default
      */
      explicit RedirectDialog( QWidget *parent=0, bool immediate=true );

      /** Return the addresses for the redirection */
      QString to() { return mResentTo; }

      /** Returns the send mode */
      bool sendImmediate() { return mImmediate; }

    protected:
      /** Evaluate the settings, an empty To field is not allowed. */
      void accept();

    protected slots:
      /** Open addressbook editor dialog. */
      void slotAddrBook();

      void slotUser1();
      void slotUser2();
      void slotEmailChanged( const QString & );
    private:
      QLabel      *mLabelTo;
      KMLineEdit  *mEditTo;
      QPushButton *mBtnTo;
      QString     mResentTo;
      bool        mImmediate;
  };

} // namespace KMail

#endif // KMAIL_REDIRECTDIALOG_H
