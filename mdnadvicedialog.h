/*
  Copyright (c) 2009 Michael Leupold <lemma@confuego.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef MDNADVICEDIALOG_H
#define MDNADVICEDIALOG_H

#include <QtCore/QString>
#include <KDialog>
#include <kmime/kmime_message.h>
#include <messagecomposer/messagefactory.h>

class MDNAdviceDialog : public KDialog
{
  Q_OBJECT

  ~MDNAdviceDialog();

public:
  /**
   * Checks the MDN headers to see if the user needs to be asked for any
   *  confirmations. Will ask the user if action is required.
   *
   * Returns the sending mode for the MDN to be created.
   */
  static KMime::MDN::SendingMode checkMDNHeaders( KMime::Message::Ptr msg );

private:
  MDNAdviceDialog( const QString &text, bool canDeny, QWidget *parent = 0 );
  
  static int requestAdviceOnMDN( const char * what );
  static MessageComposer::MDNAdvice questionIgnoreSend( const QString &text, bool canDeny );
  
  MessageComposer::MDNAdvice m_result;

protected:

  // Reimplemented
  virtual void slotButtonClicked( int button );
};

#endif
