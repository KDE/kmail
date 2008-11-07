/****************************************************************************** *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef __KMAIL_COMMANDS_EXPORTTOHTML_H__
#define __KMAIL_COMMANDS_EXPORTTOHTML_H__

#include "kmcommands.h"

class KMMessage;

namespace KMail
{

class MessageTreeCollection;
class MessageTree;

namespace Commands
{

/**
 * This command exports a structured collection of messages to a html file.
 */
class ExportToHtml : public KMCommand
{
  Q_OBJECT
public:

private:
  MessageTreeCollection * mMessageTreeCollection;
  bool mOpenInBrowser;

public:
  ExportToHtml( QWidget * parent, MessageTreeCollection * collection, bool openInBrowser = false );
  ~ExportToHtml();

private:
  virtual Result execute();

  KMMessage * findMessageBySerial( const QList< KMMessage * > &messages, unsigned long serial );
  QString getHtmlForTreeList( QList< MessageTree * > * treeList, const QList< KMMessage * > &messages );
};

} // namespace Commands

} // namespace KMail


#endif //!__KMAIL_COMMANDS_EXPORTTOHTML_H__
