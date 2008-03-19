/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@ubiz.ru>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CUSTOMTEMPLATESMENU_H
#define CUSTOMTEMPLATESMENU_H

#include <QVector>
#include <QList>
#include <QObject>

class QSignalMapper;

class KActionCollection;
class KAction;
class KActionMenu;

class CustomTemplatesMenu : public QObject
{
  Q_OBJECT

  public:
    CustomTemplatesMenu( QWidget *parent, KActionCollection *ac );
    ~CustomTemplatesMenu();

    KActionMenu *replyActionMenu() const { return (mCustomReplyActionMenu); }
    KActionMenu *replyAllActionMenu() const { return (mCustomReplyAllActionMenu); }
    KActionMenu *forwardActionMenu() const { return (mCustomForwardActionMenu); }

  public slots:
    void update();

  signals:
    void replyTemplateSelected( const QString &tmpl );
    void replyAllTemplateSelected( const QString &tmpl );
    void forwardTemplateSelected( const QString &tmpl );

  private slots:
    void slotReplySelected( int idx );
    void slotReplyAllSelected( int idx );
    void slotForwardSelected( int idx );

  private:
    void clear();

    KActionCollection *mOwnerActionCollection;

    QVector<QString> mCustomTemplates;
    QList<KAction*> mCustomTemplateActions;

    // Custom template actions menu
    KActionMenu *mCustomReplyActionMenu, *mCustomReplyAllActionMenu, *mCustomForwardActionMenu;

    // Signal mappers for custom template actions
    QSignalMapper *mCustomReplyMapper, *mCustomReplyAllMapper, *mCustomForwardMapper;
};

#endif // CUSTOMTEMPLATESMENU_H
