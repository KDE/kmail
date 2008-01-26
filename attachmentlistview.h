/*  -*- c++ -*-
    attachmentlistview.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#ifndef _KMAIL_ATTACHMENTLISTVIEW_H_
#define _KMAIL_ATTACHMENTLISTVIEW_H_

#include <klistview.h>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class KMComposeWin;

namespace KMail {

class Composer;

class AttachmentListView : public KListView
{
  Q_OBJECT
public:
  AttachmentListView( KMail::Composer * composer = 0, QWidget* parent = 0,
                      const char* name = 0 );
  virtual ~AttachmentListView();

  /** Drag and drop methods */
  void contentsDragEnterEvent( QDragEnterEvent* );
  void contentsDragMoveEvent( QDragMoveEvent* );
  void contentsDropEvent( QDropEvent* );

protected:
  virtual void keyPressEvent( QKeyEvent * e );
  virtual void startDrag();

private:
  KMail::Composer * mComposer;

signals:
  void attachmentDeleted();
  void dragStarted();

};

} // namespace KMail

#endif // _KMAIL_ATTACHMENTLISTVIEW_H_

