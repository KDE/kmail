/*  -*- c++ -*-
    attachmentlistview.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/


#ifndef _KMAIL_ATTACHMENTLISTVIEW_H_
#define _KMAIL_ATTACHMENTLISTVIEW_H_

#include <klistview.h>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class KMComposeWin;

namespace KMail {

class AttachmentListView : public KListView
{
  Q_OBJECT
public:
  AttachmentListView( KMComposeWin* composer = 0, QWidget* parent = 0,
                      const char* name = 0 );
  virtual ~AttachmentListView();

  /** Drag and drop methods */
  void contentsDragEnterEvent( QDragEnterEvent* );
  void contentsDragMoveEvent( QDragMoveEvent* );
  void contentsDropEvent( QDropEvent* );

protected:
  virtual void keyPressEvent( QKeyEvent * e );

private:
  KMComposeWin* mComposer;

signals:
  void attachmentDeleted();

};

} // namespace KMail

#endif // _KMAIL_ATTACHMENTLISTVIEW_H_

