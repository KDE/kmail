/*
 * This file is part of KMail.
 * Copyright (c) 2012 Laurent Montel <montel@kde.org>
 * Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>
 *
 * Parts based on KMail code by:
 * Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>
 * Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KMAIL_ATTACHMENTVIEW_H
#define KMAIL_ATTACHMENTVIEW_H

#include <QTreeView>

class QContextMenuEvent;
class QToolButton;

namespace Message {
class AttachmentModel;
}

namespace KMail {

class AttachmentView : public QTreeView
{
  Q_OBJECT

  public:
    /// can't change model afterwards.
    explicit AttachmentView( Message::AttachmentModel *model, QWidget *parent = 0 );
    ~AttachmentView();

    QWidget *widget();

  public slots:
    /// model sets these
    void setEncryptEnabled( bool enabled );
    void setSignEnabled( bool enabled );
    void hideIfEmpty();
    void selectNewAttachment();

    void updateAttachmentLabel();
  protected:
    /** reimpl to avoid default drag cursor */
    virtual void startDrag( Qt::DropActions supportedActions );
    /* reimpl */
    virtual void contextMenuEvent( QContextMenuEvent *event );
    /* reimpl */
    virtual void keyPressEvent( QKeyEvent *event );
    /** reimpl to avoid drags from ourselves */
    virtual void dragEnterEvent( QDragEnterEvent *event );

  private Q_SLOTS:
    void slotShowHideAttchementList(bool);
  private:
    void saveHeaderState();
    void restoreHeaderState();

  signals:
    void contextMenuRequested();
    void modified(bool);

  private:
    class Private;
    Private *const d;
};

} // namespace KMail

#endif // KMAIL_ATTACHMENTVIEW_H
