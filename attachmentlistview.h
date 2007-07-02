/*  -*- c++ -*-
    attachmentlistview.h

    KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>
    Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/


#ifndef _KMAIL_ATTACHMENTLISTVIEW_H_
#define _KMAIL_ATTACHMENTLISTVIEW_H_

#include <QTreeWidget>

class QKeyEvent;


namespace KMail {

class Composer;

class AttachmentListView : public QTreeWidget
{
  Q_OBJECT
public:
  explicit AttachmentListView( KMail::Composer * composer = 0, QWidget* parent = 0 );
  virtual ~AttachmentListView();

  void enableCryptoCBs( bool enable );
  bool areCryptoCBsEnabled();

  virtual void dragEnterEvent( QDragEnterEvent* );
  virtual void dragMoveEvent( QDragMoveEvent* );
  virtual void dropEvent( QDropEvent* );

public Q_SLOTS:
  void slotSort();

protected:
  virtual void keyPressEvent( QKeyEvent * e );
  virtual void contextMenuEvent( QContextMenuEvent * event );

private:
  KMail::Composer * mComposer;

  int mEncryptColWidth;
  int mSignColWidth;

signals:
  void attachmentDeleted();
  void rightButtonPressed( QTreeWidgetItem * item );
};

} // namespace KMail

#endif // _KMAIL_ATTACHMENTLISTVIEW_H_

