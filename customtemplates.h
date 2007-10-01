/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov
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
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef CUSTOMTEMPLATES_H
#define CUSTOMTEMPLATES_H

#include <QHash>

#include "ui_customtemplates_base.h"

class KShortcut;

struct CustomTemplateItem;
typedef QHash<QString,CustomTemplateItem*> CustomTemplateItemList;

class CustomTemplates : public QWidget, public Ui::CustomTemplatesBase
{
  Q_OBJECT

  public:

    enum Type { TUniversal, TReply, TReplyAll, TForward };

  public:

    explicit CustomTemplates( QWidget *parent = 0, const char *name = 0 );
    ~CustomTemplates();

    void load();
    void save();

    QString indexToType( int index );

  public slots:

    void slotInsertCommand( const QString &cmd, int adjustCursor = 0 );

    void slotTextChanged();

    void slotAddClicked();
    void slotRemoveClicked();
    void slotListSelectionChanged();
    void slotTypeActivated( int index );
    void slotShortcutChanged( const QKeySequence &newSeq );

  signals:

    void changed();

  protected:

    CustomTemplateItemList mItemList;

    QPixmap mReplyPix;
    QPixmap mReplyAllPix;
    QPixmap mForwardPix;

  private slots:

    void slotHelpLinkClicked( const QString& );
    void slotNameChanged( const QString & text );
};

struct CustomTemplateItem
{
  CustomTemplateItem() {}
  CustomTemplateItem( const QString &name,
                      const QString &content,
                      KShortcut &shortcut,
                      CustomTemplates::Type type ) :
    mName( name ), mContent( content ), mShortcut(shortcut), mType( type ) {}

  QString mName, mContent;
  //QKeySequence might do the trick too, check the rest of your code. --ahartmetz
  KShortcut mShortcut;
  CustomTemplates::Type mType;
};

#endif // CUSTOMTEMPLATES_H
